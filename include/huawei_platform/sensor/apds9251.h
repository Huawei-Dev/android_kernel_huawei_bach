/*
 * This file is part of the APDS9251x sensor driver.
 * Chip is combined proximity and ambient light sensor.
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Contact: Samu Onkalo <samu.p.onkalo@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef  __AVAGO_APDS9251_H__
#define __AVAGO_APDS9251_H__

#define APDS9251_RGB_DATA_MAX          65536
#define APDS9251_LUX_MAX                       30000

/* POWER SUPPLY VOLTAGE RANGE */
#define APDS9251_VDD_MIN_UV  2000000
#define APDS9251_VDD_MAX_UV  3300000
#define APDS9251_VIO_MIN_UV  1750000
#define APDS9251_VIO_MAX_UV  1950000


/*the rgb_apds9251_platform_data structure needs to cite the definition of rgb_apds9251_data*/
struct rgb_apds9251_data;

struct rgb_apds9251_platform_data {
	u8	   pdrive;
	int    (*setup_resources)(void);
	int    (*release_resources)(void);

	int irq_num;
	int (*power)(unsigned char onoff);
	/*add the parameters to the init and exit function*/
	int (*init)(struct rgb_apds9251_data *data);
	void (*exit)(struct rgb_apds9251_data *data);
	int (*power_on)(bool,struct rgb_apds9251_data *data);

	bool i2c_pull_up;
	bool digital_pwr_regulator;

	unsigned int irq_gpio;
	u32 irq_gpio_flags;
	int panel_id;
	int tp_color;
	unsigned int i2c_scl_gpio;
	unsigned int i2c_sda_gpio;
};

#define ARR_NUM  13
#define PRINT_LUX_TIME  30


#define APDS9251_ALS_THRESHOLD_HSYTERESIS	1	/* 1 = 1% */

#define APDS9251_ALS_CAL_LOOP			1
#define APDS9251_ALS_CAL_LUX			300
#define APDS9251_ALS_CAL_LUX_LOW		((70*APDS9251_ALS_CAL_LUX)/100)		// 70% of 300 lux
#define	APDS9251_ALS_CAL_LUX_HIGH		((130*APDS9251_ALS_CAL_LUX)/100)	// 130% of 300 lux

#define APDS9251_ALS_CALIBRATED_CCT		5000


#define APDS9251_LUX_GA1	100
#define APDS9251_LUX_GA2	100
#define APDS9251_LUX_GA3	100
	
#define APDS9251_CCT_GA1	100
#define APDS9251_CCT_GA2	100
#define APDS9251_CCT_GA3	100
	
/* Change History 
 *
 * 1.0.0	Fundamental Functions of APDS-9251
 *
 */

#define APDS_IOCTL_ALS_ENABLE				1
#define APDS_IOCTL_ALS_GET_ENABLE			2
#define APDS_IOCTL_ALS_POLL_DELAY			3
#define APDS_IOCTL_ALS_GET_IR_DATA			4	// ir_data
#define APDS_IOCTL_ALS_GET_RED_DATA			5	// red_data
#define APDS_IOCTL_ALS_GET_GREEN_DATA		6	// green_data
#define APDS_IOCTL_ALS_GET_BLUE_DATA		7	// blue_data
#define APDS_IOCTL_ALS_GET_ALS_FACTOR		8 	// als calibration factor
#define APDS_IOCTL_ALS_GET_CCT_FACTOR		9 	// cct calibration factor

#define APDS_DISABLE_ALS					0
#define APDS_ENABLE_ALS_WITH_INT			1
#define APDS_ENABLE_ALS_NO_INT				2
#define	APDS_ENABLE_ALS_CALIBRATION			3

#define APDS_ALS_POLL_SLOW					0	// 1 Hz (1000ms measurement time , 400ms conversion time)
#define APDS_ALS_POLL_MEDIUM				1	// 5 Hz (200ms measurement time, 100ms conversion time)
#define APDS_ALS_POLL_FAST					2	// 20 Hz (50ms measurement time, 25ms conversion time)

/*
 * Defines
 */

