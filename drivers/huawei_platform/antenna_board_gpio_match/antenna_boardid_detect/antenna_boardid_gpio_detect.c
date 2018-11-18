

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include "antenna_boardid_gpio_detect.h"

#define DRV_NAME "antenna_board_gpio"
#define TAG "antenna_board_gpio: "

#define ANTENNA_DETECT(_name, n, m, store)                 \
{                                                       \
    .attr = __ATTR(_name, m, antenna_detect_show, store),    \
    .name = ANTENNA_##n,                            \
}

#define ANTENNA_DETECT_RO(_name, n)            \
        ANTENNA_DETECT(_name, n, S_IRUGO, NULL)

static ssize_t antenna_detect_show(struct device *dev,
        struct device_attribute *attr, char *buf);

struct antenna_detect_info {
    struct device_attribute attr;
    u8 name;
};

#define MAX_GPIO_NUM    3
static int g_gpio_count = 0;
static int g_gpio[MAX_GPIO_NUM] = {0};
static int g_board_version = -1;

static struct antenna_detect_info antenna_detect_tb[] = {
    ANTENNA_DETECT_RO(antenna_board_match,    BOARDID_GPIO_DETECT),
    ANTENNA_DETECT_RO(antenna_boardid_status,    BOARDID_GPIO_STATUS),
};

static struct attribute *antenna_sysfs_attrs[ARRAY_SIZE(antenna_detect_tb) + 1];
static const struct attribute_group antenna_sysfs_attr_group = {
    .attrs = antenna_sysfs_attrs,
};

static void antenna_sysfs_init_attrs(void)
{
    int i, limit = ARRAY_SIZE(antenna_detect_tb);

    for (i = 0; i < limit; i++)
    {
        antenna_sysfs_attrs[i] = &antenna_detect_tb[i].attr.attr;
    }
    antenna_sysfs_attrs[limit] = NULL;
}

static struct antenna_detect_info *antenna_detect_lookup(const char *name)
{
    int i, limit = ARRAY_SIZE(antenna_detect_tb);

    for (i = 0; i< limit; i++)
    {
        if (!strncmp(name, antenna_detect_tb[i].attr.attr.name,strlen(name)))
            break;
    }
    if (i >= limit)
        return NULL;
    return &antenna_detect_tb[i];
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

static int get_boardid_det_status(void)
{
    int i = 0, ret = 0, err = 0;
    int temp_value = 0, gpio_value = 0;

    for (i = 0; i < g_gpio_count; i++) {
        temp_value = gpio_get_value(g_gpio[i]);
        gpio_value += (temp_value << i);
    }
    pr_info(TAG "gpio_value = %d g_gpio_count = %d\n", gpio_value, g_gpio_count);

    if (MAX_GPIO_NUM == g_gpio_count)
    {
        return gpio_value;
    }

    switch(gpio_value){
        case STATUS_UP_DOWN:
        case STATUS_DOWN_UP:
            ret = gpio_value;
            break;
        case STATUS_DOWN_DOWN:
            err = gpio_direction_output(g_gpio[0], 1);
            if (err) {
                pr_err(TAG "pio_direction_output high fail, err %d\n", err);
            }
            msleep(1);
            temp_value = gpio_get_value(g_gpio[1]);
            if (temp_value){
                ret = STATUS_DOWN_DOWN_OTHER;
            }
            else {
                ret =  STATUS_DOWN_DOWN;
            }
            break;
        case STATUS_UP_UP:
            gpio_direction_output(g_gpio[0], 0);
            if (err) {
                pr_err(TAG "pio_direction_output low fail, err %d\n", err);
            }
            msleep(1);
            temp_value = gpio_get_value(g_gpio[1]);
            if (temp_value){
                ret =  STATUS_UP_UP;
            }
            else {
                ret = STATUS_UP_UP_OTHER;
            }
            break;
        default:
            ret = STATUS_ERROR;
            pr_err(TAG "(%s)STATUS ERR!!HAVE NO STATUS:(%d)\n",__func__,gpio_value);
            break;
    }

    gpio_direction_input(g_gpio[0]);
    return ret;
}

static ssize_t antenna_detect_show(struct device *dev,
         struct device_attribute *attr, char *buf)
{
    int det_status = 0, match = 0;
    struct antenna_detect_info *info = NULL;

    info = antenna_detect_lookup(attr->attr.name);
    if (!info)
        return -EINVAL;

