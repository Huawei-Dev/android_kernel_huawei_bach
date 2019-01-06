#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/wakelock.h>
#include "fingerprint.h"
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
extern void adreno_force_waking_gpu(void);
unsigned int snr_flag = 0;

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
struct dsm_dev dsm_fingerprint =
{
	.name = "dsm_fingerprint",
	.device_name = "fpc",
	.ic_name = "NNN",
	.module_name = "NNN",
	.fops = NULL,
	.buff_size = 1024,
};
struct dsm_client *fingerprint_dclient = NULL;
#endif

//add for fingerprint autotest
static ssize_t result_show(struct device* device,
                       struct device_attribute* attribute,
                       char* buffer)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    return scnprintf(buffer, PAGE_SIZE, "%i\n", fingerprint->autotest_input);
}

static ssize_t result_store(struct device* device,
                       struct device_attribute* attribute,
                       const char* buffer, size_t count)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    fingerprint->autotest_input = simple_strtoul(buffer, NULL, 10);
    sysfs_notify(&fingerprint->pf_dev->dev.kobj, NULL, "result");
    return count;
}

static DEVICE_ATTR(result, S_IRUSR | S_IWUSR, result_show, result_store);

/**
 * sysf node to check the interrupt status of the sensor, the interrupt
 * handler should perform sysf_notify to allow userland to poll the node.
 */
static ssize_t irq_get(struct device* device,
                       struct device_attribute* attribute,
                       char* buffer)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    int irq = gpio_get_value(fingerprint->irq_gpio);

    if ((1 == irq) && (fp_LCD_POWEROFF == atomic_read(&fingerprint->state)))
    {
       adreno_force_waking_gpu();
    }

    return scnprintf(buffer, PAGE_SIZE, "%i\n", irq);
}

/**
 * writing to the irq node will just drop a printk message
 * and return success, used for latency measurement.
 */
static ssize_t irq_ack(struct device* device,
                       struct device_attribute* attribute,
                       const char* buffer, size_t count)
{
    //struct fp_data* fingerprint = dev_get_drvdata(device);
    fpc_log_info("buffer=%s.\n", buffer);
    return count;
}

static DEVICE_ATTR(irq, S_IRUSR | S_IWUSR, irq_get, irq_ack);

static ssize_t read_image_flag_show(struct device* device,
                       struct device_attribute* attribute,
                       char* buffer)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    return scnprintf(buffer, PAGE_SIZE, "%u", (unsigned int)fingerprint->read_image_flag);
}
static ssize_t read_image_flag_store(struct device* device,
                       struct device_attribute* attribute,
                       const char* buffer, size_t count)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    fingerprint->read_image_flag = simple_strtoul(buffer, NULL, 10);
    return (ssize_t)count;
}

static DEVICE_ATTR(read_image_flag, S_IRUSR | S_IWUSR, read_image_flag_show, read_image_flag_store);

static ssize_t snr_show(struct device* device,
                       struct device_attribute* attribute,
                       char* buffer)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    return scnprintf(buffer, PAGE_SIZE, "%d", fingerprint->snr_stat);
}

static ssize_t snr_store(struct device* device,
                       struct device_attribute* attribute,
                       const char* buffer, size_t count)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    fingerprint->snr_stat = simple_strtoul(buffer, NULL, 10);
    if (fingerprint->snr_stat)
    {
        snr_flag = 1;
    }
    else
        snr_flag = 0;

    fpc_log_err("snr_store snr_flag = %u\n", snr_flag);
    return count;
}

static DEVICE_ATTR(snr, S_IRUSR | S_IWUSR |S_IRGRP |S_IWGRP, snr_show, snr_store);

static ssize_t nav_show(struct device* device,
                       struct device_attribute* attribute,
                       char* buffer)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    if (NULL == fingerprint)
    {return -EINVAL;}

    return scnprintf(buffer, PAGE_SIZE, "%d", fingerprint->nav_stat);
}

static ssize_t nav_store(struct device* device,
                       struct device_attribute* attribute,
                       const char* buffer, size_t count)
{
    struct fp_data* fingerprint = dev_get_drvdata(device);
    if (NULL == fingerprint)
    {return -EINVAL;}

    fingerprint->nav_stat = simple_strtoul(buffer, NULL, 10);
    return count;
}

/*lint -save -e* */
static DEVICE_ATTR(nav, S_IRUSR | S_IWUSR |S_IRGRP |S_IWGRP, nav_show, nav_store);
/*lint -restore*/

