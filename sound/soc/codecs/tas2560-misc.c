/*
** =============================================================================
** Copyright (c) 2016  Texas Instruments Inc.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; version 2.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
** Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
** File:
**     tas2560-misc.c
**
** Description:
**     misc driver for Texas Instruments TAS2560 High Performance 4W Smart Amplifier
**
** =============================================================================
*/

//#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
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
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#include "tas2560.h"
#include "tas2560-core.h"
#include "tas2560-misc.h"
#include <linux/dma-mapping.h>

static int g_logEnable = 1;

static int tas2560_file_open(struct inode *inode, struct file *file)
{
	struct miscdevice *misc = file->private_data;
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type = 0xffffffff;

	if (!misc) {
		pr_err("%s, misc file error\n", __func__);
		return -ENODEV;
	}

	if (!strncmp(misc->name, "tas2560_pri_l", strlen("tas2560_pri_l")))
		type = TAS2560_PRI_L;
	else if (!strncmp(misc->name, "tas2560_pri_r", strlen("tas2560_pri_r")))
		type = TAS2560_PRI_R;
	else if (!strncmp(misc->name, "tas2560_sec_l", strlen("tas2560_sec_l")))
		type = TAS2560_SEC_L;
	else if (!strncmp(misc->name, "tas2560_sec_r", strlen("tas2560_sec_r")))
		type = TAS2560_SEC_R;

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560)
		return -ENODEV;

	file->private_data = (void*)pTAS2560;

	if(g_logEnable) dev_info(pTAS2560->dev,
				"%s\n", __FUNCTION__);
	return 0;
}

