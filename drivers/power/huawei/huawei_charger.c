/*
 * drivers/power/huawei_charger.c
 *
 *huawei charger driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wakelock.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/spmi.h>
#include <linux/sysfs.h>
#include <linux/power/huawei_charger.h>

#include <dsm/dsm_pub.h>

#define DEFAULT_FCP_TEST_DELAY	6000
#define DEFAULT_IIN_CURRENT	1000
#define MAX_CURRENT	3000
#define MIN_CURRENT	100

static struct dsm_client *dsm_chargemonitor_dclient = NULL;
static struct dsm_dev dsm_charge_monitor =
{
	.name = "dsm_charge_monitor",
	.fops = NULL,
	.buff_size = 4096,
};

struct class *power_class = NULL;
struct device *charge_dev = NULL;
struct charge_device_info *g_charger_device_para = NULL;

static int set_property_on_psy(struct power_supply *psy,
		enum power_supply_property prop, int val)
{
	int rc;
	union power_supply_propval ret = {0, };

	ret.intval = val;
	rc = psy->set_property(psy, prop, &ret);
	if (rc) {
		pr_err("psy does not allow set prop %d rc = %d\n",
			prop, rc);
	}
	return rc;
}

static int get_property_from_psy(struct power_supply *psy,
		enum power_supply_property prop)
{
	int rc;
	int val = 0;
	union power_supply_propval ret = {0, };

	rc = psy->get_property(psy, prop, &ret);
	if (rc) {
		pr_err("psy doesn't support reading prop %d rc = %d\n",
				prop, rc);
		return rc;
	}
	val = ret.intval;
	return val;
}

void get_log_info_from_psy(struct power_supply *psy,
			   enum power_supply_property prop, char *buf)
{
	int rc;
	union power_supply_propval val = {0, };

	val.strval = buf;
	rc = psy->get_property(psy, prop, &val);
	if (rc) {
		pr_err("psy does not allow get prop %d rc = %d\n", prop, rc);
	}
}

static struct kobject *g_sysfs_poll = NULL;

static ssize_t get_poll_charge_start_event(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct charge_device_info *chip = g_charger_device_para;

	if (chip) {
		return snprintf(buf, PAGE_SIZE, "%d\n", chip->input_event);
	} else {
		return 0;
	}
}

static ssize_t set_poll_charge_event(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct charge_device_info *chip = g_charger_device_para;
	long val = 0;

	if (chip) {
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 3000)) {
			return -EINVAL;
		}
		chip->input_event = val;
		sysfs_notify(g_sysfs_poll, NULL, "poll_charge_start_event");
	}
	return count;
}

static DEVICE_ATTR(poll_charge_start_event, (S_IWUSR | S_IRUGO),
				get_poll_charge_start_event,
				set_poll_charge_event);
static ssize_t get_poll_charge_done_event(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return 0;
}

static DEVICE_ATTR(poll_charge_done_event, (S_IWUSR | S_IRUGO),
		   get_poll_charge_done_event, NULL);

static int charge_event_poll_register(struct device *dev)
{
	int ret;

	ret = sysfs_create_file(&dev->kobj, &dev_attr_poll_charge_start_event.attr);
	if (ret) {
		pr_err("fail to create poll node for %s\n", dev->kobj.name);
		return ret;
	}
	ret = sysfs_create_file(&dev->kobj, &dev_attr_poll_charge_done_event.attr);
	if (ret) {
		pr_err("fail to create poll node for %s\n", dev->kobj.name);
		return ret;
	}
	g_sysfs_poll = &dev->kobj;
	return ret;
}

void cap_learning_event_done_notify(void)
{
	struct charge_device_info *chip = g_charger_device_para;

	if (!chip) {
		pr_info("smb device is not init, do nothing!\n");
		return;
	}

	pr_info("fg cap learning event notify!\n");
	if (g_sysfs_poll) {
		sysfs_notify(g_sysfs_poll, NULL, "poll_charge_done_event");
	}
}

static void charge_event_notify(unsigned int event)
{
	struct charge_device_info *chip = g_charger_device_para;

	if (!chip) {
		pr_info("smb device is not init, do nothing!\n");
		return;
	}
	/* avoid notify charge stop event continuously without charger inserted */
	if ((chip->input_event != event) || (event == SMB_START_CHARGING)) {
		chip->input_event = event;
		if (g_sysfs_poll) {
			sysfs_notify(g_sysfs_poll, NULL, "poll_charge_start_event");
		}
	}
}

static void smb_update_status(struct charge_device_info *di)
{
	unsigned int events = 0;
	int charging_enabled = 0;
	int battery_present = 0;

	charging_enabled = get_property_from_psy(di->batt_psy,
					POWER_SUPPLY_PROP_CHARGING_ENABLED);
	battery_present = get_property_from_psy(di->batt_psy,
					POWER_SUPPLY_PROP_PRESENT);
	if (!battery_present) {
		events = SMB_STOP_CHARGING;
	}
	if (!events) {
		if (charging_enabled && battery_present) {
			events = SMB_START_CHARGING;
		}
	}
	charge_event_notify(events);
}