static ssize_t fingerprint_chip_info_show(struct device *device, struct device_attribute *attribute, char *buf)
{
    char module[5] = {0};
    char sensor_id[10]={0};
    const char* module_info;
    int ret = 0;
    struct fp_data* fingerprint= dev_get_drvdata(device);
    struct device* dev = fingerprint->dev;
    struct device_node* np = dev->of_node;
    switch (fingerprint->module_vendor_info)
    {
        case MODULEID_LOW:
            ret = of_property_read_string(np, "fingerprint,moduleid_low", &module_info);
            if (ret)
            {
                strncpy(module, "NN", 3);
                fpc_log_err("failed to get moduleid_low from device tree\n");
                break;
            }
            strncpy(module, module_info, 3);
            break;
        case MODULEID_HIGH:
            ret = of_property_read_string(np, "fingerprint,moduleid_high", &module_info);
            if (ret)
            {
                strncpy(module, "NN", 3);
                fpc_log_err("failed to get moduleid_high from device tree\n");
                break;
            }
            strncpy(module, module_info, 3);
            break;
        case MODULEID_FLOATING:
            ret = of_property_read_string(np, "fingerprint,moduleid_float", &module_info);
            if (ret)
            {
                strncpy(module, "NN",3);
                fpc_log_err("failed to get moduleid_float from device tree\n");
                break;
            }
            strncpy(module, module_info, 3);
            break;
        default:
            strncpy(module, "NN", 3);
            break;
    }
    sprintf(sensor_id, "%x", fingerprint->sensor_id);
    return scnprintf(buf, 50, "%s-%s\n", sensor_id, module);
}

static DEVICE_ATTR(fingerprint_chip_info, S_IRUSR  | S_IRGRP | S_IROTH, fingerprint_chip_info_show, NULL);

static struct attribute* attributes[] =
{
    &dev_attr_irq.attr,
    &dev_attr_fingerprint_chip_info.attr,
    &dev_attr_result.attr,
    &dev_attr_read_image_flag.attr,
    &dev_attr_snr.attr,
    &dev_attr_nav.attr,
    NULL
};

static const struct attribute_group attribute_group =
{
    .attrs = attributes,
};

static irqreturn_t fingerprint_irq_handler(int irq, void* handle)
{
    struct fp_data* fingerprint = handle;

    smp_rmb();

    if (fingerprint->wakeup_enabled )
    {
        wake_lock_timeout(&fingerprint->ttw_wl, msecs_to_jiffies(fingerprint->irq_hold_time));
    }

    sysfs_notify(&fingerprint->pf_dev->dev.kobj, NULL, dev_attr_irq.attr.name);
    return IRQ_HANDLED;
}

void fingerprint_get_navigation_adjustvalue(const struct device* dev, struct fp_data* fp_data)
{
    struct device_node* np;
    unsigned int adjust1 = NAVIGATION_ADJUST_NOREVERSE;
    unsigned int adjust2 = NAVIGATION_ADJUST_NOTURN;

    if (!dev || !dev->of_node)
    {
        fpc_log_err("%s failed dev or dev node is NULL\n", __func__);
        return;
    }

    np = dev->of_node;

    (void)of_property_read_u32(np, "fingerprint,navigation_adjust1", &adjust1);

    if(adjust1 != NAVIGATION_ADJUST_NOREVERSE && adjust1 != NAVIGATION_ADJUST_REVERSE)
    {
        adjust1 = NAVIGATION_ADJUST_NOREVERSE;
        fpc_log_err("%s navigation_adjust1 set err only support 0 and 1.\n", __func__);
    }

    (void)of_property_read_u32(np, "fingerprint,navigation_adjust2", &adjust2);

    if(adjust2 != NAVIGATION_ADJUST_NOTURN && adjust2 != NAVIGATION_ADJUST_TURN90 &&
            adjust2 != NAVIGATION_ADJUST_TURN180 && adjust2 != NAVIGATION_ADJUST_TURN270)
    {
        adjust2 = NAVIGATION_ADJUST_NOTURN;
        fpc_log_err("%s navigation_adjust2 set err only support 0 90 180 and 270.\n", __func__);
    }

    fp_data->navigation_adjust1 = (int)adjust1;
    fp_data->navigation_adjust2 = (int)adjust2;

    fpc_log_info("%s get navigation_adjust1 = %d, navigation_adjust2 = %d.\n", __func__,
                  fp_data->navigation_adjust1, fp_data->navigation_adjust2);
    return;
}

