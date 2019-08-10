/*
 * ALSA SoC Texas Instruments TAS2560 High Performance 4W Smart Amplifier
 *
 * Copyright (C) 2016 Texas Instruments, Inc.
 *
 * Author: saiprasad
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <sound/soc.h>
#include <sound/hw_audio_info.h>

#include "tas2560.h"
#include "tas2560-core.h"
#include "tas2560-misc.h"
#include "tas2560-codec.h"

struct list_head tas2560_codecs_list = LIST_HEAD_INIT(tas2560_codecs_list);
struct mutex tas2560_codecs_lock = __MUTEX_INITIALIZER(tas2560_codecs_lock);

static void tas2560_change_page(struct tas2560_priv *pTAS2560, int page)
{
	if (pTAS2560->mnCurrentPage == page)
		return;

	regmap_write(pTAS2560->regmap, TAS2560_BOOKCTL_PAGE, page);
	pTAS2560->mnCurrentPage = page;
}

static void tas2560_change_book(struct tas2560_priv *pTAS2560, int book)
{
	if (pTAS2560->mnCurrentBook == book)
		return;

	tas2560_change_page(pTAS2560, 0);
	regmap_write(pTAS2560->regmap, TAS2560_BOOKCTL_REG, book);
	pTAS2560->mnCurrentBook = book;
}

static int tas2560_dev_read(struct tas2560_priv *pTAS2560,
				 unsigned int reg, unsigned int *pValue)
{
	int ret = -1;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is null\n", __func__);
		return ret;
	}

	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u\n", __func__,
		TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg));

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	ret = regmap_read(pTAS2560->regmap, TAS2560_PAGE_REG(reg), pValue);
	if (ret) {
		dev_err(pTAS2560->dev,
			"%s: read BOOK:PAGE:REG %u:%u:%u failed %d\n",
			__func__,
			TAS2560_BOOK_ID(reg),
			TAS2560_PAGE_ID(reg), TAS2560_PAGE_REG(reg), ret);
		audio_dsm_report_info(DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO,
			"TAS%d RD %u:%u:%u ERR %d\n",
			pTAS2560->type,
			TAS2560_BOOK_ID(reg),
			TAS2560_PAGE_ID(reg), TAS2560_PAGE_REG(reg), ret);
	}
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static int tas2560_dev_write(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned int value)
{
	int ret = -1;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is null\n", __func__);
		return ret;
	}

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u, VAL: 0x%02x\n",
		__func__, TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg), value);
	ret = regmap_write(pTAS2560->regmap, TAS2560_PAGE_REG(reg), value);
	if (ret) {
		dev_err(pTAS2560->dev,
			"%s: write BOOK:PAGE:REG %u:%u:%u failed %d\n",
			__func__,
			TAS2560_BOOK_ID(reg),
			TAS2560_PAGE_ID(reg), TAS2560_PAGE_REG(reg), ret);
		audio_dsm_report_info(DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO,
			"TAS%d WR %u:%u:%u ERR %d\n",
			pTAS2560->type,
			TAS2560_BOOK_ID(reg),
			TAS2560_PAGE_ID(reg), TAS2560_PAGE_REG(reg), ret);
	}
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static int tas2560_dev_bulk_write(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned char *pData, unsigned int nLength)
{
	int ret = -1;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is null\n", __func__);
		return ret;
	}

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u, len: 0x%02x\n",
		__func__, TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg), nLength);
	ret = regmap_bulk_write(pTAS2560->regmap, TAS2560_PAGE_REG(reg), pData, nLength);
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static int tas2560_dev_bulk_read(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned char *pData, unsigned int nLength)
{
	int ret = -1;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is null\n", __func__);
		return ret;
	}

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u, len: 0x%02x\n",
		__func__, TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg), nLength);
	ret = regmap_bulk_read(pTAS2560->regmap, TAS2560_PAGE_REG(reg), pData, nLength);
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static int tas2560_dev_update_bits(struct tas2560_priv *pTAS2560, unsigned int reg,
			 unsigned int mask, unsigned int value)
{
	int ret = -1;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is null\n", __func__);
		return ret;
	}

	mutex_lock(&pTAS2560->dev_lock);
	tas2560_change_book(pTAS2560, TAS2560_BOOK_ID(reg));
	tas2560_change_page(pTAS2560, TAS2560_PAGE_ID(reg));
	dev_dbg(pTAS2560->dev, "%s: BOOK:PAGE:REG %u:%u:%u, mask: 0x%x, val=0x%x\n",
		__func__, TAS2560_BOOK_ID(reg), TAS2560_PAGE_ID(reg),
		TAS2560_PAGE_REG(reg), mask, value);
	ret = regmap_update_bits(pTAS2560->regmap, TAS2560_PAGE_REG(reg), mask, value);
	if (ret) {
		dev_err(pTAS2560->dev,
			"%s: update BOOK:PAGE:REG %u:%u:%u failed %d\n",
			__func__,
			TAS2560_BOOK_ID(reg),
			TAS2560_PAGE_ID(reg), TAS2560_PAGE_REG(reg), ret);
		audio_dsm_report_info(DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO,
			"TAS%d UT %u:%u:%u ERR %d\n",
			pTAS2560->type,
			TAS2560_BOOK_ID(reg),
			TAS2560_PAGE_ID(reg), TAS2560_PAGE_REG(reg), ret);
	}
	mutex_unlock(&pTAS2560->dev_lock);
	return ret;
}

static bool tas2560_volatile(struct device *dev, unsigned int reg)
{
	return false;
}

static bool tas2560_writeable(struct device *dev, unsigned int reg)
{
	return true;
}

static int reg_set_optimum_mode_check(struct regulator *reg, int load_uA)
{
	return (regulator_count_voltages(reg) > 0) ?
		regulator_set_optimum_mode(reg, load_uA) : 0;
}

static int tas2560_regulator_config(struct i2c_client *i2c, bool pullup, bool on)
{
	struct regulator *vcc_i2c = NULL, *vdd = NULL;
	int rc = 0;
	#define VCC_I2C_MIN_UV	1800000
	#define VCC_I2C_MAX_UV	1800000
	#define I2C_LOAD_UA		300000

	vdd = regulator_get(&i2c->dev, "vdd");
	if (IS_ERR(vdd)) {
		pr_err("%s: regulator get failed rc=%d\n", __func__, rc);
		return PTR_ERR(vdd);
	}

	if (regulator_count_voltages(vdd) > 0) {
		rc = regulator_set_voltage(vdd, VCC_I2C_MIN_UV, VCC_I2C_MAX_UV);
		if (rc) {
			pr_err("%s: regulator set_vtg failed rc=%d\n", __func__, rc);
			goto error_set_vtg_vdd;
		}
	}

	if (pullup) {
		vcc_i2c = regulator_get(&i2c->dev, "vcc_i2c");
		if (IS_ERR(vcc_i2c)) {
			rc = PTR_ERR(vcc_i2c);
			pr_err("%s: regulator get failed rc=%d\n", __func__, rc);
			goto error_get_vtg_i2c;
		}

		if (regulator_count_voltages(vcc_i2c) > 0) {
			rc = regulator_set_voltage(vcc_i2c, VCC_I2C_MIN_UV, VCC_I2C_MAX_UV);
			if (rc) {
				pr_err("%s: regulator set_vtg failed rc=%d\n", __func__, rc);
				goto error_set_vtg_i2c;
			}
		}
	}

	if (on) {
		rc = reg_set_optimum_mode_check(vdd, I2C_LOAD_UA);
		if (rc < 0) {
			pr_err("%s: regulator vcc_i2c set_opt failed rc=%d\n", __func__, rc);
			goto error_reg_opt_vdd;
		}

		rc = regulator_enable(vdd);
		if (rc) {
			pr_err("%s: regulator vcc_i2c enable failed rc=%d\n", __func__, rc);
			goto error_reg_en_vdd;
		}

		if (vcc_i2c) {
			rc = reg_set_optimum_mode_check(vcc_i2c, I2C_LOAD_UA);
			if (rc < 0) {
				pr_err("%s: regulator vcc_i2c set_opt failed rc=%d\n", __func__, rc);
				goto error_reg_opt_i2c;
			}

			rc = regulator_enable(vcc_i2c);
			if (rc) {
				pr_err("%s: regulator vcc_i2c enable failed rc=%d\n", __func__, rc);
				goto error_reg_en_vcc_i2c;
			}
		}
	} else {
		if (vcc_i2c) {
			reg_set_optimum_mode_check(vcc_i2c, 0);
			regulator_disable(vcc_i2c);
		}

		reg_set_optimum_mode_check(vdd, 0);
		regulator_disable(vdd);
	}

	return rc;

error_set_vtg_i2c:
	if (NULL != vcc_i2c)
		regulator_put(vcc_i2c);
error_get_vtg_i2c:
	if (NULL != vcc_i2c) {
		if (regulator_count_voltages(vcc_i2c) > 0)
			(void)regulator_set_voltage(vcc_i2c, 0, VCC_I2C_MAX_UV);
	}
error_set_vtg_vdd:
	if (NULL != vdd)
		regulator_put(vdd);
error_reg_en_vcc_i2c:
	if (NULL != vcc_i2c)
		reg_set_optimum_mode_check(vcc_i2c, 0);
error_reg_opt_i2c:
	if (NULL != vcc_i2c)
		regulator_disable(vcc_i2c);
error_reg_en_vdd:
	if (NULL != vdd)
		reg_set_optimum_mode_check(vdd, 0);
error_reg_opt_vdd:
	if (NULL != vdd)
		regulator_disable(vdd);

	return rc;
}

static const struct regmap_config tas2560_i2c_regmap = {
	.reg_bits = BIT_RATE_8,	/* 8 bits for register */
	.val_bits = BIT_RATE_8,	/* 8 bits for value */
	.writeable_reg = tas2560_writeable,
	.volatile_reg = tas2560_volatile,
	.cache_type = REGCACHE_NONE,
	.max_register = BIT_RATE_128,	/* max register number */
};

