/*
 * Copyright (c) 2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/leds-aw2015.h>

/* register address */
#define AW2015_REG_RESET            0x00
#define AW2015_REG_GCR              0x01
#define AW2015_REG_STATUS           0x02
#define AW2015_REG_IMAX             0x03
#define AW2015_REG_LCFG1            0x04
#define AW2015_REG_LEDEN            0x07
#define AW2015_REG_PAT_RUN          0x09
#define AW2015_REG_ILED1            0x10
#define AW2015_REG_PWM1             0x1C
#define AW2015_REG_PAT1_T1          0x30
#define AW2015_REG_PAT1_T2          0x31

/* register bits */
#define AW2015_CHIPID               0x31
#define AW2015_LED_RESET_MASK           0x55
#define AW2015_LED_CHIP_DISABLE         0x00
#define AW2015_LED_CHIP_ENABLE_MASK     0x01
#define AW2015_LED_CHARGE_DISABLE_MASK      0x02
#define AW2015_LED_BREATHE_MODE_MASK        0x01
#define AW2015_LED_PWM_MODE_MASK        0x00
#define AW2015_LED_BREATHE_PWM_MASK     0xFF
#define AW2015_LED_ON_PWM_MASK          0xFF
#define AW2015_LED_FADEIN_MODE_MASK     0x02
#define AW2015_LED_FADEOUT_MODE_MASK        0x04

#define MAX_RISE_TIME_MS            15
#define MAX_HOLD_TIME_MS            15
#define MAX_FALL_TIME_MS            15
#define MAX_OFF_TIME_MS             15

/*LED TIME MS
    TISRE    TIME    TISRE    TIME
    0000     0s      1000     2.1s
    0001     0.13s   1001     2.6s
    0010     0.26s   1010     3.1s
    0011     0.38s   1011     4.2s
    0100     0.51s   1100     5.2s
    0101     0.77s   1101     6.2s
    0110     1.04s   1110     7.3s
    0111     1.6s    1111     8.3s
*/
/* add blink mode 3 time for huawei when the battery is low on charge */
#define BLINK3_RISE_TIME_MS     4
#define BLINK3_HOLD_TIME_MS     1
#define BLINK3_FALL_TIME_MS     4
#define BLINK3_OFF_TIME_MS      11

struct aw2015_led {
    struct i2c_client *client;
    struct led_classdev cdev;
    struct aw2015_platform_data *pdata;
    struct work_struct brightness_work;
    struct mutex lock;
    int num_leds;
    int id;
    bool chipen;
};

static int aw2015_write(struct aw2015_led *led, u8 reg, u8 val)
{
    s32 ret, retry_times = 0;

    do {
        ret = i2c_smbus_write_byte_data(led->client, reg, val);
        retry_times ++;
        if(retry_times == 5)
            break;
    }while (ret < 0);

    return ret;
}

static int aw2015_read(struct aw2015_led *led, u8 reg, u8 *val)
{
    s32 ret, retry_times = 0;

    do{
        ret = i2c_smbus_read_byte_data(led->client, reg);
        retry_times ++;
        if(retry_times == 5)
            break;
    }while (ret < 0);

    if (ret < 0)
        return ret;

    *val = ret;
    return 0;
}