int fingerprint_get_dts_data(struct device *dev, struct fp_data *fingerprint)
{
    struct device_node *np;
    int ret = 0;
    char const *fingerprint_vdd = NULL;
    char const *fingerprint_avdd = NULL;
    unsigned int irq_hold_time = 0;
    if (!dev || !dev->of_node || !fingerprint)
    {
        fpc_log_err("dev or dev node is NULL\n");
        return -EINVAL;
    }

    fingerprint->irq_hold_time = FPC_TTW_HOLD_TIME;

    np = dev->of_node;

    fingerprint->rst_gpio = of_get_named_gpio(np, "fingerprint,reset_gpio", 0);
    if (!gpio_is_valid(fingerprint->rst_gpio))
    {
        fpc_log_err("failed to get reset gpio from device tree\n");
    }
    else
    {
        fpc_log_info("rst_gpio = %d\n", fingerprint->rst_gpio);
    }

    fingerprint->irq_gpio = of_get_named_gpio(np, "fingerprint,irq_gpio", 0);
    if (!gpio_is_valid(fingerprint->irq_gpio))
    {
        fpc_log_err("failed to get irq gpio from device tree\n");
    }
    else
    {
        fpc_log_info("irq_gpio = %d\n", fingerprint->irq_gpio);
    }

    fingerprint->moduleID_gpio = of_get_named_gpio(np, "fingerprint,moduleid_gpio", 0);
    if (!gpio_is_valid(fingerprint->moduleID_gpio))
    {
        fpc_log_err("failed to get moduleid gpio from device tree\n");
    }
    else
    {
        fpc_log_info("moduleID_gpio = %d\n", fingerprint->moduleID_gpio);
    }

    ret = of_property_read_string(dev->of_node, "fingerprint,vdd", &fingerprint_vdd);
    if (ret)
    {
        fpc_log_err("failed to get vdd from device tree\n");
    }

    if (fingerprint_vdd)
    {
        fingerprint->vdd = regulator_get(dev, fingerprint_vdd);
        if (IS_ERR(fingerprint->vdd))
        {
            fpc_log_err("failed to get vdd regulator\n");
        }
    }

    ret = of_property_read_string(dev->of_node, "fingerprint,avdd", &fingerprint_avdd);
    if (ret)
    {
        fpc_log_err("failed to get avdd from device tree, some project don't need this power\n");
    }

    if (fingerprint_avdd)
    {
        fingerprint->avdd = regulator_get(dev, fingerprint_avdd);
        if (IS_ERR(fingerprint->avdd))
        {
            fpc_log_err("failed to get avdd regulator\n");
        }
    }

    fingerprint->avdd_en_gpio = of_get_named_gpio(np, "fingerprint,avdd_en_gpio", 0);
    if (!gpio_is_valid(fingerprint->avdd_en_gpio))
    {
        fpc_log_err("failed to get avdd enable gpio from device tree, some project don't need this gpio\n");
    }
    else
    {
        fpc_log_info("avdd_en_gpio = %d\n", fingerprint->avdd_en_gpio);
    }

    fingerprint->vdd_en_gpio = of_get_named_gpio(np, "fingerprint,vdd_en_gpio", 0);
    if (!gpio_is_valid(fingerprint->vdd_en_gpio))
    {
        fpc_log_err("failed to get vdd enable gpio from device tree, some project don't need this gpio\n");
    }
    else
    {
        fpc_log_info("vdd_en_gpio = %d\n", fingerprint->vdd_en_gpio);
    }

    ret = of_property_read_u32(np, "fingerprint,irq_hold_time", &irq_hold_time);
    if (ret)
    {
       fpc_log_err("failed to get irq hold time from device tree ,some project don't use this time\n");
    }
    else
    {
       fingerprint->irq_hold_time = (int)irq_hold_time;
       fpc_log_info("irq_hold_time = %d\n", fingerprint->irq_hold_time);
    }
    return 0;
}

int fingerprint_gpio_reset(struct fp_data* fingerprint)
{
    int error = 0;
    int counter = FP_RESET_RETRIES;

    fpc_log_info("Enter!\n");

    while (counter)
    {
        counter--;

        gpio_set_value(fingerprint->rst_gpio, 1);
        udelay(FP_RESET_HIGH1_US);

        gpio_set_value(fingerprint->rst_gpio, 0);
        udelay(FP_RESET_LOW_US);

        gpio_set_value(fingerprint->rst_gpio, 1);
        udelay(FP_RESET_HIGH2_US);

        error = gpio_get_value(fingerprint->irq_gpio) ? 0 : -EIO;
        if (!error)
        {
            counter = 0;
        }
        else
        {
            fpc_log_err("irq expected high, got low, retrying ...\n");
        }
    }
    return error;
}

