/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#define pr_fmt(fmt)	"TYPEC: %s: " fmt, __func__

#include <linux/bitops.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/regulator/machine.h>
#include <linux/slab.h>
#include <linux/spmi.h>
#include <linux/usb/class-dual-role.h>

#ifdef CONFIG_SWITCH_FSA9685
#include <linux/usb/switch_chip.h>
#endif
#ifdef CONFIG_HUAWEI_TYPEC
#include <linux/usb/hw_typec.h>
#endif
#ifdef CONFIG_HUAWEI_USB
#include <linux/usb/huawei_usb.h>
#endif

#ifdef CONFIG_HUAWEI_USB
#undef pr_err
#define pr_err usb_logs_err

#undef pr_info
#define pr_info usb_logs_info

/* log here is important so set log level to info */
#undef pr_debug
#define pr_debug usb_logs_info

#undef dev_info
#define dev_info usb_dev_info

#undef dev_err
#define dev_err usb_dev_err

/* log here is important so set log level to info */
#undef dev_dbg
#define dev_dbg usb_dev_info
#endif


#define CREATE_MASK(NUM_BITS, POS) \
	((unsigned char) (((1 << (NUM_BITS)) - 1) << (POS)))
#define TYPEC_MASK(MSB_BIT, LSB_BIT) \
	CREATE_MASK(MSB_BIT - LSB_BIT + 1, LSB_BIT)

/* Interrupt offsets */
#define INT_RT_STS_REG(base)		(base + 0x10)
#define DFP_DETECT_BIT			BIT(3)
#define UFP_DETECT_BIT			BIT(1)

#define TYPEC_UFP_STATUS_REG(base)	(base +	0x08)
#define TYPEC_CCOUT_BIT			BIT(7)
#define TYPEC_CCOUT_OPEN_BIT		BIT(6)
#define TYPEC_CURRENT_MASK		TYPEC_MASK(2, 0)
#define TYPEC_RDSTD_BIT			BIT(2)
#define TYPEC_RD1P5_BIT			BIT(1)

#define TYPEC_DFP_STATUS_REG(base)	(base +	0x09)
#define VALID_DFP_MASK			TYPEC_MASK(6, 4)

#define TYPEC_INT_EN_CLR(base)          (base + 0x16)
#define VBUS_ERR_CLEAR                   BIT(6)

/*
 * 80-P2536-2X
 * XX00 XXXX : DRP, default
 * XX10 XXXX : UFP
 * XX01 XXXX : DFP
 * XXXX XX0X : OTG Threshold is 1.6V, default
 * XXXX XX1X : OTG Threshold is 0.6V
 */
/* Milan do not support USB3.0, default mode current should be 500mA */
#define TYPEC_SW_CTL_REG(base)		(base + 0x52)
#define TYPEC_SW_CTL(base)		(base + 0x52)
#define TYPEC_MODE_MASK			TYPEC_MASK(5, 4)
#define TYPEC_UFP_MODE_BIT		BIT(5)
#define TYPEC_DFP_MODE_BIT		BIT(4)
#define OTG_THRESHOLD_BIT       BIT(1)

#define TYPEC_STD_MA			500
#define TYPEC_MED_MA			1500
#define TYPEC_HIGH_MA			3000

#define QPNP_TYPEC_DEV_NAME	"qcom,qpnp-typec"
#define TYPEC_PSY_NAME		"typec"
#define DUAL_ROLE_DESC_NAME	"otg_default"

enum cc_line_state {
	CC_1,
	CC_2,
	OPEN,
};

struct typec_wakeup_source {
	struct wakeup_source	source;
	unsigned long		enabled;
};

static void typec_stay_awake(struct typec_wakeup_source *source)
{
	if (!__test_and_set_bit(0, &source->enabled)) {
		__pm_stay_awake(&source->source);
		pr_debug("enabled source %s\n", source->source.name);
	}
}

static void typec_relax(struct typec_wakeup_source *source)
{
	if (__test_and_clear_bit(0, &source->enabled)) {
		__pm_relax(&source->source);
		pr_debug("disabled source %s\n", source->source.name);
	}
}

struct qpnp_typec_chip {
	struct device		*dev;
	struct spmi_device	*spmi;
	struct power_supply	*batt_psy;
	struct power_supply	type_c_psy;
	struct regulator	*ss_mux_vreg;
	struct mutex		typec_lock;
	spinlock_t		rw_lock;

	u16			base;

	/* IRQs */
	int			vrd_changed;
	int			ufp_detach;
	int			ufp_detect;
	int			dfp_detach;
	int			dfp_detect;
	int			vbus_err;
	int			vconn_oc;

	/* Configurations */
	int			cc_line_state;
	int			current_ma;
	int			ssmux_gpio;
	enum of_gpio_flags	gpio_flag;
	int			typec_state;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct typec_device_info di;
#endif
	/* Dual role support */
	bool				role_reversal_supported;
	bool				in_force_mode;
	int				force_mode;
	struct dual_role_phy_instance	*dr_inst;
	struct dual_role_phy_desc	dr_desc;
	struct delayed_work		role_reversal_check;
	struct typec_wakeup_source	role_reversal_wakeup_source;
};

/* current mode */
static char *mode_text[] = {
	"ufp", "dfp", "none"
};

#ifdef CONFIG_HUAWEI_TYPEC
static struct qpnp_typec_chip *g_chip = NULL;

static enum typec_cc_orient qpnp_typec_detect_cc_orientation(void)
{
	enum typec_cc_orient cc_orient = TYPEC_ORIENT_NOT_READY;

	if (!g_chip) {
		pr_err("no chip found\n");
		return cc_orient;
	}

	pr_info("cc_line_state = %d\n", g_chip->cc_line_state);
	switch (g_chip->cc_line_state) {
	case CC_1:
		cc_orient = TYPEC_ORIENT_CC1;
		break;
	case CC_2:
		cc_orient = TYPEC_ORIENT_CC2;
		break;
	default:
		cc_orient = TYPEC_ORIENT_NOT_READY;
	}

	return cc_orient;
}

static void typec_wake_lock(struct typec_device_info *di)
{
	if (!wake_lock_active(&di->wake_lock)) {
		pr_err("usb typec wake lock\n");
		wake_lock(&di->wake_lock);
	}
}

static void typec_wake_unlock(struct typec_device_info *di)
{
	if (wake_lock_active(&di->wake_lock)) {
		pr_err("usb typec wake unlock\n");
		wake_unlock(&di->wake_lock);
	}
}

static enum typec_input_current qpnp_typec_detect_input_current(void)
{
	enum typec_input_current input_current = TYPEC_DEV_CURRENT_NOT_READY;

	if (!g_chip) {
		pr_err("no chip found\n");
		return input_current;
	}