static void aw2015_brightness_work(struct work_struct *work)
{
    struct aw2015_led *led = container_of(work, struct aw2015_led,
                    brightness_work);
    u8 val;
    int io_value;

    mutex_lock(&led->pdata->led->lock);

    /* enable aw2015 if disabled */
    aw2015_read(led, AW2015_REG_GCR, &val);
    if (!(val&0x01)) {
        aw2015_write(led, AW2015_REG_GCR, AW2015_LED_CHIP_ENABLE_MASK | AW2015_LED_CHARGE_DISABLE_MASK);
    }

    if (led->cdev.brightness > 0) {
        if (led->cdev.brightness > led->cdev.max_brightness)
            led->cdev.brightness = led->cdev.max_brightness;
        aw2015_write(led, AW2015_REG_IMAX , led->pdata->max_current);
        aw2015_write(led, AW2015_REG_LCFG1 + led->id, AW2015_LED_PWM_MODE_MASK);
        aw2015_write(led, AW2015_REG_ILED1 + led->id, led->cdev.brightness);
        aw2015_write(led, AW2015_REG_PWM1 + led->id, AW2015_LED_ON_PWM_MASK);
        io_value = aw2015_read(led, AW2015_REG_LEDEN, &val);
        dev_err(&led->pdata->led->client->dev,"aw2015 brightness on read val %d, result %d",val, io_value);
        io_value = aw2015_write(led, AW2015_REG_LEDEN, val | (1 << led->id));
        dev_err(&led->pdata->led->client->dev,"aw2015 brightness on write led %d, val %d, result %d",led->id, (val | (1<< led->id)), io_value);
    } else {
        io_value = aw2015_read(led, AW2015_REG_LEDEN, &val);
        dev_err(&led->pdata->led->client->dev,"aw2015 LED off read led %d, val %d, result %d",led->id, val, io_value);
        io_value = aw2015_write(led, AW2015_REG_LEDEN, val & (~((unsigned)1 << led->id)));
        dev_err(&led->pdata->led->client->dev,"aw2015 LED off  write led %d, val %d, result %d",led->id, (val & (~((unsigned)1 << led->id))), io_value);
    }

    io_value = aw2015_read(led, AW2015_REG_LEDEN, &val);
    dev_err(&led->pdata->led->client->dev,"aw2015 final read led enable %d, result %d",val, io_value);
    /*
     * If value in AW2015_REG_LEDEN is 0, it means the RGB leds are
     * all off. So we need to power it off.
     */
    if (val == 0) {
        aw2015_write(led, AW2015_REG_GCR, AW2015_LED_CHIP_DISABLE);
        mutex_unlock(&led->pdata->led->lock);
        return;
    }

    mutex_unlock(&led->pdata->led->lock);
}

static void aw2015_led_blink_set(struct aw2015_led *led, unsigned long blinking)
{
    u8 val;

    /* enable regulators if they are disabled */
    /* enable aw2015 if disabled */
    aw2015_read(led, AW2015_REG_GCR, &val);
    if (!(val&0x01)) {
        aw2015_write(led, AW2015_REG_GCR, AW2015_LED_CHIP_ENABLE_MASK | AW2015_LED_CHARGE_DISABLE_MASK);
    }

    led->cdev.brightness = blinking ? led->cdev.max_brightness : 0;

    if (blinking > 0) {
       /* add blink mode 3 for huawei when the battery is low on charge */
        if (3 == blinking){
            aw2015_write(led, AW2015_REG_GCR, AW2015_LED_CHIP_ENABLE_MASK | AW2015_LED_CHARGE_DISABLE_MASK);
            aw2015_write(led, AW2015_REG_LCFG1 + led->id, AW2015_LED_BREATHE_MODE_MASK);
            aw2015_write(led, AW2015_REG_ILED1 + led->id, led->cdev.brightness);
            aw2015_write(led, AW2015_REG_PWM1 + led->id, AW2015_LED_BREATHE_PWM_MASK);
            aw2015_write(led, AW2015_REG_PAT1_T1 + led->id*5,
                                    (BLINK3_RISE_TIME_MS << 4 | BLINK3_HOLD_TIME_MS));
            aw2015_write(led, AW2015_REG_PAT1_T2 + led->id*5,
                                    (BLINK3_FALL_TIME_MS << 4 | BLINK3_OFF_TIME_MS));

            aw2015_read(led, AW2015_REG_LEDEN, &val);
            aw2015_write(led, AW2015_REG_LEDEN, val | (1 << led->id));

            aw2015_write(led, AW2015_REG_PAT_RUN, (1 << led->id));
        }else{
	        aw2015_write(led, AW2015_REG_GCR, AW2015_LED_CHIP_ENABLE_MASK | AW2015_LED_CHARGE_DISABLE_MASK);
	        aw2015_write(led, AW2015_REG_LCFG1 + led->id, AW2015_LED_BREATHE_MODE_MASK);
	        aw2015_write(led, AW2015_REG_ILED1 + led->id, led->cdev.brightness);
	        aw2015_write(led, AW2015_REG_PWM1 + led->id, AW2015_LED_BREATHE_PWM_MASK);
	        aw2015_write(led, AW2015_REG_PAT1_T1 + led->id*5,
	                    (led->pdata->rise_time_ms << 4 | led->pdata->hold_time_ms));
	        aw2015_write(led, AW2015_REG_PAT1_T2 + led->id*5,
	                    (led->pdata->fall_time_ms << 4 | led->pdata->off_time_ms));

	        aw2015_read(led, AW2015_REG_LEDEN, &val);
	        aw2015_write(led, AW2015_REG_LEDEN, val | (1 << led->id));

	        aw2015_write(led, AW2015_REG_PAT_RUN, (1 << led->id));
        }
    } else {
        aw2015_read(led, AW2015_REG_LEDEN, &val);
        aw2015_write(led, AW2015_REG_LEDEN, val & (~((unsigned)1 << led->id)));
    }

    aw2015_read(led, AW2015_REG_LEDEN, &val);
    /*
     * If value in AW2015_REG_LEDEN is 0, it means the RGB leds are
     * all off. So we need to power it off.
     */
    if (val == 0) {
        aw2015_write(led, AW2015_REG_GCR, AW2015_LED_CHIP_DISABLE);
        mutex_unlock(&led->pdata->led->lock);
        return;
    }
}

