/*
 * AR0003UHPE: add a driver to detect antenna cable state
 * antenna_cable_state.c
 *
 * Copyright (c) 2012-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/pm.h>
#include <linux/delay.h>

#define DRV_NAME "antenna_cable"
#define TAG "antenna_cable: "


#define MAX_ANT_DET_GPIO_NUM 3
static int g_gpio_count = 0;
static int g_gpio[MAX_ANT_DET_GPIO_NUM] = {0};
static int g_debounce_interval = 5;
static int g_irq[MAX_ANT_DET_GPIO_NUM] = {0};

struct ant_device {
	struct device *dev;
	struct class *class;
	dev_t devno;
	struct delayed_work dwork;
	/* previous cable state */
	unsigned int prev_state;
	/* current cable state */
	unsigned int curr_state;
};

/*
 * the return value is binary of (GPIO0GPIO1...), GPIOX stands for
 * GPIOX level, for example all cables not not connected, return 0.
 */
static int ant_state_get(const struct ant_device* antdev)
{
	unsigned int gpio_val = 0;
	unsigned int temp_val = 0;
	int i = 0;
	for (i = 0; i < g_gpio_count; i++) {
		temp_val = gpio_get_value(g_gpio[i]);
		gpio_val += (temp_val << i);
	}
	return gpio_val;
}

static void ant_event_report(struct ant_device *antdev)
{
	char *uevent_envp[2] = {NULL};
	char buf[50] = {0};
	int ret = 0;

	if (ant_state_get(antdev)) {
		/* any cable in position, need do sar backoff */
		antdev->curr_state = 1;
		pr_info(TAG "antenna cable is in position.\n");
	} else {
		/* no cable in position, restore to default state */
		antdev->curr_state = 0;
		pr_info(TAG "antenna cable is not in position.\n");
	}

	/* report uevent if previous state is not equal to current state */
	if (antdev->prev_state != antdev->curr_state) {
		uevent_envp[1] = NULL;

		sprintf(buf, "ANTENNA_CABLE_STATE=%d", antdev->curr_state);
		uevent_envp[0] = buf;
		pr_info(TAG "send uevent, %s\n", uevent_envp[0]);

		/* notify the uplayer to do sar backoff or restore */
		ret = kobject_uevent_env(&antdev->dev->kobj, KOBJ_CHANGE, uevent_envp);
		if (ret < 0) {
			pr_err(TAG "send uevent failed, ret = %d\n", ret);
		} else {
			pr_info(TAG "send uevent, %s\n", uevent_envp[0]);
		}

		/* save the current state */
		antdev->prev_state = antdev->curr_state;
	}
}

static void ant_det_work_func(struct work_struct *work)
{
	struct ant_device *antdev =
		container_of(work, struct ant_device, dwork.work);

	ant_event_report(antdev);
}

static irqreturn_t ant_pull_in_out_irq(int irq, void *dev_id)
{
	struct ant_device *antdev = dev_id;
	pr_info(TAG "Interrupt occured\n");

	schedule_delayed_work(&antdev->dwork,
			msecs_to_jiffies(g_debounce_interval));

	return IRQ_HANDLED;
}

static struct of_device_id ant_det_of_match[] = {
	{ .compatible = "huawei,antenna_cable_detect", },
	{ },
};
MODULE_DEVICE_TABLE(of, ant_det_of_match);

static void ant_dt_parse(struct device *dev)
{
	const struct of_device_id *of_id =
				of_match_device(ant_det_of_match, dev);
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

	for (i = 0; i < g_gpio_count; i++) {
	    g_gpio[i] = of_get_gpio_flags(np, i, &flags);
	    pr_debug(TAG "g_gpio[%d] = %d\n", i, g_gpio[i]);

		if (!gpio_is_valid(g_gpio[i])) {
			return;
		}
	}

	if (of_property_read_u32(np, "debounce-interval",
				&g_debounce_interval)) {
		/* default 5 us */
		g_debounce_interval = 5;
	}

}

static int ant_det_gpio_process(struct ant_device *antdev, int irq, int gpio)
{
	int err = -1;

	/* request a gpio and set the function direction in  */
	err = gpio_request_one(gpio, GPIOF_IN, DRV_NAME);
	if (err) {
		pr_err(TAG "Unable to request GPIO %d, err %d\n", gpio, err);
		return err;
	}

	/* request irq */
	irq = gpio_to_irq(gpio);
	if (irq < 0) {
		err = irq;
		pr_err(TAG "Unable to get irq number for GPIO %d, err %d\n", gpio, err);
		return err;
	}
	
	err = request_threaded_irq(irq, NULL, &ant_pull_in_out_irq,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				DRV_NAME, antdev);
	if (err) {
		gpio_free(gpio);
		pr_err(TAG "Unable to request IRQ %d, err %d\n", irq, err);
		return err;
	}

	pr_info(TAG "Request GPIO %d, IRQ: %d success\n", gpio, irq);
	return 0;
}

