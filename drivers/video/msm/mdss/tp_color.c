/*
 * Copyright (C) huawei company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/module.h>

#define WHITE	0xE1
#define BLACK	0xD2
#define PINK	0xC3
#define RED	0xB4
#define CAFE	0xB8
#define YELLOW	0xA5
#define BLUE	0x96
#define GOLD	0x87
#define TP_COLOR_BUF_SIZE	20
static unsigned char tp_color_buf[TP_COLOR_BUF_SIZE];

/******************************************************************************
Function:		read_tp_color
******************************************************************************/
int read_tp_color(void)
{
	int tp_color;

	tp_color = (int)simple_strtol(tp_color_buf, NULL, 0);

	return tp_color;
}

static int __init early_parse_tp_color_cmdline(char *arg)
{
	int len = 0;
	memset(tp_color_buf, 0, sizeof(tp_color_buf));
	if (arg) {
		len = strlen(arg);

		if (len > sizeof(tp_color_buf)) {
			len = sizeof(tp_color_buf);
		}
		memcpy(tp_color_buf, arg, len);
	} else {
		pr_info("%s : arg is NULL\n", __func__);
	}

	return 0;
}
early_param("TP_COLOR", early_parse_tp_color_cmdline);

static ssize_t attr_get_tp_color_info(struct device *dev, struct device_attribute *attr, char *buf)
{
	u8 phone_color=0;
	phone_color = read_tp_color();
	if (phone_color != 0xff) {
		pr_info("phone_color is %u from read tp color\n", phone_color);
	}
	switch(phone_color)
	{
	case WHITE:
			strlcpy(buf,"white", sizeof("white"));
			break;
	case BLACK:
			strlcpy(buf,"black", sizeof("black"));
			break;
	case PINK:
			strlcpy(buf,"pink", sizeof("pink"));
			break;
	case RED:
			strlcpy(buf,"red", sizeof("red"));
			break;
	case CAFE:
			strlcpy(buf,"cafe", sizeof("cafe"));
			break;
	case YELLOW:
			strlcpy(buf,"yellow", sizeof("yellow"));
			break;
	case BLUE:
			strlcpy(buf,"blue", sizeof("blue"));
			break;
	case GOLD:
			strlcpy(buf,"gold", sizeof("gold"));
			break;
	default:
			strlcpy(buf,"fail", sizeof("fail"));
			break;
	}
	return strlen(buf);
}

static ssize_t attr_put_tp_color_info(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	pr_info("write tp color buf=%s\n",buf);
	return count;
}

static struct device_attribute tp_color_file =
	__ATTR(tp_color_info, 0664, attr_get_tp_color_info, attr_put_tp_color_info);

static struct platform_device huawei_tp_color = {
	.name = "huawei_tp_color",
	.id = -1,
};

static int __init tp_color_info_init(void)
{
	int ret = 0;

	ret = platform_device_register(&huawei_tp_color);
	if (ret) {
		pr_err("%s: platform_device_register failed, ret:%d.\n",
				__func__, ret);
		goto REGISTER_ERR;
	}
	if (device_create_file(&huawei_tp_color.dev, &tp_color_file)) {
		pr_err("%s:Unable to create interface\n", __func__);
		goto SYSFS_CREATE_FILE_ERR;
	}
	return 0;

SYSFS_CREATE_FILE_ERR:
	platform_device_unregister(&huawei_tp_color);
REGISTER_ERR:
	return ret;

}

device_initcall(tp_color_info_init);
MODULE_DESCRIPTION("tp color info");
MODULE_LICENSE("GPL");