static void aw2015_set_brightness(struct led_classdev *cdev,
                 enum led_brightness brightness)
{
    struct aw2015_led *led = container_of(cdev, struct aw2015_led, cdev);

    led->cdev.brightness = brightness;

    schedule_work(&led->brightness_work);
}

static ssize_t aw2015_store_blink(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    unsigned long blinking;
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw2015_led *led =
            container_of(led_cdev, struct aw2015_led, cdev);
    ssize_t ret = -EINVAL;

    ret = kstrtoul(buf, 10, &blinking);
    if (ret)
        return ret;
    mutex_lock(&led->pdata->led->lock);
    aw2015_led_blink_set(led, blinking);
    mutex_unlock(&led->pdata->led->lock);

    return len;
}

static ssize_t aw2015_led_time_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw2015_led *led =
            container_of(led_cdev, struct aw2015_led, cdev);

    return snprintf(buf, PAGE_SIZE, "%d %d %d %d\n",
            led->pdata->rise_time_ms, led->pdata->hold_time_ms,
            led->pdata->fall_time_ms, led->pdata->off_time_ms);
}

static ssize_t aw2015_led_time_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw2015_led *led =
            container_of(led_cdev, struct aw2015_led, cdev);
    int rc, rise_time_ms, hold_time_ms, fall_time_ms, off_time_ms;

    rc = sscanf(buf, "%d %d %d %d",
            &rise_time_ms, &hold_time_ms,
            &fall_time_ms, &off_time_ms);

    mutex_lock(&led->pdata->led->lock);
    led->pdata->rise_time_ms = (rise_time_ms > MAX_RISE_TIME_MS) ?
                MAX_RISE_TIME_MS : rise_time_ms;
    led->pdata->hold_time_ms = (hold_time_ms > MAX_HOLD_TIME_MS) ?
                MAX_HOLD_TIME_MS : hold_time_ms;
    led->pdata->fall_time_ms = (fall_time_ms > MAX_FALL_TIME_MS) ?
                MAX_FALL_TIME_MS : fall_time_ms;
    led->pdata->off_time_ms = (off_time_ms > MAX_OFF_TIME_MS) ?
                MAX_OFF_TIME_MS : off_time_ms;
    aw2015_led_blink_set(led, 1);
    mutex_unlock(&led->pdata->led->lock);
    return rc; //chang from len to rc
}

static ssize_t aw2015_reg_show(struct device *dev,
                struct device_attribute *attr, char *buf)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw2015_led *led =
            container_of(led_cdev, struct aw2015_led, cdev);
    unsigned char i, reg_val;
    ssize_t len = 0;

    for(i=0; i<0x3F; i++)
    {
        aw2015_read(led, i, &reg_val);
        len += snprintf(buf+len, PAGE_SIZE-len, "reg%2x=0x%2x, ", i, reg_val);
    }

    return len;
}

static ssize_t aw2015_reg_store(struct device *dev,
                 struct device_attribute *attr,
                 const char *buf, size_t len)
{
    struct led_classdev *led_cdev = dev_get_drvdata(dev);
    struct aw2015_led *led =
            container_of(led_cdev, struct aw2015_led, cdev);

    unsigned int databuf[2];

    if(2 == sscanf(buf, "%x %x", &databuf[0], &databuf[1]))
    {
        aw2015_write(led, databuf[0] , databuf[1]);
    }
    return len;
}

static DEVICE_ATTR(blink, 0664, NULL, aw2015_store_blink);
static DEVICE_ATTR(led_time, 0664, aw2015_led_time_show, aw2015_led_time_store);
static DEVICE_ATTR(reg, 0664, aw2015_reg_show, aw2015_reg_store);