static void smb_charger_work(struct work_struct *work)
{
	struct charge_device_info *chip = container_of(work,
				struct charge_device_info, smb_charger_work.work);

	smb_update_status(chip);
	schedule_delayed_work(&chip->smb_charger_work,
				msecs_to_jiffies(QPNP_SMBCHARGER_TIMEOUT));
}

int huawei_handle_charger_event(void)
{
	struct charge_device_info *di = NULL;
	static int smb_charge_work_flag = 1;
	int usb_present = 0;

	di = g_charger_device_para;
	if(NULL == di) {
		pr_err(" %s charge ic  is unregister !\n", __func__);
		return -1 ;
	}
	usb_present = get_property_from_psy(di->usb_psy, POWER_SUPPLY_PROP_ONLINE);
	if (usb_present && smb_charge_work_flag) {
		charge_event_notify(SMB_START_CHARGING);
		schedule_delayed_work(&di->smb_charger_work, msecs_to_jiffies(0));
		smb_charge_work_flag = 0;
	}
	if (!usb_present) {
		charge_event_notify(SMB_STOP_CHARGING);
		cancel_delayed_work_sync(&di->smb_charger_work);
		smb_charge_work_flag = 1;
	}
	return 0;
}

static int check_current_voltage_temp(int in_cur, int in_vol, int in_temp, int *check_result)
{
	int check_is_ok = 1;

	if ((CHARGE_OCP_THR > in_cur) || (BATTERY_OCP_THR < in_cur)) {
		*check_result |= OCP_ABNORML;
		check_is_ok = 0;
		pr_info("Find OCP! current_ma is %d\n", in_cur);
	}

	if ((BATTERY_VOL_THR_HI < in_vol) || (BATTERY_VOL_THR_LO > in_vol)) {
		*check_result |= BATTERY_VOL_ABNORML;
		check_is_ok = 0;
		pr_info("Battery voltage is abnormal! voltage is %d\n", in_vol);
	}

	if ((BATTERY_TEMP_HI < in_temp) || (BATTERY_TEMP_LO > in_temp)) {
		*check_result |= BATTERY_TEMP_ABNORML;
		check_is_ok = 0;
		pr_info("Battery temperature is abnormal! temp is %d\n", in_temp);
	}

	if (!check_is_ok) {
		*check_result |= CHARGE_STATUS_FAIL;
		pr_info("running test find abnormal battery status, the result is 0x%x\n",
                        *check_result);
		return 0;
	}
	return 1;
}

static int charging_limit_by_temp(struct charge_device_info *di, int in_temp,
							int in_vol, int *in_status)
{
	int check_result = 0;
	int health = 0;

	health = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_HEALTH);
	if (((POWER_SUPPLY_HEALTH_OVERHEAT == health) && (HOT_TEMP_THR <= in_temp))
			|| (POWER_SUPPLY_HEALTH_COLD == health)) {
		check_result |= BATTERY_HEALTH;
		pr_info("battery is not health,health = %d\n", health);
	}
	if ((in_vol >= (WARM_VOL_THR -WARM_VOL_BUFFER)*1000)
			&& (WARM_TEMP_THR <= in_temp)) {
		check_result |= CHARGE_LIMIT;
		pr_info("charging is limit because battery warm, temp=%d\n", in_temp);
	}

	if ((*in_status == POWER_SUPPLY_STATUS_DISCHARGING)
			|| (*in_status == POWER_SUPPLY_STATUS_NOT_CHARGING)) {
		if (((in_temp >= (HOT_TEMP - COLD_HOT_TEMP_BUFFER))
				&& (HOT_TEMP_THR > in_temp))
				|| (in_temp <= (COLD_TEMP + COLD_HOT_TEMP_BUFFER))) {
			check_result |= CHARGE_LIMIT;
			pr_info("charging is limit, temp=%d\n", in_temp);
		}
	}

	return check_result;
}

/*if battery and usb is prasent return 0*/
static int check_batt_usb_present(struct charge_device_info *di)
{
	int check_result = 0;
	int battery_present = 0, usb_present = 0;

	battery_present = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_PRESENT);
	usb_present = get_property_from_psy(di->usb_psy, POWER_SUPPLY_PROP_ONLINE);

	/* usb/charger is absent*/
	if (!usb_present) {
		check_result |= USB_NOT_PRESENT;
		pr_err("not charging because usb/charger is absent\n");
	}
	/* battery is absent */
	if (!battery_present) {
		check_result |= BATTERY_ABSENT;
		pr_err("not charging because battery is absent\n");
	}
	return check_result;
}

