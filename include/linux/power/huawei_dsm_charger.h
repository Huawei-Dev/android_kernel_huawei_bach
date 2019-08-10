/************************************************************
*
* Copyright (C), 1988-2015, Huawei Tech. Co., Ltd.
* FileName: huawei_dsm_charger.h
* Author: jiangfei(00270021)       Version : 0.1      Date:  2015-03-20
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
*  Description:    .h file for switch chip
*  Version:
*  Function List:
*  History:
*  <author>  <time>   <version >   <desc>
***********************************************************/
#ifdef CONFIG_HUAWEI_PMU_DSM

#ifndef _HW_DSM_CHARGER
#define _HW_DSM_CHARGER

#include <dsm/dsm_pub.h>
#include <chipset_common/dsm/dsm_pub.h>

#define DSM_COUNT		3
#define CHECKING_TIME	15000
#define DELAY_TIME		20000
#define VOL_THR1		3600000
#define VOL_THR2		3700000
#define VOL_HIGH		4350000
#define VOL_TOO_LOW		3200000
#define VOL_REGULATION_MAX	4450000
#define SOC_THR1		2
#define SOC_ZERO		0
#define SOC_HIGH		90
#define SOC_HIGH_THR	95
#define OVER_CURRENT	5000000
#define HIGH_VOL	4450000
#define LOW_VOL		2500000
#define HOT_TEMP_60    600
#define LOW_TEMP	0
#define TEMP_UPPER_THR	400
#define TEMP_LOWER_THR	200
#define TEMP_BUFFER		20
#define TEMP_DELTA		50
#define INIT_TEMP		-2730
#define HALF_MINUTE		30
#define WARM_COOL_CURRENT_LIMIT		1000000
#define START_DISMATCH_COUNT		3
#define BATT_FULL_LEVEL			100
#define NOT_CHARGE_COUNT		3
#define WARM_VOL_BUFFER	100 /* warm_bat_mv need have a 100mV buffer */
#define MSG_MAX_SIZE		1024
#define ABNORMAL_UVLO_VOL_THR	3700000

#define ERR_MAX				2048 /* error no array max number*/
#define PMU_ERR_NO_MIN	920009000 //10100
#define PMU_ERR_NO_MAX	(ERR_MAX + PMU_ERR_NO_MIN -1)
#define REPORT_MAX			3
#define DELAY_COUNT		3
#define ONE_MINUTE			60  /* 60 seconds */
#define TEMP_BUFFER		20

#define DSM_POST_BUF_SIZE	(50)
#define QUEUE_INIT_SIZE		(6)

enum DSM_BMS_ERR {
	DSM_BMS_NORMAL_SOC_CHANGE_MUCH = 920010000, //10100,
	DSM_BMS_VOL_SOC_DISMATCH_1, /* battery voltage is over 3.6V, but soc = 0 */
	DSM_BMS_VOL_SOC_DISMATCH_2, /* battery voltage is over 3.7V, but soc < 2 */
	DSM_BMS_VOL_SOC_DISMATCH_3, /* battery voltage is over 4.35V, but soc < 90 */
	DSM_VM_BMS_VOL_SOC_DISMATCH_4, /* battery voltage is too low when soc = 2 */
	DSM_BMS_SOC_CHANGE_PLUG_INOUT,
	DSM_BMS_POWON_SOC_CHANGE_MUCH,
	DSM_BMS_BATT_PROFILE_EXCEPTION,
	DSM_BMS_VOL_SOC_DISMATCH,
	DSM_BMS_HIGH_VOLTAGE_UVLO,
	DSM_BMS_NOT_STANDARD_BATTERY,
	DSM_BMS_LEARN_CC,
};