/* Register Addresses define */
#define APDS9251_DD_MAIN_CTRL_ADDR			0x00
#define APDS9251_DD_ALS_MEAS_RATE_ADDR		0x04
#define APDS9251_DD_ALS_GAIN_ADDR			0x05
#define APDS9251_DD_PART_ID_ADDR			0x06
#define APDS9251_DD_MAIN_STATUS_ADDR		0x07
#define APDS9251_DD_IR_DATA_ADDR			0x0A
#define APDS9251_DD_IR_DATA_0_ADDR			0x0A
#define APDS9251_DD_IR_DATA_1_ADDR			0x0B
#define APDS9251_DD_IR_DATA_2_ADDR			0x0C
#define APDS9251_DD_GREEN_DATA_ADDR			0x0D
#define APDS9251_DD_GREEN_DATA_0_ADDR		0x0D
#define APDS9251_DD_GREEN_DATA_1_ADDR		0x0E
#define APDS9251_DD_GREEN_DATA_2_ADDR		0x0F
#define APDS9251_DD_BLUE_DATA_ADDR			0x10
#define APDS9251_DD_BLUE_DATA_0_ADDR		0x10
#define APDS9251_DD_BLUE_DATA_1_ADDR		0x11
#define APDS9251_DD_BLUE_DATA_2_ADDR		0x12
#define APDS9251_DD_RED_DATA_ADDR			0x13
#define APDS9251_DD_RED_DATA_0_ADDR			0x13
#define APDS9251_DD_RED_DATA_1_ADDR			0x14
#define APDS9251_DD_RED_DATA_2_ADDR			0x15
#define APDS9251_DD_COMP_DATA_ADDR			0x16
#define APDS9251_DD_COMP_DATA_0_ADDR		0x16
#define APDS9251_DD_COMP_DATA_1_ADDR		0x17
#define APDS9251_DD_COMP_DATA_2_ADDR		0x18
#define APDS9251_DD_INT_CFG_ADDR			0x19
#define APDS9251_DD_INT_PERSISTENCE_ADDR	0x1A
#define	APDS9251_DD_ALS_THRES_UP_ADDR		0x21
#define	APDS9251_DD_ALS_THRES_UP_0_ADDR		0x21
#define	APDS9251_DD_ALS_THRES_UP_1_ADDR		0x22
#define	APDS9251_DD_ALS_THRES_UP_2_ADDR		0x23
#define	APDS9251_DD_ALS_THRES_LOW_ADDR		0x24
#define	APDS9251_DD_ALS_THRES_LOW_0_ADDR	0x24
#define	APDS9251_DD_ALS_THRES_LOW_1_ADDR	0x25
#define	APDS9251_DD_ALS_THRES_LOW_2_ADDR	0x26
#define	APDS9251_DD_ALS_THRES_VAR_ADDR		0x27
#define	APDS9251_DD_DEVICE_CONFIG_ADDR		0x2F

/* Register Value define : MAIN_CTRL */
#define APDS9251_DD_ALS_EN					0x02
#define APDS9251_DD_CS_MODE					0x04	/* All channels */
#define APDS9251_DD_SW_RESET				0x10

/* Register Value define : ALS_MEAS_RATE */
#define APDS9251_DD_ALS_MEAS_RATE_25_MS		0x00  /* ALS Measurement rate = 25 ms */
#define APDS9251_DD_ALS_MEAS_RATE_50_MS		0x01  /* ALS Measurement rate = 50 ms */
#define APDS9251_DD_ALS_MEAS_RATE_100_MS	0x02  /* ALS Measurement rate = 100 ms */
#define APDS9251_DD_ALS_MEAS_RATE_200_MS	0x03  /* ALS Measurement rate = 200 ms */
#define APDS9251_DD_ALS_MEAS_RATE_500_MS	0x04  /* ALS Measurement rate = 500 ms */
#define APDS9251_DD_ALS_MEAS_RATE_1000_MS	0x05  /* ALS Measurement rate = 1000 ms */
#define APDS9251_DD_ALS_MEAS_RATE_2000_MS	0x06  /* ALS Measurement rate = 2000 ms */

#define APDS9251_DD_ALS_MEAS_RES_20_BIT		0x00  /* ALS resolution 20 bit (full range : 0 ~ 1048575) [ADC conversion time = 400ms] */
#define APDS9251_DD_ALS_MEAS_RES_19_BIT		0x10  /* ALS resolution 19 bit (full range : 0 ~ 524287) [ADC conversion time = 200ms]  */
#define APDS9251_DD_ALS_MEAS_RES_18_BIT		0x20  /* ALS resolution 18 bit (full range : 0 ~ 262143) [ADC conversion time = 100ms]  */
#define APDS9251_DD_ALS_MEAS_RES_17_BIT		0x30  /* ALS resolution 17 bit (full range : 0 ~ 131071) [ADC conversion time = 50ms]  */
#define APDS9251_DD_ALS_MEAS_RES_16_BIT		0x40  /* ALS resolution 16 bit (full range : 0 ~ 65535) [ADC conversion time = 25ms]  */

