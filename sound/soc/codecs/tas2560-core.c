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
**     tas2560-core.c
**
** Description:
**     TAS2560 common functions for Android Linux
**
** =============================================================================
*/
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
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/hw_audio_info.h>
#include "tas2560.h"

#define DIV_P_1		1
#define DIV_P_2		2
#define DIV_P_3		3

#define MAX_IRQ_TYPE 	9
#define MAX_CLIENTS 	8
#define REG_CAP_MAX 	102
#define RECOVERY_RETRY_TIME 	1
static int8_t dump_regs = 1;
static unsigned int irq_status_reg1 = 0;
static unsigned int irq_status_reg2 = 0;

static unsigned int p_tas2560_startup_data[] =
{
	/* reg address			size	values	*/
	TAS2560_CLK_SEL, 		0x01,	0x01,
	TAS2560_SET_FREQ, 		0x01,	0x10,
	TAS2560_MUTE_REG,		0x01,	0x41,
	TAS2560_MDELAY,			0x01,	0x10,
	REG_END, REG_END
};

static unsigned int p_tas2560_idle_chnl_detect[] = {
	TAS2560_IDLE_CHNL_DETECT,	0x04,	0, 0, 0, 0,
	REG_END, REG_END,
};

static unsigned int p_tas2560_boost_headroom_data[] =
{
	TAS2560_BOOST_HEAD,		0x04,	0x06, 0x66, 0x66, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_thermal_foldback[] =
{
	TAS2560_THERMAL_FOLDBACK,		0x04,	0x39, 0x80, 0x00, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_HPF_data[] =
{
	/* reg address			size	values */
	/*Isense path HPF cut off -> 2Hz*/
	TAS2560_ISENSE_PATH_CTL1,	0x04,	0x7F, 0xFB, 0xB5, 0x00,
	TAS2560_ISENSE_PATH_CTL2,	0x04,	0x80, 0x04, 0x4C, 0x00,
	TAS2560_ISENSE_PATH_CTL3,	0x04,	0x7F, 0xF7, 0x6A, 0x00,
	/*all pass*/
	TAS2560_HPF_CUTOFF_CTL1,	0x04,	0x7F, 0xFF, 0xFF, 0xFF,
	TAS2560_HPF_CUTOFF_CTL2,	0x04,	0x00, 0x00, 0x00, 0x00,
	TAS2560_HPF_CUTOFF_CTL3,	0x04,	0x00, 0x00, 0x00, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_Vsense_biquad_data[] =
{
	/* vsense delay in biquad = 3/8 sample @48KHz */
	TAS2560_VSENSE_DEL_CTL1,	0x04,	0x3a, 0x46, 0x74, 0x00,
	TAS2560_VSENSE_DEL_CTL2,	0x04,	0x22, 0xf3, 0x07, 0x00,
	TAS2560_VSENSE_DEL_CTL3,	0x04,	0x80, 0x77, 0x61, 0x00,
	TAS2560_VSENSE_DEL_CTL4,	0x04,	0x22, 0xa7, 0xcc, 0x00,
	TAS2560_VSENSE_DEL_CTL5,	0x04,	0x3a, 0x0c, 0x93, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_voltage_limiter_data[] =
{
	TAS2560_VLIMIT_THRESHOLD1,	0x04,	0x44, 0x0e, 0x56, 0x00,
	TAS2560_VLIMIT_THRESHOLD2,	0x04,	0x41, 0xa5, 0xe3, 0x00,
	TAS2560_VLIMIT_THRESHOLD3,	0x04,	0x40, 0x94, 0x13, 0x00,
	TAS2560_VLIMIT_THRESHOLD4,	0x04,	0x00, 0x22, 0x1d, 0x00,
	TAS2560_VLIMIT_THRESHOLD5,	0x04,	0x02, 0x00, 0x00, 0x00,
	TAS2560_VLIMIT_THRESHOLD6,	0x04,	0x7c, 0xf7, 0x66, 0x00,
	TAS2560_VLIMIT_THRESHOLD7,	0x04,	0x00, 0x88, 0x40, 0x00,
	TAS2560_VLIMIT_THRESHOLD8,	0x04,	0x00, 0x06, 0xd3, 0x00,
	TAS2560_VLIMIT_THRESHOLD10,	0x04,	0x33, 0x43, 0x96, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_voltage_limiter_disable_data[] =
{
	TAS2560_VLIMIT_THRESHOLD1,	0x04,	0x3d, 0x99, 0x9a, 0x00,
	TAS2560_VLIMIT_THRESHOLD2,	0x04,	0x30, 0x00, 0x00, 0x00,
	TAS2560_VLIMIT_THRESHOLD3,	0x04,	0x50, 0x00, 0x00, 0x00,
	TAS2560_VLIMIT_THRESHOLD4,	0x04,	0x00, 0x22, 0x1d, 0x00,
	TAS2560_VLIMIT_THRESHOLD5,	0x04,	0x02, 0x00, 0x00, 0x00,
	TAS2560_VLIMIT_THRESHOLD6,	0x04,	0x7c, 0xf7, 0x66, 0x00,
	TAS2560_VLIMIT_THRESHOLD7,	0x04,	0x00, 0x88, 0x40, 0x00,
	TAS2560_VLIMIT_THRESHOLD8,	0x04,	0x00, 0x06, 0xd3, 0x00,
	TAS2560_VLIMIT_THRESHOLD10,	0x04,	0x28, 0x00, 0x00, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_48khz_data[] =
{
	/* reg address			size	values */
	TAS2560_SR_CTRL1,		0x01,	0x01,
	TAS2560_SR_CTRL2,		0x01,	0x08,
	TAS2560_SR_CTRL3,		0x01,	0x10,
	REG_END, REG_END
};

static unsigned int p_tas2560_16khz_data[] =
{
	/* reg address			size	values */
	TAS2560_SR_CTRL1,		0x01,	0x01,
	TAS2560_SR_CTRL2,		0x01,	0x18,
	TAS2560_SR_CTRL3,		0x01,	0x20,
	REG_END, REG_END
};

static unsigned int p_tas2560_8khz_data[] =
{
	/* reg address			size	values */
	TAS2560_SR_CTRL1,		0x01,	0x01,
	TAS2560_SR_CTRL2,		0x01,	0x30,
	TAS2560_SR_CTRL3,		0x01,	0x20,
	REG_END, REG_END
};

static unsigned int p_tas2560_4Ohm_data[] =
{
	/* reg address			size	values */
	TAS2560_BOOST_ON,		0x04,	0x6f, 0x5c, 0x28, 0xf5,
	TAS2560_BOOST_OFF,		0x04,	0x67, 0xae, 0x14, 0x7a,
	TAS2560_BOOST_TABLE_CTRL1,	0x04,	0x1c, 0x00, 0x00, 0x00,
	TAS2560_BOOST_TABLE_CTRL2,	0x04,	0x1f, 0x0a, 0x3d, 0x70,
	TAS2560_BOOST_TABLE_CTRL3,	0x04,	0x22, 0x14, 0x7a, 0xe1,
	TAS2560_BOOST_TABLE_CTRL4,	0x04,	0x25, 0x1e, 0xb8, 0x51,
	TAS2560_BOOST_TABLE_CTRL5,	0x04,	0x28, 0x28, 0xf5, 0xc2,
	TAS2560_BOOST_TABLE_CTRL6,	0x04,	0x2b, 0x33, 0x33, 0x33,
	TAS2560_BOOST_TABLE_CTRL7,	0x04,	0x2e, 0x3d, 0x70, 0xa3,
	TAS2560_BOOST_TABLE_CTRL8,	0x04,	0x31, 0x47, 0xae, 0x14,
	REG_END, REG_END
};

static unsigned int p_tas2560_6Ohm_data[] =
{
	/* reg address			size	values */
	TAS2560_BOOST_ON,		0x04,	0x73, 0x33, 0x33, 0x33,
	TAS2560_BOOST_OFF,		0x04,	0x6b, 0x85, 0x1e, 0xb8,
	TAS2560_BOOST_TABLE_CTRL1,	0x04,	0x1d, 0x99, 0x99, 0x99,
	TAS2560_BOOST_TABLE_CTRL2,	0x04,	0x20, 0xcc, 0xcc, 0xcc,
	TAS2560_BOOST_TABLE_CTRL3,	0x04,	0x24, 0x00, 0x00, 0x00,
	TAS2560_BOOST_TABLE_CTRL4,	0x04,	0x27, 0x33, 0x33, 0x33,
	TAS2560_BOOST_TABLE_CTRL5,	0x04,	0x2a, 0x66, 0x66, 0x66,
	TAS2560_BOOST_TABLE_CTRL6,	0x04,	0x2d, 0x99, 0x99, 0x99,
	TAS2560_BOOST_TABLE_CTRL7,	0x04,	0x30, 0xcc, 0xcc, 0xcc,
	TAS2560_BOOST_TABLE_CTRL8,	0x04,	0x34, 0x00, 0x00, 0x00,
	REG_END, REG_END
};

static unsigned int p_tas2560_8Ohm_data[] =
{
	/* reg address			size	values */
	TAS2560_BOOST_ON,		0x04,	0x75, 0xC2, 0x8e, 0x00,
	TAS2560_BOOST_OFF,		0x04,	0x6E, 0x14, 0x79, 0x00,
	TAS2560_BOOST_TABLE_CTRL1,	0x04,	0x1e, 0x00, 0x00, 0x00,
	TAS2560_BOOST_TABLE_CTRL2,	0x04,	0x21, 0x3d, 0x71, 0x00,
	TAS2560_BOOST_TABLE_CTRL3,	0x04,	0x24, 0x7a, 0xe1, 0x00,
	TAS2560_BOOST_TABLE_CTRL4,	0x04,	0x27, 0xb8, 0x52, 0x00,
	TAS2560_BOOST_TABLE_CTRL5,	0x04,	0x2a, 0xf5, 0xc3, 0x00,
	TAS2560_BOOST_TABLE_CTRL6,	0x04,	0x2e, 0x33, 0x33, 0x00,
	TAS2560_BOOST_TABLE_CTRL7,	0x04,	0x31, 0x70, 0xa4, 0x00,
	TAS2560_BOOST_TABLE_CTRL8,	0x04,	0x34, 0xae, 0x14, 0x00,
	TAS2560_BOOST_TABLE_CTRL9,	0x04,	0x00, 0x00, 0x00, 0x00,
	REG_END, REG_END
};

static struct tas2560_irq irqtable[MAX_IRQ_TYPE] =
{
	{INT_SAR, 	0x00000002, 0,	0x26,	"INT_SAR"	},
	{INT_CLK2,	0x00000004,	0,	0x26,	"INT_CLK2"	},
	{INT_BRNO,	0x00000008,	0,	0x26,	"INT_BRNO"	},
	{INT_OVRT,	0x00000010,	0,	0x26,	"INT_OVRT"	},
	{INT_CLK1,	0x00000020,	0,	0x26,	"INT_CLK1"	},
	{INT_AUV,	0x00000040,	0,	0x26,	"INT_AUV"	},
	{INT_OVRI,	0x00000080,	0,	0x26,	"INT_OVRI"	},
	{INT_MCHLT,	0x00000040,	0,	0x27,	"INT_MCHLT"	},
	{INT_WCHLT,	0x00000080,	0,	0x27,	"INT_WCHLT"	},
};

static struct tas2560_reg regtable[REG_CAP_MAX] =
{
	{0,	0},	{1,	0},	{2,	0},	{3,	0},	{4,	0},
	{5,	0},	{6,	0},	{7,	0},	{8,	0},	{9,	0},
	{10,	0},	{11,	0},	{12,	0},	{13,	0},	{14,	0},
	{15,	0},	{16,	0},	{17,	0},	{18,	0},	{19,	0},
	{20,	0},	{21,	0},	{22,	0},	{23,	0},	{24,	0},
	{25,	0},	{26,	0},	{27,	0},	{28,	0},	{29,	0},
	{30,	0},	{31,	0},	{32,	0},	{33,	0},	{34,	0},
	{35,	0},	{36,	0},	{37,	0},	{38,	0},	{39,	0},
	{40,	0},	{41,	0},	{42,	0},	{43,	0},	{44,	0},
	{45,	0},	{46,	0},	{47,	0},	{48,	0},	{49,	0},
	{50,	0},	{51,	0},	{52,	0},	{53,	0},	{54,	0},
	{55,	0},	{56,	0},	{57,	0},	{58,	0},	{59,	0},
	{60,	0},	{61,	0},	{62,	0},	{63,	0},	{64,	0},
	{65,	0},	{66,	0},	{67,	0},	{68,	0},	{69,	0},
	{70,	0},	{71,	0},	{72,	0},	{73,	0},	{74,	0},
	{75,	0},	{76,	0},	{77,	0},	{78,	0},	{79,	0},
	{80,	0},	{81,	0},	{82,	0},	{83,	0},	{84,	0},
	{85,	0},	{86,	0},	{87,	0},	{88,	0},	{89,	0},
	{90,	0},	{91,	0},	{92,	0},	{93,	0},	{94,	0},
	{95,	0},	{96,	0},	{97,	0},	{98,	0},	{99,	0},
	{100,0},	{101,0},
};

static void tas2560_i2c_load_data(struct tas2560_priv *pTAS2560,
				  unsigned int *pData) {
	unsigned int nRegister = 0;
	unsigned int *nData = 0;
	unsigned int nLength = 0;
	unsigned int nLoop = 0;
	unsigned int i = 0;

	if ((pTAS2560 == NULL) || (pData == NULL)) {
		pr_err("%s, pTAS2560 or pData is error\n", __func__);
		return;
	}

	do {
		nRegister = pData[nLength];		/* the 1st value in array */
		nLoop = pData[nLength + 1];		/* the 2nd value in array */
		nData = &pData[nLength + 2];	/* the 3rd value in array */
		if (nRegister == TAS2560_MDELAY){
			mdelay(nData[0]);
		} else {
			if (nRegister != REG_END){
				i = 0;
				while (nLoop != 0) {
					pTAS2560->write(pTAS2560,((nRegister+i)), nData[i]);
					nLoop--;
					i++;
				}
			}
		}
		nLength = nLength + 2 + pData[nLength+1] ;
	} while (nRegister != REG_END);		/* the and of registers array must be 0xFFFFFFFF */
}

void tas2560_sw_shutdown(struct tas2560_priv *pTAS2560, int sw_shutdown)
{
	if (sw_shutdown)
		pTAS2560->update_bits(pTAS2560, TAS2560_PWR_REG,
				    TAS2560_PWR_BIT_MASK,0);
	else
		pTAS2560->update_bits(pTAS2560, TAS2560_PWR_REG,
				    TAS2560_PWR_BIT_MASK, TAS2560_PWR_BIT_MASK);
}

int tas2560_set_samplerate(struct tas2560_priv *pTAS2560, unsigned int nSamplingRate)
{
	int ret = 0;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return -EINVAL;
	}

	dev_dbg(pTAS2560->dev,"%s: sample rate: %u\n", __func__, nSamplingRate);
	switch(nSamplingRate){
	case 48000:
		tas2560_i2c_load_data(pTAS2560, p_tas2560_48khz_data);
		break;
	case 44100:
		pTAS2560->write(pTAS2560, TAS2560_SR_CTRL1, 0x11);	/* register values */
		break;
	case 16000:
		tas2560_i2c_load_data(pTAS2560, p_tas2560_16khz_data);
		break;
	case 8000:
		tas2560_i2c_load_data(pTAS2560, p_tas2560_8khz_data);
		break;
	default:
		dev_err(pTAS2560->dev, "Invalid Sampling rate, %d\n", nSamplingRate);
		ret = -1;
		break;
	}

	if(ret >= 0)
		pTAS2560->mnSamplingRate = nSamplingRate;

	return ret;
}

int tas2560_set_bit_rate(struct tas2560_priv *pTAS2560, unsigned int nBitRate)
{
	int ret = 0, n = -1;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return -EINVAL;
	}

	if (smartpa_is_four_tas2560())
		nBitRate = BIT_RATE_16; /* force to 16bit */

	dev_dbg(pTAS2560->dev, "%s: nBitRate=%d\n", __func__, nBitRate);
	switch(nBitRate){
		case BIT_RATE_16:
			n = REG_VALUE_0;	/* set word length to 16 bits */
 			break;
		case BIT_RATE_20:
			n = REG_VALUE_1;	/* set word length to 20 bits */
			break;
		case BIT_RATE_24:
			n = REG_VALUE_2;	/* set word length to 24 bits */
			break;
		case BIT_RATE_32:
			n = REG_VALUE_3;	/* set word length to 32 bits */
			break;
	}

	if (n >= 0)
		ret = pTAS2560->update_bits(pTAS2560,
			TAS2560_DAI_FMT, TAS2560_DAI_WORDLEN_MASK, n);

	return ret;
}

int tas2560_get_bit_rate(struct tas2560_priv *pTAS2560)
{
	int nBitRate = -1, value = -1, ret = 0;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return -EINVAL;
	}

	ret = pTAS2560->read(pTAS2560, TAS2560_DAI_FMT, &value);
	value &= TAS2560_DAI_WORDLEN_MASK;

	switch (value) {
		case REG_VALUE_0:
			nBitRate = BIT_RATE_16;
			break;
		case REG_VALUE_1:
			nBitRate = BIT_RATE_20;
			break;
		case REG_VALUE_2:
			nBitRate = BIT_RATE_24;
			break;
		case REG_VALUE_3:
			nBitRate = BIT_RATE_32;
			break;
		default:
			break;
	}

	return nBitRate;
}

int tas2560_set_ASI_fmt(struct tas2560_priv *pTAS2560, unsigned int fmt)
{
	u8 serial_format = 0, asi_cfg_1=0;
	int ret = 0;

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		asi_cfg_1 = 0x00;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		asi_cfg_1 = TAS2560_WCLKDIR;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		asi_cfg_1 = TAS2560_BCLKDIR;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		asi_cfg_1 = (TAS2560_BCLKDIR | TAS2560_WCLKDIR);
		break;
	default:
		dev_err(pTAS2560->dev, "ASI format master is not found\n");
		ret = -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		asi_cfg_1 |= 0x00;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		asi_cfg_1 |= TAS2560_WCLKINV;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		asi_cfg_1 |= TAS2560_BCLKINV;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		asi_cfg_1 = (TAS2560_WCLKINV | TAS2560_BCLKINV);
		break;
	default:
		dev_err(pTAS2560->dev, "ASI format Inverse is not found\n");
		ret = -EINVAL;
	}

	asi_cfg_1 |= TAS2560_TRISTATE;

	dev_dbg(pTAS2560->dev, "%s: set ASI fmt: %x\n", __func__, asi_cfg_1);
	pTAS2560->update_bits(pTAS2560, TAS2560_ASI_CFG_1, TAS2560_DIRINV_MASK, asi_cfg_1);

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK ){
	case (SND_SOC_DAIFMT_I2S ):
		serial_format |= TAS2560_DATAFORMAT_I2S;
		break;
	case (SND_SOC_DAIFMT_DSP_A ):
	case (SND_SOC_DAIFMT_DSP_B ):
		serial_format |= TAS2560_DATAFORMAT_DSP;
		break;
	case (SND_SOC_DAIFMT_RIGHT_J):
		serial_format |= TAS2560_DATAFORMAT_RIGHT_J;
		break;
	case (SND_SOC_DAIFMT_LEFT_J):
		serial_format |= TAS2560_DATAFORMAT_LEFT_J;
		break;
	default:
		dev_err(pTAS2560->dev, "DAI Format is not found, fmt=0x%x\n", fmt);
		ret = -EINVAL;
		break;
	}

	pTAS2560->update_bits(pTAS2560, TAS2560_DAI_FMT, TAS2560_DAI_FMT_MASK, serial_format);

	return ret;
}

int tas2560_set_pll_clkin(struct tas2560_priv *pTAS2560, int clk_id,
				  unsigned int freq)
{
	int ret = 0;
	unsigned char pll_in = 0;
	dev_dbg(pTAS2560->dev, "%s: clkid=%d\n", __func__, clk_id);

	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return -EINVAL;
	}

	switch (clk_id) {
	case TAS2560_PLL_CLKIN_BCLK:
		pll_in = TAS2560_PLL_CLKSRC_BCLK;
		break;
	case TAS2560_PLL_CLKIN_MCLK:
		pll_in = TAS2560_PLL_CLKSRC_MCLK;
		break;
	case TAS2560_PLL_CLKIN_PDMCLK:
		pll_in = TAS2560_PLL_CLKSRC_PDMCLK;
		break;
	default:
		dev_err(pTAS2560->dev, "Invalid clk id: %d\n", clk_id);
		ret = -EINVAL;
		break;
	}

	if (ret >= 0) {
		pTAS2560->update_bits(pTAS2560,
			TAS2560_CLK_SEL, TAS2560_PLL_SRC_MASK, pll_in << 6);
		pTAS2560->mnClkid = clk_id;
		pTAS2560->mnClkin = freq;
	}
	return ret;
}