enum DSM_CHARGER_ERR {
	DSM_NOT_CHARGE_WHEN_ALLOWED = 920011000,//10200,
	DSM_SPMI_ABNORMAL_ERROR_NO,
	DSM_CHG_OVP_ERROR_NO,
	DSM_BATT_PRES_ERROR_NO,
	DSM_WARM_CURRENT_LIMIT_FAIL,
	DSM_COOL_CURRENT_LIMIT_FAIL,
	DSM_FULL_WHEN_CHARGER_ABSENT,
	DSM_CHARGER_ONLINE_ABNORMAL_ERROR_NO,
	DSM_BATT_VOL_OVER_4400,
	DSM_BATT_VOL_TOO_LOW,
	DSM_FAKE_FULL,
	DSM_ABNORMAL_CHARGE_STATUS,
	DSM_NONSTANDARD_CHARGER_DETETED,
	DSM_STILL_CHARGE_WHEN_HOT,
	DSM_STILL_CHARGE_WHEN_COLD,
	DSM_STILL_CHARGE_WHEN_SET_DISCHARGE,
	DSM_STILL_CHARGE_WHEN_VOL_OVER_4350,
	DSM_HEATH_OVERHEAT,
	DSM_BATTERY_ID_UNKNOW,
	DSM_BATT_TEMP_JUMP,
	DSM_BATT_TEMP_BELOW_0,
	DSM_BATT_TEMP_OVER_60,
	DSM_NOT_CHARGING_WHEN_HOT,
	DSM_NOT_CHARGING_WHEN_COLD,
	DSM_USBIN_IRQ_INVOKE_TOO_QUICK,
	DSM_CHARGE_DISABLED_IN_USER_VERSION,
	DSM_SET_FACTORY_DIAG_IN_USER_VERSION,
	DSM_BQ_ENABLE_OTG_FAIL,
	DSM_BQ_I2C_ERROR,
	DSM_BQ_CHARGE_WHEN_OTGEN,
	DSM_BQ_WATCHDOG_FAULT,
	DSM_BQ_OTG_FAULT,
	DSM_BQ_POWER_SUPPLY_OVP,
	DSM_BQ_THERMAL_SHUTDOWM,
	DSM_BQ_BAT_FAULT_OVP,
	DSM_BQ_CHRG_TIMER_EXPIRED,
	DSM_LINEAR_USB_OVERTEMP,
	DSM_LINEAR_CHG_FAILED,
	DSM_LINEAR_CHG_OVERCURRENT,
	DSM_LINEAR_BAT_OVERCURRENT,
	DSM_ABNORMAL_CHARGE_FULL_STATUS,
	DSM_BATT_VOL_OVER_4450,
	DSM_STILL_CHARGE_WHEN_VOL_OVER_4450,
	DSM_CHG_TIMEOUT,
	DSM_CHG_SPMI_INT_ERR,
	DSM_CHG_BATT_OV,
	DSM_CHG_BAD_CHARGER,
};

enum DSM_PMU_ERR {
	DSM_ABNORMAL_POWERON_REASON_1 		= 920009000,//10300,
	DSM_ABNORMAL_POWEROFF_REASON_1,
	DSM_ABNORMAL_POWEROFF_REASON_2,
	DSM_ABNORMAL_POWEROFF_REASON_3,
	DSM_ABNORMAL_POWEROFF_REASON_4,
	DSM_CPU_OVERTEMP,
	DSM_PA_OVERTEMP,
	DSM_THERMAL_ZONE2_OVERTEMP,
	DSM_THERMAL_ZONE4_OVERTEMP,
};

struct dsm_err_info
{
	int err_no; /* error number */
	int count[ERR_MAX]; /* error count, if exceed 3 times, not report */
};

extern struct dsm_err_info dsm_err;

struct hw_batt_temp_info
{
	int    cold_bat_degree;    /*lowest temperature to stop charging */
	int    cool_bat_degree;    /*cool temprature to limit charging current and voltage*/
	int    imaxma_cool_bat;    /* max battery charging input current in ma */
	int    vmaxmv_cool_bat;    /* max battery terminate voltage in mv*/
	int    warm_bat_degree;    /*warm temprature to limit charging current and voltage*/
	int    imaxma_warm_bat;    /* max battery charging input current in ma */
	int    vmaxmv_warm_bat;    /* max battery terminate voltage in mv */
	int    hot_bat_degree;    /*highest temperature to stop charging */
};

extern struct hw_batt_temp_info temp_info;

int dsm_dump_log(struct dsm_client *dclient, int err_no);

extern struct dsm_client *bms_dclient;
extern struct dsm_client *charger_dclient;

/*Transfer the msg to device state monitor*/
#define DSM_PMU_LOG(client, err_num, fmt, a...) \
	do { \
		char msg[MSG_MAX_SIZE]; \
		snprintf(msg, MSG_MAX_SIZE-1, fmt, ## a); \
		if ((PMU_ERR_NO_MIN > err_num) \
				|| (PMU_ERR_NO_MAX < err_num)) { \
			pr_err("err_no is exceed available number, do nothing!\n"); \
			break; \
		} \
		dsm_err.err_no = err_num - PMU_ERR_NO_MIN; \
		if (dsm_err.count[dsm_err.err_no]++ < REPORT_MAX) { \
			if (!dsm_client_ocuppy(client)) { \
				dsm_client_record(client,msg); \
				dsm_client_notify(client, err_num); \
			} \
		} else { \
			dsm_err.count[dsm_err.err_no] = REPORT_MAX; \
		} \
	} while(0)

typedef struct chg_dsm_post_erro_info
{
	int erro_number;
	char content_info[DSM_POST_BUF_SIZE];
}Queue_ElemeType;

typedef struct
{
	Queue_ElemeType base[QUEUE_INIT_SIZE];
	int head;
	int tail;
}SqQueue;

int dsm_post_chg_bms_info(int erro_no, char *pstr);

#endif
#endif