	pr_info("current_ma = %d\n", g_chip->current_ma);
	switch (g_chip->current_ma) {
	case TYPEC_STD_MA:
		input_current = TYPEC_DEV_CURRENT_DEFAULT;
		break;
	case TYPEC_MED_MA:
		input_current = TYPEC_DEV_CURRENT_MID;
		break;
	case TYPEC_HIGH_MA:
		input_current = TYPEC_DEV_CURRENT_HIGH;
		break;
	default:
		input_current = TYPEC_DEV_CURRENT_NOT_READY;
	}

	return input_current;
}

static struct typec_device_ops qpnp_typec_ops = {
	.detect_cc_orientation = qpnp_typec_detect_cc_orientation,
	.detect_input_current = qpnp_typec_detect_input_current,
};
#endif

/* SPMI operations */
static int __qpnp_typec_read(struct spmi_device *spmi, u8 *val, u16 addr,
			int count)
{
	int rc;

	rc = spmi_ext_register_readl(spmi->ctrl, spmi->sid, addr, val, count);
	if (rc)
		pr_err("spmi read failed addr=0x%02x sid=0x%02x rc=%d\n",
				addr, spmi->sid, rc);

	return rc;
}

static int __qpnp_typec_write(struct spmi_device *spmi, u8 *val, u16 addr,
			int count)
{
	int rc;

	rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, addr, val, count);
	if (rc)
		pr_err("spmi write failed addr=0x%02x sid=0x%02x rc=%d\n",
				addr, spmi->sid, rc);
	return rc;
}

static int qpnp_typec_read(struct qpnp_typec_chip *chip, u8 *val, u16 addr,
			int count)
{
	int rc;
	unsigned long flags;
	struct spmi_device *spmi = chip->spmi;

	if (addr == 0) {
		pr_err("addr cannot be zero addr=0x%02x sid=0x%02x rc=%d\n",
				addr, spmi->sid, rc);
		return -EINVAL;
	}

	spin_lock_irqsave(&chip->rw_lock, flags);
	rc = __qpnp_typec_read(spmi, val, addr, count);
	spin_unlock_irqrestore(&chip->rw_lock, flags);

	return rc;
}

static int qpnp_typec_masked_write(struct qpnp_typec_chip *chip, u16 base,
			u8 mask, u8 val)
{
	u8 reg;
	int rc;
	unsigned long flags;
	struct spmi_device *spmi = chip->spmi;

	spin_lock_irqsave(&chip->rw_lock, flags);
	rc = __qpnp_typec_read(spmi, &reg, base, 1);
	if (rc) {
		pr_err("spmi read failed: addr=%03X, rc=%d\n", base, rc);
		goto out;
	}

	reg &= ~mask;
	reg |= val & mask;

	pr_debug("addr = 0x%x writing 0x%x\n", base, reg);

	rc = __qpnp_typec_write(spmi, &reg, base, 1);
	if (rc) {
		pr_err("spmi write failed: addr=%03X, rc=%d\n", base, rc);
		goto out;
	}

out:
	spin_unlock_irqrestore(&chip->rw_lock, flags);
	return rc;
}

static int qpnp_typec_write(struct qpnp_typec_chip *chip, u8 *val, u16 addr,
			    int count)
{
	int rc = 0;
	struct spmi_device *spmi = chip->spmi;

	if (addr == 0) {
		dev_err(chip->dev,
			"addr cannot be zero addr=0x%04x sid=0x%02x rc=%d\n",
			addr, spmi->sid, rc);
		return -EINVAL;
	}

	rc = spmi_ext_register_writel(spmi->ctrl, spmi->sid, addr, val, count);
	if (rc) {
		dev_err(chip->dev,
			"write failed addr=0x%04x sid=0x%02x rc=%d\n", addr,
			spmi->sid, rc);
		return rc;
	}

	return 0;
}

static void qpnp_typec_mode_ufp(struct qpnp_typec_chip *chip)
{
	pr_info("\n");
	qpnp_typec_masked_write(chip,TYPEC_SW_CTL(chip->base),
                                TYPEC_MODE_MASK,TYPEC_UFP_MODE_BIT);
}

static void qpnp_typec_mode_dfp(struct qpnp_typec_chip *chip)
{
	pr_info("\n");
	qpnp_typec_masked_write(chip,TYPEC_SW_CTL(chip->base),
                                 TYPEC_MODE_MASK, TYPEC_DFP_MODE_BIT);
}

static void qpnp_typec_mode_drp(struct qpnp_typec_chip *chip)
{
	pr_info("\n");
	qpnp_typec_masked_write(chip,TYPEC_SW_CTL(chip->base),
				TYPEC_MODE_MASK,0x00);
}

#define THRESHOLD_1P6 0
#define THRESHOLD_0P6 1
static void qpnp_typec_otg_threshold(struct qpnp_typec_chip *chip,
				     int threshold)
{
	if (THRESHOLD_1P6 == threshold) {
		pr_info("set to 1.6\n");
		qpnp_typec_masked_write(chip,TYPEC_SW_CTL(chip->base),
					OTG_THRESHOLD_BIT,0x00);
	} else if (THRESHOLD_0P6 == threshold) {
		pr_info("set to 0.6\n");
		qpnp_typec_masked_write(chip,TYPEC_SW_CTL(chip->base),
					OTG_THRESHOLD_BIT, OTG_THRESHOLD_BIT);
	} else {
		pr_err("invalid input\n");
	}
}

static ssize_t mode_show(struct device *pdev,
			 struct device_attribute *attr, char *buf)
{
	struct qpnp_typec_chip *chip = g_chip;
	int rc = 0;
	u8 reg = 0;

	rc = qpnp_typec_read(chip, &reg, TYPEC_SW_CTL(chip->base), 1);
	pr_info("reg TYPEC_SW_CTL =0x%02x\n", reg);

	return snprintf(buf, PAGE_SIZE, "TYPEC_SW_CTL=0x%02x\n", reg);
}

static ssize_t mode_store(struct device *pdev, struct device_attribute *attr,
			  const char *buf, size_t size)
{
	char buffer[64] = { 0 };

	if (sscanf(buf, "%s", buffer) != 1) {
		pr_err("invalid input\n");
	}

	if (!strcmp(buffer, "ufp")) {
		qpnp_typec_mode_ufp(g_chip);
	} else if (!strcmp(buffer, "dfp")) {
		qpnp_typec_mode_dfp(g_chip);
	} else if (!strcmp(buffer, "drp")) {
		qpnp_typec_mode_drp(g_chip);
	} else if (!strcmp(buffer, "0p6")) {
		qpnp_typec_otg_threshold(g_chip, THRESHOLD_0P6);
	} else if (!strcmp(buffer, "1p6")) {
		qpnp_typec_otg_threshold(g_chip, THRESHOLD_1P6);
	} else {
		pr_err("invalid input, only ufp dfp 0p6 1p6 supported\n");
	}
	return size;
}

static DEVICE_ATTR(mode, (S_IRUGO | S_IWUSR), mode_show, mode_store);
static struct attribute *qpnp_attributes[] = {
	&dev_attr_mode.attr,
	NULL,
};

