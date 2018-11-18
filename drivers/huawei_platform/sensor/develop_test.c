/*
 * Copyright (C) 2015-11-19 huawei sensor driver group
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*==============================================================================
History

Problem NO.         Name        Time         Reason

==============================================================================*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/sensors.h>
#include <linux/module.h>
#include <misc/app_info.h>

static int sns_dt_debug_mask= 1;

#define DTMODE_ERR(x...) do {\
    if (sns_dt_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)

#define DTMODE_INFO(x...) do {\
    if (sns_dt_debug_mask >=1) \
        printk(KERN_ERR x);\
    } while (0)
#define DTMODE_FLOW(x...) do {\
    if (sns_dt_debug_mask >=2) \
        printk(KERN_ERR x);\
    } while (0)


module_param_named(sensor_dt_debug, sns_dt_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

bool sensorDT_mode=false;//sensor DT mode
int als_data_count=0;	 //ALS sensor upload data times
int ps_data_count=0;	 //ps sensor upload data times


static ssize_t store_sensor_DT_test(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
    unsigned long val = 0;
    if (strict_strtoul(buf, 10, &val))
    {
        DTMODE_ERR("[sensor]%s:store_sensor_DT_test val, val = %ld\n", __func__, val);
        return -EINVAL;
    }
    DTMODE_INFO("[sensor]%s:store_sensor_DT_test val=%ld\n", __func__, val);
    if (1 == val)
    {
        sensorDT_mode = true;
        als_data_count = 0;
        ps_data_count = 0;
    }
    else
    {
        sensorDT_mode = false;
        als_data_count = 0;
        ps_data_count = 0;
    }
    return count;
}

static ssize_t show_sensor_DT_test(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (dev == NULL) {
		DTMODE_ERR("[sensor]%s:dev info is null\n", __func__);
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", sensorDT_mode);
}
static DEVICE_ATTR(sensor_dt_enable, 0660,show_sensor_DT_test, store_sensor_DT_test);


static ssize_t show_als_test_result(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (dev == NULL) {
		DTMODE_ERR("[sensor]%s:als_chip info is null.\n",__func__);
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", als_data_count);
}
static DEVICE_ATTR(als_read_data, S_IRUGO,show_als_test_result, NULL);


static ssize_t show_ps_test_result(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (dev == NULL) {
		DTMODE_ERR("[sensor]%s:ps_chip info is null.\n",__func__);
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", ps_data_count);
}
static DEVICE_ATTR(ps_read_data, S_IRUGO,show_ps_test_result, NULL);




static struct platform_device sensor_dt_node = {
	.name = "sensor_dt_nodes",
	.id = -1,
};

static struct attribute *sensor_dt_attributes[] = {
	&dev_attr_sensor_dt_enable.attr,
	&dev_attr_als_read_data.attr,
	&dev_attr_ps_read_data.attr,
	NULL
};

static const struct attribute_group snstest_attr = {
	.attrs = sensor_dt_attributes,
};

static int __init sns_dvlptest_init(void)
{
    int ret = 0;
	
	//register sensor dt node on platform
    ret = platform_device_register(&sensor_dt_node);
    if (ret)
    {
        DTMODE_ERR("[sensor]%s: register sensor_dt_node on platform failed, ret:%d.\n",
               __func__, ret);
        goto REGISTER_ERR;
    }

	//create all test node
    ret = sysfs_create_group(&sensor_dt_node.dev.kobj, &snstest_attr);
    if (ret)
    {
        DTMODE_ERR("[sensor]%s: sysfs_create_group error ret =%d.\n", __func__, ret);
        goto SYSFS_CREATE_CGOUP_ERR;
    }

	DTMODE_INFO("[sensor]%s: probe success.\n",__func__);

    return 0;

SYSFS_CREATE_CGOUP_ERR:
    platform_device_unregister(&sensor_dt_node);
REGISTER_ERR:
    return ret;

}

device_initcall(sns_dvlptest_init);

MODULE_AUTHOR("huawei driver group of sensor");
MODULE_DESCRIPTION("sensor module developer selftest");
MODULE_LICENSE("GPL");