static struct attribute *aw2015_led_attributes[] = {
    &dev_attr_blink.attr,
    &dev_attr_led_time.attr,
    &dev_attr_reg.attr,
    NULL,
};

static struct attribute_group aw2015_led_attr_group = {
    .attrs = aw2015_led_attributes
};
static int aw_2015_check_chipid(struct aw2015_led *led)
{
    u8 val;
    u8 cnt;
    int ret = -1;

    for(cnt = 5; cnt > 0; cnt --)
    {
        aw2015_read(led, AW2015_REG_RESET, &val);
        dev_err(&led->client->dev,"AW2015 chip id %0x",val);
        if (val == AW2015_CHIPID){
            ret = aw2015_write(led, AW2015_REG_RESET, AW2015_LED_RESET_MASK);
            if (0 > ret){
                return -EINVAL;
            }
            dev_info(&led->client->dev, "[aw2015_led_shutdown]: soft reset\n");
            return 0;
        }
    }

    return -EINVAL;
}

static int aw2015_led_err_handle(struct aw2015_led *led_array,
                int parsed_leds)
{
    int i;
    /*
     * If probe fails, cannot free resource of all LEDs, only free
     * resources of LEDs which have allocated these resource really.
     */
    for (i = 0; i < parsed_leds; i++) {
        sysfs_remove_group(&led_array[i].cdev.dev->kobj,
                &aw2015_led_attr_group);
        led_classdev_unregister(&led_array[i].cdev);
        cancel_work_sync(&led_array[i].brightness_work);
        devm_kfree(&led_array->client->dev, led_array[i].pdata);
        led_array[i].pdata = NULL;
    }
    return i;
}

static int aw2015_led_parse_child_node(struct aw2015_led *led_array,
                struct device_node *node)
{
    struct aw2015_led *led;
    struct device_node *temp;
    struct aw2015_platform_data *pdata;
    int rc = 0, parsed_leds = 0;

    for_each_child_of_node(node, temp) {
        led = &led_array[parsed_leds];
        led->client = led_array->client;

        pdata = devm_kzalloc(&led->client->dev,
                sizeof(struct aw2015_platform_data),
                GFP_KERNEL);
        if (!pdata) {
            dev_err(&led->client->dev,
                "Failed to allocate memory\n");
            goto free_err;
        }
        pdata->led = led_array;
        led->pdata = pdata;

        rc = of_property_read_string(temp, "aw2015,name",
            &led->cdev.name);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading led name, rc = %d\n", rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,id",
            &led->id);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading id, rc = %d\n", rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,max-brightness",
            &led->cdev.max_brightness);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading max-brightness, rc = %d\n",
                rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,max-current",
            &led->pdata->max_current);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading max-current, rc = %d\n", rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,rise-time-ms",
            &led->pdata->rise_time_ms);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading rise-time-ms, rc = %d\n", rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,hold-time-ms",
            &led->pdata->hold_time_ms);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading hold-time-ms, rc = %d\n", rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,fall-time-ms",
            &led->pdata->fall_time_ms);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading fall-time-ms, rc = %d\n", rc);
            goto free_pdata;
        }

        rc = of_property_read_u32(temp, "aw2015,off-time-ms",
            &led->pdata->off_time_ms);
        if (rc < 0) {
            dev_err(&led->client->dev,
                "Failure reading off-time-ms, rc = %d\n", rc);
            goto free_pdata;
        }

/*lint -save -esym(550, aw2015_brightness_work) -esym(550, INIT_WORK) -esym(550, __key)*/
        INIT_WORK(&led->brightness_work, aw2015_brightness_work);
/*lint -restore*/

        led->cdev.brightness_set = aw2015_set_brightness;

        rc = led_classdev_register(&led->client->dev, &led->cdev);
        if (rc) {
            dev_err(&led->client->dev,
                "unable to register led %d,rc=%d\n",
                led->id, rc);
            goto free_pdata;
        }

        rc = sysfs_create_group(&led->cdev.dev->kobj,
                &aw2015_led_attr_group);
        if (rc) {
            dev_err(&led->client->dev, "led sysfs rc: %d\n", rc);
            goto free_class;
        }
        parsed_leds++;
    }

    return 0;

free_class:
    aw2015_led_err_handle(led_array, parsed_leds);
    led_classdev_unregister(&led_array[parsed_leds].cdev);
    cancel_work_sync(&led_array[parsed_leds].brightness_work);
    devm_kfree(&led->client->dev, led_array[parsed_leds].pdata);
    led_array[parsed_leds].pdata = NULL;
    return rc;

