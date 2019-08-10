

#ifndef __APDS9110_H__
#define __APDS9110_H__

#define APDS9110_PS_POLLING 0 // 1 = polling, 0 = interrupt
#define APDS_IOCTL_PS_ENABLE 1
#define APDS_IOCTL_PS_GET_ENABLE 2
#define APDS_IOCTL_PS_POLL_DELAY 3
#define APDS_IOCTL_PS_GET_PDATA 4	// ps_data
#define APDS_IOCTL_PS_CALIBRATION 5 // ps calibration

#define APDS_DISABLE_PS 0
#define APDS_ENABLE_PS 1

/* Register Addresses define */
#define APDS9110_DD_MAIN_CTRL_ADDR 0x00
#define APDS9110_DD_PRX_LED_ADDR 0x01
#define APDS9110_DD_PRX_PULSES_ADDR 0x02
#define APDS9110_DD_PRX_MEAS_RATE_ADDR 0x03
#define APDS9110_DD_PART_ID_ADDR 0x06
#define APDS9110_DD_MAIN_STATUS_ADDR 0x07
#define APDS9110_DD_PRX_DATA_ADDR 0x08
#define APDS9110_DD_PRX_DATA_0_ADDR 0x08
#define APDS9110_DD_PRX_DATA_1_ADDR 0x09
#define APDS9110_DD_INT_CFG_ADDR 0x19
#define APDS9110_DD_INT_PERSISTENCE_ADDR 0x1A
#define APDS9110_DD_PRX_THRES_UP_ADDR 0x1B
#define APDS9110_DD_PRX_THRES_UP_0_ADDR 0x1B
#define APDS9110_DD_PRX_THRES_UP_1_ADDR 0x1C
#define APDS9110_DD_PRX_THRES_LOW_ADDR 0x1D
#define APDS9110_DD_PRX_THRES_LOW_0_ADDR 0x1D
#define APDS9110_DD_PRX_THRES_LOW_1_ADDR 0x1E
#define APDS9110_DD_PRX_CAN_ADDR 0x1F
#define APDS9110_DD_PRX_CAN_0_ADDR 0x1F
#define APDS9110_DD_PRX_CAN_1_ADDR 0x20
#define APDS9110_DD_PRX_GAIN_ADDR 0x29

/* Register Value define : MAIN_CTRL */
#define APDS9110_DD_PRX_EN 0x01
#define APDS9110_DD_SW_RESET 0x10

/* Register Value define : PS_LED */
#define APDS9110_DD_LED_CURRENT_10_MA 0x02 /* 10 mA */

#define APDS9110_DD_LED_FREQ_60_KHZ 0x30 /* LED Pulse frequency = 60KHz */
#define APDS9110_DD_LED_FREQ_70_KHZ 0x40 /* LED Pulse frequency = 70KHz */
#define APDS9110_DD_LED_FREQ_80_KHZ 0x50 /* LED Pulse frequency = 80KHz */
#define APDS9110_DD_LED_FREQ_90_KHZ 0x60 /* LED Pulse frequency = 90KHz */
#define APDS9110_DD_LED_FREQ_100_KHZ 0x70 /* LED Pulse frequency = 100KHz */

/* Register Value define : PS_MEAS_RATE */
#define APDS9110_DD_PRX_MEAS_RATE_6_25_MS	0x01 /* PS Measurement rate = 6.25 ms */
#define APDS9110_DD_PRX_MEAS_RATE_12_5_MS	0x02 /* PS Measurement rate = 12.5 ms */
#define APDS9110_DD_PRX_MEAS_RATE_25_MS 0x03 /* PS Measurement rate = 25 ms */
#define APDS9110_DD_PRX_MEAS_RATE_50_MS 0x04 /* PS Measurement rate = 50 ms */
#define APDS9110_DD_PRX_MEAS_RATE_100_MS 0x05 /* PS Measurement rate = 100 ms */
#define APDS9110_DD_PRX_MEAS_RATE_200_MS 0x06 /* PS Measurement rate = 200 ms */
#define APDS9110_DD_PRX_MEAS_RATE_400_MS 0x07 /* PS Measurement rate = 400 ms */

#define APDS9110_DD_PRX_MEAS_RES_8_BIT 0x00 /* PS resolution 8 bit (full range : 0 ~ 255) */
#define APDS9110_DD_PRX_MEAS_RES_9_BIT 0x08 /* PS resolution 9 bit (full range : 0 ~ 511) */
#define APDS9110_DD_PRX_MEAS_RES_10_BIT 0x10 /* PS resolution 10 bit (full range : 0 ~ 1023) */
#define APDS9110_DD_PRX_MEAS_RES_11_BIT 0x18 /* PS resolution 11 bit (full range : 0 ~ 2047) */