static int check_battery_is_full(struct charge_device_info *di, int *in_status)
{
	int capacity = 0;
	int check_result = 0;

	capacity = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_CAPACITY);

	if ((*in_status ==POWER_SUPPLY_STATUS_DISCHARGING)
				&& (BATT_FULL == capacity) && (!check_batt_usb_present(di))) {
		*in_status = POWER_SUPPLY_STATUS_FULL;
	}

	if (*in_status == POWER_SUPPLY_STATUS_FULL) {
		check_result |= BATTERY_FULL;
	}
	return check_result;
}
/*
 * For running test apk to get the running test result and status
 * a int value, we use bit0 to bit10 to tell running test apk the test
 * result and status, if bit0 is 0, the result is fail, bit5 to bit10 is the
 * failed reason
 * if bit0 is 1, the result is pass.
 */
static int get_running_test_result(struct charge_device_info *di)
{
	int result = 0;
	int cur_status = 0;
	int is_temp_vol_current_ok = 1;
	int vol = 0, temp = 250, current_ma =0;

	if ((NULL == di) || (NULL == di->batt_psy) || (NULL == di->usb_psy)) {
		pr_err("charge and usb device is not ready!\n");
		return -1;
	}

#ifdef CONFIG_HLTHERM_RUNTEST
	pr_info("Open HL therm test ,runningtest return success.\n");
	return 1;
#endif
	current_ma = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_CURRENT_NOW);
	vol = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW);
	temp = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_TEMP);

	is_temp_vol_current_ok = check_current_voltage_temp(current_ma, vol, temp, &result);
	if (!is_temp_vol_current_ok) {
		return result;
	}

	cur_status = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_STATUS);
	if (cur_status == di->running_test_settled_status) {
		result |= CHARGE_STATUS_PASS;
		return result;
	} else if ((POWER_SUPPLY_STATUS_CHARGING == cur_status)
 			&& (POWER_SUPPLY_STATUS_DISCHARGING == di->running_test_settled_status)) {
		result |= CHARGE_STATUS_FAIL;
		pr_info("when set discharging but charging, test failed! result is 0x%x\n", result);
		return result;
	} else if (POWER_SUPPLY_STATUS_CHARGING == di->running_test_settled_status) {
		/* charge ic is in boost mode*/
		if (get_property_from_psy(di->usb_psy,
				POWER_SUPPLY_PROP_USB_OTG)) {
			result |= REGULATOR_BOOST;
		}
		/* OV condition is detected */
		if ((get_property_from_psy(di->batt_psy,
				POWER_SUPPLY_PROP_CHARGER_OVP))) {
			result |= CHARGER_OVP;
		}
		result |= charging_limit_by_temp(di, temp, vol, &cur_status);
		result |= check_batt_usb_present(di);
		result |= check_battery_is_full(di, &cur_status);

		if ((result & PASS_MASK) && (!(result & FAIL_MASK))) {
			result |= CHARGE_STATUS_PASS;
		} else {
			result |= CHARGE_STATUS_FAIL;
			pr_info("get_running_test_result: batt_volt=%d batt_temp=%d cur_status=%d"
				"current_ma=%d setting status=%d result=0x%x\n", vol, temp,
				cur_status, current_ma, di->running_test_settled_status, result);
		}
		return result;
	} else {
	/*
	 * if the setting status is discharging, meanwhile current status is not charging
	 * and discharging. We return 1(PASS) directly,
	 * do not to care high temperature, battery full or unknow
	 */
		pr_info("batt_volt=%d batt_temp=%d cur_status=%d current_ma=%d"
				" setting status=%d\n", vol, temp, cur_status, current_ma,
				di->running_test_settled_status);
		return 1;
	}
}

static int set_running_test_flag(struct charge_device_info *di, int value)
{
	if (NULL == di) {
		pr_err("charge_device is not ready! cannot set runningtest flag\n");
		return -1;
	}
	if (value) {
		di->running_test_settled_status = POWER_SUPPLY_STATUS_CHARGING;
	} else {
		di->running_test_settled_status = POWER_SUPPLY_STATUS_DISCHARGING;
	}
	return 0;
}

static int huawei_charger_set_runningtest(struct charge_device_info *di, int val)
{
	int iin_rt = 0;

	if ((di == NULL)||(di->batt_psy == NULL)) {
		pr_err("charge_device is not ready! cannot set runningtest\n");
		return -1;
	}

	set_property_on_psy(di->batt_psy, POWER_SUPPLY_PROP_CHARGING_ENABLED, !!val);
	set_running_test_flag(di, val);
	iin_rt = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_CHARGING_ENABLED);
	di->sysfs_data.iin_rt = iin_rt;

	return 0 ;
}
static int huawei_charger_factory_diag_charge(struct charge_device_info *di, int val)
{
	if ((di == NULL) || (di->batt_psy== NULL)) {
		pr_err("charge_device is not ready! cannot set factory diag\n");
		return -1;
	}

	set_property_on_psy(di->batt_psy, POWER_SUPPLY_PROP_BATTERY_CHARGING_ENABLED, !!val);
	di->factory_diag = get_property_from_psy(di->batt_psy,
					POWER_SUPPLY_PROP_BATTERY_CHARGING_ENABLED);
	return 0;
}