static int tas2560_file_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static ssize_t tas2560_file_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	struct tas2560_priv *pTAS2560 = (struct tas2560_priv *)file->private_data;
	int ret = 0;
	unsigned int nValue = 0;
	unsigned char value = 0;
	unsigned char *p_kBuf = NULL;

	if ((file == NULL) || (buf == NULL)) {
		pr_err("%s, file or buf is error\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&pTAS2560->file_lock);

	switch(pTAS2560->mnDBGCmd)
	{
		case TIAUDIO_CMD_REG_READ:
		{
			if (g_logEnable)
				dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_READ: current_reg = 0x%x, count=%d\n", pTAS2560->mnCurrentReg, (int)count);
			if (count == 1) {
				ret = pTAS2560->read(pTAS2560, pTAS2560->mnCurrentReg, &nValue);
				if( 0 > ret) {
					dev_err(pTAS2560->dev, "dev read fail %d\n", ret);
					break;
				}

				value = (u8)nValue;
				if(g_logEnable) dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_READ: nValue=0x%x, value=0x%x\n", nValue, value);
				ret = copy_to_user(buf, &value, 1);
				if (0 != ret) {
					/* Failed to copy all the data, exit */
					dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
				}
			}else if (count > REG_COUNT_1){
				p_kBuf = (unsigned char *)kzalloc(count, GFP_KERNEL);
				if (p_kBuf != NULL){
					ret = pTAS2560->bulk_read(pTAS2560, pTAS2560->mnCurrentReg, p_kBuf, count);
					if ( 0 > ret) {
						dev_err(pTAS2560->dev, "dev bulk read fail %d\n", ret);
					} else {
						ret = copy_to_user(buf, p_kBuf, count);
						if (0 != ret) {
							/* Failed to copy all the data, exit */
							dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
						}
					}

					kfree(p_kBuf);
				} else {
					dev_err(pTAS2560->dev, "read no mem\n");
				}
			}
		}
		break;

		case TIAUDIO_CMD_SAMPLERATE:
		{
			if(g_logEnable) dev_info(pTAS2560->dev,
						"TIAUDIO_CMD_SAMPLERATE: count = %d\n", (int)count);
			if(count == REG_COUNT_4){
				p_kBuf = (unsigned char *)kzalloc(count, GFP_KERNEL);
				if(p_kBuf != NULL) {
					p_kBuf[0] = (pTAS2560->mnSamplingRate&0x000000ff);
					p_kBuf[1] = ((pTAS2560->mnSamplingRate&0x0000ff00)>>8);
					p_kBuf[2] = ((pTAS2560->mnSamplingRate&0x00ff0000)>>16);
					p_kBuf[3] = ((pTAS2560->mnSamplingRate&0xff000000)>>24);

					ret = copy_to_user(buf, p_kBuf, count);
					if (0 != ret) {
						/* Failed to copy all the data, exit */
						dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
					}

					kfree(p_kBuf);
				}else{
					dev_err(pTAS2560->dev, "read no mem\n");
				}
			}
		}
		break;

		case TIAUDIO_CMD_BITRATE:
		{
			if(g_logEnable) dev_info(pTAS2560->dev,
						"TIAUDIO_CMD_BITRATE: count = %d\n", (int)count);

			if(count == REG_COUNT_1){
				int bitRate = tas2560_get_bit_rate(pTAS2560);
				if(bitRate >=0) {
					ret = copy_to_user(buf, &bitRate, 1);
					if (0 != ret) {
					/* Failed to copy all the data, exit */
						dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
					}
				}
			}
		}
		break;

		case TIAUDIO_CMD_DACVOLUME:
		{
			if(g_logEnable) dev_info(pTAS2560->dev, "TIAUDIO_CMD_DACVOLUME: count = %d\n", (int)count);

			if(count == REG_COUNT_1){
				int volume = tas2560_get_volume(pTAS2560);
				if(volume >=0){
					ret = copy_to_user(buf, &volume, 1);
					if (0 != ret) {
					/* Failed to copy all the data, exit */
						dev_err(pTAS2560->dev, "copy to user fail %d\n", ret);
					}
				}
			}
		}
		break;
	}
	 pTAS2560->mnDBGCmd = 0;

	mutex_unlock(&pTAS2560->file_lock);
	return count;
}

static ssize_t tas2560_file_write(struct file *file, const char *buf, size_t count, loff_t *ppos)
{
	struct tas2560_priv *pTAS2560 = (struct tas2560_priv *)file->private_data;
	int ret = 0;
	unsigned char *p_kBuf = NULL;
	unsigned int reg = 0;
	unsigned int len = 0;

	if (!buf) {
		dev_err(pTAS2560->dev,"buf is NULL\n");
		return count;
	}

	if (count == 0) {
		dev_err(pTAS2560->dev, "count is 0, return\n");
		return count;
	}

	mutex_lock(&pTAS2560->file_lock);

	p_kBuf = (unsigned char *)kzalloc(count, GFP_KERNEL);
	if(p_kBuf == NULL) {
		dev_err(pTAS2560->dev, "write no mem\n");
		goto err;
	}

	ret = copy_from_user(p_kBuf, buf, count);
	if (0 != ret) {
		dev_err(pTAS2560->dev,"copy_from_user failed.\n");
		goto err;
	}

	pTAS2560->mnDBGCmd = p_kBuf[0];
	switch(pTAS2560->mnDBGCmd)
	{
		case TIAUDIO_CMD_REG_WITE:
		if(count > REG_COUNT_5){	/* if count is larger than 5, we need to write more data */
			reg = ((unsigned int)p_kBuf[1] << 24) +
				((unsigned int)p_kBuf[2] << 16) +
				((unsigned int)p_kBuf[3] << 8) +
				(unsigned int)p_kBuf[4];	/* to make the whole register value */
			len = count - REG_COUNT_5;
			if(len == 1){
				ret = pTAS2560->write(pTAS2560, reg, p_kBuf[REG_COUNT_5]);
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_WITE, Reg=0x%x, Val=0x%x\n",
					reg, p_kBuf[REG_COUNT_5]);
			}else{
				ret = pTAS2560->bulk_write(pTAS2560, reg, &p_kBuf[REG_COUNT_5], len);
			}
		}else{
			dev_err(pTAS2560->dev,"%s, write len fail, count=%d.\n", __FUNCTION__, (int)count);
		}
		pTAS2560->mnDBGCmd = 0;
		break;

		case TIAUDIO_CMD_REG_READ:
		if(count == REG_COUNT_5){
			pTAS2560->mnCurrentReg = ((unsigned int)p_kBuf[1] << 24) +
				((unsigned int)p_kBuf[2] << 16) +
				((unsigned int)p_kBuf[3] << 8) 	+
				(unsigned int)p_kBuf[4];
			if(g_logEnable){
				dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_REG_READ, whole=0x%x\n",
					pTAS2560->mnCurrentReg);
			}
		}else{
			dev_err(pTAS2560->dev,"read len fail.\n");
		}
		break;

		case TIAUDIO_CMD_DEBUG_ON:
		{
			if(count == REG_COUNT_2){
				g_logEnable = p_kBuf[1];
			}
			pTAS2560->mnDBGCmd = 0;
		}
		break;

		case TIAUDIO_CMD_SAMPLERATE:
		{
			if(count == REG_COUNT_5){
				unsigned int nSampleRate = ((unsigned int)p_kBuf[1] << 24) +
					((unsigned int)p_kBuf[2] << 16) +
					((unsigned int)p_kBuf[3] << 8) 	+
					(unsigned int)p_kBuf[4];
				if(g_logEnable)
					dev_info(pTAS2560->dev,
					"TIAUDIO_CMD_SAMPLERATE, set to %d\n",
					nSampleRate);

				tas2560_set_samplerate(pTAS2560, nSampleRate);
			}
		}
		break;

		case TIAUDIO_CMD_BITRATE:
		{
			if(count == REG_COUNT_2){
				if(g_logEnable)
					dev_info(pTAS2560->dev, "TIAUDIO_CMD_BITRATE, set to %d\n", p_kBuf[1]);

				tas2560_set_bit_rate(pTAS2560, p_kBuf[1]);
			}
		}
		break;

		case TIAUDIO_CMD_DACVOLUME:
		{
			if(count == REG_COUNT_2){
				unsigned char volume = (p_kBuf[1] & 0x0f);
				if(g_logEnable)
					dev_info(pTAS2560->dev, "TIAUDIO_CMD_DACVOLUME, set to %d\n", volume);

				tas2560_set_volume(pTAS2560, volume);
			}
		}
		break;

		case TIAUDIO_CMD_SPEAKER:
		{
			if(count == REG_COUNT_2){
				if(g_logEnable)
					dev_info(pTAS2560->dev, "TIAUDIO_CMD_SPEAKER, set to %d\n", p_kBuf[1]);
				tas2560_enable(pTAS2560, (p_kBuf[1]>0));
			}
		}
		break;

		default:
			pTAS2560->mnDBGCmd = 0;
		break;
	}