/* Register Value define : ALS_GAIN */
#define APDS9251_DD_ALS_GAIN_1				0x00  /* ALS Gain 1 */
#define APDS9251_DD_ALS_GAIN_3				0x01  /* ALS Gain 3 */
#define APDS9251_DD_ALS_GAIN_6				0x02  /* ALS Gain 6 */
#define APDS9251_DD_ALS_GAIN_9				0x03  /* ALS Gain 9 */
#define APDS9251_DD_ALS_GAIN_18				0x04  /* ALS Gain 18 */

/* Register Value define : MAIN_STATUS */
#define APDS9251_DD_ALS_DATA_STATUS			0x08  /* 1: New data, not read yet (cleared after read) */
#define APDS9251_DD_ALS_INT_STATUS			0x10  /* 1: Interrupt condition fulfilled (cleared after read) */
#define APDS9251_DD_POWER_ON_STATUS			0x20  /* 1: Power on cycle */

#define APDS9251_DD_ALS_INT_EN				0x04  /* 1: ALS Interrupt enabled */
#define APDS9251_DD_ALS_VAR_MODE			0x08  /* 1: ALS variation interrupt mode */
#define APDS9251_DD_ALS_INT_SEL_IR			0x00  /* IR channel selected for interrupt */
#define APDS9251_DD_ALS_INT_SEL_ALS			0x10  /* ALS/GREEN channel selected for interrupt */
#define APDS9251_DD_ALS_INT_SEL_RED			0x20  /* RED channel selected for interrupt */
#define APDS9251_DD_ALS_INT_SEL_BLUE		0x40  /* BLUE channel selected for interrupt */

/* Register Value define : INT_PERSISTENCE */
#define APDS9251_DD_ALS_PERS_1				0x00  /* Every ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_2				0x10  /* 2 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_3				0x20  /* 3 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_4				0x30  /* 4 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_5				0x40  /* 5 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_6				0x50  /* 6 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_7				0x60  /* 7 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_8				0x70  /* 8 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_9				0x80  /* 9 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_10				0x90  /* 10 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_11				0xA0  /* 11 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_12				0xB0  /* 12 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_13				0xC0  /* 13 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_14				0xD0  /* 14 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_15				0xE0  /* 15 consecutive ALS value out of threshold range */
#define APDS9251_DD_ALS_PERS_16				0xF0  /* 16 consecutive ALS value out of threshold range */

/* Register Value define : ALS_THRES_VAR */
#define APDS9251_DD_ALS_VAR_8_COUNT			0x00  /* ALS result varies by 8 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_16_COUNT		0x01  /* ALS result varies by 16 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_32_COUNT		0x02  /* ALS result varies by 32 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_64_COUNT		0x03  /* ALS result varies by 64 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_128_COUNT		0x04  /* ALS result varies by 128 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_256_COUNT		0x05  /* ALS result varies by 256 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_512_COUNT		0x06  /* ALS result varies by 512 counts compared to previous result */
#define APDS9251_DD_ALS_VAR_1024_COUNT		0x07  /* ALS result varies by 1024 counts compared to previous result */

typedef enum
{
	APDS9251_DD_ALS_RES_16BIT = 0,  /* 25ms integration time */
	APDS9251_DD_ALS_RES_17BIT = 1,  /* 50ms integration time */
	APDS9251_DD_ALS_RES_18BIT = 2,  /* 100ms integration time */
	APDS9251_DD_ALS_RES_19BIT = 3,  /* 200ms integration time */
	APDS9251_DD_ALS_RES_20BIT = 4   /* 400ms integration time */
} apds9251_dd_als_res_e;

typedef enum
{
	APDS9251_DD_ALS_GAIN_1X = 0,    /* 1x ALS GAIN */
	APDS9251_DD_ALS_GAIN_3X = 1,    /* 3x ALS GAIN */
	APDS9251_DD_ALS_GAIN_6X = 2,    /* 6x ALS GAIN */
	APDS9251_DD_ALS_GAIN_9X = 3,    /* 9x ALS GAIN */
	APDS9251_DD_ALS_GAIN_18X = 4    /* 18x ALS GAIN */
} apds9251_dd_als_gain_e;

#define APDS9251_DD_LUX_FACTOR					128	

#define APDS9251_DD_ALS_DEFAULT_RES				APDS9251_DD_ALS_MEAS_RES_18_BIT
#define APDS9251_DD_ALS_DEFAULT_MEAS_RATE		APDS9251_DD_ALS_MEAS_RATE_100_MS
#define APDS9251_DD_ALS_DEFAULT_GAIN				APDS9251_DD_ALS_GAIN_18

#define APDS_READ_BLOCK_DATA_SIZE	12
#define APDS_LOOP_COUNT	10
#endif
enum Als_Parse_Return{
SUCCESE_ALS = 0,
READ_TP_FAIL,
UNKNOW_TP_COLOR
};