static int huawei_charger_enable_charge(struct charge_device_info *di, int val)
{
	if ((di == NULL)||(di->batt_psy== NULL)) {
		pr_err("charge_device is not ready! cannot enable charge\n");
		return -1;
	}

	set_property_on_psy(di->batt_psy, POWER_SUPPLY_PROP_CHARGING_ENABLED, !!val);
	di->chrg_config = get_property_from_psy(di->batt_psy,
					POWER_SUPPLY_PROP_CHARGING_ENABLED);
	return 0;
}

static int huawei_charger_set_in_thermal(struct charge_device_info *di, int val)
{
	if (di == NULL) {
		pr_err("charge_device is not ready! cannot get in thermal\n");
		return -1;
	}

	set_property_on_psy(di->batt_psy, POWER_SUPPLY_PROP_IN_THERMAL, val);
	di->sysfs_data.iin_thl = get_property_from_psy(di->batt_psy,
					POWER_SUPPLY_PROP_IN_THERMAL);

	return 0;
}

static int huawei_charger_set_rt_current(struct charge_device_info *di, int val)
{
    int iin_rt_curr = 0;
    int ret = 0;

    if ((di == NULL)||(di->batt_psy == NULL)) {
        pr_err("charge_device is not ready! cannot set runningtest\n");
        return -1;
    }

    if (0 == val)
        iin_rt_curr = get_property_from_psy(di->batt_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_MAX);
    else if ((val <= MIN_CURRENT)&&(val > 0))
         iin_rt_curr = MIN_CURRENT;
    else
        iin_rt_curr = val;

    set_property_on_psy(di->batt_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_MAX, iin_rt_curr);

    di->sysfs_data.iin_rt_curr= iin_rt_curr;
    return 0 ;
}

static int huawei_charger_set_hz_mode(struct charge_device_info *di, int val)
{
    int hiz_mode = 0;

    if ((di == NULL)||(di->batt_psy == NULL)) {
        pr_err("charge_device is not ready! cannot set runningtest\n");
        return -1;
    }
    hiz_mode = !!val;
    set_property_on_psy(di->batt_psy, POWER_SUPPLY_PROP_HIZ_MODE,hiz_mode);
    di->sysfs_data.hiz_mode = hiz_mode;
    return 0 ;
}

#define CHARGE_SYSFS_FIELD(_name, n, m, store)	\
{	\
	.attr = __ATTR(_name, m, charge_sysfs_show, store),	\
	.name = CHARGE_SYSFS_##n,	\
}

#define CHARGE_SYSFS_FIELD_RW(_name, n)	\
	CHARGE_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO,	\
		charge_sysfs_store)

#define CHARGE_SYSFS_FIELD_RO(_name, n)	\
	CHARGE_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t charge_sysfs_show(struct device *dev,
 				 struct device_attribute *attr, char *buf);
static ssize_t charge_sysfs_store(struct device *dev,
				  struct device_attribute *attr, const char *buf, size_t count);

struct charge_sysfs_field_info
{
	char name;
	struct device_attribute    attr;
};

enum fcp_test_result {
	FCP_TEST_SUCC,
	FCP_TEST_FAIL,
	FCP_TEST_NOT_SUPPORT,
};

static struct charge_sysfs_field_info charge_sysfs_field_tbl[] =
{
	CHARGE_SYSFS_FIELD_RW(iin_thermal,       IIN_THERMAL),
	CHARGE_SYSFS_FIELD_RW(iin_runningtest,    IIN_RUNNINGTEST),
	CHARGE_SYSFS_FIELD_RW(iin_rt_current,   IIN_RT_CURRENT),
	CHARGE_SYSFS_FIELD_RW(enable_hiz, HIZ),
	CHARGE_SYSFS_FIELD_RW(enable_charger,    ENABLE_CHARGER),
	CHARGE_SYSFS_FIELD_RW(factory_diag,    FACTORY_DIAG_CHARGER),
	CHARGE_SYSFS_FIELD_RO(running_test_status,    RUNNING_TEST_STATUS),
	CHARGE_SYSFS_FIELD_RO(chargelog_head,    CHARGELOG_HEAD),
	CHARGE_SYSFS_FIELD_RO(chargelog,    CHARGELOG),
	CHARGE_SYSFS_FIELD_RW(update_volt_now,    UPDATE_VOLT_NOW),
	CHARGE_SYSFS_FIELD_RO(ibus,    IBUS),
	CHARGE_SYSFS_FIELD_RO(fcp_support, FCP_SUPPORT),
};

