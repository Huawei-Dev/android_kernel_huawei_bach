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
**     tas2560.h
**
** Description:
**     definitions and data structures for TAS2560 Android Linux driver
**
** =============================================================================
*/

#ifndef _TAS2560_H
#define _TAS2560_H

#include "../msm/qdsp6v2/msm-pcm-routing-v2.h"

#define TAS2560_PRI_BE	MSM_BACKEND_DAI_QUINARY_MI2S_RX
#define TAS2560_SEC_BE	MSM_BACKEND_DAI_QUATERNARY_MI2S_RX

#define TAS2560_PRI_DAI		0
#define TAS2560_SEC_DAI		1

#define TAS2560_MDELAY				0xfffffffe

/* Page Control Register */
#define TAS2560_PAGECTL_REG			0

/* Book Control Register (available in page0 of each book) */
#define TAS2560_BOOKCTL_PAGE			0
#define TAS2560_BOOKCTL_REG			127

#define TAS2560_REG(book, page, reg)		(((book * 256 * 128) + \
						 (page * 128)) + reg)

#define TAS2560_BOOK_ID(reg)			(reg / (256 * 128))
#define TAS2560_PAGE_ID(reg)			((reg % (256 * 128)) / 128)
#define TAS2560_BOOK_REG(reg)			(reg % (256 * 128))
#define TAS2560_PAGE_REG(reg)			((reg % (256 * 128)) % 128)

/* Book0, Page0 registers */
#define TAS2560_SW_RESET_REG                    TAS2560_REG(0, 0, 1)
#define TAS2560_DEV_MODE_REG                    TAS2560_REG(0, 0, 2)
#define TAS2560_SPK_CTRL_REG                    TAS2560_REG(0, 0, 4)
#define TAS2560_MUTE_REG			TAS2560_REG(0, 0, 7)
#define TAS2560_PWR_REG				TAS2560_REG(0, 0, 7)
#define TAS2560_PWR_BIT_MASK			(0x3 << 6)
#define TAS2560_MUTE_MASK				(0x7)

#define TAS2560_SR_CTRL1			TAS2560_REG(0, 0, 8)
#define TAS2560_LOAD	            TAS2560_REG(0, 0, 9)
#define TAS2560_SR_CTRL2			TAS2560_REG(0, 0, 13)
#define TAS2560_SR_CTRL3			TAS2560_REG(0, 0, 14)
#define TAS2560_ISENSE_SCALE_MASK	0x18

#define TAS2560_CLK_SEL	                        TAS2560_REG(0, 0, 15)
#define TAS2560_PLL_SRC_MASK					(0xc0)
#define TAS2560_PLL_CLKIN_BCLK			(0)
#define TAS2560_PLL_CLKIN_MCLK			(1)
#define TAS2560_PLL_CLKIN_PDMCLK		(2)
#define TAS2560_PLL_CLKSRC_BCLK			(0)
#define TAS2560_PLL_CLKSRC_MCLK			(1)
#define TAS2560_PLL_CLKSRC_PDMCLK		(2)
#define TAS2560_PLL_P_MASK					(0x3f)

#define TAS2560_SET_FREQ                        TAS2560_REG(0, 0, 16)
#define TAS2560_PLL_J_MASK						(0x7f)

#define TAS2560_PLL_D_LSB                        TAS2560_REG(0, 0, 17)
#define TAS2560_PLL_D_MSB                        TAS2560_REG(0, 0, 18)
#define TAS2560_PLL_D_LSB_MASK			0x00ff
#define TAS2560_PLL_D_MSB_MASK			0x3f00