static const struct attribute_group qpnp_attr_group = {
	.attrs = qpnp_attributes,
};

static int qpnp_create_sysfs(void)
{
	int ret = 0;
	struct class *typec_class = NULL;
	struct device *new_dev = NULL;

	typec_class = hw_typec_get_class();
	if (typec_class) {
		new_dev = device_create(typec_class, NULL, 0, NULL, "qpnp");
		ret = sysfs_create_group(&new_dev->kobj, &qpnp_attr_group);
		if (ret) {
			pr_err("sysfs create error\n");
		}
	}

	return ret;
}

#ifdef CONFIG_DUAL_ROLE_USB_INTF
static int qpnp_typec_handle_detach(struct qpnp_typec_chip *chip);

static enum dual_role_property fusb_drp_properties[] = {
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
};

 /* Callback for "cat /sys/class/dual_role_usb/otg_default/<property>" */
static int dual_role_get_local_prop(struct dual_role_phy_instance *dual_role,
				    enum dual_role_property prop,
				    unsigned int *val)
{
	struct typec_device_info *di = dual_role_get_drvdata(dual_role);
	int typec_state = 0;

	if (!di || !g_chip)
		return -EINVAL;

	mutex_lock(&g_chip->typec_lock);
	typec_state = g_chip->typec_state;
	mutex_unlock(&g_chip->typec_lock);
	if (typec_state == POWER_SUPPLY_TYPE_DFP) {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_DFP;
		else if (prop == DUAL_ROLE_PROP_PR)
			*val = DUAL_ROLE_PROP_PR_SRC;
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_HOST;
		else
			return -EINVAL;
	} else if (typec_state == POWER_SUPPLY_TYPE_UFP) {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_UFP;
		else if (prop == DUAL_ROLE_PROP_PR)
			*val = DUAL_ROLE_PROP_PR_SNK;
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_DEVICE;
		else
			return -EINVAL;
	} else {
		if (prop == DUAL_ROLE_PROP_MODE)
			*val = DUAL_ROLE_PROP_MODE_NONE;
		else if (prop == DUAL_ROLE_PROP_PR)
			*val = DUAL_ROLE_PROP_PR_NONE;
		else if (prop == DUAL_ROLE_PROP_DR)
			*val = DUAL_ROLE_PROP_DR_NONE;
		else
			return -EINVAL;
	}

	return 0;
}

/* Decides whether userspace can change a specific property */
static int dual_role_is_writeable(struct dual_role_phy_instance *drp,
				  enum dual_role_property prop)
{
	if (prop == DUAL_ROLE_PROP_MODE)
		return 1;
	else
		return 0;
}

/* 1. Check to see if current attached_state is same as requested state
 * if yes, then, return.
 * 2. Disonect current session
 * 3. Set approrpriate mode (dfp or ufp)
 * 4. wait for 1.5 secs to see if we get into the corresponding target state
 * if yes, return
 * 5. if not, fallback to Try.SNK
 * 6. wait for 1.5 secs to see if we get into one of the attached states
 * 7. return -EIO
 * Also we have to fallback to Try.SNK state machine on cable disconnect
 */
static int dual_role_set_mode_prop(struct dual_role_phy_instance *dual_role,
				   enum dual_role_property prop,
				   const unsigned int *val)
{
	struct typec_device_info *di = dual_role_get_drvdata(dual_role);
	struct qpnp_typec_chip *chip = g_chip;
	int typec_state = 0;
	int timeout = 0;
	int ret = 0;

	if (!di || !chip)
		return -EINVAL;

	if (*val != DUAL_ROLE_PROP_MODE_DFP && *val != DUAL_ROLE_PROP_MODE_UFP)
		return -EINVAL;

	typec_state = chip->typec_state;

	if (typec_state != POWER_SUPPLY_TYPE_DFP
	    && typec_state != POWER_SUPPLY_TYPE_UFP)
		return 0;

	if (typec_state == POWER_SUPPLY_TYPE_DFP
	    && *val == DUAL_ROLE_PROP_MODE_DFP)
		return 0;

	if (typec_state == POWER_SUPPLY_TYPE_UFP
	    && *val == DUAL_ROLE_PROP_MODE_UFP)
		return 0;

	pr_info("start\n");
	mutex_lock(&chip->typec_lock);

	/* AS DFP now, try reversing, form Source to Sink */
	if (typec_state == POWER_SUPPLY_TYPE_DFP) {
		pr_info("try reversing, form Source to Sink\n");
		di->reverse_state = REVERSE_ATTEMPT;
		di->sink_attached = false;

		/* turns off VBUS first */
		qpnp_typec_handle_detach(chip);
		msleep(WAIT_VBUS_OFF_MS);

		qpnp_typec_mode_ufp(chip);

		/* AS UFP now, try reversing, form Sink to Source */
	} else if (typec_state == POWER_SUPPLY_TYPE_UFP) {
		pr_info("try reversing, form Sink to Source\n");
		di->reverse_state = REVERSE_ATTEMPT;
		qpnp_typec_handle_detach(chip);
		qpnp_typec_mode_dfp(chip);
	}

	mutex_unlock(&chip->typec_lock);

	reinit_completion(&di->reverse_completion);
	timeout =
	    wait_for_completion_timeout(&di->reverse_completion,
					msecs_to_jiffies
					(DUAL_ROLE_SET_MODE_WAIT_MS));
	if (!timeout) {
		pr_info("reverse failed, set mode to DRP\n");

		/* set mode to DRP */
		di->reverse_state = 0;
		qpnp_typec_mode_drp(chip);

		pr_info("wait for the attached state\n");
		reinit_completion(&di->reverse_completion);
		wait_for_completion_timeout(&di->reverse_completion,
					    msecs_to_jiffies
					    (DUAL_ROLE_SET_MODE_WAIT_MS));

		ret = -EIO;
	}

	pr_info("end ret = %d\n", ret);

	return ret;
}

/* Callback for "echo <value> >
 *                      /sys/class/dual_role_usb/<name>/<property>"
 * Block until the entire final state is reached.
 * Blocking is one of the better ways to signal when the operation
 * is done.
 * This function tries to switch to Attached.SRC or Attached.SNK
 * by forcing the mode into SRC or SNK.
 * On failure, we fall back to Try.SNK state machine.
 */
static int dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			      enum dual_role_property prop,
			      const unsigned int *val)
{
	if (prop == DUAL_ROLE_PROP_MODE)
		return dual_role_set_mode_prop(dual_role, prop, val);
	else
		return -EINVAL;
}

static void typec_wdog_work(struct work_struct *w)
{
	struct typec_device_info *di =
	    container_of(w, struct typec_device_info, g_wdog_work.work);
	struct qpnp_typec_chip *chip =
	    container_of(di, struct qpnp_typec_chip, di);

	pr_info("start\n");
	qpnp_typec_mode_drp(chip);
	typec_wake_unlock(di);
	pr_info("end\n");
}

#endif