int tas2560_set_pll(struct tas2560_priv *pTAS2560, int pll_clkin)
{
	unsigned int pll_clk = pTAS2560->mnSamplingRate * VAL_1K;
	unsigned int power = 0, temp = 0;
	unsigned int d = 0, pll_clkin_divide = 0;
	u8 j = 0, p = 0;
	int ret = 0;

	if (!pll_clkin || (pll_clkin < 0)) {
		if (pTAS2560->mnClkid != TAS2560_PLL_CLKIN_BCLK){
			dev_err(pTAS2560->dev,
				"pll_in %d, pll_clkin frequency err:%d\n", pTAS2560->mnClkid, pll_clkin);
			return -EINVAL;
		}

		pll_clkin = pTAS2560->mnSamplingRate * pTAS2560->mnFrameSize;
		pr_debug("%s: pll_clk=%u, rate=%d, frame_sz=%d, pll_clkin=%u\n",
			__func__, pll_clk, pTAS2560->mnSamplingRate, pTAS2560->mnFrameSize, pll_clkin);
	}

	pTAS2560->read(pTAS2560, TAS2560_PWR_REG, &power);
	if (power & TAS2560_PWR_BIT_MASK) {
		dev_dbg(pTAS2560->dev, "power down to update PLL\n");
		pTAS2560->write(pTAS2560, TAS2560_PWR_REG, TAS2560_PWR_BIT_MASK|TAS2560_MUTE_MASK);
	}

	/* Fill in the PLL control registers for J & D
	 * pll_clk = (pll_clkin * J.D) / P
	 * Need to fill in J and D here based on incoming freq
	 */
	if (pll_clkin <= TAS_PLL_FREQ_LEVEL_1)
		p = DIV_P_1;	/* divide value is 1 */
	else if (pll_clkin <= TAS_PLL_FREQ_LEVEL_2)
		p = DIV_P_2;	/* divide value is 2 */
	else if (pll_clkin <= TAS_PLL_FREQ_LEVEL_3)
		p = DIV_P_3;	/* divide value is 3 */
	else {
		dev_err(pTAS2560->dev, "PLL Clk In %d not covered here\n", pll_clkin);
		ret = -EINVAL;
	}

	if(ret >= 0){
		j = (pll_clk * p) / pll_clkin;
		d = (pll_clk * p) % pll_clkin;
		d /= (pll_clkin / TAS_PLL_DIV);

		pr_debug("%s: d=%u, p=%d, j=%u\n", __func__, d, p, j);
		pll_clkin_divide = pll_clkin/(1<<p);
		pr_debug("%s: pll_clkin_divide=%u\n", __func__, pll_clkin_divide);
		if ((d == 0) &&((pll_clkin_divide < TAS_PLL_DIV_LEVEL_1) || (pll_clkin_divide > TAS_PLL_DIV_LEVEL_4))) {
			dev_err(pTAS2560->dev, "#1 PLL cal ERROR!!!, pll_in=%d\n", pll_clkin);
			ret = -EINVAL;
		}

		if ((d != 0) &&((pll_clkin_divide < TAS_PLL_DIV_LEVEL_3) || (pll_clkin_divide > TAS_PLL_DIV_LEVEL_4))) {
			dev_err(pTAS2560->dev, "#2 PLL cal ERROR!!!, pll_in=%d\n", pll_clkin);
			ret = -EINVAL;
		}

		if (j == 0) {
			dev_err(pTAS2560->dev, "#3 PLL cal ERROR!!!, j ZERO\n");
			ret = -EINVAL;
		}
	}

	if (ret >= 0) {
		pr_info("%s: type=%u, clk_in=%d, P=%d, J.D=%d.%d\n",
				__func__, pTAS2560->type, pll_clkin, p, j, d);
		//update P
		if (p == TAS_P_VAL)
			temp = 0;
		else
			temp = p;
		pTAS2560->update_bits(pTAS2560, TAS2560_CLK_SEL, TAS2560_PLL_P_MASK, temp);

		//Update J
		temp = j;
		if (pll_clkin < TAS_PLL_DIV_LEVEL_2)
			temp |= 0x80;	/* register bit value */
		pTAS2560->write(pTAS2560, TAS2560_SET_FREQ, temp);

		//Update D
		temp = (d & TAS2560_PLL_D_LSB_MASK);
		pTAS2560->write(pTAS2560, TAS2560_PLL_D_LSB, temp);
		temp = ((d & TAS2560_PLL_D_MSB_MASK) >> 8);
		pTAS2560->write(pTAS2560, TAS2560_PLL_D_MSB, temp);
	}

	/* Restore PLL status */
	if (power & TAS2560_PWR_BIT_MASK)
		pTAS2560->write(pTAS2560, TAS2560_PWR_REG, power);

	return ret;
}

