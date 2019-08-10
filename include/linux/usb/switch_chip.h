/************************************************************
*
* Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
* FileName: switch_chip.h
* Author: lixiuna(00213837)       Version : 0.1      Date:  2013-09-01
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

#ifndef __DRIVERS_SWITCH_SMART_CHIP_H__
#define __DRIVERS_SWITCH_SMART_CHIP_H__

/* Register addresses */
#define FSA9685_REG_DEVICE_ID		1
#define FSA9685_REG_CONTROL		2
#define FSA9685_REG_INTERRUPT		3
#define FSA9685_REG_INTERRUPT_MASK	4
#define FSA9685_REG_ADC			5
#define FSA9685_REG_TIMING_SET_1	6
#define FSA9685_REG_DETACH_CONTROL	7
#define FSA9685_REG_DEVICE_TYPE_1	8
#define FSA9685_REG_DEVICE_TYPE_2	9
#define FSA9685_REG_DEVICE_TYPE_3	10
#define FSA9685_REG_MANUAL_SW_1		11
#define FSA9685_REG_MANUAL_SW_2		12
#define FSA9685_REG_TIMING_SET_2	13
#define FSA9685_REG_CONTROL_2		0x0e
#define FSA9685_REG_VBUS_STATUS		0x1b
#define FSA9685_REG_DCD			0x1f
#define FSA9685_REG_WD_CTRL		0x30

/* Register FSA9685_REG_CONTROL (02) */
#define FSA9685_INT_MASK	(1<<0)
#define FSA9685_WAIT		(1<<1)
#define FSA9685_MANUAL_SW	(1<<2)
#define FSA9685_RAW_DATA	(1<<3)
#define FSA9685_SWITCH_OPEN	(1<<4)

/* Register FSA9685_REG_INTERRUPT (03) */
#define FSA9685_ATTACH		(1<<0)
#define FSA9685_DETACH		(1<<1)
#define FSA9685_VBUS_CHANGE	(1<<5)
#define FSA9685_RESERVED_ATTACH	(1<<6)
#define FSA9685_ADC_CHANGE	(1<<7)

/* Register FSA9685_REG_DEVICE_TYPE_1 (08)*/
#define FSA9685_FC_USB_DETECTED			(1<<0)
#define FSA9685_FC_RF_DETECTED			(1<<1)
#define FSA9685_USB_DETECTED			(1<<2)
#define FSA9685_UART_DETECTED			(1<<3)
#define FSA9685_MHL_DETECTED			(1<<4)
#define FSA9685_CDP_DETECTED			(1<<5)
#define FSA9685_DCP_DETECTED			(1<<6)
#define FSA9685_USBOTG_DETECTED			(1<<7)
#define FSA9685_DEVICE_TYPE1_UNAVAILABLE	(0x0a)

/* Register FSA9685_REG_DEVICE_TYPE_2 (09)*/
#define FSA9685_JIG_UART			(1<<2)
#define FSA9685_DEVICE_TYPE2_UNAVAILABLE	(0xf8)

/* Register FSA9685_REG_DEVICE_TYPE_3 (0A)*/
#define FSA9685_CUSTOMER_ACCESSORY5		(1<<5)
#define FSA9685_CUSTOMER_ACCESSORY6		(1<<6)
#define FSA9685_CUSTOMER_ACCESSORY7		(1<<7)
#define FSA9685_FM8_ACCESSORY			(1<<0)
#define FSA9685_DEVICE_TYPE3_UNAVAILABLE	(0x5e)

/* Register FSA9685_REG_CONTROL_2 (0x0e) */
#define FSA9685_DCD_TIMEOUT			(1<<0)
#define FSA9685_FM1_ENABLE			(1<<1)
#define FSA9685_FM6_OPTION_ENABLE		(1<<3)
#define FSA9685_FM8_OPTION_ENABLE		(1<<4)

/*Register VBUS_VALID (1B)*/
#define FSA9685_VBUS_VALID	(1<<1)

/*ID validity detection*/
#define ID_VALID		1
#define ID_INVALID		0
#define MAX_DETECTION_TIMES	3

/*USB state*/
#define FSA9685_OPEN			0
#define FSA9685_USB1_ID_TO_IDBYPASS	1
#define FSA9685_USB2_ID_TO_IDBYPASS	2
#define FSA9685_UART_ID_TO_IDBYPASS	3
#define FSA9685_MHL_ID_TO_CBUS		4
#define FSA9685_USB1_ID_TO_VBAT		5

/*Register FSA9685_REG_MANUAL_SW_1 value in different USB state*/
#define REG_VAL_FSA9685_OPEN			0
#define REG_VAL_FSA9685_USB1_ID_TO_IDBYPASS	((2<<0) | (1<<2) | (1<<5))
#define REG_VAL_FSA9685_USB2_ID_TO_IDBYPASS	((2<<0) | (2<<2) | (2<<5))
#define REG_VAL_FSA9685_UART_ID_TO_IDBYPASS	((2<<0) | (3<<2) | (3<<5))
#define REG_VAL_FSA9685_MHL_ID_TO_CBUS		((3<<0) | (4<<2) | (4<<5))
#define REG_VAL_FSA9685_USB1_ID_TO_VBAT		((1<<5) | (1<<2) | (1<<0))
#define REG_VAL_FSA9685_USB1_BYPASS			((1<<2) | (1<<5))

/* Register FSA9685_REG_WATCHDOG_CONTROL (0x30) */
#define FSA9685_WD_CTRL_JIG_MANUAL_EN		(1<<2)
#define FSA9685_WD_CTRL_ID_DETECT_EN		(1<<3)

/* FSA9683 i2c client address */
#define FSA9683_I2C_ADDR			0x4c
/* FSA9682 i2c client address */
#define FSA9682_I2C_ADDR			0x26

/*for phone-off current drain test*/
#define MANUAL_DETACH		0
#define MANUAL_SWITCH		1
#define JIG_PULL_DEFAULT_DOWN	0
#define JIG_PULL_UP		1
#define REG_JIG_DEFAULT_DOWN	0x03
#define REG_JIG_UP		0x02
#define REG_JIG_MASK		0x03

enum err_oprt_reg_num {
	ERR_FSA9685_REG_MANUAL_SW_1 = 1,
	ERR_FSA9685_READ_REG_CONTROL = 2,
	ERR_FSA9685_WRITE_REG_CONTROL = 3,
};

enum err_oprt_irq_num {
	ERR_REQUEST_THREADED_IRQ = 50,
	ERR_GPIO_DIRECTION_INPUT = 51,
	ERR_GPIO_REQUEST = 52,
	ERR_GPIO_TO_IRQ = 53,
	ERR_OF_GET_NAME_GPIO = 54,
	ERR_SWITCH_USB_DEV_REGISTER = 55,
	ERR_NO_DEV = 56,
};

extern int get_swstate_value(void);
extern void switch_usb2_access_through_ap(void);
extern void switch_usb_set_state_to_mhl(void);
extern void usb_custom_acc5_event(int pedestal_attach);
extern int fsa9685_manual_sw(int input_select);

#endif /* __DRIVERS_SWITCH_SMART_CHIP_H__ */