static int set_property_on_battery(struct qpnp_typec_chip *chip,
				enum power_supply_property prop)
{
	int rc = 0;
	union power_supply_propval ret = {0, };

	if (!chip->batt_psy) {
		chip->batt_psy = power_supply_get_by_name("battery");
		if (!chip->batt_psy) {
			pr_err("no batt psy found\n");
			return -ENODEV;
		}
	}

	switch (prop) {
	case POWER_SUPPLY_PROP_CURRENT_CAPABILITY:
		ret.intval = chip->current_ma;
		rc = chip->batt_psy->set_property(chip->batt_psy,
			POWER_SUPPLY_PROP_CURRENT_CAPABILITY, &ret);
		if (rc)
			pr_err("failed to set current max rc=%d\n", rc);
		break;
	case POWER_SUPPLY_PROP_TYPEC_MODE:
		/*
		 * Notify the typec mode to charger. This is useful in the DFP
		 * case where there is no notification of OTG insertion to the
		 * charger driver.
		 */
		ret.intval = chip->typec_state;
		rc = chip->batt_psy->set_property(chip->batt_psy,
				POWER_SUPPLY_PROP_TYPEC_MODE, &ret);
		if (rc)
			pr_err("failed to set typec mode rc=%d\n", rc);
		break;
	default:
		pr_err("invalid request\n");
		rc = -EINVAL;
	}

	return rc;
}

static int get_max_current(u8 reg)
{
	if (!reg)
		return 0;

	return (reg & TYPEC_RDSTD_BIT) ? TYPEC_STD_MA :
		(reg & TYPEC_RD1P5_BIT) ? TYPEC_MED_MA : TYPEC_HIGH_MA;
}

static int qpnp_typec_configure_ssmux(struct qpnp_typec_chip *chip,
				enum cc_line_state cc_line)
{
	int rc = 0;

	if (cc_line != chip->cc_line_state) {
		switch (cc_line) {
		case OPEN:
			if (chip->ss_mux_vreg) {
				rc = regulator_disable(chip->ss_mux_vreg);
				if (rc) {
					pr_err("failed to disable ssmux regulator rc=%d\n",
							rc);
					return rc;
				}
			}

			if (chip->ssmux_gpio) {
				rc = gpio_direction_input(chip->ssmux_gpio);
				if (rc) {
					pr_err("failed to configure ssmux gpio rc=%d\n",
							rc);
					return rc;
				}
			}
			break;
		case CC_1:
		case CC_2:
			if (chip->ss_mux_vreg) {
				rc = regulator_enable(chip->ss_mux_vreg);
				if (rc) {
					pr_err("failed to enable ssmux regulator rc=%d\n",
							rc);
					return rc;
				}
			}

			if (chip->ssmux_gpio) {
				rc = gpio_direction_output(chip->ssmux_gpio,
					(chip->gpio_flag == OF_GPIO_ACTIVE_LOW)
						? !cc_line : cc_line);
				if (rc) {
					pr_err("failed to configure ssmux gpio rc=%d\n",
							rc);
					return rc;
				}
			}
			break;
		}
	}

	return 0;
}

#define UFP_EN_BIT			BIT(5)
#define DFP_EN_BIT			BIT(4)
#define FORCE_MODE_MASK			TYPEC_MASK(5, 4)
static int qpnp_typec_force_mode(struct qpnp_typec_chip *chip, int mode)
{
	int rc = 0;
	u8 reg = (mode == DUAL_ROLE_PROP_MODE_UFP) ? UFP_EN_BIT
			: (mode == DUAL_ROLE_PROP_MODE_DFP) ? DFP_EN_BIT : 0x0;

	if (chip->force_mode != mode) {
		rc = qpnp_typec_masked_write(chip,
			TYPEC_SW_CTL_REG(chip->base), FORCE_MODE_MASK, reg);
		if (rc) {
			pr_err("Failed to force typeC mode rc=%d\n", rc);
		} else {
			chip->force_mode = mode;
			pr_debug("Forced mode: %s\n",
					mode_text[chip->force_mode]);
		}
	}

	return rc;
}

static int qpnp_typec_handle_usb_insertion(struct qpnp_typec_chip *chip, u8 reg)
{
	int rc;
	enum cc_line_state cc_line_state;

	cc_line_state = (reg & TYPEC_CCOUT_OPEN_BIT) ?
		OPEN : (reg & TYPEC_CCOUT_BIT) ? CC_2 : CC_1;
	rc = qpnp_typec_configure_ssmux(chip, cc_line_state);
	if (rc) {
		pr_err("failed to configure ss-mux rc=%d\n", rc);
		return rc;
	}

	chip->cc_line_state = cc_line_state;

	pr_debug("CC_line state = %d\n", cc_line_state);

	return 0;
}

static int qpnp_typec_handle_detach(struct qpnp_typec_chip *chip)
{
	int rc;

	rc = qpnp_typec_configure_ssmux(chip, OPEN);
	if (rc) {
		pr_err("failed to configure SSMUX rc=%d\n", rc);
		return rc;
	}

	chip->cc_line_state = OPEN;
	chip->current_ma = 0;
	chip->typec_state = POWER_SUPPLY_TYPE_UNKNOWN;
	chip->type_c_psy.type = POWER_SUPPLY_TYPE_UNKNOWN;
	rc = set_property_on_battery(chip, POWER_SUPPLY_PROP_TYPEC_MODE);
	if (rc)
		pr_err("failed to set TYPEC MODE on battery psy rc=%d\n", rc);

	pr_debug("CC_line state = %d current_ma = %d in_force_mode = %d\n",
			chip->cc_line_state, chip->current_ma,
			chip->in_force_mode);

	/* handle role reversal */
	if (chip->role_reversal_supported && !chip->in_force_mode) {
		rc = qpnp_typec_force_mode(chip, DUAL_ROLE_PROP_MODE_NONE);
		if (rc)
			pr_err("Failed to set DRP mode rc=%d\n", rc);
	}

	if (chip->dr_inst)
		dual_role_instance_changed(chip->dr_inst);

	return rc;
}

/* Interrupt handlers */
static irqreturn_t vrd_changed_handler(int irq, void *_chip)
{
	int rc, old_current;
	u8 reg;
	struct qpnp_typec_chip *chip = _chip;

	pr_debug("vrd changed triggered\n");

	mutex_lock(&chip->typec_lock);
	rc = qpnp_typec_read(chip, &reg, TYPEC_UFP_STATUS_REG(chip->base), 1);
	if (rc) {
		pr_err("failed to read status reg rc=%d\n", rc);
		goto out;
	}

	old_current = chip->current_ma;
	chip->current_ma = get_max_current(reg & TYPEC_CURRENT_MASK);

	/* only notify if current is valid and changed at runtime */
	if (chip->current_ma && (old_current != chip->current_ma)) {
		rc = set_property_on_battery(chip,
				POWER_SUPPLY_PROP_CURRENT_CAPABILITY);
		if (rc)
			pr_err("failed to set INPUT CURRENT MAX on battery psy rc=%d\n",
					rc);
	}

	pr_debug("UFP status reg = 0x%x old current = %dma new current = %dma\n",
			reg, old_current, chip->current_ma);

out:
	mutex_unlock(&chip->typec_lock);
	return IRQ_HANDLED;
}

