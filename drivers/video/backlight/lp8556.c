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
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include "lp8556.h"

/*add backlight register*/
#define LP8556_LED_ENABLE		0X16
#define LP8556_DEVICE_CONTROL		0X01
#define LP8556_BRIGHTNESS_CONTROL	0X00
#define LP8556_BOOST_FREQ		0XA6
#define LP8556_CURRENT_LSB_CFG0		0XA0
#define LP8556_CURRENT_LSB_CFG1		0XA1
#define LP8556_CURRENT_LSB_CFG5		0XA5

struct lp8556 {
	struct i2c_client *client;
	struct mutex lock;
	struct regulator *vdd;
};

struct lp8556 *lp;

static int lp8556_write(struct lp8556 *lp, u8 reg, u8 val)
{
	return i2c_smbus_write_byte_data(lp->client, reg, val);
}

static int lp8556_backlight_init(struct lp8556 *lp)
{
	int ret;

	ret = lp8556_write(lp, LP8556_LED_ENABLE, LED_ENABLE_VALUE);
	if (ret)
		return ret;


	ret = lp8556_write(lp, LP8556_BOOST_FREQ, BOOST_FREQ_VALUE);
	if (ret)
		return ret;

	ret = lp8556_write(lp, LP8556_CURRENT_LSB_CFG0, CURRENT_LSB_CFG0_VALUE);
	if (ret)
		return ret;

	ret = lp8556_write(lp, LP8556_CURRENT_LSB_CFG1, CURRENT_LSB_CFG1_VALUE);
	if (ret)
		return ret;

	ret = lp8556_write(lp, LP8556_CURRENT_LSB_CFG5, CURRENT_LSB_CFG5_VALUE);
	if (ret)
		return ret;

	ret = lp8556_write(lp, LP8556_DEVICE_CONTROL, DEVICE_CONTROL_VALUE_2);
	if (ret)
		return ret;

	mdelay(10);
	pr_info("%s:lp8556 init ok\n",__func__);
	return 0;
}

ssize_t lp8556_set_backlight_reg(uint32_t bl_level)
{
	int ret;
	ret = lp8556_write(lp, LP8556_BRIGHTNESS_CONTROL, bl_level);
	if (ret)
		return ret;
	if(bl_level == 0)  {
		ret = lp8556_write(lp, LP8556_DEVICE_CONTROL, DEVICE_CONTROL_VALUE_1);
		if (ret)
			return ret;
	}
	pr_info("lp8556  bl_level = %d\n", bl_level);
	return ret;
}
EXPORT_SYMBOL(lp8556_set_backlight_reg);

static int lp8556_vdd_supply(struct lp8556 *lp, int on)
{
	struct regulator *vdd = lp->vdd;
	int rc = 0;

	if (on) {
		vdd = regulator_get(&lp->client->dev, "vdd");

		if (IS_ERR(vdd)) {
				rc = PTR_ERR(vdd);
				dev_err(&lp->client->dev, "Regulator get failed vdd rc=%d\n", rc);
				return rc;
				}

		lp->vdd = vdd;

		if (regulator_count_voltages(vdd) > 0) {
				rc = regulator_set_voltage(vdd, 1800000, 1800000); // set 1.8v
				pr_info("%s:vdd supply ok\n",__func__);
				if (rc) {
					dev_err(&lp->client->dev, "Regulator set_vtg failed vdd rc=%d\n", rc);
					regulator_put(vdd);
				}
		}
		rc = regulator_enable(vdd);
	  } else {
		  if (!IS_ERR_OR_NULL(lp->vdd))
		  {
			  regulator_disable(lp->vdd);
			  regulator_put(lp->vdd);
		  }
	  }
	  return rc;
}

void lp8556_reset(void)
{
	lp8556_backlight_init(lp);
}
EXPORT_SYMBOL(lp8556_reset);

static int lp8556_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	struct device_node *node;
	int ret;
	node = client->dev.of_node;
	if (node == NULL)
		return -EINVAL;

	lp = devm_kzalloc(&client->dev,
		sizeof(struct lp8556) , GFP_KERNEL);

	if (!lp)
		return -ENOMEM;

	lp->client = client;

	ret = lp8556_vdd_supply(lp, 1);
	if (ret) {
		dev_err(&client->dev,
				"failed to register backlight. err: %d\n", ret);
		devm_kfree(&client->dev, lp);
		return ret;
	}

	if (!ret)
		ret = lp8556_backlight_init(lp);

	pr_info("%s:lp8556_i2c_probe_ok\n",__func__);

	return 0;
}

static int lp8556_remove(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id lp8556_id[] = {
	{"lp8556", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, lp8556_id);

static struct of_device_id lp8556_match_table[] = {
	{ .compatible = "ti,lp8556",},
	{},
};

static struct i2c_driver lp8556_driver = {
	.probe = lp8556_probe,
	.remove = lp8556_remove,
	.driver = {
		.name = "lp8556",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(lp8556_match_table),
	},
	.id_table = lp8556_id,
};

static int __init lp8556_init(void)
{
	return i2c_add_driver(&lp8556_driver);
}
module_init(lp8556_init);

static void __exit lp8556_exit(void)
{
	i2c_del_driver(&lp8556_driver);
}
module_exit(lp8556_exit);

MODULE_DESCRIPTION("AWINIC lp8556 LED driver");
MODULE_LICENSE("GPL v2");