int tas2560_setLoad(struct tas2560_priv *pTAS2560, int load)
{
	int ret = 0;
	int value = -1;

	switch (load) {
	case LOAD_8OHM:
		value = 0; /* Set full-scale of Isense channel to 1.25A */
		tas2560_i2c_load_data(pTAS2560,p_tas2560_8Ohm_data);
		break;
	case LOAD_6OHM:
		value = 1; /* Set full-scale of Isense channel to 1.5A */
		tas2560_i2c_load_data(pTAS2560,p_tas2560_6Ohm_data);
		break;
	case LOAD_4OHM:
		value = 2; /* Set full-scale of Isense channel to 1.75A */
		tas2560_i2c_load_data(pTAS2560,p_tas2560_4Ohm_data);
		break;
	default:
		break;
	}

	if (value >= 0)
		pTAS2560->update_bits(pTAS2560, TAS2560_LOAD, LOAD_MASK, value << 3);	/* LOAD register bit */
	return ret;
}

int tas2560_getLoad(struct tas2560_priv *pTAS2560)
{
	int ret = -1;
	int value = -1;

	pTAS2560->read(pTAS2560, TAS2560_LOAD, &value);

	/* get current scale of Isense */
	value = (value & TAS2560_ISENSE_SCALE_MASK) >> 3;	/* LOAD register bit */

	switch(value){
	case REG_VALUE_0:
		ret = LOAD_8OHM;
		break;
	case REG_VALUE_1:
		ret = LOAD_6OHM;
		break;
	case REG_VALUE_2:
		ret = LOAD_4OHM;
		break;
	default:
		break;
	}

	return ret;
}