static int tas2560_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct tas2560_priv *pTAS2560 = NULL;
	int ret = 0;
	bool pullup = of_property_read_bool(client->dev.of_node, "i2c-pull-up");

	dev_info(&client->dev, "%s: addr: 0x%x, id: %lu\n",
		__func__, client->addr, id->driver_data);

	pTAS2560 = devm_kzalloc(&client->dev, sizeof(struct tas2560_priv),
			GFP_KERNEL);
	if (pTAS2560 == NULL) {
		dev_err(&client->dev,
			"%s: malloc tas2560_priv failed !\n", __func__);
		ret = -EPROBE_DEFER;
		goto err_exit;
	}

	ret = tas2560_regulator_config(client, pullup, 1);
	if (ret)
		dev_warn(&client->dev,
			"%s: Failed to config regulator, ret=%d\n", __func__, ret);

	tas2560_parse_dt(&client->dev, pTAS2560);

	pTAS2560->dev = &client->dev;
	i2c_set_clientdata(client, pTAS2560);

	pTAS2560->type = id->driver_data;
	if (pTAS2560->type == TAS2560_PRI_L)
		tas2560_hw_reset(pTAS2560);

	if (pTAS2560->type == TAS2560_PRI_L || pTAS2560->type == TAS2560_PRI_R)
		pTAS2560->beid = TAS2560_PRI_BE;
	else
		pTAS2560->beid = TAS2560_SEC_BE;

	pTAS2560->regmap = regmap_init_i2c(client, &tas2560_i2c_regmap);
	if (IS_ERR(pTAS2560->regmap)) {
		ret = PTR_ERR(pTAS2560->regmap);
		dev_err(&client->dev,
			"%s: Failed to allocate register map: %d\n", __func__, ret);
		goto err_regmap;
	}

	pTAS2560->mnCurrentBook = -1;
	pTAS2560->mnCurrentPage = -1;

	pTAS2560->read = tas2560_dev_read;
	pTAS2560->write = tas2560_dev_write;
	pTAS2560->bulk_read = tas2560_dev_bulk_read;
	pTAS2560->bulk_write = tas2560_dev_bulk_write;
	pTAS2560->update_bits = tas2560_dev_update_bits;
	mutex_init(&pTAS2560->dev_lock);
	mutex_init(&pTAS2560->file_lock);
	INIT_WORK(&pTAS2560->irq_work, tas2560_state_check_work);

	ret = tas2560_dev_write(pTAS2560, TAS2560_SW_RESET_REG, 0x01);
	if (ret < 0) {
		dev_err(pTAS2560->dev, "%s: ERROR I2C comm, %d\n", __func__, ret);
		goto err_comm;
	}
	usleep_range(DELAY_MS_MIN*1, DELAY_MS_MAX*1);	// 1ms from TI FAE's mail.

	tas2560_hw_setup(pTAS2560);

	mutex_lock(&tas2560_codecs_lock);
	if (list_empty(&tas2560_codecs_list)) {
		dev_set_name(&client->dev, TAS_CODEC_NAME);
		ret = tas2560_register_codec(pTAS2560);
		if (ret) {
			dev_err(pTAS2560->dev,
				"%s: register codec failed %d !\n", __func__, ret);
			goto err_reg_codec;
		}
		tas_calib_register();
	}
	list_add_tail(&pTAS2560->list, &tas2560_codecs_list);
	mutex_unlock(&tas2560_codecs_lock);

	tas2560_irq_register(pTAS2560);
	tas2560_register_misc(pTAS2560);

	dev_info(pTAS2560->dev, "%s: probe tas2560 successfully\n", __func__);
	return ret;

