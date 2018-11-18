/*
 * drivers/power/huawei_fuelguage.h
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

#ifndef _HUAWEI_FUEL_GUAGE
#define _HUAWEI_FUEL_GUAGE
#define MAX_SIZE    2048
#define CHARGELOG_SIZE   2048

struct fuelguage_sysfs_data {
	char reg_value[CHARGELOG_SIZE];
	char reg_head[CHARGELOG_SIZE];
	struct mutex dump_reg_lock;
	struct mutex dump_reg_head_lock;
};

struct fuelguage_device_info {
	struct device *dev;
	struct power_supply *bms_psy;
	struct fuelguage_sysfs_data sysfs_data;
};

extern void strncat_length_protect(char *dest, char* src);

#endif