#define TAS2560_DAI_FMT							TAS2560_REG(0, 0, 20)
#define TAS2560_ASI_CHANNEL						TAS2560_REG(0, 0, 21)
#define TAS2560_ASI_OFFSET_1					TAS2560_REG(0, 0, 22)
#define TAS2560_ASI_CFG_1						TAS2560_REG(0, 0, 24)
#define TAS2560_CLK_ERR_1						TAS2560_REG(0, 0, 33)
#define TAS2560_IRQ_PIN_CFG					 	TAS2560_REG(0, 0, 35)
#define TAS2560_INT_CFG_1						TAS2560_REG(0, 0, 36)
#define TAS2560_INT_CFG_2						TAS2560_REG(0, 0, 37)
#define TAS2560_INT_DET_1						TAS2560_REG(0, 0, 38)
#define TAS2560_INT_DET_2						TAS2560_REG(0, 0, 39)
#define TAS2560_PWR_STATUS_REG                  TAS2560_REG(0, 0, 42)
#define TAS2560_DAI_WORDLEN_MASK		0x03
#define	TAS2560_DIRINV_MASK				0x3e
#define TAS2560_TRISTATE				(1 << 1)
#define TAS2560_BCLKINV					(1 << 2)
#define TAS2560_WCLKINV					(1 << 3)
#define TAS2560_BCLKDIR					(1 << 4)
#define TAS2560_WCLKDIR					(1 << 5)

#define TAS2560_DR_BOOST_REG_2				TAS2560_REG(0, 0, 60)
#define TAS2560_DR_BOOST_REG_1              TAS2560_REG(0, 0, 73)
#define TAS2560_CLK_ERR_2					TAS2560_REG(0, 0, 80)
#define TAS2560_ID_REG                    TAS2560_REG(0, 0, 125)

/* Book0, Page50 registers */
#define TAS2560_HPF_CUTOFF_CTL1			TAS2560_REG(0,50, 28)
#define TAS2560_HPF_CUTOFF_CTL2			TAS2560_REG(0,50, 32)
#define TAS2560_HPF_CUTOFF_CTL3			TAS2560_REG(0,50, 36)

#define TAS2560_ISENSE_PATH_CTL1		TAS2560_REG(0,50, 40)
#define TAS2560_ISENSE_PATH_CTL2		TAS2560_REG(0,50, 44)
#define TAS2560_ISENSE_PATH_CTL3		TAS2560_REG(0,50, 48)

#define TAS2560_VLIMIT_THRESHOLD1		TAS2560_REG(0,50, 60)
#define TAS2560_VLIMIT_THRESHOLD2		TAS2560_REG(0,50, 64)
#define TAS2560_VLIMIT_THRESHOLD3		TAS2560_REG(0,50, 68)
#define TAS2560_VLIMIT_THRESHOLD4		TAS2560_REG(0,50, 72)
#define TAS2560_VLIMIT_THRESHOLD5		TAS2560_REG(0,50, 76)
#define TAS2560_VLIMIT_THRESHOLD6		TAS2560_REG(0,50, 80)
#define TAS2560_VLIMIT_THRESHOLD7		TAS2560_REG(0,50, 84)
#define TAS2560_VLIMIT_THRESHOLD8		TAS2560_REG(0,50, 88)
#define TAS2560_VLIMIT_THRESHOLD9		TAS2560_REG(0,50, 92)
#define TAS2560_VLIMIT_THRESHOLD10		TAS2560_REG(0,50, 96)

#define TAS2560_IDLE_CHNL_DETECT		TAS2560_REG(0,50, 108)

/* Book0, Page51 registers */
#define TAS2560_BOOST_HEAD			TAS2560_REG(0,51, 24)
#define TAS2560_BOOST_ON			TAS2560_REG(0,51, 16)
#define TAS2560_BOOST_OFF			TAS2560_REG(0,51, 20)
#define TAS2560_BOOST_TABLE_CTRL1		TAS2560_REG(0,51, 32)
#define TAS2560_BOOST_TABLE_CTRL2		TAS2560_REG(0,51, 36)
#define TAS2560_BOOST_TABLE_CTRL3		TAS2560_REG(0,51, 40)
#define TAS2560_BOOST_TABLE_CTRL4		TAS2560_REG(0,51, 44)
#define TAS2560_BOOST_TABLE_CTRL5		TAS2560_REG(0,51, 48)
#define TAS2560_BOOST_TABLE_CTRL6		TAS2560_REG(0,51, 52)
#define TAS2560_BOOST_TABLE_CTRL7		TAS2560_REG(0,51, 56)
#define TAS2560_BOOST_TABLE_CTRL8		TAS2560_REG(0,51, 60)
#define TAS2560_BOOST_TABLE_CTRL9		TAS2560_REG(0,51, 64)
#define TAS2560_THERMAL_FOLDBACK		TAS2560_REG(0,51, 100)