int tas2560_get_volume(struct tas2560_priv *pTAS2560)
{
	int ret = -1;
	int value = -1;

	dev_dbg(pTAS2560->dev,"%s\n", __func__);
	ret = pTAS2560->read(pTAS2560, TAS2560_SPK_CTRL_REG, &value);
	if(ret >=0)
		return (value&0x0f);	/* register bit value */

	return ret;
}

int tas2560_set_volume(struct tas2560_priv *pTAS2560, int volume)
{
	return pTAS2560->update_bits(pTAS2560, TAS2560_SPK_CTRL_REG, 0x0f, volume&0x0f);
}

int tas2560_set_output_channel(struct tas2560_priv *pTAS2560, unsigned int channel)
{
	int ret = -1;
	unsigned int val = 0x1;

	dev_dbg(pTAS2560->dev, "%s: select channel: %d\n", __func__, channel);

	if (channel == 0) /* L */
		val = 0x0;
	else if (channel == 1) /* R */
		val = REG_VALUE_1;
	else if (channel == 2) /* L+R/2 */
		val = REG_VALUE_2;
	else if (channel == 3) /* mono PCM */
		val = REG_VALUE_3;
	else {
		pr_err("invalid channel number !\n");
		return ret;
	}
	ret = pTAS2560->update_bits(pTAS2560, TAS2560_ASI_CHANNEL, 0x03, val);	/* register bit value */
	return ret;
}

