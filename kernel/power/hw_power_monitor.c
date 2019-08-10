/*
 * Copyright (c) Huawei Technologies Co., Ltd. 1998-2014. All rights reserved.
 *
 * File name: hw_power_monitor.c
 * Description: This file use to record power state for upper layer
 * Author: ivan.chengfeifei@huawei.com
 * Version: 0.1
 * Date:  2014/11/27
 */
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/debugfs.h>
#include <linux/hw_power_monitor.h>
#include "power.h"



unsigned int suspend_total = 0;
bool freezing_wakeup_check = false;
static struct power_monitor_info pm_info[POWER_MONITOR_MAX];
static struct wake_up_info wp_info[WAKE_UP_RECORD_MAX_NUM];
static struct wake_gpio_info wp_gpio_info[GPIO_RECORD_MAX_NUM];
static DEFINE_MUTEX(power_monitor_mutex);


/*
 * Function: is_id_valid
 * Description: check ID valid or not
 * Input:  @id - id to check
 * Output:
 * Return: false -- invalid
 *         true -- valid
 */
static inline bool is_id_valid(int id)
{
	return (id >= AP_SLEEP && id < POWER_MONITOR_MAX);
}


/*
 * Function: handle_wake_gpio
 * Description: record wake up name and times
 * Input: buf --- wake up name
 * Return: -x--failed, 0--success
 */

static int handle_wake_gpio(char *buf)
{
        char buff[WAKE_UP_GPIO_SIZE] = {0};
        int index = 0;

        memset(&buff, 0, sizeof(buff));
        if(buf ==NULL)
       {
	   pr_err("%s: buf is null !\n", __func__);
	   return -1;
        }

       strlcpy(buff, buf, sizeof(buff));

       for( index = 0;index<GPIO_RECORD_MAX_NUM;index++ ){

           if (!strcmp(wp_gpio_info[index].wake_gpio_name, buff)){//have be recorded,only +1
	         wp_gpio_info[index].count++;
	       break;
            }

            if(!strlen(wp_gpio_info[index].wake_gpio_name)){//empty
	        strlcpy(wp_gpio_info[index].wake_gpio_name, buff, sizeof(wp_gpio_info[index].wake_gpio_name));
	        wp_gpio_info[index].count++;
	        break;
            }
     }

         return 0;
}



/*
 * Function: handle_wake_up
 * Description: record wake up name and times
 * Input: buf --- wake up name
 * Return: -x--failed, 0--success
 */

static int handle_wake_up(char *buf)
{
        char buff[WAKE_UP_NAME_SIZE] = {0};
        int index = 0;

        memset(&buff, 0, sizeof(buff));
        if(buf ==NULL)
       {
	   pr_err("%s: buf is null !\n", __func__);
	   return -1;
        }

       strlcpy(buff, buf, sizeof(buff));

       for( index = 0;index<WAKE_UP_RECORD_MAX_NUM;index++ ){

           if (!strcmp(wp_info[index].wake_up_name, buff)){//have be recorded,only +1
	         wp_info[index].count++;
	       break;
            }

            if(!strlen(wp_info[index].wake_up_name)){//empty
	        strlcpy(wp_info[index].wake_up_name, buff, sizeof(wp_info[index].wake_up_name));
	        wp_info[index].count++;
	        break;
            }
     }

         return 0;
}


/*
 * Function: report_handle
 * Description: Packet and Send data to power node
 * Input: id --- message mask
 *        fmt -- string
 * Return: -1--failed, 0--success
 */