static irqreturn_t vconn_oc_handler(int irq, void *_chip)
{
	pr_warn("vconn oc triggered\n");

	return IRQ_HANDLED;
}

static irqreturn_t ufp_detect_handler(int irq, void *_chip)
{
	int rc;
	u8 reg;
	struct qpnp_typec_chip *chip = _chip;
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct typec_device_info *di = &chip->di;
#endif

	pr_debug("ufp detect triggered\n");

	mutex_lock(&chip->typec_lock);
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	/*
	 * exception handling:
	 * if CC pin is not stable, the state transition may from
	 * AS DFP to AS UFP direct, VBUS should be turned off first
	 */
	if (di->sink_attached) {
		pr_err("remove Sink first\n");
		rc = qpnp_typec_handle_detach(chip);
		if (rc)
			pr_err("failed to handle DFP detach rc=%d\n", rc);
		di->reverse_state = 0;
		di->trysnk_attempt = false;
		di->sink_attached = false;
	}

	if (REVERSE_ATTEMPT == di->reverse_state) {
		pr_info("reversed success, Source and VBUS detected\n");
		di->reverse_state = REVERSE_COMPLETE;
	}

	if (di->trysnk_attempt) {
		pr_info("TrySNK success, Source and VBUS detected\n");
		typec_wake_unlock(di);
		di->trysnk_attempt = false;
		/* cancel watch dog work */
		cancel_delayed_work(&di->g_wdog_work);
	}

	/* notify the attached state */
	complete(&di->reverse_completion);
#endif

	rc = qpnp_typec_read(chip, &reg, TYPEC_UFP_STATUS_REG(chip->base), 1);
	if (rc) {
		pr_err("failed to read status reg rc=%d\n", rc);
		goto out;
	}

	rc = qpnp_typec_handle_usb_insertion(chip, reg);
	if (rc) {
		pr_err("failed to handle USB insertion rc=%d\n", rc);
		goto out;
	}

	chip->current_ma = get_max_current(reg & TYPEC_CURRENT_MASK);
	/* device in UFP state */
	chip->typec_state = POWER_SUPPLY_TYPE_UFP;
	chip->type_c_psy.type = POWER_SUPPLY_TYPE_UFP;
	rc = set_property_on_battery(chip, POWER_SUPPLY_PROP_TYPEC_MODE);
	if (rc)
		pr_err("failed to set TYPEC MODE on battery psy rc=%d\n", rc);

	if (chip->dr_inst)
		dual_role_instance_changed(chip->dr_inst);

	pr_debug("UFP status reg = 0x%x current = %dma\n",
			reg, chip->current_ma);

out:
	mutex_unlock(&chip->typec_lock);
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	/* notify userspace the state might have changed */
	if (di->dual_role)
		dual_role_instance_changed(di->dual_role);
#endif
	return IRQ_HANDLED;
}

static irqreturn_t ufp_detach_handler(int irq, void *_chip)
{
	int rc;
	struct qpnp_typec_chip *chip = _chip;
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct typec_device_info *di = &chip->di;
#endif

	pr_debug("ufp detach triggered\n");

	mutex_lock(&chip->typec_lock);
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	if (di->reverse_state == REVERSE_ATTEMPT) {
		pr_info("triggered by mode change, ignore\n");
	} else {
		di->reverse_state = 0;
		qpnp_typec_mode_drp(chip);
		rc = qpnp_typec_handle_detach(chip);
		if (rc)
			pr_err("failed to handle UFP detach rc=%d\n", rc);

		/* notify userspace the state might have changed */
		if (di->dual_role)
			dual_role_instance_changed(di->dual_role);
	}
#else
	rc = qpnp_typec_handle_detach(chip);
	if (rc)
		pr_err("failed to handle UFP detach rc=%d\n", rc);
#endif
	mutex_unlock(&chip->typec_lock);

	return IRQ_HANDLED;
}

#ifdef CONFIG_DUAL_ROLE_USB_INTF
static void typec_open_otg(struct qpnp_typec_chip *chip, u8 reg)
{
	int rc = 0;

#ifdef CONFIG_SWITCH_FSA9685
	/*
	 * ID pin of the switch chip is not used when OTG
	 * so switch the chip manually
	 */
	fsa9685_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
#endif

	rc = qpnp_typec_handle_usb_insertion(chip, reg);
	if (rc) {
		pr_err("failed to handle USB insertion rc=%d\n", rc);
	}

	chip->typec_state = POWER_SUPPLY_TYPE_DFP;
	chip->type_c_psy.type = POWER_SUPPLY_TYPE_DFP;
	chip->current_ma = 0;
	rc = set_property_on_battery(chip, POWER_SUPPLY_PROP_TYPEC_MODE);
	if (rc)
		pr_err("failed to set TYPEC MODE on battery psy rc=%d\n", rc);

}
#endif

static irqreturn_t dfp_detect_handler(int irq, void *_chip)
{
	int rc;
	u8 reg[2];
	struct qpnp_typec_chip *chip = _chip;
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct typec_device_info *di = &chip->di;
#endif

	pr_debug("dfp detect trigerred\n");

	mutex_lock(&chip->typec_lock);
	rc = qpnp_typec_read(chip, reg, TYPEC_UFP_STATUS_REG(chip->base), 2);
	if (rc) {
		pr_err("failed to read status reg rc=%d\n", rc);
		goto out;
	}

	if (reg[1] & VALID_DFP_MASK) {
#ifdef CONFIG_DUAL_ROLE_USB_INTF
		if (REVERSE_ATTEMPT == di->reverse_state) {
			pr_info("reversed success, Sink detected\n");
			di->reverse_state = REVERSE_COMPLETE;
			di->sink_attached = true;
			typec_open_otg(chip, reg[0]);

			/* notify the attached state */
			complete(&di->reverse_completion);

			/* notify userspace the state might have changed */
			if (di->dual_role)
				dual_role_instance_changed(di->dual_role);
		} else if (di->trysnk_attempt) {
			pr_info("TrySNK fail, Sink detected again\n");
			di->trysnk_attempt = false;
			di->sink_attached = true;
			typec_open_otg(chip, reg[0]);

			/* notify the attached state */
			complete(&di->reverse_completion);

			/* notify userspace the state might have changed */
			if (di->dual_role)
				dual_role_instance_changed(di->dual_role);
		} else {
			pr_info("Sink detected, perform TrySNK\n");
			di->trysnk_attempt = true;
			qpnp_typec_mode_ufp(chip);
			typec_wake_lock(di);
			schedule_delayed_work(&di->g_wdog_work,
					      msecs_to_jiffies
					      (TRYSNK_TIMEOUT_MS));
		}
#else
#ifdef CONFIG_SWITCH_FSA9685
		/*
		 * ID pin of the switch chip is not used when OTG
		 * so switch the chip manually
		 */
		fsa9685_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
#endif
		rc = qpnp_typec_handle_usb_insertion(chip, reg[0]);
		if (rc) {
			pr_err("failed to handle USB insertion rc=%d\n", rc);
			goto out;
		}

		chip->typec_state = POWER_SUPPLY_TYPE_DFP;
		chip->type_c_psy.type = POWER_SUPPLY_TYPE_DFP;
		chip->current_ma = 0;
		rc = set_property_on_battery(chip,
				POWER_SUPPLY_PROP_TYPEC_MODE);
		if (rc)
			pr_err("failed to set TYPEC MODE on battery psy rc=%d\n",
					rc);
#endif
	}

	if (chip->dr_inst)
		dual_role_instance_changed(chip->dr_inst);

	pr_debug("UFP status reg = 0x%x DFP status reg = 0x%x\n",
			reg[0], reg[1]);

out:
	mutex_unlock(&chip->typec_lock);
	return IRQ_HANDLED;
}