err_reg_codec:
	mutex_unlock(&tas2560_codecs_lock);
err_comm:
	regmap_exit(pTAS2560->regmap);
err_regmap:
	i2c_set_clientdata(client, NULL);
	tas2560_regulator_config(client, pullup, 0);
	devm_kfree(&client->dev, pTAS2560);
err_exit:
	dev_warn(&client->dev, "%s: probe tas2560 failed %d !\n", __func__, ret);
	return ret;
}

static int tas2560_i2c_remove(struct i2c_client *client)
{
	struct tas2560_priv *pTAS2560 = i2c_get_clientdata(client);

	if (!pTAS2560) {
		dev_err(&client->dev, "%s: tas2560_priv is null !\n", __func__);
		return 0;
	}
	if (!strncmp(dev_name(&client->dev),
			TAS_CODEC_NAME, strlen(TAS_CODEC_NAME))) {
		tas_calib_deregister();
	}
	tas2560_deregister_codec(pTAS2560);
	tas2560_deregister_misc(pTAS2560);
	mutex_destroy(&pTAS2560->file_lock);
	mutex_destroy(&pTAS2560->dev_lock);

	return 0;
}

static const struct i2c_device_id tas2560_i2c_id[] = {
	{ "tas2560_pri_l", TAS2560_PRI_L },
	{ "tas2560_pri_r", TAS2560_PRI_R },
	{ "tas2560_sec_l", TAS2560_SEC_L },
	{ "tas2560_sec_r", TAS2560_SEC_R },
};
MODULE_DEVICE_TABLE(i2c, tas2560_i2c_id);

static const struct of_device_id tas2560_of_match[] = {
	{ .compatible = "ti,tas2560_pri_l", },
	{ .compatible = "ti,tas2560_pri_r", },
	{ .compatible = "ti,tas2560_sec_l", },
	{ .compatible = "ti,tas2560_sec_r", },
	{ }
};
MODULE_DEVICE_TABLE(of, tas2560_of_match);


static struct i2c_driver tas2560_i2c_driver = {
	.driver = {
		.name   = "tas2560",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(tas2560_of_match),
	},
	.probe      = tas2560_i2c_probe,
	.remove     = tas2560_i2c_remove,
	.id_table   = tas2560_i2c_id,
};

module_i2c_driver(tas2560_i2c_driver);

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 I2C Smart Amplifier driver");
MODULE_LICENSE("GPLv2");