static int report_handle(unsigned int id,  va_list args, const char *fmt)
{
	int length = 0;
	char buff[BUFF_SIZE] = {0};

	memset(&buff, 0, sizeof(buff));
	length = vscnprintf(buff, BUFF_SIZE - 1, fmt, args);
	if (length > 0) {
		length ++;
		pr_info("%s: id = %d length = %d buff = %s\n", __func__, id, length, buff);
	}

	if (BUFF_SIZE <= length) // or < ?
	{
		pr_err("%s: string too long!\n", __func__);
		return -ENAMETOOLONG;
	}

	if (buff == NULL) {
                pr_err("%s: string is null!\n", __func__);
                return -1;
        }

	switch (id) {
	case AP_SLEEP:
		if (strncmp(buff, "[ap_sleep]:", 11) == 0){
			pm_info[id].count ++;
			strlcpy(pm_info[id].buffer, buff, sizeof(pm_info[id].buffer));
		}
		break;
	case MODEM_SLEEP:
		pm_info[id].count ++;
		strlcpy(pm_info[id].buffer, buff, sizeof(pm_info[id].buffer));
		break;
	case SUSPEND_FAILED:
		if (strncmp(buff, "[suspend_total]", 15) == 0){
			suspend_total ++;
		}
		else if (strncmp(buff, "[error_dev_name]:", 17) == 0){
			pm_info[id].count ++;
			strlcpy(pm_info[id].buffer, buff, sizeof(pm_info[id].buffer));
		}
		break;
	case FREEZING_FAILED:
	       if (pm_info[id].buffer != NULL) {
                   pm_info[id].count++;
                   memset(pm_info[id].buffer, '\0',sizeof(pm_info[id].buffer));
                   strlcpy(pm_info[id].buffer, buff, sizeof(pm_info[id].buffer));
                }

		break;
	case WAKEUP_ABNORMAL:
		pm_info[id].count ++;
		strlcpy(pm_info[id].buffer, buff, sizeof(pm_info[id].buffer));
		break;
	case DRIVER_ABNORMAL:
		pm_info[id].count ++;
		strlcpy(pm_info[id].buffer, buff, sizeof(pm_info[id].buffer));
		break;
        case WAKEUP_IRQ:
                handle_wake_up(buff);
		break;
        case WAKEUP_GPIO:
                handle_wake_gpio(buff);
		break;
	default:
		break;
	}

	return 0;
}


/*
 * Function: power_monitor_report
 * Description: report data to power nodes
 * Input: id --- power radar nodes data struct
 *        fmt -- args from reported devices
 * Return: -x--failed, 0--success
 */
int power_monitor_report(unsigned int id, const char *fmt, ...)
{
	va_list args;
	int ret = -EINVAL;

	if (!is_id_valid(id)) {
		pr_err("%s: id %d is invalid!\n", __func__, id);
		return ret;
	}

	va_start(args, fmt);
	ret = report_handle(id, args, fmt);
	va_end(args);

	return ret;
}

EXPORT_SYMBOL(power_monitor_report);

static ssize_t ap_sleep_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned int id = AP_SLEEP;

	return sprintf(buf, "count:%8d %s\n", pm_info[id].count, pm_info[id].buffer);
}

static ssize_t ap_sleep_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = AP_SLEEP;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(ap_sleep);

static ssize_t modem_sleep_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned int id = MODEM_SLEEP;

	return sprintf(buf, "count:%8d %s\n", pm_info[id].count, pm_info[id].buffer);
}

static ssize_t modem_sleep_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = MODEM_SLEEP;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(modem_sleep);

static ssize_t suspend_failed_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned int id = SUSPEND_FAILED;

	return sprintf(buf, "count:%8d %s\n", pm_info[id].count, pm_info[id].buffer);
}

static ssize_t suspend_failed_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = SUSPEND_FAILED;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(suspend_failed);

static ssize_t freezing_failed_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
        unsigned int id = FREEZING_FAILED;
        ssize_t ret = 0;

        if (pm_info[id].buffer != NULL) {
        ret = sprintf(buf, "freezing_failed:%s,times:%d\n",
                  pm_info[id].buffer, pm_info[id].count);
        pm_info[id].count = 0;
        }

        return ret;
}

static ssize_t freezing_failed_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = FREEZING_FAILED;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(freezing_failed);

static ssize_t wakeup_abnormal_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned int id = WAKEUP_ABNORMAL;

	return sprintf(buf, "count:%8d %s\n", pm_info[id].count, pm_info[id].buffer);
}

static ssize_t wakeup_abnormal_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = WAKEUP_ABNORMAL;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(wakeup_abnormal);

static ssize_t driver_abnormal_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	unsigned int id = DRIVER_ABNORMAL;

	return sprintf(buf, "count:%8d %s\n", pm_info[id].count, pm_info[id].buffer);
}

static ssize_t driver_abnormal_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = DRIVER_ABNORMAL;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(driver_abnormal);