/* Book0, Page52 registers */
#define TAS2560_VSENSE_DEL_CTL1			TAS2560_REG(0,52, 52)
#define TAS2560_VSENSE_DEL_CTL2			TAS2560_REG(0,52, 56)
#define TAS2560_VSENSE_DEL_CTL3			TAS2560_REG(0,52, 60)
#define TAS2560_VSENSE_DEL_CTL4			TAS2560_REG(0,52, 64)
#define TAS2560_VSENSE_DEL_CTL5			TAS2560_REG(0,52, 68)

#define TAS2560_INT_ALL_ENABLED			0xff
#define TAS2560_INT_ALL_DISABLED		0xff
#define TAS2560_INT_DET_OVRV_MASK		0x80
#define TAS2560_INT_DET_OVRI_MASK		0x40
#define TAS2560_INT_DET_CLK1_MASK		0x20
#define TAS2560_INT_DET_OVRT_MASK		0x10
#define TAS2560_INT_DET_BRNO_MASK		0x08
#define TAS2560_INT_DET_CLK2_MASK		0x04
#define TAS2560_INT_DET_SAR_MASK		0x02
#define TAS2560_INT_DET_RESV_MASK		0x01

#define TAS2560_INT_DET_ALL_MASK	\
	(TAS2560_INT_DET_OVRV_MASK | TAS2560_INT_DET_OVRI_MASK | \
	TAS2560_INT_DET_CLK1_MASK | TAS2560_INT_DET_OVRT_MASK | \
	TAS2560_INT_DET_BRNO_MASK | TAS2560_INT_DET_CLK2_MASK)

#define TAS2560_INT_DET2_ALL_MASK		0xc0

#define TAS2560_PWR_UP_WITH_BOOST_MASK	0x40
#define TAS2560_IRQ_PIN_CFG_HIGH		0x51
#define TAS2560_IRQ_PIN_CFG_LOW			0x21

#define TAS2560_OVRI_DET_TIMEOUT		30000
#define TAS2560_OVRI_DET_CNT_LIMIT		10

#define TAS2560_DATAFORMAT_I2S			(0x0 << 2)
#define TAS2560_DATAFORMAT_DSP			(0x1 << 2)
#define TAS2560_DATAFORMAT_RIGHT_J		(0x2 << 2)
#define TAS2560_DATAFORMAT_LEFT_J		(0x3 << 2)

#define TAS2560_DAI_FMT_MASK			(0x7 << 2)

#define LOAD_MASK				0x18
#define LOAD_8OHM				(0)
#define LOAD_6OHM				(1)
#define LOAD_4OHM				(2)

#define TAS2560_DAI_BITWIDTH	16
#define TAS2560_DAI_FORMAT		\
	SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_IB_IF | SND_SOC_DAIFMT_DSP_A

#define TAS2560_SUPPORTED_BITFMT	\
	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
	SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

#define TAS2560_DSP_CHAN_OFFSET_L		1
#define TAS2560_DSP_CHAN_OFFSET_R		33

#define TAS2560_CHAN_L			(0)
#define TAS2560_CHAN_R			(1)

#define REG_END		0xFFFFFFFF
#define VAL_1K		1024

#define BIT_RATE_8		8
#define BIT_RATE_16		16
#define BIT_RATE_20		20
#define BIT_RATE_24		24
#define BIT_RATE_32		32
#define BIT_RATE_128	128