static int fingerprint_reset_init(struct fp_data* fingerprint)
{
    int error = 0;

    if (gpio_is_valid(fingerprint->rst_gpio))
    {
        fpc_log_info("Assign RESET -> GPIO%d\n", fingerprint->rst_gpio);
        error = gpio_request(fingerprint->rst_gpio, "fingerprint_reset");
        if (error)
        {
            fpc_log_err("gpio_request (reset) failed\n");
            return error;
        }

        error = gpio_direction_output(fingerprint->rst_gpio, 0);
        if (error)
        {
            fpc_log_err("gpio_direction_output (reset) failed\n");
            return error;
        }
    }
    else
    {
        fpc_log_info("Using soft reset\n");
    }

    return error;
}

static int fingerprint_irq_init(struct fp_data* fingerprint)
{
    int error = 0;

    if (gpio_is_valid(fingerprint->irq_gpio))
    {
        fpc_log_info("Assign IRQ -> GPIO%d\n", fingerprint->irq_gpio);
        error = gpio_request(fingerprint->irq_gpio, "fingerprint_irq");
        if (error)
        {
            fpc_log_err("gpio_request (irq) failed\n");
            return error;
        }

        error = gpio_direction_input(fingerprint->irq_gpio);
        if (error)
        {
            fpc_log_err("gpio_direction_input (irq) failed\n");
            return error;
        }
    }
    else
    {
        fpc_log_err("invalid irq gpio\n");
        return -EINVAL;
    }

    fingerprint->irq = gpio_to_irq(fingerprint->irq_gpio);
    if (fingerprint->irq < 0)
    {
        fpc_log_err("gpio_to_irq failed\n");
        error = fingerprint->irq;
        return error;
    }

    return error;
}

static int fingerprint_power_init(struct fp_data* fingerprint)
{
    int error = 0;

    fpc_log_info("Enter!\n");

    if (fingerprint->avdd)
    {
        error = regulator_set_voltage(fingerprint->avdd, 2800000, 3000000);
        if (error)
        {
            fpc_log_err("set avdd voltage failed\n");
            goto out_err;
        }
        error = regulator_enable(fingerprint->avdd);
        if (error)
        {
            fpc_log_err("enable avdd failed\n");
            goto out_err;
        }
    }
    else
    {
        fpc_log_err("fingerprint->avdd is NULL, some project don't need this power\n");
    }

    if (fingerprint->vdd)
    {
        error = regulator_set_voltage(fingerprint->vdd, 1800000, 1800000);
        if (error)
        {
            fpc_log_err("set vdd voltage failed\n");
            goto out_err;
        }
        error = regulator_enable(fingerprint->vdd);
        if (error)
        {
            fpc_log_err("enable vdd failed\n");
            goto out_err;
        }
    }
    else
    {
        fpc_log_err("fingerprint->vdd is NULL\n");
        return -EINVAL;
    }

    if (gpio_is_valid(fingerprint->avdd_en_gpio))
    {
        fpc_log_info("fingerprint_avdd_en_gpio -> GPIO%d\n", fingerprint->avdd_en_gpio);
        error = gpio_request(fingerprint->avdd_en_gpio, "fingerprint_avdd_en_gpio");
        if (error)
        {
            fpc_log_err("gpio_request (avdd_en_gpio) failed\n");
            goto out_err;
        }

        error = gpio_direction_output(fingerprint->avdd_en_gpio, 1);
        if (error)
        {
            fpc_log_err("gpio_direction_output (avdd_en_gpio) failed\n");
            goto out_err;
        }
        mdelay(100);
    }
    else
    {
        fpc_log_info("fingerprint->avdd_en_gpio is NULL, some project don't need this gpio\n");
    }

    if (gpio_is_valid(fingerprint->vdd_en_gpio))
    {
        fpc_log_info("fingerprint_vdd_en_gpio -> GPIO%d\n", fingerprint->vdd_en_gpio);
        error = gpio_request(fingerprint->vdd_en_gpio, "fingerprint_vdd_en_gpio");
        if (error)
        {
            fpc_log_err("gpio_request (vdd_en_gpio) failed\n");
            goto out_err;
        }

        error = gpio_direction_output(fingerprint->vdd_en_gpio, 1);
        if (error)
        {
            fpc_log_err("gpio_direction_output (vdd_en_gpio) failed\n");
            goto out_err;
        }
    }
    else
    {
        fpc_log_info("fingerprint->vdd_en_gpio is NULL, some project don't need this gpio\n");
    }

out_err:
    return error;
}

static int fingerprint_key_remap_reverse(int key)
{
    switch(key)
    {
        case EVENT_LEFT:
            key = EVENT_RIGHT;
            break;
        case EVENT_RIGHT:
            key = EVENT_LEFT;
            break;
        default:
            break;
    }

    return key;
}