static ssize_t wakeup_irq_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{

        ssize_t ret = 0;
	int index = 0;
        char show_irq_buffer[256] = { 0 };
        char show_irq_temp_buffer[60] = { 0 };

        memset(&show_irq_buffer, 0, sizeof(show_irq_buffer));
        memset(&show_irq_temp_buffer, 0, sizeof(show_irq_temp_buffer));

        for( index =0 ;index<WAKE_UP_RECORD_MAX_NUM;index++){
            if(strlen(wp_info[index].wake_up_name)){

                ret=sprintf(show_irq_temp_buffer, "%s:%d\n",
                wp_info[index].wake_up_name, wp_info[index].count);
		if((strlen(show_irq_temp_buffer)+strlen(show_irq_buffer)) < sizeof(show_irq_buffer))
		     strcat(show_irq_buffer, show_irq_temp_buffer);

		memset(&show_irq_temp_buffer, 0, sizeof(show_irq_temp_buffer));

           }
       //clear info
	memset(wp_info[index].wake_up_name, 0, sizeof(wp_info[index].wake_up_name));
	wp_info[index].count = 0;
	}

        if(strlen(show_irq_buffer))//exist info
	    return sprintf(buf, "wakeup irq: %s", show_irq_buffer);
       else
            return ret;

}

static ssize_t wakeup_irq_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = WAKEUP_IRQ;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}

power_attr(wakeup_irq);

static ssize_t wakeup_gpio_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{

        ssize_t ret = 0;
	int index = 0;
        char show_irq_buffer[100] = { 0 };
        char show_irq_temp_buffer[50] = { 0 };

        memset(&show_irq_buffer, 0, sizeof(show_irq_buffer));
        memset(&show_irq_temp_buffer, 0, sizeof(show_irq_temp_buffer));

        for( index =0 ;index<GPIO_RECORD_MAX_NUM;index++){
            if(strlen(wp_gpio_info[index].wake_gpio_name)){

               ret =  sprintf(show_irq_temp_buffer, "-%s:%d-\n",
                wp_gpio_info[index].wake_gpio_name, wp_gpio_info[index].count);
		if((strlen(show_irq_temp_buffer)+strlen(show_irq_buffer)) < sizeof(show_irq_buffer))
		     strcat(show_irq_buffer, show_irq_temp_buffer);

		memset(&show_irq_temp_buffer, 0, sizeof(show_irq_temp_buffer));

           }
       //clear info
	memset(wp_gpio_info[index].wake_gpio_name, 0, sizeof(wp_gpio_info[index].wake_gpio_name));
	wp_gpio_info[index].count = 0;
	}

       if(strlen(show_irq_buffer))//exist info
	    return sprintf(buf, "gpio%s", show_irq_buffer);
       else
            return ret;

}

static ssize_t wakeup_gpio_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char *buf, size_t n)
{
	unsigned int size = 0;
	unsigned int id = WAKEUP_GPIO;

	if (sscanf(buf, "%d", &size) == 1) {
		pm_info[id].count = size;
		return n;
	}

	return -EINVAL;
}


power_attr(wakeup_gpio);



static struct attribute * monitor_attrs[] = {
	&ap_sleep_attr.attr,
	&modem_sleep_attr.attr,
	&suspend_failed_attr.attr,
	&freezing_failed_attr.attr,
	&wakeup_abnormal_attr.attr,
	&driver_abnormal_attr.attr,
	&wakeup_irq_attr.attr,
	&wakeup_gpio_attr.attr,
	NULL,
};

static struct attribute_group monitor_attr_group = {
	.name = "monitor", /* Directory of power monitor */
	.attrs = monitor_attrs,
};

static int __init power_monitor_init(void)
{
	int ret = -1;
	int i = 0, length = 0;

	/* power_kobj is created in kernel/power/main.c */
	if (!power_kobj){
		pr_err("%s: power_kobj is null!\n", __func__);
		return -ENOMEM;
	}

	/* Initialized struct data */
	length = sizeof(struct power_monitor_info);
	for (i = 0; i < POWER_MONITOR_MAX; i++) {
		memset(&pm_info[i], 0, length);
	}

	length = sizeof(struct wake_up_info);
	for (i = 0; i < WAKE_UP_RECORD_MAX_NUM; i++) {
		memset(&wp_info[i], 0, length);
	}

	length = sizeof(struct wake_gpio_info);
	for (i = 0; i < GPIO_RECORD_MAX_NUM; i++) {
		memset(&wp_gpio_info[i], 0, length);
	}

	/* create all nodes under power sysfs */
	ret = sysfs_create_group(power_kobj, &monitor_attr_group);
	if (ret < 0) {
		pr_err("%s: sysfs_create_group power_kobj error\n", __func__);
	} else {
		pr_info("%s: sysfs_create_group power_kobj success\n", __func__);
	}

	return ret;
}

core_initcall(power_monitor_init);