int tas2560_set_mute(struct tas2560_priv *pTAS2560, unsigned int mute)
{
	return pTAS2560->update_bits(pTAS2560, TAS2560_MUTE_REG, 0x01, mute);	/* register bit value */
}

int tas2560_set_dsp_mode(struct tas2560_priv *pTAS2560, unsigned value)
{
	return pTAS2560->update_bits(pTAS2560, TAS2560_DEV_MODE_REG, 0x07, value);	/* register bit value */
}

int tas2560_set_offset(struct tas2560_priv *pTAS2560, unsigned int offset)
{
	return pTAS2560->write(pTAS2560, TAS2560_ASI_OFFSET_1, offset);
}

/*
 * Select level to limit boost current for power consumption
 */
void tas2560_set_edge(struct tas2560_priv *pTAS2560, int level)
{
	unsigned int val;

	dev_info(pTAS2560->dev,
		"%s: set level %d\n", __func__, level);
	switch (level) {
	case TAS2560_VOLTAGE_MV_NO_LIMIT:
		val = 0x83; /* default, without voltage limit */
		break;
	case TAS2560_VOLTAGE_MV_ABOVE_4100:
		val = 0x43; /* limit with voltage above 4.1v */
		break;
	case TAS2560_VOLTAGE_MV_3900_4100:
		val = 0x42; /* limit with voltage from 3.9v to 4.1v */
		break;
	case TAS2560_VOLTAGE_MV_3700_3900:
		val = 0x41; /* limit with voltage from 3.7v to 3.9v */
		break;
	case TAS2560_VOLTAGE_MV_BELOW_3700:
		val = 0x40; /* limit with voltage below 3.7v */
		break;
	default:
		val = 0x83; /* default, without voltage limit */
		break;
	}
	pTAS2560->write(pTAS2560, TAS2560_LOAD, val);
}

