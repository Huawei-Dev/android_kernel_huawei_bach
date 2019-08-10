#include "ilitek_ts.h"
#ifdef HALL_CHECK
extern struct i2c_data i2c;
int tpd_sensitivity_status = 0;
static struct kobject *ilitek_android_touch_kobj = NULL;


int ilitek_into_hall_halfmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0x0C;
	cmd[1] = 0x00;
	tp_log_info("ilitek_into_hall_halfmode\n");
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_info("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}
int ilitek_into_hall_normalmode(void) {
	int ret = 0;
	uint8_t cmd[2] = {0};
	struct i2c_msg msgs_cmd[] = {
		{.addr = i2c.client->addr, .flags = 0, .len = 2, .buf = cmd,},
	};
	cmd[0] = 0x0C;
	cmd[1] = 0x01;
	tp_log_info("ilitek_into_hall_normalmode\n");
	ret = ilitek_i2c_transfer(i2c.client, msgs_cmd, 1);
	if(ret < 0){
		tp_log_info("%s,  err, ret %d\n", __func__, ret);
		return ret;
	}
	return 0;
}

static ssize_t ilitek_tpd_sensitivity_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "tpd_sensitivity_status = %d\n", tpd_sensitivity_status);
}
static ssize_t ilitek_tpd_sensitivity_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	int ret = 0;
	ret = kstrtoint(buf, 10, &tpd_sensitivity_status);
	tp_log_info("ilitek_tpd_sensitivity_store tpd_sensitivity_status = %d\n", tpd_sensitivity_status);
	if (ret) {
		tp_log_info("ilitek_tpd_sensitivity_store kstrtoint error\n");
		return ret;
	}
	if(tpd_sensitivity_status)
	{
		ilitek_into_hall_normalmode();
	}
	else
	{
		ilitek_into_hall_halfmode();
	}
	return size;
}

static DEVICE_ATTR(ilitek_tpd_sensitivity, S_IRUGO | S_IWUSR, ilitek_tpd_sensitivity_show, ilitek_tpd_sensitivity_store);

static struct attribute *ilitek_sysfs_attrs_ctrl[] = {
	&dev_attr_ilitek_tpd_sensitivity.attr,
	NULL
};
static struct attribute_group ilitek_attribute_group[] = {
	{.attrs = ilitek_sysfs_attrs_ctrl },
};

void ilitek_hall_check_hw_init(void)
{
	int ret = 0;
	if (!ilitek_android_touch_kobj) {
		ilitek_android_touch_kobj = kobject_create_and_add("tpd_sensitivity", NULL) ;
		if (ilitek_android_touch_kobj == NULL) {
			tp_log_info("[ilitek]%s: kobject_create_and_add failed\n", __func__);
			//return;
		}
	}
	ret = sysfs_create_group(ilitek_android_touch_kobj, ilitek_attribute_group);
	if (ret < 0) {
		tp_log_info("[ilitek]%s: sysfs_create_group failed\n", __func__);
	}
	return;
}

void ilitek_hall_check_hw_deinit(void)
{
	if (ilitek_android_touch_kobj) {
		sysfs_remove_group(ilitek_android_touch_kobj, ilitek_attribute_group);
		kobject_del(ilitek_android_touch_kobj);
	}
}

#endif
