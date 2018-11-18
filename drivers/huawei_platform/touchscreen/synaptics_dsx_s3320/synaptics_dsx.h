/*Add synaptics new driver "Synaptics DSX I2C V2.0"*/
/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SYNAPTICS_DSX_H_
#define _SYNAPTICS_DSX_H_
#define CONFIG_HUAWEI_KERNEL 1

#include "synaptics_dsx_i2c.h"

#ifdef CONFIG_HUAWEI_KERNEL
//#define ENABLE_VIRTUAL_KEYS

#define PID_JDI_S3320 "MLAN11080"
#define PID_JDI_LEN 9
#define PID_BOE_PLK11130 "PLK11130"
#define PID_BOE_LEN 8
#define PID_BOE_CANN_HIC "CANN11130"
#define PID_BOE_CANN_LEN 9
#define PID_BOE_MALN "MLAN21130"
#define PID_BOE_MALN_LEN 9
#define PID_BOE_CANN "CANN27130"
#define PID_BOE_CANN_LEN 9
#define PID_TM_CANN "CANN27110"
#define PID_TM_CANN_LEN 9
#define PID_LG_CANN "CANN271006"
#define PID_LG_CANN_LEN 10
#define PID_LG_VENUS "S332U"
#define PID_LG_LEN 5
/*
 * synaptics_dsx_cap_button_map - 0d button map
 * @nbuttons: number of 0d buttons
 * @map: pointer to array of button types
 */
struct synaptics_dsx_cap_button_map {
	unsigned char nbuttons;
	unsigned char *map;
};

#ifdef CONFIG_HUAWEI_KERNEL
#ifdef ENABLE_VIRTUAL_KEYS
struct syanptics_virtual_keys {
	struct kobj_attribute kobj_attr;
	u16 *data;
	int size;
};
#endif

struct syanptics_wakeup_keys {
	const uint16_t  *keys;
	uint8_t         size;
	uint8_t         enable_wkeys;/*just padding, not used*/
} __packed;

enum syanptics_gesture_flags {
	RMI4_EASY_WAKE_UP_GESTURE_DISABLED = 0x00,
	RMI4_EASY_WAKE_UP_GESTURE_ENABLED = 0x01,
};

enum syanptics_palm_flags {
	RMI4_PALM_SLEEP_GESTURE_DISABLED = 0x00,
	RMI4_PALM_SLEEP_GESTURE_ENABLED = 0x01,
};

enum syanptics_glove_mode {
	SYSTEM_START_IN_SKIN_MODE = 0,       //TP starts in skin mode by default,but can auto switch with glove mode.
	SYSTEM_START_IN_GLOVE_MODE = 1,    //TP starts in glove mode by default,but can auto switch with skin mode
	SYSTEM_LOCKED_TO_SKIN_MODE = 2,    //TP is locked to skin mode 
	SYSTEM_LOCKED_TO_GLOVE_MODE = 3, //TP is locked to glove mode
};

enum syanptics_glove_flags {
	RMI4_GLOV_FUNC_DISABLED = 0x00,
	RMI4_GLOV_FUNC_ENABLED = 0x01,
};

enum syanptics_holster_flags {
	RMI4_HOLSTER_FUNC_DISABLED = 0x00,
	RMI4_HOLSTER_FUNC_ENABLED = 0x01,
};
enum s4322_syanptics_holster_flags {
	S4322_RMI4_HOLSTER_FUNC_DISABLED = 0x00,
	S4322_RMI4_HOLSTER_FUNC_ENABLED = 0x02,
};
#endif /*CONFIG_HUAWEI_KERNEL*/