#define TAS_PLL_FREQ_LEVEL_1	40000000
#define TAS_PLL_FREQ_LEVEL_2	80000000
#define TAS_PLL_FREQ_LEVEL_3	160000000
#define TAS_PLL_DIV_LEVEL_1		512000
#define TAS_PLL_DIV_LEVEL_2		1000000
#define TAS_PLL_DIV_LEVEL_3		10000000
#define TAS_PLL_DIV_LEVEL_4		20000000
#define TAS_PLL_DIV				10000
#define TAS_P_VAL				64

#define DELAY_MS_MIN			1000
#define DELAY_MS_MAX			1100

#define TAS_CODEC_NAME			"tas2560-smartpa"

enum register_values {
	REG_VALUE_0 = 0,
	REG_VALUE_1 = 1,
	REG_VALUE_2 = 2,
	REG_VALUE_3 = 3
};

enum tas2560_edge_level {
	TAS2560_VOLTAGE_MV_NO_LIMIT = 0,
	TAS2560_VOLTAGE_MV_ABOVE_4100,
	TAS2560_VOLTAGE_MV_3900_4100,
	TAS2560_VOLTAGE_MV_3700_3900,
	TAS2560_VOLTAGE_MV_BELOW_3700
};

enum tas2560_type {
    TAS2560_PRI_L,
    TAS2560_PRI_R,
    TAS2560_SEC_L,
    TAS2560_SEC_R,
};

enum tas2560_irq_index {
	INT_SAR    	= 1,
	INT_CLK2 	= 2,
	INT_BRNO 	= 3,
	INT_OVRT 	= 4,
	INT_CLK1 	= 5,
	INT_AUV 	= 6,
	INT_OVRI 	= 7,
	INT_MCHLT 	= 8,
	INT_WCHLT	= 9,
};

struct tas2560_register {
	int book;
	int page;
	int reg;
};

struct tas2560_dai_cfg {
	unsigned int dai_fmt;
	unsigned int tdm_delay;
};

struct tas2560_reg {
	unsigned int reg_index;
	unsigned int reg_val;
};
struct tas2560_irq {
	int irq_index;
	int reg_mask;
	char last_status;
	char pendreg;
	char *descript;
};

struct tas2560_priv {
	u8 i2c_regs_status;
	struct device *dev;

	struct regmap *regmap;

	u8 channel:1;
	u8 mute:1;
	u8 enabled:1;
	u8 type:2;
	bool agc_disabled;
	int beid;
	struct list_head list;

	struct mutex dev_lock;
	int mnClkin;
	int mnClkid;
	bool mbPowerUp;
	int mnCurrentBook;
	int mnCurrentPage;
	int mnAddr;
	int mnLoad ;
	int mnLevel;
	int mnResetGPIO;
	int mnIrqGPIO;
	int mnSamplingRate;
	int mnFrameSize;
	uint8_t ovriv_count;
	unsigned long jiffies;

	struct work_struct irq_work;
	int (*read) (struct tas2560_priv * pTAS2555, unsigned int reg,
		unsigned int *pValue);
	int (*write) (struct tas2560_priv * pTAS2555, unsigned int reg,
		unsigned int Value);
	int (*bulk_read) (struct tas2560_priv * pTAS2555, unsigned int reg,
		unsigned char *pData, unsigned int len);
	int (*bulk_write) (struct tas2560_priv * pTAS2555, unsigned int reg,
		unsigned char *pData, unsigned int len);
	int (*update_bits) (struct tas2560_priv * pTAS2555, unsigned int reg,
		unsigned int mask, unsigned int value);

	int mnDBGCmd;
	int mnCurrentReg;
	struct mutex file_lock;
	struct miscdevice *tas_misc;
};

extern struct list_head tas2560_codecs_list;
extern struct mutex tas2560_codecs_lock;

#endif