static struct attribute *charge_sysfs_attrs[ARRAY_SIZE(charge_sysfs_field_tbl) + 1];

static const struct attribute_group charge_sysfs_attr_group =
{
	.attrs = charge_sysfs_attrs,
};

 /* initialize charge_sysfs_attrs[] for charge attribute */
static void charge_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		charge_sysfs_attrs[i] = &charge_sysfs_field_tbl[i].attr.attr;
	}

	charge_sysfs_attrs[limit] = NULL; /* Has additional entry for this */
}

/*
 * get the current device_attribute from charge_sysfs_field_tbl
 * by attr's name
 */
static struct charge_sysfs_field_info *charge_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(charge_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strcmp(name, charge_sysfs_field_tbl[i].attr.attr.name)) {
			break;
		}
	}

	if (i >= limit)	{
		return NULL;
	}

	return &charge_sysfs_field_tbl[i];
}

int get_loginfo_int(struct power_supply *psy, int propery)
{
	int rc = 0;
	union power_supply_propval ret = {0, };

	if (!psy) {
		pr_err("get input source power supply node failed!\n");
		return -EINVAL;
	}

	rc = psy->get_property(psy, propery, &ret);
	if (rc) {
		//pr_err("Couldn't get type rc = %d\n", rc);
		ret.intval = -EINVAL;
	}

	return ret.intval;
}
EXPORT_SYMBOL_GPL(get_loginfo_int);

void strncat_length_protect(char *dest, char *src)
{
	int str_length = 0;

	if (NULL == dest || NULL == src) {
		pr_err("the dest or src is NULL");
		return;
	}
	if (strlen(dest) >= CHARGELOG_SIZE) {
		pr_err("strncat dest is full!\n");
		return;
	}

	str_length = min(CHARGELOG_SIZE - strlen(dest), strlen(src));
	if (str_length > 0) {
		strncat(dest, src, str_length);
	}
}
EXPORT_SYMBOL_GPL(strncat_length_protect);

static void conver_usbtype(int val, char *p_str)
{
	if (NULL == p_str) {
		pr_err("the p_str is NULL\n");
		return;
	}

	switch (val) {
	case POWER_SUPPLY_TYPE_UNKNOWN:
		strncpy(p_str, "UNKNOWN", sizeof("UNKNOWN"));
		break;
	case POWER_SUPPLY_TYPE_USB_CDP:
		strncpy(p_str, "CDP", sizeof("CDP"));
		break;
	case POWER_SUPPLY_TYPE_USB:
		strncpy(p_str, "USB", sizeof("USB"));
		break;
	case POWER_SUPPLY_TYPE_USB_DCP:
		strncpy(p_str, "DC", sizeof("DC"));
		break;
	case POWER_SUPPLY_TYPE_USB_HVDCP:
		strncpy(p_str, "HVDCP", sizeof("HVDCP"));
		break;
	case POWER_SUPPLY_TYPE_USB_HVDCP_3:
		strncpy(p_str, "HVDCP_3", sizeof("HVDCP_3"));
		break;
	default:
		strncpy(p_str, "UNSTANDARD", sizeof("UNSTANDARD"));
		break;
	}
}

static void conver_charging_status(int val, char *p_str)
{
	if (NULL == p_str) {
		pr_err("the p_str is NULL\n");
		return;
	}

	switch (val) {
	case POWER_SUPPLY_STATUS_UNKNOWN:
		strncpy(p_str, "UNKNOWN", sizeof("UNKNOWN"));
		break;
	case POWER_SUPPLY_STATUS_CHARGING:
		strncpy(p_str, "CHARGING", sizeof("CHARGING"));
		break;
	case POWER_SUPPLY_STATUS_DISCHARGING:
		strncpy(p_str, "DISCHARGING", sizeof("DISCHARGING"));
		break;
	case POWER_SUPPLY_STATUS_NOT_CHARGING:
		strncpy(p_str, "NOTCHARGING", sizeof("NOTCHARGING"));
		break;
	case POWER_SUPPLY_STATUS_FULL:
		strncpy(p_str, "FULL", sizeof("FULL"));
		break;
	default:
		break;
	}
}

static void conver_charger_health(int val, char *p_str)
{
	if (NULL == p_str) {
		pr_err("the p_str is NULL\n");
		return;
	}

	switch (val) {
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		strncpy(p_str, "OVERHEAT", sizeof("OVERHEAT"));
		break;
	case POWER_SUPPLY_HEALTH_COLD:
		strncpy(p_str, "COLD", sizeof("COLD"));
		break;
	case POWER_SUPPLY_HEALTH_WARM:
		strncpy(p_str, "WARM", sizeof("WARM"));
		break;
	case POWER_SUPPLY_HEALTH_COOL:
		strncpy(p_str, "COOL", sizeof("COOL"));
		break;
	case POWER_SUPPLY_HEALTH_GOOD:
		strncpy(p_str, "GOOD", sizeof("GOOD"));
		break;
	default:
		break;
	}
}

