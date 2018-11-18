/*
 * drivers/power/huawei_fuelguage.c
 *
 *huawei fuelguage driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/string.h>
#include <linux/power/huawei_fuelguage.h>

struct device *fuelguage_dev;
struct fuelguage_device_info *g_fg_device_para;
extern int get_loginfo_int(struct power_supply *phy, int propery);
extern struct class *hw_power_get_class(void);
extern void get_log_info_from_psy(struct power_supply *psy,
				  enum power_supply_property prop, char *buf);

enum coul_sysfs_type {
	FUELGUAGE_SYSFS_GAUGELOG_HEAD = 0,
	FUELGUAGE_SYSFS_GAUGELOG,
	FUELGUAGE_SYSFS_TEST,
	FUELGUAGE_SYSFS_CURRENT_NOW,
};

#define FUELGUAGE_SYSFS_FIELD(_name, n, m, store)	\
{	\
	.attr = __ATTR(_name, m, fuelguage_sysfs_show, store),	\
	.name = FUELGUAGE_SYSFS_##n,	\
}

#define FUELGUAGE_SYSFS_FIELD_RW(_name, n)	\
	FUELGUAGE_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO,	\
                fuelguage_sysfs_store)

#define FUELGUAGE_SYSFS_FIELD_RO(_name, n)	\
	FUELGUAGE_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t fuelguage_sysfs_show(struct device *dev,
				    struct device_attribute *attr, char *buf);
static ssize_t fuelguage_sysfs_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count);

struct fuelguage_sysfs_field_info {
	struct device_attribute attr;
	char name;
};

static struct fuelguage_sysfs_field_info fuelguage_sysfs_field_tbl[] = {
	FUELGUAGE_SYSFS_FIELD_RO(gaugelog_head,	GAUGELOG_HEAD),
	FUELGUAGE_SYSFS_FIELD_RO(gaugelog, GAUGELOG),
	FUELGUAGE_SYSFS_FIELD_RW(test, TEST),
	FUELGUAGE_SYSFS_FIELD_RO(current_now, CURRENT_NOW),
};

static struct attribute *fuelguage_sysfs_attrs[ARRAY_SIZE(fuelguage_sysfs_field_tbl) + 1];

static const struct attribute_group fuelguage_sysfs_attr_group = {
	.attrs = fuelguage_sysfs_attrs,
};

/**********************************************************
*  Function:       fuelguage_sysfs_init_attrs
*  Discription:    initialize fuelguage_sysfs_attrs[] for charge attribute
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
static void fuelguage_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(fuelguage_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		fuelguage_sysfs_attrs[i] = &fuelguage_sysfs_field_tbl[i].attr.attr;
	}

	/* Has additional entry for this */
	fuelguage_sysfs_attrs[limit] = NULL;
}

/**********************************************************
*  Function:       fuelguage_sysfs_field_lookup
*  Discription:    get the current device_attribute from charge_sysfs_field_tbl by attr's name
*  Parameters:     name:device attribute name
*  return value:   fuelguage_sysfs_field_tbl[]
**********************************************************/
static struct fuelguage_sysfs_field_info *fuelguage_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(fuelguage_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strcmp(name, fuelguage_sysfs_field_tbl[i].attr.attr.name)) {
			break;
		}
	}

	if (i >= limit) {
		return NULL;
	}

	return &fuelguage_sysfs_field_tbl[i];
}

static void get_loginfo_str(struct power_supply *psy, int propery, char *p_str)
{
	int rc = 0;
	union power_supply_propval ret = {0, };

	if (!psy) {
		pr_err("get input source power supply node failed!\n");
		return;
	}

	if (NULL == p_str) {
		pr_err("the p_str is NULL\n");
		return;
	}

	rc = psy->get_property(psy, propery, &ret);
	if (rc) {
		pr_err("Couldn't get type rc = %d\n", rc);
		return;
	}
	strncpy(p_str, ret.strval, strlen(ret.strval));
}

/**********************************************************
*  Function:       fuelguage_sysfs_show
*  Discription:    show the value for all charge device's node
*  Parameters:     dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:   0-success or others-fail
**********************************************************/
static ssize_t fuelguage_sysfs_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct fuelguage_sysfs_field_info *info = NULL;
	struct fuelguage_device_info *di = dev_get_drvdata(dev);
	int ocv = 0, ret = 0, cur = 0;
	char cType[50] = {0};

	info = fuelguage_sysfs_field_lookup(attr->attr.name);
	if (!info) {
		return -EINVAL;
	}

	switch (info->name) {
	case FUELGUAGE_SYSFS_GAUGELOG_HEAD:
		mutex_lock(&di->sysfs_data.dump_reg_head_lock);
		get_log_info_from_psy(di->bms_psy,
				POWER_SUPPLY_PROP_REGISTER_HEAD,
				di->sysfs_data.reg_head);

		ret = snprintf(buf, MAX_SIZE, "ocv        battery_type                %s",
				di->sysfs_data.reg_head);
		mutex_unlock(&di->sysfs_data.dump_reg_head_lock);
		break;
	case FUELGUAGE_SYSFS_GAUGELOG:
		ocv = get_loginfo_int(di->bms_psy,
				POWER_SUPPLY_PROP_VOLTAGE_OCV);
		get_loginfo_str(di->bms_psy, POWER_SUPPLY_PROP_BATTERY_TYPE,
				cType);
		mutex_lock(&di->sysfs_data.dump_reg_lock);
		get_log_info_from_psy(di->bms_psy,
				POWER_SUPPLY_PROP_DUMP_REGISTER,
				di->sysfs_data.reg_value);
		ret = snprintf(buf, MAX_SIZE, "%-10d %-27s %s",
				ocv, cType, di->sysfs_data.reg_value);
		mutex_unlock(&di->sysfs_data.dump_reg_lock);
		break;
	case FUELGUAGE_SYSFS_CURRENT_NOW:
		cur = get_loginfo_int(di->bms_psy, POWER_SUPPLY_PROP_CURRENT_NOW);
		ret = snprintf(buf, MAX_SIZE, "%d\n", cur);
		break;
	default:
		pr_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}

	return ret;
}