void tas2560_hw_setup(struct tas2560_priv *pTAS2560)
{
	pTAS2560->write(pTAS2560, TAS2560_DR_BOOST_REG_1, 0x0c);	/* register bit value */
	pTAS2560->write(pTAS2560, TAS2560_DR_BOOST_REG_2, 0x33);	/* register bit value */
	pTAS2560->write(pTAS2560, TAS2560_CLK_ERR_2, 0x21);
	tas2560_set_edge(pTAS2560, pTAS2560->mnLevel);
	pTAS2560->write(pTAS2560, TAS2560_DEV_MODE_REG, 0x02);	/* register bit value */

	tas2560_set_ASI_fmt(pTAS2560, TAS2560_DAI_FORMAT);

	if (pTAS2560->type == TAS2560_PRI_L ||
				pTAS2560->type == TAS2560_SEC_L)
		tas2560_set_offset(pTAS2560, TAS2560_DSP_CHAN_OFFSET_L);
	else if (pTAS2560->type == TAS2560_PRI_R ||
				pTAS2560->type == TAS2560_SEC_R)
		tas2560_set_offset(pTAS2560, TAS2560_DSP_CHAN_OFFSET_R);

	tas2560_set_bit_rate(pTAS2560, TAS2560_DAI_BITWIDTH);

	tas2560_set_mute(pTAS2560, 1);
	dev_info(pTAS2560->dev,
		"%s: setup tas %u done\n", __func__, pTAS2560->type);
}

int tas2560_parse_dt(struct device *dev, struct tas2560_priv *pTAS2560)
{
	struct device_node *np = dev->of_node;
	int ret = 0;

	ret = of_property_read_u32(np, "ti,load", &pTAS2560->mnLoad);
	ret = of_property_read_u32(np, "ti,edge-level", &pTAS2560->mnLevel);
	pTAS2560->agc_disabled = of_property_read_bool(np, "ti,AGC-disable");
	pr_info("%s: AGC %s\n",
		__func__, pTAS2560->agc_disabled ? "disabled" : "enabled");

	pTAS2560->mnResetGPIO = of_get_named_gpio(np, "ti,reset-gpio", 0);
	pTAS2560->mnIrqGPIO = of_get_named_gpio(np, "ti,irq-gpio", 0);

	return ret;
}

void tas2560_hw_reset(struct tas2560_priv *pTAS2560)
{
	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return;
	}

	if (gpio_is_valid(pTAS2560->mnResetGPIO)) {
		if (gpio_request(pTAS2560->mnResetGPIO, "TAS2560_RESET")) {
			pr_err("%s: Failed to request gpio %d\n",
					__func__, pTAS2560->mnResetGPIO);
			return;
		}
		gpio_direction_output(pTAS2560->mnResetGPIO, 0);
		usleep_range(DELAY_MS_MIN*10, DELAY_MS_MAX*10);	// 10ms from TI FAE's mail.
		gpio_direction_output(pTAS2560->mnResetGPIO, 1);
		usleep_range(DELAY_MS_MIN*10, DELAY_MS_MAX*10);	// 10ms from TI FAE's mail.
	}
}

