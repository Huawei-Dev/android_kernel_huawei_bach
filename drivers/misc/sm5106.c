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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/string.h>
#include <linux/of_gpio.h>

static struct i2c_client *sm5106_i2c_client = NULL;

static int sm5106_write_bytes(u8 reg, u8 val)
{
	s32 ret, retry_times = 0;

	if (sm5106_i2c_client == NULL) {
		pr_err("[%s,%d]:NULL point for sm5106_i2c_client\n", __FUNCTION__,__LINE__);
		return -1;
	}

	do {
		ret = i2c_smbus_write_byte_data(sm5106_i2c_client, reg, val);
		retry_times ++;
		if(retry_times == 3)
			break;
	}while (ret < 0);

	return ret;
}

static int sm5106_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret_vsp = 0, ret_vsn = 0;

	if (client == NULL) {
		pr_err("[%s,%d]:NULL point for client\n", __FUNCTION__,__LINE__);
		return -1;
	}

	sm5106_i2c_client = client;

	/* write Vsp & Vsn value */
	ret_vsp = sm5106_write_bytes(0x00, 0x08);		// set VSP
	ret_vsn = sm5106_write_bytes(0x01, 0x0D);		// set VSN
	if ((ret_vsp < 0) || (ret_vsn < 0) ) {
		pr_err("Set Vsp or Vsn fail! ret_vsp = %d, ret_vsn = %d", ret_vsp, ret_vsn);
		return -1;
	}

	return 0;
}

static const struct i2c_device_id sm5106_id[] = {
	{"sm5106", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, sm5106_id);

static struct of_device_id sm5106_match_table[] = {
	{ .compatible = "hw,sm5106",},
	{ },
};

static struct i2c_driver sm5106_driver = {
	.probe		= sm5106_probe,
	.driver = {
		.name = "sm5106",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sm5106_match_table),
	},
	.id_table = sm5106_id,
};

static int __init sm5106_init(void)
{
	return i2c_add_driver(&sm5106_driver);
}
module_init(sm5106_init);

static void __exit sm5106_exit(void)
{
	i2c_del_driver(&sm5106_driver);
}
module_exit(sm5106_exit);

MODULE_DESCRIPTION("sm5106 driver");
MODULE_LICENSE("GPL");