/**********************************************************
*  Function:       fuelguage_sysfs_store
*  Discription:    set the value for charge_data's node which is can be written
*  Parameters:     dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:   0-success or others-fail
**********************************************************/
static ssize_t fuelguage_sysfs_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct fuelguage_sysfs_field_info *info = NULL;
	long val = 0;

	info = fuelguage_sysfs_field_lookup(attr->attr.name);
	if (!info) {
		return -EINVAL;
	}

	switch (info->name) {
	case FUELGUAGE_SYSFS_TEST:
		if ((kstrtol(buf, 10, &val) < 0) || val > 10) {
			return -EINVAL;
		}
		pr_info("fuelguage_sysfs_store test %ld\n", val);
		break;
	default:
		pr_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n",
				__func__, info->name);
		break;
	}

	return count;
}

/**********************************************************
*  Function:       fuelguage_sysfs_create_group
*  Discription:    create the charge device sysfs group
*  Parameters:     di:fuelguage_device_info
*  return value:   0-success or others-fail
**********************************************************/
static int fuelguage_sysfs_create_group(struct fuelguage_device_info *di)
{
	fuelguage_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj, &fuelguage_sysfs_attr_group);
}

/**********************************************************
*  Function:       fuelguage_sysfs_remove_group
*  Discription:    remove the charge device sysfs group
*  Parameters:     di:charge_device_info
*  return value:   NULL
**********************************************************/
static inline void fuelguage_sysfs_remove_group(struct fuelguage_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &fuelguage_sysfs_attr_group);
}

/*******************************************************
   Function:        fuel_guage_probe
   Description:     probe function
   Input:           struct platform_device *pdev   ---- platform device
   Output:          NULL
   Return:          NULL
********************************************************/
static int fuel_guage_probe(struct platform_device *pdev)
{
	struct fuelguage_device_info *di = NULL;
	struct power_supply *bms_psy;
	struct class *power_class = NULL;
	struct device_node *np = NULL;
	const  char *fuelguage_type_string;
	int ret = 0;

	np = pdev->dev.of_node;
	ret = of_property_read_string(np, "fuelguage_type",
			&fuelguage_type_string);
	if (ret) {
		pr_err("Failed to get fuelguage type from dts ,ret=%d\n", ret);
		return -EPROBE_DEFER;
	}

	bms_psy = power_supply_get_by_name(fuelguage_type_string);
	if (!bms_psy) {
		pr_err("bms supply not found deferring probe\n");
		return -EPROBE_DEFER;
	}

	di = devm_kzalloc(&pdev->dev, sizeof(struct fuelguage_device_info),
			GFP_KERNEL);
	if (!di) {
		pr_err("memory allocation failed.\n");
		return -ENOMEM;
	}
	di->dev = &(pdev->dev);
	dev_set_drvdata(&(pdev->dev), di);
	di->bms_psy = bms_psy;
	mutex_init(&di->sysfs_data.dump_reg_lock);
	mutex_init(&di->sysfs_data.dump_reg_head_lock);

	/* create sysfs */
	ret = fuelguage_sysfs_create_group(di);
	if (ret) {
		pr_err("can't create fuel guage sysfs entries\n");
		goto fuel_guage_fail_0;
	}

	power_class = hw_power_get_class();
	if (power_class) {
		if (NULL == fuelguage_dev) {
			fuelguage_dev = device_create(power_class, NULL, 0,
					"%s", "coul");
			if (IS_ERR(fuelguage_dev)) {
				fuelguage_dev = NULL;
			}
		}
		if (fuelguage_dev) {
			ret = sysfs_create_link(&fuelguage_dev->kobj,
					&di->dev->kobj, "coul_data");
			if (0 != ret) {
				pr_err("%s failed to create sysfs link!!!\n",
						__func__);
			}
		} else {
			pr_err("%s failed to create new_dev!!!\n", __func__);
		}
	}
	g_fg_device_para = di;

	return ret;
fuel_guage_fail_0:
	dev_set_drvdata(&pdev->dev, NULL);
	kfree(di);
	di = NULL;

	return ret;
}

static int fuel_guage_remove(struct platform_device *pdev)
{
	struct fuelguage_device_info *di = dev_get_drvdata(&pdev->dev);

	fuelguage_sysfs_remove_group(di);
	kfree(di);
	di = NULL;

	return 0;
}

static struct of_device_id fuel_guage_match_table[] = {
	{
		.compatible = "huawei,fuelguage",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver fuel_guage_driver = {
	.probe = fuel_guage_probe,
	.remove = fuel_guage_remove,
	.driver = {
		.name = "huawei,fuelguage",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(fuel_guage_match_table),
	},
};

static int __init fuel_guage_init(void)
{
	return platform_driver_register(&fuel_guage_driver);
}

static void __exit fuel_guage_exit(void)
{
	platform_driver_unregister(&fuel_guage_driver);
}

late_initcall(fuel_guage_init);
module_exit(fuel_guage_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei fuel guage module driver");
MODULE_AUTHOR("HUAWEI Inc");