void tas2560_enable(struct tas2560_priv *pTAS2560, bool bEnable)
{
	if (pTAS2560 == NULL) {
		pr_err("%s, pTAS2560 is error\n", __func__);
		return;
	}

	if (bEnable) {
		if (!pTAS2560->mbPowerUp) {
			tas2560_i2c_load_data(pTAS2560, p_tas2560_startup_data);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_idle_chnl_detect);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_HPF_data);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_8Ohm_data);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_boost_headroom_data);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_thermal_foldback);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_Vsense_biquad_data);
			if (pTAS2560->agc_disabled)
				tas2560_i2c_load_data(pTAS2560, p_tas2560_voltage_limiter_disable_data);
			else
				tas2560_i2c_load_data(pTAS2560, p_tas2560_voltage_limiter_data);
			tas2560_i2c_load_data(pTAS2560, p_tas2560_48khz_data);
			pTAS2560->write(pTAS2560, TAS2560_MUTE_REG, 0x41);	/* register bit value */
			pTAS2560->write(pTAS2560, TAS2560_CLK_ERR_1, 0x0b);
			pTAS2560->mbPowerUp = true;
		}
	} else {
		if (pTAS2560->mbPowerUp) {
			pTAS2560->write(pTAS2560, TAS2560_CLK_ERR_1, 0x0);
			pTAS2560->write(pTAS2560, TAS2560_MUTE_REG, 0x41);	/* register bit value */
			pTAS2560->write(pTAS2560, TAS2560_MUTE_REG, 0x01);	/* register bit value */
			msleep(30);	/* delay for 30 ms */
			pTAS2560->mbPowerUp = false;
		}
	}
}

void tas2560_irq_enable(struct tas2560_priv *pTAS2560)
{
	if (!pTAS2560)
		return;
	dev_info(pTAS2560->dev, "%s: enable irq\n", __func__);
	pTAS2560->write(pTAS2560, TAS2560_IRQ_PIN_CFG, TAS2560_IRQ_PIN_CFG_HIGH);
	pTAS2560->write(pTAS2560, TAS2560_INT_CFG_1, 0x0);
	pTAS2560->write(pTAS2560, TAS2560_INT_CFG_2, 0xFF);
}

void tas2560_dump_regs(struct tas2560_priv *pTAS2560)
{
	int i = 0;
	dev_info(pTAS2560->dev, "%s: ----------- TAS2560 %u RegDump ------------\n",
		__func__, pTAS2560->type);
	for (i = 0; i < REG_CAP_MAX; i++)
		pTAS2560->read(pTAS2560, regtable[i].reg_index, &(regtable[i].reg_val));
	for (i = 0; i < REG_CAP_MAX/4; i++) {
		dev_info(pTAS2560->dev,
			"%s: 0x%02x=0x%02x, 0x%02x=0x%02x, 0x%02x=0x%02x, 0x%02x=0x%02x\n",
			__func__,
			regtable[4 * i].reg_index, regtable[4 * i].reg_val,
			regtable[4 * i + 1].reg_index, regtable[4 * i + 1].reg_val,
			regtable[4 * i + 2].reg_index, regtable[4 * i + 2].reg_val,
			regtable[4 * i + 3].reg_index, regtable[4 * i + 3].reg_val);
	}
	if (REG_CAP_MAX % 4) {
		for (i = 4 * (REG_CAP_MAX / 4); i < REG_CAP_MAX; i++)
			dev_info(pTAS2560->dev, "%s: 0x%02x=0x%02x\n",
				__func__, regtable[i].reg_index, regtable[i].reg_val);
	}
	dev_info(pTAS2560->dev, "%s: ------------------------------------------\n",
		__func__);
}

void tas2560_dump_regs_all(int __user *arg)
{
	struct tas2560_priv *pTAS2560 = NULL;
	int type = -1;

	if (!arg || get_user(type, arg))
		return;

	pr_info("%s: type = %d\n", __func__, type);
	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			tas2560_dump_regs(pTAS2560);
	}
	mutex_unlock(&tas2560_codecs_lock);
}

void tas2560_reprogram_chip(struct tas2560_priv *pTAS2560)
{
	if (gpio_is_valid(pTAS2560->mnResetGPIO)) {
		gpio_direction_output(pTAS2560->mnResetGPIO, 0);
		mdelay(3); /* delay 3ms */
		gpio_direction_output(pTAS2560->mnResetGPIO, 1);
		msleep(1); /* delay 1ms */
	}
	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		tas2560_set_mute(pTAS2560, 1);
		if (pTAS2560->write(pTAS2560, TAS2560_SW_RESET_REG, 0x01) < 0) {
			dev_err(pTAS2560->dev, "%s: ERROR I2C comm\n", __func__);
			continue;
		}
		dev_info(pTAS2560->dev, "%s: reprogram tas %u\n", __func__, pTAS2560->type);
		usleep_range(DELAY_MS_MIN*10, DELAY_MS_MAX*10);
		tas2560_hw_setup(pTAS2560);
		usleep_range(DELAY_MS_MIN, DELAY_MS_MAX);
		tas2560_enable(pTAS2560, false);
		tas2560_enable(pTAS2560, true);
		tas2560_set_mute(pTAS2560, 0);
	}
	mutex_unlock(&tas2560_codecs_lock);
}