/* Register Value define : MAIN_STATUS */
#define APDS9110_DD_PRX_DATA_STATUS 0x01 /* 1: New data, not read yet (cleared after read) */
#define APDS9110_DD_PRX_INT_STATUS 0x02 /* 1: Interrupt condition fulfilled (cleared after read) */
#define APDS9110_DD_PRX_LOGICAL_STATUS 0x04 /* 1: object is close */
#define APDS9110_DD_POWER_ON_STATUS 0x20 /* 1: Power on cycle */

/* Register Value define : INT_CFG */
#define APDS9110_DD_PRX_INT_EN 0x01 /* 1: PS Interrupt enabled */
#define APDS9110_DD_PRX_LOGIC_MODE 0x02 /* 1: PS Logic Output Mode: INT pad is updated after every measurement and maintains output state between measurements */

/* Register Value define : INT_PERSISTENCE */
#define APDS9110_DD_PRX_PERS_1 0x00 /* Every PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_2 0x01 /* 2 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_3 0x02 /* 3 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_4 0x03 /* 4 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_5 0x04 /* 5 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_6 0x05 /* 6 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_7 0x06 /* 7 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_8 0x07 /* 8 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_9 0x08 /* 9 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_10 0x09 /* 10 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_11 0x0A /* 11 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_12 0x0B /* 12 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_13 0x0C /* 13 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_14 0x0D /* 14 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_15 0x0E /* 15 consecutive PS value out of threshold range */
#define APDS9110_DD_PRX_PERS_16 0x0F /* 16 consecutive PS value out of threshold range */

#define APDS9110_DD_PRX_DEFAULT_PULSE 32	// drop to 16 if crosstalk is too high
#define APDS9110_DD_PRX_DEFAULT_LED_CURRENT APDS9110_DD_LED_CURRENT_10_MA
#define APDS9110_DD_PRX_DEFAULT_LED_FREQ APDS9110_DD_LED_FREQ_60_KHZ
#define APDS9110_DD_PRX_DEFAULT_RES APDS9110_DD_PRX_MEAS_RES_8_BIT
#define APDS9110_DD_PRX_DEFAULT_MEAS_RATE APDS9110_DD_PRX_MEAS_RATE_400_MS

#define APDS9110_PS_DETECTION_THRESHOLD 100	
#define APDS9110_PS_HSYTERESIS_THRESHOLD 70

#define APDS9110_PS_CAL_LOOP 5
#define APDS9110_PS_CAL_CROSSTALK_LOW	0
#define	APDS9110_PS_CAL_CROSSTALK_HIGH  90	// 75% Full Scale ADC

#define APDS_CAL_SKIP_COUNT 0
#define APDS_MAX_CAL (1 + APDS_CAL_SKIP_COUNT)
#define CAL_NUM 99
#define CALIBRATE_PS_DELAY 10000 /* us */

/* POWER SUPPLY VOLTAGE RANGE */
#define APDS9110_VDD_MIN_UV  2000000
#define APDS9110_VDD_MAX_UV  3300000
#define APDS9110_VIO_MIN_UV  1750000
#define APDS9110_VIO_MAX_UV  1950000
#define PS_WAKEUP_TIME 400


#define APDS9110_I2C_BYTE 0
#define APDS9110_I2C_WORD 1

#define APDS9110_FAR_FLAG 1
#define APDS9110_CLOSE_FLAG 0
#define APDS9110_PS_TH_MAX  1023

//#define APDS9110_FAR_INIT 0
//#define APDS9110_NEAR_INIT 1
#define APDS9110_I2C_RETRY_COUNT 3
#define APDS9110_I2C_RETRY_TIMEOUT 1
#define APDS9110_REG_MAX 0x20
#define APDS9110_REG_LEN 0x10

#define APDS9110_CALIBRATION_SENSOR_ACTIVE          -1
#define APDS9110_CALIBRATION_READ_RATE_ERROR     -2
#define APDS9110_CALIBRATION_READ_PDARA_ERROR   -3
#define APDS9110_CALIBRATION_PDATA_BIG_ERROR      -4

#define  FAR_THRESHOLD(x)          ((x) > data->ps_min_threshold ? (x):data->ps_min_threshold)
#define  NEAR_THRESHOLD(x)       ((FAR_THRESHOLD(x) + pdata->pwindow - 1)>254 ? 254:(FAR_THRESHOLD(x) + pdata->pwindow - 1))

struct apds9110_data;

struct apds9110_platform_data {
	u8	   pdrive;
	unsigned int  ppcount;
	int    (*setup_resources)(void);
	int    (*release_resources)(void);

	int irq_num;
	int (*power)(unsigned char onoff);
	/*add the parameters to the init and exit function*/
	int (*init)(struct apds9110_data *data);
	void (*exit)(struct apds9110_data *data);
	int (*power_on)(bool,struct apds9110_data *data);
	
	unsigned int pwave;
	unsigned int pwindow;
	unsigned int threshold_value;
	unsigned int max_noise; /* The maximum value of the noise floor */
	
	unsigned int irq_gpio;
	int flag;
	int ir_current;
};

#endif