static int fingerprint_key_remap_turn90(int key)
{
    switch(key)
    {
        case EVENT_LEFT:
            key = EVENT_UP;
            break;
        case EVENT_RIGHT:
            key = EVENT_DOWN;
            break;
        case EVENT_UP:
            key = EVENT_RIGHT;
            break;
        case EVENT_DOWN:
            key = EVENT_LEFT;
            break;
        default:
            break;
    }

    return key;
}

static int fingerprint_key_remap_turn180(int key)
{
    switch(key)
    {
        case EVENT_LEFT:
            key = EVENT_RIGHT;
            break;
        case EVENT_RIGHT:
            key = EVENT_LEFT;
            break;
        case EVENT_UP:
            key = EVENT_DOWN;
            break;
        case EVENT_DOWN:
            key = EVENT_UP;
            break;
        default:
            break;
    }

    return key;
}

static int fingerprint_key_remap_turn270(int key)
{
    switch(key)
    {
        case EVENT_LEFT:
            key = EVENT_DOWN;
            break;
        case EVENT_RIGHT:
            key = EVENT_UP;
            break;
        case EVENT_UP:
            key = EVENT_LEFT;
            break;
        case EVENT_DOWN:
            key = EVENT_RIGHT;
            break;
        default:
            break;
    }

    return key;
}

static int fingerprint_key_remap(const struct fp_data* fingerprint, int key)
{
    if(key != EVENT_RIGHT && key != EVENT_LEFT && key != EVENT_UP && key != EVENT_DOWN)
    {
        return key;
    }

    if(fingerprint->navigation_adjust1 == NAVIGATION_ADJUST_REVERSE)
    {
        key = fingerprint_key_remap_reverse(key);
    }

    switch (fingerprint->navigation_adjust2)
    {
        case NAVIGATION_ADJUST_TURN90:
            key = fingerprint_key_remap_turn90(key);
            break;
        case NAVIGATION_ADJUST_TURN180:
            key = fingerprint_key_remap_turn180(key);
            break;
        case NAVIGATION_ADJUST_TURN270:
            key = fingerprint_key_remap_turn270(key);
            break;
        default:
            break;
    }

    return key;
}

static void fingerprint_input_report(struct fp_data* fingerprint, int key)
{
    key = fingerprint_key_remap(fingerprint, key);
    fpc_log_info("key = %d\n", key);
    input_report_key(fingerprint->input_dev, key, 1);
    input_sync(fingerprint->input_dev);
    input_report_key(fingerprint->input_dev, key, 0);
    input_sync(fingerprint->input_dev);
}

static int fingerprint_open(struct inode* inode, struct file* file)
{
    struct fp_data* fingerprint;
    fpc_log_info("Enter!\n");
    fingerprint = container_of(inode->i_cdev, struct fp_data, cdev);
    file->private_data = fingerprint;
    return 0;
}

static int fingerprint_get_irq_status(struct fp_data* fingerprint)
{
    int status = 0;
    status = gpio_get_value_cansleep(fingerprint->irq_gpio);
    return status;
}

static long fingerprint_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{

    int error = 0;

    struct fp_data* fingerprint;
    void __user* argp = (void __user*)arg;
    int key;
    int status;
    unsigned int sensor_id;
    fingerprint = (struct fp_data*)file->private_data;

    if (_IOC_TYPE(cmd) != FP_IOC_MAGIC)
    { return -ENOTTY; }

    switch (cmd)
    {
        case FP_IOC_CMD_ENABLE_IRQ:
            fpc_log_info("FP_IOC_CMD_ENABLE_IRQ\n");
            enable_irq(gpio_to_irq(fingerprint->irq_gpio ));
            break;

        case FP_IOC_CMD_DISABLE_IRQ:
            fpc_log_info("FP_IOC_CMD_DISABLE_IRQ\n");
            disable_irq(gpio_to_irq(fingerprint->irq_gpio ));
            break;

        case FP_IOC_CMD_SEND_UEVENT:
            if (copy_from_user(&key, argp, sizeof(key)))
            {
                fpc_log_err("copy_from_user failed");
                return -EFAULT;
            }

            fingerprint_input_report(fingerprint, key);
            fpc_log_info("FP_IOC_CMD_SEND_UEVENT\n");
            break;

        case FP_IOC_CMD_GET_IRQ_STATUS:

            status = fingerprint_get_irq_status(fingerprint);

            error = copy_to_user(argp, &status, sizeof(status));

            if (error)
            {
                fpc_log_err("copy_to_user failed, error = %d", error);
                return -EFAULT;
            }

            fpc_log_info("FP_IOC_CMD_GET_IRQ_STATUS, status = %d\n", status);
            break;

        case FP_IOC_CMD_SET_WAKELOCK_STATUS:
            if (copy_from_user(&key, argp, sizeof(key)))
            {
                fpc_log_err("copy_from_user failed");
                return -EFAULT;
            }

            if (key == 1)
            {
                fingerprint->wakeup_enabled=true;
            }
            else
            {
                fingerprint->wakeup_enabled=false;
            }

            fpc_log_info("FP_IOC_CMD_SET_WAKELOCK_STATUS key = %d\n", key);

            break;

        case FP_IOC_CMD_SEND_SENSORID:
            if (copy_from_user(&sensor_id, argp, sizeof(sensor_id)))
            {
                fpc_log_err("copy_from_user failed\n");
                return -EFAULT;
            }

            fingerprint->sensor_id = sensor_id;
            fpc_log_info("sensor_id = %x\n", sensor_id);
            break;

        default:
            fpc_log_err("error = -EFAULT\n");
            error = -EFAULT;
            break;
    }

    return error;
}

