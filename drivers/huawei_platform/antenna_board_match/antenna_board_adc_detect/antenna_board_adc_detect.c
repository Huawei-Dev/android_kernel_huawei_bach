
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/qpnp/qpnp-adc.h>
#include "antenna_board_adc_detect.h"

#define ANTENNA_SYSFS_FIELD(_name, n, m, store)      \
{                                                    \
    .attr = __ATTR(_name, m, antenna_show, store),    \
    .name = ANTENNA_##n,                            \
}

#define ANTENNA_SYSFS_FIELD_RO(_name, n)            \
        ANTENNA_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t antenna_show(struct device *dev,
        struct device_attribute *attr, char *buf);

struct antenna_detect_info {
    struct device_attribute attr;
    u8 name;
};

static int antenna_adc_match_range[2] = {820,975};//ADC match range


static struct antenna_detect_info antenna_tb[] = {
    ANTENNA_SYSFS_FIELD_RO(antenna_board_match,    BOARD_ADC_DETECT),
};

static struct attribute *antenna_sysfs_attrs[ARRAY_SIZE(antenna_tb) + 1];
static const struct attribute_group antenna_sysfs_attr_group = {
    .attrs = antenna_sysfs_attrs,
};
static void antenna_sysfs_init_attrs(void)
{
    int i;
    int limit = ARRAY_SIZE(antenna_tb);

    for (i = 0; i < limit; i++) {
        antenna_sysfs_attrs[i] = &antenna_tb[i].attr.attr;
    }
    antenna_sysfs_attrs[limit] = NULL;
}

static struct antenna_detect_info *antenna_board_lookup(const char *name)
{
    int i;
    int limit = ARRAY_SIZE(antenna_tb);

    for (i = 0; i< limit; i++) {
        if (!strncmp(name, antenna_tb[i].attr.attr.name, strlen(name)))
            break;
    }
    if (i >= limit)
        return NULL;
    return &antenna_tb[i];
}

static int antenna_detect_sysfs_create_group(struct antenna_device_info *di)
{
    antenna_sysfs_init_attrs();
    return sysfs_create_group(&di->dev->kobj, &antenna_sysfs_attr_group);
}

static inline void antenna_detect_sysfs_remove_group(struct antenna_device_info *di)
{
    sysfs_remove_group(&di->dev->kobj, &antenna_sysfs_attr_group);
}

/*********************************************************
*  Function:       check_match_by_adc
*  Discription:    check if main board is match with the rf board
*  Parameters:     none
*  Return value:   0-not match or 1-match
*********************************************************/
static int check_match_by_adc(void)
{
    int ret = 0;
    int rf_voltage = -50;

    rf_voltage = get_pmi_sub_voltage();
    pr_info("Antenna board adc voltage = %d\n", rf_voltage);

    if (rf_voltage >= antenna_adc_match_range[0] && rf_voltage <= antenna_adc_match_range[1]) {
        ret = 1;
    }else {
        pr_err("adc voltage is not in range, Antenna_board_match error!\n");
        ret = 0;
    }

    return ret;
}

static ssize_t antenna_show(struct device *dev,
         struct device_attribute *attr, char *buf)
{
    int adc_ret = -1;
    struct antenna_detect_info *info = NULL;

    info = antenna_board_lookup(attr->attr.name);
    if (!info) {
        adc_ret = -EINVAL;
        pr_err("%s Invalid argument: %d\n", __func__, adc_ret);
        return snprintf(buf, PAGE_SIZE, "%d\n", adc_ret);
    }

    switch(info->name) {
        case ANTENNA_BOARD_ADC_DETECT:
            adc_ret = check_match_by_adc();
            pr_info("%s Get adc match status is %d\n", __func__, adc_ret);
            break;
        default:
            pr_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n",__func__,info->name);
            break;
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", adc_ret);
}

static struct class *hw_antenna_class = NULL;
static struct class *antenna_board_detect_class = NULL;
struct device * antenna_dev = NULL;

/*get new class*/
struct class *hw_antenna_get_class(void)
{
    if (NULL == hw_antenna_class) {
        hw_antenna_class = class_create(THIS_MODULE, "hw_antenna");
        if (NULL == hw_antenna_class) {
            pr_err("hw_antenna_class create fail");
            return NULL;
        }
    }
    return hw_antenna_class;
}

static void antenna_board_get_dts(struct antenna_device_info *di)
{
    struct device_node* np;

    np = di->dev->of_node;
    if(NULL == np){
        pr_err("%s np is null!\n",__FUNCTION__);
        return;
    }

    //match range
    if (of_property_read_u32_array(np, "antenna_board_match_range", antenna_adc_match_range, 2)) {
        pr_err("%s, antenna_board_match_range not exist, use default array!\n", __func__);
    }
    pr_info("antenna_adc_match_range: min = %d,max = %d\n",antenna_adc_match_range[0],antenna_adc_match_range[1]);
}


static int antenna_board_detect_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct antenna_device_info *di;

    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di)
    {
        pr_err("alloc di failed\n");
        return -ENOMEM;
    }
    di->dev = &pdev->dev;

    /*get dts data*/
    antenna_board_get_dts(di);

    ret = antenna_detect_sysfs_create_group(di);
    if (ret) {
        pr_err("can't create antenna_detect sysfs entries\n");
        goto Antenna_board_failed;
    }
    antenna_board_detect_class = hw_antenna_get_class();
    if (antenna_board_detect_class) {
        if (NULL == antenna_dev)
        antenna_dev = device_create(antenna_board_detect_class, NULL, 0, NULL,"antenna_board");
        if(IS_ERR(antenna_dev)) {
            antenna_dev = NULL;
            pr_err("create rf_dev failed!\n");
            goto Antenna_board_failed;
        }

        ret = sysfs_create_link(&antenna_dev->kobj, &di->dev->kobj, "antenna_board_data");
        if (ret) {
            pr_err("create link to board_detect fail.\n");
            goto Antenna_board_failed;
        }
    }else {
        pr_err("get antenna_detect_class fail.\n");
        goto Antenna_board_failed;
    }
    pr_info("huawei antenna board detect probe ok!\n");
    return 0;

Antenna_board_failed:
    kfree(di);
    di = NULL;
    return -1;
}

/*
 *probe match table
*/
static struct of_device_id antenna_board_table[] = {
    {
        .compatible = "huawei,antenna_board_detect",
        .data = NULL,
    },
    {},
};

/*
 *antenna board detect driver
 */
static struct platform_driver antenna_board_detect_driver = {
    .probe = antenna_board_detect_probe,
    .driver = {
        .name = "huawei,antenna_board_detect",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(antenna_board_table),
    },
};
/***************************************************************
 * Function: antenna_board_detect_init
 * Description: antenna board detect module initialization
 * Parameters:  Null
 * return value: 0-sucess or others-fail
 * **************************************************************/
static int __init antenna_board_detect_init(void)
{
    return platform_driver_register(&antenna_board_detect_driver);
}
/*******************************************************************
 * Function:       antenna_board_detect_exit
 * Description:    antenna board detect module exit
 * Parameters:   NULL
 * return value:  NULL
 * *********************************************************/
static void __exit antenna_board_detect_exit(void)
{
    platform_driver_unregister(&antenna_board_detect_driver);
}
module_init(antenna_board_detect_init);
module_exit(antenna_board_detect_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei antenna board detect driver");
MODULE_AUTHOR("HUAWEI Inc");
