/************************************************************
*
* Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
* FileName: hw_typec.c
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*  Description:    .c file for type-c core layer which is used to handle
*                  pulic logic management for different chips and to
*                  provide interfaces for exteranl modules.
***********************************************************/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/usb/hw_typec.h>

static struct class *typec_class;

static ssize_t typec_cc_orientation_show(struct device *pdev,
					struct device_attribute *attr, char *buf)
{
	struct typec_device_ops *ops = dev_get_drvdata(pdev);
	enum typec_cc_orient cc_orient = ops->detect_cc_orientation();
	return scnprintf(buf, PAGE_SIZE, "%d\n", cc_orient);
}

static ssize_t typec_input_current_show(struct device *pdev,
				   struct device_attribute *attr, char *buf)
{
	struct typec_device_ops *ops = dev_get_drvdata(pdev);
	enum typec_input_current input_current = ops->detect_input_current();
	return snprintf(buf, PAGE_SIZE, "%d\n", input_current);
}

static DEVICE_ATTR(cc_orientation, S_IRUGO, typec_cc_orientation_show, NULL);
static DEVICE_ATTR(input_current, S_IRUGO, typec_input_current_show, NULL);

static struct attribute *typec_ctrl_attributes[] = {
	&dev_attr_cc_orientation.attr,
	&dev_attr_input_current.attr,
	NULL,
};

static const struct attribute_group typec_attr_group = {
	.attrs = typec_ctrl_attributes,
};

int typec_chip_register(struct typec_device_ops *ops)
{
	struct device *typec_dev;
	int ret = 0;

	if (!ops || !ops->detect_cc_orientation || !ops->detect_input_current) {
		pr_err("%s: ops is NULL\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(typec_class)) {
		pr_err("%s: no typec class\n", __func__);
		return -EINVAL;
	}

	typec_dev = device_create(typec_class, NULL, MKDEV(0, 0), ops, "typec");
	if (IS_ERR(typec_dev)) {
		pr_err("%s: device_create fail\n", __func__);
		return PTR_ERR(typec_dev);
	}

	ret = sysfs_create_group(&typec_dev->kobj, &typec_attr_group);
	if (ret) {
		pr_err("%s: typec sysfs group create error\n", __func__);
	}

	return ret;
}

struct class *hw_typec_get_class(void)
{
	if (IS_ERR_OR_NULL(typec_class)) {
		pr_err("%s: no typec class\n", __func__);
		return NULL;
	}

	return typec_class;
}

static int __init typec_init(void)
{
	typec_class = class_create(THIS_MODULE, "hw_typec");
	if (IS_ERR(typec_class)) {
		pr_err("failed to create typec class\n");
		return PTR_ERR(typec_class);
	}

	return 0;
}

subsys_initcall(typec_init);

static void __exit typec_exit(void)
{
	class_destroy(typec_class);
}

module_exit(typec_exit);

MODULE_AUTHOR("HUAWEI");
MODULE_DESCRIPTION("Type-C connector Framework");
MODULE_LICENSE("GPL v2");