static bool charger_shutdown_flag;
static int __init early_parse_shutdown_flag(char *p)
{
	if (p) {
		if (!strcmp(p, "charger")) {
			charger_shutdown_flag = true;
		}
	}
	return 0;
}
early_param("androidboot.mode", early_parse_shutdown_flag);

static void get_charger_shutdown_flag(bool flag, char *p_str)
{
	if (NULL == p_str) {
		pr_err("the p_str is NULL\n");
		return;
	}
	if (flag) {
		strncpy(p_str, "OFF", sizeof("OFF"));
	} else {
		strncpy(p_str, "ON", sizeof("ON"));
	}
}

/* show the value for all charge device's node */
static ssize_t charge_sysfs_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct charge_sysfs_field_info *info = NULL;
	struct charge_device_info *di = dev_get_drvdata(dev);
	int online = 0, in_type = 0, ch_en = 0, status = 0, health = 0, bat_present = 0;
	int temp = 0, vol = 0, cur = 0, capacity = 0, ibus = 0, usb_vol = 0;
	char cType[30] = {0}, cStatus[30] = {0}, cHealth[30] = {0}, cOn[30] = {0};
	int fcp_support = 0, count = 0;

	info = charge_sysfs_field_lookup(attr->attr.name);
	if (!info) {
 		return -EINVAL;
	}

	switch (info->name) {
	case CHARGE_SYSFS_IIN_THERMAL:
		ret = snprintf(buf, MAX_SIZE, "%u\n", di->sysfs_data.iin_thl);
		break;
	case CHARGE_SYSFS_IIN_RUNNINGTEST:
		ret = snprintf(buf, MAX_SIZE, "%u\n", di->sysfs_data.iin_rt);
		break;
	case CHARGE_SYSFS_IIN_RT_CURRENT:
		ret = snprintf(buf, MAX_SIZE, "%u\n", di->sysfs_data.iin_rt_curr);
		break;
	case CHARGE_SYSFS_RUNNING_TEST_STATUS:
		ret = snprintf(buf, MAX_SIZE, "%u\n", get_running_test_result(di));
		break;
	case CHARGE_SYSFS_ENABLE_CHARGER:
		ret = snprintf(buf, MAX_SIZE, "%u\n", di->chrg_config);
		break;
	case CHARGE_SYSFS_FACTORY_DIAG_CHARGER:
		ret = snprintf(buf, MAX_SIZE, "%u\n", di->factory_diag);
		break;
	case CHARGE_SYSFS_CHARGELOG_HEAD:
		mutex_lock(&di->sysfs_data.dump_reg_head_lock);
		get_log_info_from_psy(di->batt_psy,
				POWER_SUPPLY_PROP_REGISTER_HEAD,
				di->sysfs_data.reg_head);
		ret = snprintf(buf, MAX_SIZE, " online   in_type     usb_vol     iin_thl     ch_en   status         health    bat_present   temp    vol       cur       capacity   ibus       %s Mode\n",
		di->sysfs_data.reg_head);
		mutex_unlock(&di->sysfs_data.dump_reg_head_lock);
		break;
	case CHARGE_SYSFS_CHARGELOG:
		online = get_loginfo_int(di->usb_psy, POWER_SUPPLY_PROP_ONLINE);
		in_type = get_loginfo_int(di->usb_psy, POWER_SUPPLY_PROP_TYPE);
		conver_usbtype(in_type, cType);
		usb_vol = get_loginfo_int(di->usb_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW);
		ch_en = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_CHARGING_ENABLED);
		status = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_STATUS);
		conver_charging_status(status, cStatus);
		health = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_HEALTH);
		conver_charger_health(health, cHealth);
		bat_present = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_PRESENT);
		temp = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_TEMP);
		vol = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_VOLTAGE_NOW);
		cur = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_CURRENT_NOW);
		capacity = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_CAPACITY);
		ibus = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_NOW);
		get_charger_shutdown_flag(charger_shutdown_flag, cOn);
		mutex_lock(&di->sysfs_data.dump_reg_lock);
		get_log_info_from_psy(di->batt_psy,
				POWER_SUPPLY_PROP_DUMP_REGISTER,
				di->sysfs_data.reg_value);
		ret = snprintf(buf, MAX_SIZE, " %-8d %-11s %-11d %-11d %-7d %-14s %-9s %-13d %-7d %-9d %-9d %-10d %-10d %-16s %s\n",
				online, cType, usb_vol, di->sysfs_data.iin_thl,
				ch_en, cStatus, cHealth, bat_present, temp, vol,
				cur, capacity, ibus, di->sysfs_data.reg_value, cOn);
		mutex_unlock(&di->sysfs_data.dump_reg_lock);
		break;
	case CHARGE_SYSFS_UPDATE_VOLT_NOW:
		ret = snprintf(buf, MAX_SIZE, "%u\n", 1);
		break;
	case CHARGE_SYSFS_IBUS:
		ibus = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_INPUT_CURRENT_NOW);
		return snprintf(buf, MAX_SIZE, "%d\n", ibus);
		break;
	case CHARGE_SYSFS_FCP_SUPPORT:
		fcp_support = get_loginfo_int(di->batt_psy, POWER_SUPPLY_PROP_ALLOW_HVDCP);
		if (!fcp_support) {
			ret = FCP_TEST_NOT_SUPPORT;
		} else {
			ret = FCP_TEST_FAIL;
			do {
				in_type = get_loginfo_int(di->usb_psy, POWER_SUPPLY_PROP_TYPE);
				if ((POWER_SUPPLY_TYPE_USB_HVDCP == in_type) || (POWER_SUPPLY_TYPE_USB_HVDCP_3 == in_type)) {
					ret = FCP_TEST_SUCC;
					break;
				}
				msleep(100);
			} while (count++ <= di->fcp_test_delay / 100);
		}
		return snprintf(buf, MAX_SIZE, "%d\n", ret);
		break;
	case CHARGE_SYSFS_HIZ:
		ret = snprintf(buf, MAX_SIZE, "%u\n", di->sysfs_data.hiz_mode);
		break;
	default:
		pr_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}

	return ret;
}