static int fingerprint_release(struct inode* inode, struct file* file)
{
    fpc_log_info("Enter!\n");
    return 0;
}

static int fingerprint_get_module_info(struct fp_data* fingerprint)
{
    int error = 0;
    int pd_value = 0, pu_value = 0;

    error = gpio_request(fingerprint->moduleID_gpio, "fingerprint_moduleID");
    if (error)
    {
        fpc_log_err("gpio_request (moduleID) failed\n");
        return error;
    }

    fingerprint->pctrl = devm_pinctrl_get(&fingerprint->spi->dev);
    if (IS_ERR(fingerprint->pctrl))
    {
        fpc_log_err("failed to get moduleid pin\n");
        error = -EINVAL;
        return error;
    }

    fingerprint->pins_default = pinctrl_lookup_state(fingerprint->pctrl, "default");
    if (IS_ERR(fingerprint->pins_default))
    {
        fpc_log_err("failed to lookup state fingerprint_moduleid_default\n");
        error = -EINVAL;
        goto error_pinctrl_put;
    }

    fingerprint->pins_idle = pinctrl_lookup_state(fingerprint->pctrl, "idle");
    if (IS_ERR(fingerprint->pins_idle))
    {
        fpc_log_err("failed to lookup state fingerprint_moduleid_idle\n");
        error = -EINVAL;
        goto error_pinctrl_put;
    }

    error = pinctrl_select_state(fingerprint->pctrl, fingerprint->pins_default);
    if (error < 0)
    {
        fpc_log_err("failed to select state fingerprint_moduleid_default\n");
        error = -EINVAL;
        goto error_pinctrl_put;
    }

    udelay(10);

    pu_value = gpio_get_value_cansleep(fingerprint->moduleID_gpio);
    fpc_log_info("PU module id gpio = %d \n", pu_value);


    error = pinctrl_select_state(fingerprint->pctrl, fingerprint->pins_idle);
    if (error < 0)
    {
        fpc_log_err("failed to select state fingerprint_moduleid_idle\n");
        error = -EINVAL;
        return error;
    }

    udelay(10);

    pd_value = gpio_get_value_cansleep(fingerprint->moduleID_gpio);
    fpc_log_info("PD module id gpio = %d \n", pd_value);

    if (pu_value == pd_value)
    {
        if (pu_value == 1)
        {
            fingerprint->module_vendor_info = MODULEID_HIGH;
            fpc_log_info("fingerprint moduleID pin is HIGH\n");
        }

        if (pd_value == 0)
        {
            fingerprint->module_vendor_info = MODULEID_LOW;
            fpc_log_info("fingerprint moduleID pin is LOW\n");
        }
    }
    else
    {
        fingerprint->module_vendor_info = MODULEID_FLOATING;
        fpc_log_info("fingerprint moduleID pin is FLOATING\n");
    }

    return error;

error_pinctrl_put:
    devm_pinctrl_put(fingerprint->pctrl);
    return error;
}

static const struct file_operations fingerprint_fops =
{
    .owner			= THIS_MODULE,
    .open			= fingerprint_open,
    .release		= fingerprint_release,
    .unlocked_ioctl	= fingerprint_ioctl,
};

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self, unsigned long event, void *data)
{
    struct fb_event *evdata = data;
    int *blank;
    struct fp_data *fingerprint = container_of(self, struct fp_data, fb_notify);

    if (evdata && evdata->data && fingerprint)
    {
        if (event == FB_EVENT_BLANK)
        {
            blank = evdata->data;
            if (*blank == FB_BLANK_UNBLANK)
                atomic_set(&fingerprint->state, fp_LCD_UNBLANK);
            else if (*blank == FB_BLANK_POWERDOWN)
                atomic_set(&fingerprint->state, fp_LCD_POWEROFF);
        }
    }

    return 0;
}
#endif