static irqreturn_t dfp_detach_handler(int irq, void *_chip)
{
	int rc;
	struct qpnp_typec_chip *chip = _chip;
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct typec_device_info *di = &chip->di;
#endif

	pr_debug("dfp detach triggered\n");

	mutex_lock(&chip->typec_lock);
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	if (di->trysnk_attempt || di->reverse_state == REVERSE_ATTEMPT) {
		pr_info("triggered by mode change, ignore\n");
	} else {
		/* clear flags and set the port mode to DRP when unattached */
		di->reverse_state = 0;
		di->sink_attached = false;
		qpnp_typec_mode_drp(chip);
		rc = qpnp_typec_handle_detach(chip);
		if (rc)
			pr_err("failed to handle DFP detach rc=%d\n", rc);

		if (di->dual_role)
			dual_role_instance_changed(di->dual_role);
	}
#else
	rc = qpnp_typec_handle_detach(chip);
	if (rc)
		pr_err("failed to handle DFP detach rc=%d\n", rc);
#endif
	mutex_unlock(&chip->typec_lock);

	return IRQ_HANDLED;
}

static irqreturn_t vbus_err_handler(int irq, void *_chip)
{
	int rc;
	struct qpnp_typec_chip *chip = _chip;

	pr_debug("vbus_err triggered\n");

	mutex_lock(&chip->typec_lock);
	rc = qpnp_typec_handle_detach(chip);
	if (rc)
		pr_err("failed to handle VBUS_ERR rc==%d\n", rc);

	mutex_unlock(&chip->typec_lock);

	return IRQ_HANDLED;
}

static int qpnp_typec_parse_dt(struct qpnp_typec_chip *chip)
{
	int rc;
	struct device_node *node = chip->dev->of_node;

	/* SS-Mux configuration gpio */
	if (of_find_property(node, "qcom,ssmux-gpio", NULL)) {
		chip->ssmux_gpio = of_get_named_gpio_flags(node,
				"qcom,ssmux-gpio", 0, &chip->gpio_flag);
		if (!gpio_is_valid(chip->ssmux_gpio)) {
			if (chip->ssmux_gpio != -EPROBE_DEFER)
				pr_err("failed to get ss-mux config gpio=%d\n",
						chip->ssmux_gpio);
			return chip->ssmux_gpio;
		}

		rc = devm_gpio_request(chip->dev, chip->ssmux_gpio,
				"typec_mux_config_gpio");
		if (rc) {
			pr_err("failed to request ss-mux gpio rc=%d\n", rc);
			return rc;
		}
	}

	/* SS-Mux regulator */
	if (of_find_property(node, "ss-mux-supply", NULL)) {
		chip->ss_mux_vreg = devm_regulator_get(chip->dev, "ss-mux");
		if (IS_ERR(chip->ss_mux_vreg))
			return PTR_ERR(chip->ss_mux_vreg);
	}

	chip->role_reversal_supported = of_property_read_bool(node,
					"qcom,role-reversal-supported");
	return 0;
}

static int qpnp_typec_determine_initial_status(struct qpnp_typec_chip *chip)
{
	int rc;
	u8 rt_reg;

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	qpnp_typec_mode_drp(chip);
#endif
        rc = qpnp_typec_masked_write(chip,TYPEC_INT_EN_CLR(chip->base),VBUS_ERR_CLEAR , VBUS_ERR_CLEAR );
        if (rc) {
                pr_err("failed to disable vbus-err interrupt rc=%d\n", rc);
        }

	rc = qpnp_typec_read(chip, &rt_reg, INT_RT_STS_REG(chip->base), 1);
	if (rc) {
		pr_err("failed to read RT status reg rc=%d\n", rc);
		return rc;
	}
	pr_debug("RT status reg = 0x%x\n", rt_reg);

	chip->cc_line_state = OPEN;
	chip->typec_state = POWER_SUPPLY_TYPE_UNKNOWN;
	chip->type_c_psy.type = POWER_SUPPLY_TYPE_UNKNOWN;

	if (rt_reg & DFP_DETECT_BIT) {
		/* we are in DFP state*/
		dfp_detect_handler(0, chip);
	} else if (rt_reg & UFP_DETECT_BIT) {
		/* we are in UFP state */
		ufp_detect_handler(0, chip);
	}

	return 0;
}

#define REQUEST_IRQ(chip, irq, irq_name, irq_handler, flags, wake, rc)	\
do {									\
	irq = spmi_get_irq_byname(chip->spmi, NULL, irq_name);		\
	if (irq < 0) {							\
		pr_err("Unable to get " irq_name " irq\n");		\
		rc |= -ENXIO;						\
	}								\
	rc = devm_request_threaded_irq(chip->dev,			\
			irq, NULL, irq_handler, flags, irq_name,	\
			chip);						\
	if (rc < 0) {							\
		pr_err("Unable to request " irq_name " irq: %d\n", rc);	\
		rc |= -ENXIO;						\
	}								\
									\
	if (wake)							\
		enable_irq_wake(irq);					\
} while (0)

static int qpnp_typec_request_irqs(struct qpnp_typec_chip *chip)
{
	int rc = 0;
	unsigned long flags = IRQF_TRIGGER_RISING | IRQF_ONESHOT;

	REQUEST_IRQ(chip, chip->vrd_changed, "vrd-change", vrd_changed_handler,
			flags, true, rc);
	REQUEST_IRQ(chip, chip->ufp_detach, "ufp-detach", ufp_detach_handler,
			flags, true, rc);
	REQUEST_IRQ(chip, chip->ufp_detect, "ufp-detect", ufp_detect_handler,
			flags, true, rc);
	REQUEST_IRQ(chip, chip->dfp_detach, "dfp-detach", dfp_detach_handler,
			flags, true, rc);
	REQUEST_IRQ(chip, chip->dfp_detect, "dfp-detect", dfp_detect_handler,
			flags, true, rc);
        //REQUEST_IRQ(chip, chip->vbus_err, "vbus-err", vbus_err_handler,
	//		flags, true, rc);
	REQUEST_IRQ(chip, chip->vconn_oc, "vconn-oc", vconn_oc_handler,
			flags, true, rc);

	return rc;
}