/* set the value for charge_data's node which is can be written */
static ssize_t charge_sysfs_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	long val = 0;
	struct charge_sysfs_field_info *info = NULL;
	struct charge_device_info *di = dev_get_drvdata(dev);

	info = charge_sysfs_field_lookup(attr->attr.name);
	if (!info)	{
		return -EINVAL;
	}

	switch (info->name) {
	/* hot current limit function*/
	case CHARGE_SYSFS_IIN_THERMAL:
		if ((kstrtol(buf, 10, &val) < 0)||(val < 0)||(val > MAX_CURRENT)) {
			return -EINVAL;
		}
		huawei_charger_set_in_thermal(di, val);
		pr_info("THERMAL set input current = %ld\n", val);
		break;
	/* running test charging/discharge function*/
	case CHARGE_SYSFS_IIN_RUNNINGTEST:
		if ((kstrtol(buf, 10, &val) < 0)||(val < 0)||(val > MAX_CURRENT)) {
			return -EINVAL;
		}
		pr_info("THERMAL set running test val = %ld\n", val);
		huawei_charger_set_runningtest(di, val);
		pr_info("THERMAL set running test iin_rt = %d\n", di->sysfs_data.iin_rt);
		break;
    /* running test charging/discharge function*/
	case CHARGE_SYSFS_IIN_RT_CURRENT:
		if ((kstrtol(buf, 10, &val) < 0)||(val < 0)||(val > MAX_CURRENT)) {
			return -EINVAL;
		}
		pr_info("THERMAL set rt test val = %ld\n", val);
		huawei_charger_set_rt_current(di, val);
		pr_info("THERMAL set rt test iin_rt = %d\n", di->sysfs_data.iin_rt);
		break;
	/* charging/discharging function*/
	case CHARGE_SYSFS_ENABLE_CHARGER:
		if ((kstrtol(buf, 10, &val) < 0)||(val < 0) || (val > 1)) {
			return -EINVAL;
		}
		pr_info("ENABLE_CHARGER set enable charger val = %ld\n", val);
		huawei_charger_enable_charge(di, val);
		pr_info("ENABLE_CHARGER set chrg_config = %d\n", di->chrg_config);
		break;
	/* factory diag function*/
	case CHARGE_SYSFS_FACTORY_DIAG_CHARGER:
		if ((kstrtol(buf, 10, &val) < 0)||(val < 0) || (val > 1)) {
			return -EINVAL;
		}
		pr_info("ENABLE_CHARGER set factory diag val = %ld\n", val);
		huawei_charger_factory_diag_charge(di, val);
		pr_info("ENABLE_CHARGER set factory_diag = %d\n", di->factory_diag);
		break;
	case CHARGE_SYSFS_UPDATE_VOLT_NOW:
		if ((kstrtol(buf, 10, &val) < 0) || (1 != val)) {
			return -EINVAL;
		}
		if (NULL == di || NULL == di->bms_psy) {
			pr_err("fail: UPDATE_VOLT_NOW, di or di->bms_psy is NULL!\n");
			return -EINVAL;
		}
		set_property_on_psy(di->bms_psy, POWER_SUPPLY_PROP_UPDATE_NOW, 1);
		break;
	case CHARGE_SYSFS_HIZ:
		if ((strict_strtol(buf, 10, &val) < 0) || (val < 0) || (val > 1))
			return -EINVAL;
		pr_info("ENABLE_CHARGER set hz mode val = %ld\n", val);
		huawei_charger_set_hz_mode(di,val);
		break;
	default:
		pr_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return count;
}