    switch(info->name){
    case ANTENNA_BOARDID_GPIO_DETECT:
        det_status = get_boardid_det_status();
        if ( g_board_version == det_status)
        {
            match = ANATENNA_DETECT_SUCCEED;
        }
        else
        {
            match = ANATENNA_DETECT_FAIL;
        }
        pr_info(TAG "%s get det_status = %d, board_version = %d, match = %d\n", __func__, det_status, g_board_version, match);
        return snprintf(buf, PAGE_SIZE, "%d\n", match);
    case ANTENNA_BOARDID_GPIO_STATUS:
        det_status = get_boardid_det_status();
        pr_info(TAG "%s get det_status = %d \n", __func__, det_status);
        return snprintf(buf, PAGE_SIZE, "%d\n", det_status);
    default:
        pr_err(TAG "(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n",__func__,info->name);
        break;
    }
    return 0;
}

static struct class *hw_antenna_detect_class = NULL;
static struct class *antenna_detect_class = NULL;
struct device *antenna_dev_gpio = NULL;

/*get new class*/
struct class *hw_antenna_detect_get_class(void)
{
    if (NULL == hw_antenna_detect_class)
    {
        hw_antenna_detect_class = class_create(THIS_MODULE, "hw_antenna");
        if (NULL == hw_antenna_detect_class)
        {
            pr_err(TAG "hw_antenna_detect_class create fail");
            return NULL;
        }
    }
    return hw_antenna_detect_class;
}

/*
 *probe match table
*/
static struct of_device_id antenna_boardid_detect_table[] = {
    {
        .compatible = "huawei,antenna_boardid_detect",
        .data = NULL,
    },
    {},
};

static void ant_boardid_dt_parse(struct device *dev)
{
    const struct of_device_id *of_id = of_match_device(antenna_boardid_detect_table, dev);
    struct device_node *np = dev->of_node;
    int i = 0;
    enum of_gpio_flags flags = 1;

    if (!of_id || !np) {
        return;
    }

    if (of_property_read_u32(np, "gpio_count", &g_gpio_count)) {
        /* default 1 antenna */
        g_gpio_count = 1;
    }

    if (of_property_read_u32(np, "board_version", &g_board_version)) {
        /* default g_board_version is -1 */
        g_board_version = -1;
    }

    for (i = 0; i < g_gpio_count; i++) {
        g_gpio[i] = of_get_gpio_flags(np, i, &flags);
        pr_debug(TAG "g_gpio[%d] = %d\n", i, g_gpio[i]);

        if (!gpio_is_valid(g_gpio[i])) {
            pr_err(TAG "gpio_is_valid g_gpio[%d] = %d\n", i, g_gpio[i]);
            return;
        }
    }

}

static int ant_boardid_det_gpio_process(int gpio)
{
    int err = -1;

    /* request a gpio and set the function direction in  */
    err = gpio_request_one(gpio, GPIOF_IN, DRV_NAME);
    if (err) {
        pr_err(TAG "Unable to request GPIO %d, err %d\n", gpio, err);
        return err;
    }

    return 0;
}

static int antenna_boardid_detect_probe(struct platform_device *pdev)
{
    /*create a node for antenna boardid detect gpio*/
    int ret = 0;
    int i = 0;
    int err = -1;
    struct antenna_device_info *di;

    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di)
    {
        pr_err(TAG "alloc di failed\n");
        return -ENOMEM;
    }
    di->dev = &pdev->dev;
	dev_set_drvdata(&(pdev->dev), di);


    /* get dts data */
    ant_boardid_dt_parse(&pdev->dev);

    for (i = 0; i < g_gpio_count; i++) {
        err = ant_boardid_det_gpio_process(g_gpio[i]);
        if (err) {
            pr_err(TAG "Process GPIO %d failed\n", g_gpio[i]);
        } else {
            pr_info(TAG "Process GPIO %d success\n", g_gpio[i]);
        }
    }

    ret = antenna_detect_sysfs_create_group(di);
    if (ret) {
        pr_err(TAG "can't create antenna_detect sysfs entries\n");
        goto free_di;
    }
    antenna_detect_class = hw_antenna_detect_get_class();
    if(antenna_detect_class)
    {
        if(antenna_dev_gpio == NULL) {
            antenna_dev_gpio = device_create(antenna_detect_class, NULL, 0, NULL,"antenna_board");
        }

        ret = sysfs_create_link(&antenna_dev_gpio->kobj, &di->dev->kobj, "antenna_board_data");
        if(ret)
        {
            pr_err(TAG "create link to boardid_detect fail.\n");
            goto free_di;
        }
    }
    pr_info(TAG "huawei antenna boardid detect probe ok!\n");
    return 0;

free_di:
    kfree(di);
    di = NULL;
    return -1;
}

static int antenna_boardid_detect_remove(struct platform_device *pdev)
{
    int i = 0;
    struct antenna_device_info *di = dev_get_drvdata(&pdev->dev);

    for (i = 0; i < g_gpio_count; i++) {
        gpio_free(g_gpio[i]);
    }

    antenna_detect_sysfs_remove_group(di);
    kfree(di);
    di = NULL;

    return 0;
}

/*
 *antenna boardid detect driver
 */
static struct platform_driver antenna_boardid_detect_driver = {
    .probe = antenna_boardid_detect_probe,
    .remove = antenna_boardid_detect_remove,
    .driver = {
        .name = "huawei,antenna_boardid_detect",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(antenna_boardid_detect_table),
    },
};
/***************************************************************
 * Function: antenna_boardid_detect_init
 * Description: antenna boardid gpio detect module initialization
 * Parameters:  Null
 * return value: 0-sucess or others-fail
 * **************************************************************/
static int __init antenna_boardid_detect_init(void)
{
    pr_info(TAG "into init");
    return platform_driver_register(&antenna_boardid_detect_driver);
}
/*******************************************************************
 * Function:       antenna_boardid_detect_exit
 * Description:    antenna boardid gpio detect module exit
 * Parameters:   NULL
 * return value:  NULL
 * *********************************************************/
static void __exit antenna_boardid_detect_exit(void)
{
    platform_driver_unregister(&antenna_boardid_detect_driver);
}
module_init(antenna_boardid_detect_init);
module_exit(antenna_boardid_detect_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei antenna boardid detect driver");
MODULE_AUTHOR("HUAWEI Inc");