static int fingerprint_probe(struct spi_device* spi)
{
    struct device* dev = &spi->dev;
    int error = 0;
    int irqf;
    struct device_node* np = dev->of_node;
    struct fp_data* fingerprint = devm_kzalloc(dev, sizeof(*fingerprint), GFP_KERNEL);

    if (!fingerprint)
    {
        fpc_log_err("failed to allocate memory for struct fp_data\n");
        error = -ENOMEM;
        goto exit;
    }

#if defined (CONFIG_HUAWEI_DSM)
    if(!fingerprint_dclient)
    {
        fingerprint_dclient = dsm_register_client(&dsm_fingerprint);
    }
#endif

    fpc_log_info("fingerprint driver v3.0 for Android M\n");

    fingerprint->dev = dev;
    dev_set_drvdata(dev, fingerprint);
    fingerprint->spi = spi;

    if (!np)
    {
        fpc_log_err("dev->of_node not found\n");
        error = -EINVAL;
        goto exit;
    }

    fingerprint_get_dts_data(&spi->dev, fingerprint);

#if defined(CONFIG_FB)
    fingerprint->fb_notify.notifier_call = NULL;
#endif
    atomic_set(&fingerprint->state, fp_UNINIT);

    error = fingerprint_irq_init(fingerprint);
    if (error)
    {
        fpc_log_err("fingerprint_irq_init failed, error = %d\n", error);
        goto exit;
    }

    fingerprint_get_navigation_adjustvalue(&spi->dev, fingerprint);

    error = fingerprint_reset_init(fingerprint);
    if (error)
    {
        fpc_log_err("fingerprint_reset_init failed, error = %d\n", error);
        goto exit;
    }

    error = fingerprint_power_init(fingerprint);
    if (error)
    {
        fpc_log_err("fingerprint_power_init failed, error = %d\n", error);
        goto exit;
    }

    error = fingerprint_get_module_info(fingerprint);
    if (error < 0)
    {
        fpc_log_err("unknow vendor info, error = %d\n", error);
    }

    fingerprint->class = class_create(THIS_MODULE, FP_CLASS_NAME);

    error = alloc_chrdev_region(&fingerprint->devno, 0, 1, FP_DEV_NAME);
    if (error)
    {
        fpc_log_err("alloc_chrdev_region failed, error = %d\n", error);
        goto exit;
    }

    fingerprint->device = device_create(fingerprint->class, NULL, fingerprint->devno,
                                    NULL, "%s", FP_DEV_NAME);

    cdev_init(&fingerprint->cdev, &fingerprint_fops);
    fingerprint->cdev.owner = THIS_MODULE;

    error = cdev_add(&fingerprint->cdev, fingerprint->devno, 1);
    if (error)
    {
        fpc_log_err("cdev_add failed, error = %d\n", error);
        goto exit;
    }

    fingerprint->input_dev = devm_input_allocate_device(dev);
    if (!fingerprint->input_dev)
    {
        error = -ENOMEM;
        fpc_log_err("devm_input_allocate_device failed, error = %d\n", error);
        goto exit;
    }

    fingerprint->input_dev->name = "fingerprint";
    /* Also register the key for wake up */
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_UP);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_DOWN);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_LEFT);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_RIGHT);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_HOLD);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_CLICK);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_HOLD);
    input_set_capability(fingerprint->input_dev, EV_KEY, EVENT_DCLICK);
    set_bit(EV_KEY, fingerprint->input_dev->evbit);
    set_bit(EVENT_UP, fingerprint->input_dev->evbit);
    set_bit(EVENT_DOWN, fingerprint->input_dev->evbit);
    set_bit(EVENT_LEFT, fingerprint->input_dev->evbit);
    set_bit(EVENT_RIGHT, fingerprint->input_dev->evbit);
    set_bit(EVENT_CLICK, fingerprint->input_dev->evbit);
    set_bit(EVENT_HOLD, fingerprint->input_dev->evbit);
    set_bit(EVENT_DCLICK, fingerprint->input_dev->evbit);

    error = input_register_device(fingerprint->input_dev);
    if (error)
    {
        fpc_log_err("input_register_device failed, error = %d\n", error);
        goto exit;
    }

    fingerprint->wakeup_enabled = false;

    fingerprint->pf_dev= platform_device_alloc(FP_DEV_NAME, -1);
    if (!fingerprint->pf_dev)
    {
        error = -ENOMEM;
        fpc_log_err("platform_device_alloc failed, error = %d\n", error);
        goto exit;
    }

    error = platform_device_add(fingerprint->pf_dev);
    if (error)
    {
        fpc_log_err("platform_device_add failed, error = %d\n", error);
        platform_device_del(fingerprint->pf_dev);
        goto exit;
    }
    else
    {
        dev_set_drvdata(&fingerprint->pf_dev->dev, fingerprint);

        error = sysfs_create_group(&fingerprint->pf_dev->dev.kobj, &attribute_group);
        if (error)
        {
            fpc_log_err("sysfs_create_group failed, error = %d\n", error);
            goto exit;
        }
   }

    irqf = IRQF_TRIGGER_RISING | IRQF_ONESHOT | IRQF_NO_SUSPEND;

    device_init_wakeup(dev, 1);
    wake_lock_init(&fingerprint->ttw_wl, WAKE_LOCK_SUSPEND, "fpc_ttw_wl");

    mutex_init(&fingerprint->lock);
    error = devm_request_threaded_irq(dev, fingerprint->irq,
                                   NULL, fingerprint_irq_handler, irqf,
                                   "fingerprint", fingerprint);

    if (error)
    {
        fpc_log_err("failed to request irq %d\n", fingerprint->irq);
        goto exit;
    }

    //fpc_log_info("requested irq %d\n", fingerprint->irq);

    /* Request that the interrupt should be wakeable */
    enable_irq_wake(fingerprint->irq);
    fingerprint->wakeup_enabled = true;
    fingerprint->snr_stat = 0;