static enum power_supply_property qpnp_typec_properties[] = {
	POWER_SUPPLY_PROP_CURRENT_CAPABILITY,
	POWER_SUPPLY_PROP_TYPE,
};

static int qpnp_typec_get_property(struct power_supply *psy,
				enum power_supply_property prop,
				union power_supply_propval *val)
{
	struct qpnp_typec_chip *chip = container_of(psy,
					struct qpnp_typec_chip, type_c_psy);

	switch (prop) {
	case POWER_SUPPLY_PROP_TYPE:
		val->intval = chip->typec_state;
		break;
	case POWER_SUPPLY_PROP_CURRENT_CAPABILITY:
		val->intval = chip->current_ma;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#define ROLE_REVERSAL_DELAY_MS		500
static void qpnp_typec_role_check_work(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct qpnp_typec_chip *chip = container_of(dwork,
				struct qpnp_typec_chip, role_reversal_check);
	int rc;

	mutex_lock(&chip->typec_lock);
	switch (chip->force_mode) {
	case DUAL_ROLE_PROP_MODE_UFP:
		if (chip->typec_state != POWER_SUPPLY_TYPE_UFP) {
			pr_debug("Role-reversal not latched to UFP in %d msec resetting to DRP mode\n",
					ROLE_REVERSAL_DELAY_MS);
			rc = qpnp_typec_force_mode(chip,
						DUAL_ROLE_PROP_MODE_NONE);
			if (rc)
				pr_err("Failed to set DRP mode rc=%d\n", rc);
		}
		chip->in_force_mode = false;
		break;
	case DUAL_ROLE_PROP_MODE_DFP:
		if (chip->typec_state != POWER_SUPPLY_TYPE_DFP) {
			pr_debug("Role-reversal not latched to DFP in %d msec resetting to DRP mode\n",
					ROLE_REVERSAL_DELAY_MS);
			rc = qpnp_typec_force_mode(chip,
						DUAL_ROLE_PROP_MODE_NONE);
			if (rc)
				pr_err("Failed to set DRP mode rc=%d\n", rc);
		}
		chip->in_force_mode = false;
		break;
	default:
		pr_debug("Already in DRP mode\n");
		break;
	}
	mutex_unlock(&chip->typec_lock);
	typec_relax(&chip->role_reversal_wakeup_source);
}

enum dual_role_property qpnp_typec_dr_properties[] = {
	DUAL_ROLE_PROP_SUPPORTED_MODES,
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
};

static int qpnp_typec_dr_is_writeable(struct dual_role_phy_instance *dual_role,
					enum dual_role_property prop)
{
	int rc;

	switch (prop) {
	case DUAL_ROLE_PROP_MODE:
		rc = 1;
		break;
	default:
		rc = 0;
	}
	return rc;
}

static int qpnp_typec_dr_set_property(struct dual_role_phy_instance *dual_role,
					enum dual_role_property prop,
					const unsigned int *val)
{
	int rc = 0;
	struct qpnp_typec_chip *chip = dual_role_get_drvdata(dual_role);

	if (!chip || (chip->typec_state == POWER_SUPPLY_TYPE_UNKNOWN))
		return -EINVAL;

	switch (prop) {
	case DUAL_ROLE_PROP_MODE:
		/* Force role */
		mutex_lock(&chip->typec_lock);
		if (chip->in_force_mode) {
			pr_debug("Already in mode transition skipping request\n");
			mutex_unlock(&chip->typec_lock);
			return 0;
		}
		switch (*val) {
		case DUAL_ROLE_PROP_MODE_UFP:
			rc = qpnp_typec_force_mode(chip,
						DUAL_ROLE_PROP_MODE_UFP);
			if (rc)
				pr_err("Failed to force UFP mode rc=%d\n", rc);
			else
				chip->in_force_mode = true;
			break;
		case DUAL_ROLE_PROP_MODE_DFP:
			rc = qpnp_typec_force_mode(chip,
						DUAL_ROLE_PROP_MODE_DFP);
			if (rc)
				pr_err("Failed to force DFP mode rc=%d\n", rc);
			else
				chip->in_force_mode = true;
			break;
		default:
			pr_debug("Invalid role(not DFP/UFP) %d\n", *val);
			rc = -EINVAL;
		}
		mutex_unlock(&chip->typec_lock);

		/*
		 * schedule delayed work to check if device latched to the
		 * requested mode.
		 */
		if (!rc && chip->in_force_mode) {
			cancel_delayed_work_sync(&chip->role_reversal_check);
			typec_stay_awake(&chip->role_reversal_wakeup_source);
			schedule_delayed_work(&chip->role_reversal_check,
				msecs_to_jiffies(ROLE_REVERSAL_DELAY_MS));
		}
		break;
	default:
		pr_debug("Invalid DUAL ROLE request %d\n", prop);
		rc = -EINVAL;
	}

	return rc;
}

static int qpnp_typec_dr_get_property(struct dual_role_phy_instance *dual_role,
					enum dual_role_property prop,
					unsigned int *val)
{
	struct qpnp_typec_chip *chip = dual_role_get_drvdata(dual_role);
	unsigned int mode, power_role, data_role;

	if (!chip)
		return -EINVAL;

	switch (chip->typec_state) {
	case POWER_SUPPLY_TYPE_UFP:
		mode = DUAL_ROLE_PROP_MODE_UFP;
		power_role = DUAL_ROLE_PROP_PR_SNK;
		data_role = DUAL_ROLE_PROP_DR_DEVICE;
		break;
	case POWER_SUPPLY_TYPE_DFP:
		mode = DUAL_ROLE_PROP_MODE_DFP;
		power_role = DUAL_ROLE_PROP_PR_SRC;
		data_role = DUAL_ROLE_PROP_DR_HOST;
		break;
	default:
		mode = DUAL_ROLE_PROP_MODE_NONE;
		power_role = DUAL_ROLE_PROP_PR_NONE;
		data_role = DUAL_ROLE_PROP_DR_NONE;
	};

	switch (prop) {
	case DUAL_ROLE_PROP_SUPPORTED_MODES:
		*val = chip->dr_desc.supported_modes;
		break;
	case DUAL_ROLE_PROP_MODE:
		*val = mode;
		break;
	case DUAL_ROLE_PROP_PR:
		*val = power_role;
		break;
	case DUAL_ROLE_PROP_DR:
		*val = data_role;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int qpnp_typec_probe(struct spmi_device *spmi)
{
	int rc;
	struct resource *resource;
	struct qpnp_typec_chip *chip;
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	struct typec_device_info *di = NULL;
	struct dual_role_phy_desc *desc = NULL;
	struct dual_role_phy_instance *dual_role = NULL;
#endif

	resource = spmi_get_resource(spmi, NULL, IORESOURCE_MEM, 0);
	if (!resource) {
		pr_err("Unable to get spmi resource for TYPEC\n");
		return -EINVAL;
	}

	chip = devm_kzalloc(&spmi->dev, sizeof(struct qpnp_typec_chip),
			GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->dev = &spmi->dev;
	chip->spmi = spmi;

	/* parse DT */
	rc = qpnp_typec_parse_dt(chip);
	if (rc) {
		pr_err("failed to parse DT rc=%d\n", rc);
		return rc;
	}

	chip->base = resource->start;
	dev_set_drvdata(&spmi->dev, chip);
	device_init_wakeup(&spmi->dev, 1);
	mutex_init(&chip->typec_lock);
	spin_lock_init(&chip->rw_lock);

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	di = &chip->di;
	INIT_DELAYED_WORK(&di->g_wdog_work, typec_wdog_work);
	init_completion(&di->reverse_completion);

	desc =
	    devm_kzalloc(&spmi->dev,
			 sizeof(struct dual_role_phy_desc), GFP_KERNEL);
	if (!desc) {
		pr_err("unable to allocate dual role descriptor\n");
		return -ENOMEM;
	}

	wake_lock_init(&di->wake_lock, WAKE_LOCK_SUSPEND, "typec_wake_lock");

	desc->name = "otg_default";
	desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	desc->get_property = dual_role_get_local_prop;
	desc->set_property = dual_role_set_prop;
	desc->properties = fusb_drp_properties;
	desc->num_properties = ARRAY_SIZE(fusb_drp_properties);
	desc->property_is_writeable = dual_role_is_writeable;
	dual_role = devm_dual_role_instance_register(&spmi->dev, desc);
	dual_role->drv_data = di;
	di->dual_role = dual_role;
	di->desc = desc;
#endif

	/* determine initial status */
	rc = qpnp_typec_determine_initial_status(chip);
	if (rc) {
		pr_err("failed to determine initial state rc=%d\n", rc);
		goto out;
	}

	chip->type_c_psy.name		= TYPEC_PSY_NAME;
	chip->type_c_psy.get_property	= qpnp_typec_get_property;
	chip->type_c_psy.properties	= qpnp_typec_properties;
	chip->type_c_psy.num_properties	= ARRAY_SIZE(qpnp_typec_properties);

	rc = power_supply_register(chip->dev, &chip->type_c_psy);
	if (rc < 0) {
		pr_err("Unable to register  type_c_psy rc=%d\n", rc);
		goto out;
	}

	if (chip->role_reversal_supported) {
		chip->force_mode = DUAL_ROLE_PROP_MODE_NONE;
		wakeup_source_init(&chip->role_reversal_wakeup_source.source,
					"typec_role_reversal_wake");
		INIT_DELAYED_WORK(&chip->role_reversal_check,
					qpnp_typec_role_check_work);
		/* Register for android TypeC dual role framework */
		chip->dr_desc.name		= DUAL_ROLE_DESC_NAME;
		chip->dr_desc.properties	= qpnp_typec_dr_properties;
		chip->dr_desc.get_property	= qpnp_typec_dr_get_property;
		chip->dr_desc.set_property	= qpnp_typec_dr_set_property;
		chip->dr_desc.property_is_writeable =
					qpnp_typec_dr_is_writeable;
		chip->dr_desc.supported_modes	=
					DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
		chip->dr_desc.num_properties	=
					ARRAY_SIZE(qpnp_typec_dr_properties);

		chip->dr_inst = devm_dual_role_instance_register(chip->dev,
					&chip->dr_desc);
		if (IS_ERR(chip->dr_inst)) {
			pr_err("Failed to initialize dual role\n");
			rc = PTR_ERR(chip->dr_inst);
			goto unregister_psy;
		} else {
			chip->dr_inst->drv_data = chip;
		}
	}

	/* All irqs */
	rc = qpnp_typec_request_irqs(chip);
	if (rc) {
		pr_err("failed to request irqs rc=%d\n", rc);
		goto unregister_psy;
	}
#ifdef CONFIG_HUAWEI_TYPEC
	rc = typec_chip_register(&qpnp_typec_ops);
	if (rc) {
		pr_err("failed to register typec chip\n");
	}
	g_chip = chip;
	qpnp_create_sysfs();

	/*set Rp connect threshold to 0.6V */
	qpnp_typec_otg_threshold(chip, THRESHOLD_0P6);
#endif
	pr_info("TypeC successfully probed state=%d CC-line-state=%d\n",
			chip->typec_state, chip->cc_line_state);
	return 0;

unregister_psy:
	power_supply_unregister(&chip->type_c_psy);
out:
	mutex_destroy(&chip->typec_lock);
	if (chip->role_reversal_supported)
		wakeup_source_trash(&chip->role_reversal_wakeup_source.source);
	return rc;
}

static int qpnp_typec_remove(struct spmi_device *spmi)
{
	int rc;
	struct qpnp_typec_chip *chip = dev_get_drvdata(&spmi->dev);

	if (chip->role_reversal_supported) {
		cancel_delayed_work_sync(&chip->role_reversal_check);
		wakeup_source_trash(&chip->role_reversal_wakeup_source.source);
	}
	rc = qpnp_typec_configure_ssmux(chip, OPEN);
	if (rc)
		pr_err("failed to configure SSMUX rc=%d\n", rc);

	mutex_destroy(&chip->typec_lock);
	dev_set_drvdata(chip->dev, NULL);

	return 0;
}

#ifdef CONFIG_DUAL_ROLE_USB_INTF
static void qpnp_typec_shutdown(struct spmi_device *spmi)
{
	struct qpnp_typec_chip *chip = dev_get_drvdata(&spmi->dev);

	qpnp_typec_mode_drp(chip);
}
#endif

static struct of_device_id qpnp_typec_match_table[] = {
	{ .compatible = QPNP_TYPEC_DEV_NAME, },
	{}
};

static struct spmi_driver qpnp_typec_driver = {
	.probe		= qpnp_typec_probe,
	.remove		= qpnp_typec_remove,
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	.shutdown	= qpnp_typec_shutdown,
#endif
	.driver		= {
		.name		= QPNP_TYPEC_DEV_NAME,
		.owner		= THIS_MODULE,
		.of_match_table	= qpnp_typec_match_table,
	},
};

/*
 * qpnp_typec_init() - register spmi driver for qpnp-typec
 */
static int __init qpnp_typec_init(void)
{
	return spmi_driver_register(&qpnp_typec_driver);
}
module_init(qpnp_typec_init);

static void __exit qpnp_typec_exit(void)
{
	spmi_driver_unregister(&qpnp_typec_driver);
}
module_exit(qpnp_typec_exit);

MODULE_DESCRIPTION("QPNP type-C driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:" QPNP_TYPEC_DEV_NAME);