/* create the charge device sysfs group */
static int charge_sysfs_create_group(struct charge_device_info *di)
{
	charge_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &charge_sysfs_attr_group);
}

/* remove the charge device sysfs group */
static inline void charge_sysfs_remove_group(struct charge_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &charge_sysfs_attr_group);
}

static struct class *hw_power_class;
struct class *hw_power_get_class(void)
{
	if (NULL == hw_power_class) {
		hw_power_class = class_create(THIS_MODULE, "hw_power");
		if (IS_ERR(hw_power_class)) {
			pr_err("hw_power_class create fail");
			return NULL;
		}
	}
	return hw_power_class;
}
EXPORT_SYMBOL_GPL(hw_power_get_class);

static int charge_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct charge_device_info *di = NULL;
	struct class *power_class = NULL;
	struct power_supply *usb_psy;
	struct power_supply *batt_psy;
	struct power_supply *bms_psy;

	usb_psy = power_supply_get_by_name("usb");
	if (!usb_psy) {
		pr_err("usb supply not found deferring probe\n");
		return -EPROBE_DEFER;
	}
	batt_psy = power_supply_get_by_name("battery");
	if (!batt_psy) {
		pr_err("batt supply not found deferring probe\n");
		return -EPROBE_DEFER;
	}
	bms_psy = power_supply_get_by_name("bms");
	if (!bms_psy) {
		pr_err("bms supply not found deferring probe\n");
		return -EPROBE_DEFER;
	}
	di = devm_kzalloc(&pdev->dev, sizeof(struct charge_device_info), GFP_KERNEL);
	if (!di) {
		pr_err("memory allocation failed.\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "huawei,fcp-test-delay", &di->fcp_test_delay);
	if (ret) {
		pr_info("There is no fcp test delay setting, use default time: 1s\n");
		di->fcp_test_delay = DEFAULT_FCP_TEST_DELAY;
	}

	INIT_DELAYED_WORK(&di->smb_charger_work, smb_charger_work);
	di->dev = &(pdev->dev);
	dev_set_drvdata(&(pdev->dev), di);
	di->usb_psy = usb_psy;
	di->batt_psy = batt_psy;
	di->bms_psy = bms_psy;
	di->running_test_settled_status = POWER_SUPPLY_STATUS_CHARGING;

	di->sysfs_data.iin_thl = 1500;
	di->sysfs_data.iin_rt = 1;
	di->sysfs_data.iin_rt_curr = DEFAULT_IIN_CURRENT;
	di->sysfs_data.hiz_mode = 0;
	di->chrg_config = 1;
	di->factory_diag = 1;

	mutex_init(&di->sysfs_data.dump_reg_lock);
	mutex_init(&di->sysfs_data.dump_reg_head_lock);
	ret = charge_sysfs_create_group(di);
	if (ret) {
		pr_err("can't create charge sysfs entries\n");
		goto charge_fail_0;
	}

	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL) {
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		}

		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "charge_data");
		if (ret) {
			pr_err("create link to charge_data fail.\n");
		}
	}

	charge_event_poll_register(charge_dev);
	dsm_register_client(&dsm_charge_monitor);
	g_charger_device_para = di;
	pr_info("huawei charger probe ok!\n");
	return 0;

charge_fail_0:
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(di);
	di = NULL;

	return 0;
}

static void charge_event_poll_unregister(struct device *dev)
{
	sysfs_remove_file(&dev->kobj, &dev_attr_poll_charge_start_event.attr);
	g_sysfs_poll = NULL;
}

static int charge_remove(struct platform_device *pdev)
{
	struct charge_device_info *di = dev_get_drvdata(&pdev->dev);

	cancel_delayed_work_sync(&di->smb_charger_work);
	charge_event_poll_unregister(charge_dev);
	dsm_unregister_client(dsm_chargemonitor_dclient, &dsm_charge_monitor);
	charge_sysfs_remove_group(di);
	kfree(di);
	di = NULL;

	return 0;
}

static void charge_shutdown(struct platform_device  *pdev)
{
	return;
}

#ifdef CONFIG_PM

static int charge_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int charge_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static struct of_device_id charge_match_table[] =
{
	{
		.compatible = "huawei,charger",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver charge_driver =
{
	.probe = charge_probe,
	.remove = charge_remove,
	.suspend = charge_suspend,
	.resume = charge_resume,
	.shutdown = charge_shutdown,
	.driver =
	{
		.name = "huawei,charger",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(charge_match_table),
	},
};

static int __init charge_init(void)
{
	platform_driver_register(&charge_driver);

	return 0;
}

static void __exit charge_exit(void)
{
	platform_driver_unregister(&charge_driver);
}

late_initcall(charge_init);
module_exit(charge_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei charger module driver");
MODULE_AUTHOR("HUAWEI Inc");