free_pdata:
    aw2015_led_err_handle(led_array, parsed_leds);
    devm_kfree(&led->client->dev, led_array[parsed_leds].pdata);
    return rc;

free_err:
    aw2015_led_err_handle(led_array, parsed_leds);
    return rc;
}

static int aw2015_led_probe(struct i2c_client *client,
               const struct i2c_device_id *id)
{
    struct aw2015_led *led_array;
    struct device_node *node;
    int ret, num_leds = 0;
    node = client->dev.of_node;
    if (node == NULL)
        return -EINVAL;

    num_leds = of_get_child_count(node);

    if (!num_leds)
        return -EINVAL;

    led_array = devm_kzalloc(&client->dev,
            (sizeof(struct aw2015_led) * num_leds), GFP_KERNEL);
    if (!led_array)
        return -ENOMEM;

    led_array->client = client;
    led_array->num_leds = num_leds;

    mutex_init(&led_array->lock);

    ret = aw_2015_check_chipid(led_array);
    if (ret) {
        dev_err(&client->dev, "Check chip id error\n");
        goto free_led_arry;
    }

    ret = aw2015_led_parse_child_node(led_array, node);
    if (ret) {
        dev_err(&client->dev, "parsed node error\n");
        goto free_led_arry;
    }

    i2c_set_clientdata(client, led_array);
    return 0;

free_led_arry:
    mutex_destroy(&led_array->lock);
    devm_kfree(&client->dev, led_array);
   // led_array = NULL; //gjc
    return ret;
}

static int aw2015_led_remove(struct i2c_client *client)
{
    struct aw2015_led *led_array = i2c_get_clientdata(client);
    int i, parsed_leds = led_array->num_leds;

    for (i = 0; i < parsed_leds; i++) {
        sysfs_remove_group(&led_array[i].cdev.dev->kobj,
                &aw2015_led_attr_group);
        led_classdev_unregister(&led_array[i].cdev);
        cancel_work_sync(&led_array[i].brightness_work);
        devm_kfree(&client->dev, led_array[i].pdata);
        led_array[i].pdata = NULL;
    }
    mutex_destroy(&led_array->lock);
    devm_kfree(&client->dev, led_array);
 //   led_array = NULL;
    return 0;
}
/*lint -save -esym(119, aw2015_led_shutdown) -esym(119, aw2015_write)*/
static int aw2015_led_shutdown(struct i2c_client *client)
{
    struct aw2015_led *led_array = i2c_get_clientdata(client);
    int ret = -1;
    ret = aw2015_write(led_array, AW2015_REG_RESET, AW2015_LED_RESET_MASK);
    if (0 > ret){
        return -EINVAL;
    }
    dev_info(&client->dev, "[aw2015_led_shutdown]: soft reset\n");
    return ret;
}
/*lint -restore*/
static const struct i2c_device_id aw2015_led_id[] = {
    {"aw2015_led", 0},
    {},
};

MODULE_DEVICE_TABLE(i2c, aw2015_led_id);

static struct of_device_id aw2015_match_table[] = {
    { .compatible = "awinic,aw2015_led",},
    { },
};

static struct i2c_driver aw2015_led_driver = {
    .probe = aw2015_led_probe,
    .remove = aw2015_led_remove,
    .shutdown = aw2015_led_shutdown,
    .driver = {
        .name = "aw2015_led",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(aw2015_match_table),
    },
    .id_table = aw2015_led_id,
};

//gjc
/*lint -save -esym(528, aw2015_led_init) -esym(528, module_init) -esym(528, __initcall_aw2015_led_init6)*/
static int __init aw2015_led_init(void)
{
    return i2c_add_driver(&aw2015_led_driver);
}
module_init(aw2015_led_init);
/*lint -restore*/

/*lint -save -esym(528, aw2015_led_exit) -esym(528, module_exit) -esym(528, __exitcall_aw2015_led_exit)*/
static void __exit aw2015_led_exit(void)
{
    i2c_del_driver(&aw2015_led_driver);
}
module_exit(aw2015_led_exit);
/*lint -restore*/

//gjc

MODULE_AUTHOR("<liweilei@awinic.com.cn>");
MODULE_DESCRIPTION("AWINIC aw2015 LED driver");
MODULE_LICENSE("GPL v2");