void tas2560_state_check_work(struct work_struct *work)
{
	struct tas2560_priv *pTAS2560 = container_of(work,
		struct tas2560_priv, irq_work);
	unsigned int power_status_reg = 0xfc;
	unsigned int config_status = 0xff;
	unsigned int power_reg = 0x40;
	int i = 0;

	dev_info(pTAS2560->dev, "%s: INT triggered, check now\n", __func__);
	msleep(20); /* wait 20ms */

	pTAS2560->read(pTAS2560, TAS2560_INT_CFG_2, &config_status);
	if (config_status != TAS2560_INT_ALL_ENABLED &&
			pTAS2560->mbPowerUp) {
		dev_info(pTAS2560->dev,
			"%s: int cfg unexceptly 0x%x, reprogram chip\n",
			__func__, config_status);

		for (i = 0; i < RECOVERY_RETRY_TIME; i++)
			tas2560_reprogram_chip(pTAS2560);
	} else if (config_status == TAS2560_INT_ALL_ENABLED) {
		pTAS2560->write(pTAS2560, TAS2560_INT_CFG_2,
			TAS2560_INT_ALL_DISABLED);
		pTAS2560->read(pTAS2560, TAS2560_INT_DET_1, &irq_status_reg1);
		pTAS2560->read(pTAS2560, TAS2560_INT_DET_2, &irq_status_reg2);
		pTAS2560->read(pTAS2560, TAS2560_PWR_REG, &power_reg);
		pTAS2560->read(pTAS2560, TAS2560_PWR_STATUS_REG, &power_status_reg);
		dev_info(pTAS2560->dev,
				"%s: read 0x07:0x%x, 0x26:0x%x, 0x27:0x%x, 0x2A:0x%x\n",
				__func__, power_reg,
				irq_status_reg1, irq_status_reg2, power_status_reg);

		for (i = 0; i < MAX_IRQ_TYPE; i++) {
			if (((irqtable[i].pendreg == TAS2560_INT_DET_1) &&
				(irq_status_reg1&irqtable[i].reg_mask)) ||
				((irqtable[i].pendreg == TAS2560_INT_DET_2) &&
				(irq_status_reg2 & irqtable[i].reg_mask))) {
				dev_info(pTAS2560->dev,
					"%s: chip detected: %s\n",
					__func__, irqtable[i].descript);
				irqtable[i].last_status = 1;
			}
		}

		if (pTAS2560->mbPowerUp &&
			((power_reg & TAS2560_PWR_UP_WITH_BOOST_MASK) == 0 ||
			power_status_reg != TAS2560_INT_DET_ALL_MASK)) {

			if (irq_status_reg1 & (TAS2560_INT_DET_OVRI_MASK |
					TAS2560_INT_DET_OVRV_MASK)) {
				if (pTAS2560->ovriv_count != 0) {
					if (time_after(jiffies, pTAS2560->jiffies +
							msecs_to_jiffies(TAS2560_OVRI_DET_TIMEOUT))) {
						dev_info(pTAS2560->dev, "%s: type %u ovriv_count %u\n",
								__func__, pTAS2560->type, pTAS2560->ovriv_count);
						pTAS2560->jiffies = jiffies;
						pTAS2560->ovriv_count = 0;
					} else {
						pTAS2560->ovriv_count++;
						if (pTAS2560->ovriv_count > TAS2560_OVRI_DET_CNT_LIMIT) {
							dev_err(pTAS2560->dev,
								"%s: type %u maybe SHORT CIRCUIT, exit !!\n",
								__func__, pTAS2560->type);
							tas2560_dump_regs(pTAS2560);
							goto _exit;
						}
					}
				} else {
					pTAS2560->jiffies = jiffies;
					pTAS2560->ovriv_count = 1;
				}
			}

			dev_info(pTAS2560->dev,
				"%s: power status error, reprogram chip\n", __func__);

			for (i = 0; i < RECOVERY_RETRY_TIME; i++)
				tas2560_reprogram_chip(pTAS2560);
		} else if (!(irq_status_reg1 & TAS2560_INT_DET_ALL_MASK) && dump_regs)
			tas2560_dump_regs(pTAS2560);

#ifdef CONFIG_HUAWEI_DSM
		if ((irq_status_reg1 & TAS2560_INT_DET_ALL_MASK) ||
			(irq_status_reg2 & TAS2560_INT_DET2_ALL_MASK) ||
			(pTAS2560->mbPowerUp &&
			(!(power_reg & TAS2560_PWR_UP_WITH_BOOST_MASK) ||
			(power_status_reg != TAS2560_INT_DET_ALL_MASK)))) {
			audio_dsm_report_info(DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO,
				"tas2560 %u exception:0x07=0x%x,0x26=0x%x,0x27=0x%x,0x2A=0x%x",
				pTAS2560->type,
				irq_status_reg1, irq_status_reg2, power_reg, power_status_reg);
		}
#endif
		irq_status_reg1 = 0;
		irq_status_reg2 = 0;
	}

_exit:
	pTAS2560->write(pTAS2560, TAS2560_INT_CFG_2, TAS2560_INT_ALL_ENABLED);
	pTAS2560->write(pTAS2560, TAS2560_IRQ_PIN_CFG, TAS2560_IRQ_PIN_CFG_HIGH);
}

irqreturn_t tas2560_irq_handler(int irq, void *data)
{
	struct tas2560_priv *pTAS2560 = (struct tas2560_priv *)data;
	if (!work_pending(&pTAS2560->irq_work))
		schedule_work(&pTAS2560->irq_work);
	return IRQ_HANDLED;
}

void tas2560_irq_register(struct tas2560_priv *pTAS2560)
{
	int rc = 0;
	if (!pTAS2560) {
		dev_err(pTAS2560->dev, "%s: pTAS2560 is null !\n", __func__);
		return;
	}
	if (gpio_is_valid(pTAS2560->mnIrqGPIO)) {
		rc = devm_gpio_request_one(pTAS2560->dev,
			pTAS2560->mnIrqGPIO, GPIOF_OPEN_DRAIN | GPIOF_IN, "tas2560-irq-gpio");
		if (rc != 0)
			dev_err(pTAS2560->dev,
				"%s: request irq gpio %d failed %d !\n",
				__func__, pTAS2560->mnIrqGPIO, rc);
		rc = devm_request_threaded_irq(pTAS2560->dev, gpio_to_irq(pTAS2560->mnIrqGPIO),
				NULL, tas2560_irq_handler, IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				kasprintf(GFP_KERNEL, "tas2560-%u-irq", pTAS2560->type), pTAS2560);
		if (rc != 0) {
			dev_err(pTAS2560->dev,
				"%s: register irq failed %d !\n", __func__, rc);
		} else {
			dev_info(pTAS2560->dev, "%s: register irq done\n", __func__);
			tas2560_irq_enable(pTAS2560);
		}
	} else {
		dev_warn(pTAS2560->dev,
			"%s: irq gpio invalid or not defined\n", __func__);
	}
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 common functions for Android Linux");
MODULE_LICENSE("GPLv2");