err:
	if(p_kBuf != NULL)
		kfree(p_kBuf);

	mutex_unlock(&pTAS2560->file_lock);

	return count;
}

static long tas2560_file_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct tas2560_priv *pTAS2560 = file->private_data;
	int ret = 0;

	mutex_lock(&pTAS2560->file_lock);

	switch (cmd) {
		case SMARTPA_SPK_DAC_VOLUME:
		{
			u8 volume = (arg & 0x0f);
			tas2560_set_volume(pTAS2560, volume);
		}
		break;

		case SMARTPA_SPK_POWER_ON:
		{
			tas2560_enable(pTAS2560, true);
		}
		break;

		case SMARTPA_SPK_POWER_OFF:
		{
			tas2560_enable(pTAS2560, false);
		}
		break;

		case SMARTPA_SPK_SET_SAMPLERATE:
		{
			tas2560_set_samplerate(pTAS2560, arg);
		}
		break;

		case SMARTPA_SPK_SET_BITRATE:
		{
			tas2560_set_bit_rate(pTAS2560, arg);
		}
		break;

		case SMARTPA_SPK_DUMP_REGISTER:
		{
			tas2560_dump_regs_all((int __user *)arg);
		}
		break;
	}

	mutex_unlock(&pTAS2560->file_lock);
	return ret;
}

static struct file_operations fops =
{
	.owner = THIS_MODULE,
	.read = tas2560_file_read,
	.write = tas2560_file_write,
	.unlocked_ioctl = tas2560_file_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = tas2560_file_unlocked_ioctl,
#endif
	.open = tas2560_file_open,
	.release = tas2560_file_release,
};

static struct miscdevice tas2560_misc_devs[] = {
	{
		.minor = MISC_DYNAMIC_MINOR,
		.name = "tas2560_pri_l",
		.fops = &fops,
	},
	{
		.minor = MISC_DYNAMIC_MINOR,
		.name = "tas2560_pri_r",
		.fops = &fops,
	},
	{
		.minor = MISC_DYNAMIC_MINOR,
		.name = "tas2560_sec_l",
		.fops = &fops,
	},
	{
		.minor = MISC_DYNAMIC_MINOR,
		.name = "tas2560_sec_r",
		.fops = &fops,
	}
};

int tas2560_register_misc(struct tas2560_priv *pTAS2560)
{
	int ret = 0;
	ret = misc_register(&tas2560_misc_devs[pTAS2560->type]);
	if (ret)
		dev_err(pTAS2560->dev,
			"%s: register misc #%u failed %d!\n",
			__func__, pTAS2560->type, ret);
	return ret;
}

int tas2560_deregister_misc(struct tas2560_priv * pTAS2560)
{
	misc_deregister(&tas2560_misc_devs[pTAS2560->type]);
	return 0;
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 Misc Smart Amplifier driver");
MODULE_LICENSE("GPLv2");