/*
* ic type use the union product id definition
* please check the document on wiki
*/
enum synaptics_ic_type{
	SYNAPTICS_S3207 = 0,
	SYNAPTICS_S3350 = 1,
	SYNAPTICS_S3320 = 11,
	SYNAPTICS_S3718 = 17,
	SYNAPTICS_S4322 = 18,/*TDDI*/
	SYNAPTICS_S4300 = 27,
};
/*
 * struct synaptics_dsx_platform_data - dsx platform data
 * @x_flip: x flip flag
 * @y_flip: y flip flag
 * @irq_gpio: attention interrupt gpio
 * @power_gpio: power switch gpio
 * @power_on_state: power switch active state
 * @reset_gpio: reset gpio
 * @reset_on_state: reset active state
 * @irq_flags: irq flags
 * @panel_x: x-axis resolution of display panel
 * @panel_y: y-axis resolution of display panel
 * @power_delay_ms: delay time to wait after power-on
 * @reset_delay_ms: delay time to wait after reset
 * @reset_active_ms: reset active time
 * @regulator_name: pointer to name of regulator
 * @gpio_config: pointer to gpio configuration function
 * @cap_button_map: pointer to 0d button map
 */
struct synaptics_dsx_platform_data {
	bool x_flip;
	bool y_flip;
	bool swap_axes;
	bool reg_en;
	unsigned char grip_left_lsb;
	unsigned char grip_left_msb;
	unsigned char grip_right_lsb;
	unsigned char grip_right_msb;
	unsigned short ub_i2c_addr;
	unsigned short i2c_addr;
	int irq_gpio;
	int power_gpio;
	int power_on_state;
	int reset_gpio;
	/* Modify JDI tp/lcd power on/off to reduce power consumption */
	int vci_gpio;
	unsigned int ic_type;
	int reset_on_state;
	unsigned long irq_flags;
	unsigned int panel_x;
	unsigned int panel_y;
	unsigned int power_delay_ms;
	unsigned int reset_delay_ms;
	unsigned int reset_active_ms;
	unsigned int sleep_strategy;
	unsigned int debug_data_type;
	struct touch_panel_reg_operate reg_operate;
#ifdef CONFIG_HUAWEI_KERNEL
	unsigned char *regulator_vdd;
	unsigned char *regulator_vbus;
	unsigned int lcd_x;
	unsigned int lcd_y;
	unsigned int lcd_all;
	/* defination of double tap gesture zone */
	int dtz_x0;
	int dtz_y0;
	int dtz_x1;
	int dtz_y1;
	const char *product_name;
#ifdef ENABLE_VIRTUAL_KEYS
	struct syanptics_virtual_keys vkeys;
#endif
	bool gesture_enabled;
	bool glove_enabled;
	unsigned int easy_wakeup_supported_gestures;
	bool glove_supported;
	bool esd_support;
	bool low_power_support;
	bool glove_edge_switch_supported;
	bool grip_algorithm_supported;
	bool grip_algorithm_enabled;
	struct syanptics_wakeup_keys *wakeup_keys;
	unsigned char fast_relax_gesture;

	bool holster_supported;
	bool roi_supported;
	bool tx_rx_support;
	bool tddi_esd_flag;
#endif /*CONFIG_HUAWEI_KERNEL*/
	int (*gpio_config)(int gpio, bool configure, int dir, int state);
	struct synaptics_dsx_cap_button_map *cap_button_map;
	bool shutdown_flag;
	bool appinfo_display_flag;
};
#ifdef CONFIG_HUAWEI_DSM
ssize_t synaptics_dsm_record_fw_err_info( int err_numb );
ssize_t synaptics_dsm_f34_pdt_err_info( int err_numb );
ssize_t synaptics_dsm_f54_pdt_err_info( int err_numb );
ssize_t synaptics_dsm_f34_read_queries_err_info( int err_numb );
ssize_t synaptics_dsm_fwu_init_pdt_props_err_info( int err_numb );
int synp_tp_report_dsm_err( int type, int err_numb );

struct dsm_client* tp_dsm_get_client(void);
#endif/*CONFIG_HUAWEI_DSM*/
const char * get_cof_module_name(char * product_id_ptr);
#endif /*CONFIG_HUAWEI_KERNEL*/
#endif