static ssize_t ant_state_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ant_device *antdev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ant_state_get(antdev));
}

static struct device_attribute dev_attr_gpio_status =
__ATTR(state, S_IRUSR | S_IRGRP, ant_state_show, NULL);

static int ant_dev_probe(struct platform_device *pdev)
{
	int err = -1;
	int i = 0;
	struct ant_device *antdev = NULL;

	antdev = kzalloc(sizeof(*antdev), GFP_KERNEL);
	if(!antdev) {
		pr_err(TAG "Request memory failed\n");
		return -ENOMEM;
	}
	antdev->dev = &pdev->dev;

	/* get dts data */
	ant_dt_parse(&pdev->dev);

	/* init delayed work */
	INIT_DELAYED_WORK(&antdev->dwork, ant_det_work_func);

	for (i = 0; i < g_gpio_count; i++) {
		err = ant_det_gpio_process(antdev, g_irq[i], g_gpio[i]);
		if (err) {
			pr_err(TAG "Process GPIO %d failed\n", g_gpio[i]);
		} else {
			pr_info(TAG "Process GPIO %d success\n", g_gpio[i]);
		}
	}

	platform_set_drvdata(pdev, antdev);

	/*
	 * Create directory "/sys/class/antenna_cable/detect"
	 * To get event, up layer will listen the directory
	 */
	antdev->class = class_create(THIS_MODULE, "antenna_cable");
	if (IS_ERR(antdev->class)) {
		pr_err(TAG "create class failed\n");
		goto exit_free_mem;
	}

	err = alloc_chrdev_region(&antdev->devno, 0, 1, DRV_NAME);
	if (err) {
		pr_err(TAG "alloc character device region failed\n");
		goto exit_class_destroy;
	}

	antdev->dev = device_create(antdev->class, &pdev->dev, antdev->devno, antdev, "detect");
	if (IS_ERR(antdev->dev)) {
		pr_err(TAG "creat device failed\n");
		goto exit_class_destroy;
	}

	err = device_create_file(antdev->dev, &dev_attr_gpio_status);
	if (err) {
		pr_err(TAG "create file failed\n");
		goto exit_device_destroy;
	}

	pr_info(TAG "huawei antenna cable state detect probe ok\n");
	return 0;

exit_device_destroy:
	device_destroy(antdev->class, antdev->devno);
exit_class_destroy:
	class_destroy(antdev->class);
exit_free_mem:
	kfree(antdev);
	antdev = NULL;
	pr_info(TAG "huawei antenna cable state detect probe failed\n");
	return err;
}

static int ant_dev_remove(struct platform_device *pdev)
{
	int i = 0;
	struct ant_device *antdev = platform_get_drvdata(pdev);

	cancel_delayed_work_sync(&antdev->dwork);

	device_remove_file(antdev->dev, &dev_attr_gpio_status);
	device_destroy(antdev->class, antdev->devno);
	class_destroy(antdev->class);

	for (i = 0; i < g_gpio_count; i++) {
		(void)free_irq(g_irq[i], antdev);
		gpio_free(g_gpio[i]);
	}

	kfree(antdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}


/*delete resume or suspend func, because cable det irq can resume device */
#if 0
static int ant_dev_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ant_device *antdev = platform_get_drvdata(pdev);

	ant_event_report(antdev);

	return 0;
}

static int ant_dev_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct ant_device *antdev = platform_get_drvdata(pdev);

	/*
	 * Because the gpios can't wake device,
	 * so we need to reset the varient "prev_state" to default 0.
	 */
	antdev->prev_state = 0;
	cancel_delayed_work_sync(&antdev->dwork);

	return 0;
}

/**
  The file Operation table for power
*/
static const struct dev_pm_ops ant_dev_pm_ops = {
	.suspend = ant_dev_resume,
	.resume = ant_dev_suspend,
};
#endif

static struct platform_driver ant_dev_driver = {
	.probe		= ant_dev_probe,
	.remove		= ant_dev_remove,
	.driver		= {
		.name	= DRV_NAME,
		.owner	= THIS_MODULE,
		//.pm	= &ant_dev_pm_ops,
		.of_match_table = of_match_ptr(ant_det_of_match),
	}
};

/*
 * Function:     ant_det_init
 * Description:  antenna det  module initialization
 * Parameters:  Null
 * return value: 0-sucess or others-fail
*/
static int __init ant_det_init(void)
{
	return platform_driver_register(&ant_dev_driver);
}

/*
 * Function:      ant_det_exit
 * Description:   antenna det module exit
 * Parameters:   NULL
 * return value:  NULL
*/
static void __exit ant_det_exit(void)
{
	platform_driver_unregister(&ant_dev_driver);
}

late_initcall(ant_det_init);
module_exit(ant_det_exit);
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRV_NAME);
MODULE_DESCRIPTION("HUAWEI Antenna Cable State Detect Driver");
MODULE_AUTHOR("HUAWEI Inc");
