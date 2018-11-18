/**********************************************************
 * Filename:	hw_typec.h
 *
 * Discription: Huawei type-c device public head file for
 *              type-c core driver and chip drivers
 *
 * Copyright: (C) 2014 huawei.
 *
 *
**********************************************************/

#ifndef __HW_TYPEC_DEV_H__
#define __HW_TYPEC_DEV_H__

#include <linux/device.h>
#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/wakelock.h>
#include <linux/usb/class-dual-role.h>
#include <linux/delay.h>
#endif

#ifdef CONFIG_DUAL_ROLE_USB_INTF
#define REVERSE_ATTEMPT    1
#define REVERSE_COMPLETE    2
#define TRYSNK_TIMEOUT_MS    500
#define DUAL_ROLE_SET_MODE_WAIT_MS    1500
#define WAIT_VBUS_OFF_MS    50

struct typec_device_info {
	bool sink_attached;
	struct dual_role_phy_instance *dual_role;
	struct dual_role_phy_desc *desc;
	struct delayed_work g_wdog_work;
	struct completion reverse_completion;
	struct wake_lock wake_lock;
	bool trysnk_attempt;
	int reverse_state;
};
#endif

enum typec_input_current {
	TYPEC_DEV_CURRENT_DEFAULT = 0,
	TYPEC_DEV_CURRENT_MID,
	TYPEC_DEV_CURRENT_HIGH,
	TYPEC_DEV_CURRENT_NOT_READY,
};

enum typec_cc_orient {
	TYPEC_ORIENT_DEFAULT = 0,
	TYPEC_ORIENT_CC1,
	TYPEC_ORIENT_CC2,
	TYPEC_ORIENT_NOT_READY,
};

struct typec_device_ops {
	enum typec_cc_orient (*detect_cc_orientation)(void);
	enum typec_input_current (*detect_input_current)(void);
};

/* for chip layer to register ops functions and interrupt status*/
extern int typec_chip_register(struct typec_device_ops *ops);
extern struct class *hw_typec_get_class(void);

#endif