#if defined(CONFIG_FB)
    if (fingerprint->fb_notify.notifier_call == NULL)
    {
        fingerprint->fb_notify.notifier_call = fb_notifier_callback;
        fb_register_client(&fingerprint->fb_notify);
    }
#endif

    fpc_log_info("fingerprint probe is successful!\n");
    return error;

exit:
    fpc_log_info("fingerprint probe failed!\n");
#if defined (CONFIG_HUAWEI_DSM)
    if (error && !dsm_client_ocuppy(fingerprint_dclient))
    {
        dsm_client_record(fingerprint_dclient,"fingerprint_probe failed, error = %d\n", error);
        dsm_client_notify(fingerprint_dclient, DSM_FINGERPRINT_PROBE_FAIL_ERROR_NO);
    }
#endif
    return error;
}

static int fingerprint_remove(struct spi_device* spi)
{
    struct  fp_data* fingerprint = dev_get_drvdata(&spi->dev);
    fpc_log_info("Enter!\n");
    sysfs_remove_group(&fingerprint->pf_dev->dev.kobj, &attribute_group);
    cdev_del(&fingerprint->cdev);
    unregister_chrdev_region(fingerprint->devno, 1);
    input_free_device(fingerprint->input_dev);
    mutex_destroy(&fingerprint->lock);
    wake_lock_destroy(&fingerprint->ttw_wl);
#if defined(CONFIG_FB)
    if (fingerprint->fb_notify.notifier_call != NULL)
    {
        fingerprint->fb_notify.notifier_call = NULL;
        fb_unregister_client(&fingerprint->fb_notify);
    }
#endif
    return 0;
}

static int fingerprint_suspend(struct device* dev)
{
    fpc_log_info("Enter!\n");
    return 0;
}

static int fingerprint_resume(struct device* dev)
{
    fpc_log_info("Enter!\n");
    return 0;
}

static const struct dev_pm_ops fingerprint_pm =
{
    .suspend = fingerprint_suspend,
    .resume = fingerprint_resume
};

static struct of_device_id fingerprint_of_match[] =
{
    { .compatible = "fpc,fingerprint", },
    {}
};

MODULE_DEVICE_TABLE(of, fingerprint_of_match);

static struct spi_driver fingerprint_driver =
{
    .driver = {
        .name	= "fingerprint",
        .owner	= THIS_MODULE,
        .of_match_table = fingerprint_of_match,
        .pm = &fingerprint_pm
    },
    .probe  = fingerprint_probe,
    .remove = fingerprint_remove
};

static int __init fingerprint_init(void)
{
    if (spi_register_driver(&fingerprint_driver))
        return -EINVAL;

    return 0;
}

static void __exit fingerprint_exit(void)
{
    fpc_log_info("Enter!\n");
    spi_unregister_driver(&fingerprint_driver);
}

module_init(fingerprint_init);
module_exit(fingerprint_exit);

MODULE_LICENSE("GPL v2");
