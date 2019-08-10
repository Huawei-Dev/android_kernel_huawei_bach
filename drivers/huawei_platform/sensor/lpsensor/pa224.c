/*huawei kernel driver for txc_pa2240*/
/*
 * txc_pa2240.c - Linux kernel modules for ambient light + proximity sensor
 *
 * Copyright (C) 2012 Lee Kai Koon <kai-koon.lee@avagotech.com>
 * Copyright (C) 2012 Avago Technologies
 * Copyright (C) 2013 LGE Inc.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <huawei_platform/sensor/pa224.h>
//#include <huawei_platform/sensor/hw_sensor_info.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/sensors.h>
#include <linux/wakelock.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#include <misc/app_info.h>

#include <linux/debugfs.h>
#include <linux/kernel.h>
#ifdef CONFIG_HUAWEI_DSM
#include 	<dsm/dsm_pub.h>
#endif

//#undef CONFIG_HUAWEI_DSM
#define TXC_PA2240_DRV_NAME	"txc_pa2240"
#define DRIVER_VERSION		"1.0.0"

#define MAX_BUF_LEN 30
#define TXC_PA2240_REG_LEN 0x13

#define TXC_PA2240_PROX_MAX_ADC_VALUE 254
#define TXC_PA2240_PS_CAL_FILE_PATH "/data/log/sar_cap/txc_calibration"
/* Register Value define : CONTROL */
#define TXC_PA2240_I2C_RETRY_COUNT		3 	/* Number of times to retry i2c */
/*wait more time to try read or write to avoid potencial risk*/
#define TXC_PA2240_I2C_RETRY_TIMEOUT	6	/* Timeout between retry (miliseconds) */
#define TXC_PA2240_I2C_RESUME_TIMEOUT	500	/* Timeout i2c contrlor resume to ready (miliseconds) */

#define TXC_PA2240_I2C_BYTE 0
#define TXC_PA2240_I2C_WORD 1
/*keep 400ms wakeup after the ps report the far or near state*/
#define PS_WAKEUP_TIME 700
/*pls parameters,it is still different for every devices*/
extern bool power_key_ps ;    //the value is true means powerkey is pressed, false means not pressed
/*dynamic debug mask to control log print,you can echo value to txc_pa2240_debug to control*/
static int txc_pa2240_debug_mask= 1;
module_param_named(txc_pa2240_debug, txc_pa2240_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
#define TXC_PA2240_ERR(x...) do {\
	if (txc_pa2240_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)
/*KERNEL_HWFLOW is for radar using to control all the log of devices*/
#define TXC_PA2240_INFO(x...) do {\
	if (txc_pa2240_debug_mask >0) \
        printk(KERN_ERR x);\
    } while (0)
#define TXC_PA2240_FLOW(x...) do {\
    if ((txc_pa2240_debug_mask >1)) \
        printk(KERN_ERR x);\
    } while (0)
#ifdef CONFIG_SENSOR_DEVELOP_TEST
extern bool sensorDT_mode;//sensor DT mode
extern int ps_data_count; //ps sensor upload data times
#endif
/*parameters for new algorithm to prevent oil effect*/
static unsigned int far_ps_min = 255;
/*Maximum count added to ps data. Depending on IR current. 40 for 10mA using VCSEL*/
static unsigned int oil_effect = 0;
static unsigned int high_threshold = 0;
static unsigned int low_threshold = 0;
static unsigned int middle_threshold = 0;
static unsigned int calibration_threshold = 0;

 struct ls_test_excep{
	int i2c_scl_val;		/* when i2c transfer err, read the gpio value*/
	int i2c_sda_val;
	int vdd_mv;
	int vdd_status;
	int vio_mv;
	int vio_status;
	int i2c_err_num;
	int excep_num;
	atomic_t ps_enable_flag;
	int ps_enable_cnt;
	int ps_report_flag;		/*check if this is one irq occur or not after enable ps */
	int last_high_threshold;
	int last_low_threshold;
	int cur_high_threshold;
	int cur_low_threshold;
	int check_type;
	int irq_val;
	struct mutex dsm_lock;
	char *reg_buf;
};
struct txc_pa2240_data {
	struct i2c_client *client;
	/*to protect the i2c read and write operation*/
	struct mutex update_lock;
	/*to protect only one thread to control the device register*/
	struct mutex single_lock;
	struct work_struct	dwork;		/* for PS interrupt */
	struct delayed_work    powerkey_work;
	wait_queue_head_t	notify_i2c_ready_event;
	struct input_dev *input_dev_ps;

	/* regulator data */
	bool power_on;
	struct regulator *vdd;
	struct regulator *vio;

	/* pinctrl data*/
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_default;
	/*sensor class for  ps*/
	struct sensors_classdev ps_cdev;
	struct txc_pa2240_platform_data *platform_data;
	/*for capture the i2c and other errors*/
	struct ls_test_excep ls_test_exception;
	struct delayed_work dsm_work;
        #ifdef CONFIG_HUAWEI_DSM
	struct delayed_work dsm_irq_work;
        #endif

	int irq;
	int count;		/* the interrupt counter*/
	/*wake lock for not losing ps event reporting*/
	struct wake_lock ps_report_wk;
	unsigned int enable;
	unsigned int pilt;
	unsigned int piht;

	/* control flag from HAL */
	unsigned int enable_ps_sensor;

	/* PS parameters */
	unsigned int ps_min_threshold; 	/*it is the min_proximity_value */
	unsigned int ps_detection;		/* 5 = near-to-far; 0 = far-to-near */
	unsigned int ps_data;/* to store PS data,it is pdata */
	unsigned int pdata_min; /*the value of Stay away from the phone's infrared hole*/
	bool saturation_flag;
	bool oil_occur;
	bool i2c_ready_flag;

	bool device_exist;
};
static struct sensors_classdev sensors_proximity_cdev = {
	.name = "pa2240-proximity",
	.vendor = "txc",
	.version = 1,
	.handle = SENSORS_PROXIMITY_HANDLE,
	.type = SENSOR_TYPE_PROXIMITY,
	.max_range = "1",
	.resolution = "1.0",
	.sensor_power = "3",
	.min_delay = 0, /* in microseconds */
	.fifo_reserved_event_count = 0,
	.fifo_max_event_count = 0,
	.enabled = 0,
	.delay_msec = 100,
	.sensors_enable = NULL,
	.sensors_poll_delay = NULL,
};

/*
 * Global data
 */
/* Proximity sensor use this work queue to report data */
static struct workqueue_struct *txc_pa2240_workqueue = NULL;
/*changeable als gain and ADC time,we don't use*/
#ifdef CONFIG_HUAWEI_DSM
static ssize_t txc_pa2240_print_reg_buf(struct device *dev,
			struct device_attribute *attr, char *buf);
static ssize_t txc_pa2240_write_reg(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count);

static struct device_attribute txc_pa2240_show_regs =
		__ATTR(dump_reg, 0440, txc_pa2240_print_reg_buf, txc_pa2240_write_reg);
#endif
static int far_init=199;
static int near_init=200;
static int txc_power_delay_flag = 1;     /*1 not always on ,0 always on*/
/*
 * Management functions,they are used to set registers in booting and enable
 */
/*init the register of device function for probe and every time the chip is powered on*/
static int txc_pa2240_init_client(struct i2c_client *client);
static int txc_pa2240_power_off_init_client(struct i2c_client *client);
static int txc_pa2240_i2c_read(struct i2c_client*client, u8 reg,bool flag);
static void operate_irq(struct txc_pa2240_data *data, int enable, bool sync);

#ifdef CONFIG_HUAWEI_DSM
static struct dsm_client *txc_pa2240_ps_dclient = NULL;
#define CLIENT_NAME_PS_PA2240		"dsm_ps_pa2240"
/* dsm client for p-sensor */
static struct dsm_dev dsm_ps_pa2240 = {
	.name 		= CLIENT_NAME_PS_PA2240,		// dsm client name
	.fops 		= NULL,						// options
	.buff_size 	= DSM_SENSOR_BUF_MAX,			// buffer size
};

static void txc_pa2240_dsm_read_regs(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;

	/*
	* read all regs to buf
	*/
	txc_pa2240_print_reg_buf(&data->client->dev,&txc_pa2240_show_regs, excep->reg_buf);
}


static int txc_pa2240_dsm_report_i2c(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;

	ssize_t size = 0;

	size = dsm_client_record(txc_pa2240_ps_dclient,
				"i2c_scl_val=%d,i2c_sda_val=%d,vdd = %d, vdd_status = %d\n"
				"vio=%d, vio_status=%d, excep_num=%d, i2c_err_num=%d\n"
				,excep->i2c_scl_val,excep->i2c_sda_val,excep->vdd_mv,excep->vdd_status
				,excep->vio_mv,excep->vio_status,excep->excep_num,excep->i2c_err_num);

	/*if device is not probe successfully or client is null, don't notify dsm work func*/
	if(!data->device_exist  || txc_pa2240_ps_dclient == NULL){
		return -ENODEV;
	}

	return size;

}

static int txc_pa2240_dsm_report_wrong_irq(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;
	int irq_gpio = data->platform_data->irq_gpio;
	ssize_t size;


	/*
	*	read  regs and irq gpio
	*/
	txc_pa2240_dsm_read_regs(data);
	excep->irq_val = gpio_get_value(irq_gpio);


	size = dsm_client_record( txc_pa2240_ps_dclient,"irq_pin = %d\n regs:%s \n",
		excep->irq_val, excep->reg_buf);

	TXC_PA2240_ERR("dsm error-> irq_pin = %d\n regs:%s\n",
				excep->irq_val, excep->reg_buf);

	return 0;

}

static int txc_pa2240_dsm_report_not_change_threshold(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;
	ssize_t size = 0;

	txc_pa2240_dsm_read_regs(data);
	excep->irq_val = gpio_get_value(data->platform_data->irq_gpio);

	size = dsm_client_record(txc_pa2240_ps_dclient, "irq_pin = %d high_thrhd = %d, low_thrhd = %d\n regs:%s",
							 excep->irq_val, excep->last_high_threshold,
							 excep->last_low_threshold, excep->reg_buf);

	TXC_PA2240_ERR("dsm error->""irq_pin = %d high_thrhd = %d, low_thrhd = %d\n regs:%s",
							 excep->irq_val, excep->last_high_threshold,
							 excep->last_low_threshold, excep->reg_buf);

	return size;
}

static int txc_pa2240_dsm_report_no_irq(struct txc_pa2240_data *data)
{
	ssize_t size = 0;

	size = txc_pa2240_dsm_report_wrong_irq(data);

	return size;
}

#if 0
static int  txc_pa2240_dsm_report_flag(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;
	ssize_t size = 0;
	txc_pa2240_dsm_read_regs(data);
	size = dsm_client_record(txc_pa2240_ps_dclient," regs:%s, enable_ps_sensor = %d\n",
		excep->reg_buf,data->enable_ps_sensor);
	TXC_PA2240_ERR("dsm error->""regs:%s,enable_ps_sensor = %d\n",
		excep->reg_buf,data->enable_ps_sensor);
	return size;
}

static int txc_pa2240_dsm_report_ps_threshold(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;
	ssize_t size = 0;
	txc_pa2240_dsm_read_regs(data);
	size = dsm_client_record(txc_pa2240_ps_dclient," regs:%s,pilt = %d,piht = %d\n",
		excep->reg_buf,data->pilt,data->piht);
	TXC_PA2240_ERR("dsm error->""regs:%s,pilt = %d,piht = %d\n",
		excep->reg_buf,data->pilt,data->piht);
	return size;
}
#endif

static int txc_pa2240_dsm_report_err(int errno,struct txc_pa2240_data *data)
{
	int size = 0;

	if(dsm_client_ocuppy(txc_pa2240_ps_dclient))
	{
		/* buffer is busy */
		TXC_PA2240_ERR("%s: buffer is busy!, errno = %d\n", __func__,errno);
		return -EBUSY;
	}

	TXC_PA2240_INFO("dsm error, errno = %d \n", errno);

	switch(errno){
		case DSM_LPS_I2C_ERROR:
			size = txc_pa2240_dsm_report_i2c(data);
			break;

		case DSM_LPS_WRONG_IRQ_ERROR:
			size = txc_pa2240_dsm_report_wrong_irq(data);
			break;

		case DSM_LPS_THRESHOLD_ERROR:
			size = txc_pa2240_dsm_report_not_change_threshold(data);
			break;

		case DSM_LPS_ENABLED_IRQ_ERROR:
			size = txc_pa2240_dsm_report_no_irq(data);
			break;

		#if 0
		case DSM_LPS_ENABLED_ERROR:
			size = txc_pa2240_dsm_report_flag(data);
			break;

		case DSM_LPS_THRESHOLD_SIZE_ERROR:
			size = txc_pa2240_dsm_report_ps_threshold(data);
			break;
		#endif

		default:
			break;

	}
	if(size > -1)
	{
		dsm_client_notify(txc_pa2240_ps_dclient, errno);
		TXC_PA2240_ERR("%s:line:%d,size = %d\n",__func__,__LINE__,size);
	}
	return size;
}

static void txc_pa2240_dsm_no_irq_check(struct txc_pa2240_data *data)
{
	struct ls_test_excep *excep = &data->ls_test_exception;


	/* add this code segment to enable ps func
	*	irq gpio status
	*/
	atomic_set(&excep->ps_enable_flag, 1);
	/*delete it, no use check_type*/

	schedule_delayed_work(&data->dsm_irq_work, msecs_to_jiffies(120));
}

static void txc_pa2240_dsm_save_threshold(struct txc_pa2240_data *data, int high, int low)
{
	struct ls_test_excep *excep = &data->ls_test_exception;

	mutex_lock(&excep->dsm_lock);
	excep->last_high_threshold = high;
	excep->last_low_threshold = low;
	mutex_unlock(&excep->dsm_lock);
}

static void txc_pa2240_dsm_no_update_threhold_check(struct txc_pa2240_data *data)
{
	schedule_delayed_work(&data->dsm_work, msecs_to_jiffies(10));
}

static void txc_pa2240_dsm_change_ps_enable_status(struct txc_pa2240_data *data)
{
	/*
	*	add this code segment to report ps event.
	*/
	if (atomic_cmpxchg(&data->ls_test_exception.ps_enable_flag, 1, 0)){
		data->ls_test_exception.ps_report_flag = 1;
	}
}

static int txc_pa2240_dump_i2c_exception_status(struct txc_pa2240_data *data)
{
	int ret = 0;
	/* print pm status and i2c gpio status*/
	struct ls_test_excep *excep = &data->ls_test_exception;

	if (data->vdd == NULL) {
		return -ENXIO;
	}

	if (data->vio == NULL) {
		return -ENXIO;
	}

	/* read i2c_sda i2c_scl gpio value*/
	mutex_lock(&data->update_lock);
	excep->i2c_scl_val = gpio_get_value(data->platform_data->i2c_scl_gpio);
	excep->i2c_sda_val = gpio_get_value(data->platform_data->i2c_sda_gpio);
	mutex_unlock(&data->update_lock);

	/* get regulator's status*/
	excep->vdd_status = regulator_is_enabled(data->vdd);
	if(excep->vdd_status < 0){
		TXC_PA2240_ERR("%s,line %d:regulator_is_enabled vdd failed\n",__func__,__LINE__);
	}
	excep->vio_status = regulator_is_enabled(data->vio);
	if(excep->vio_status < 0){
		TXC_PA2240_ERR("%s,line %d:regulator_is_enabled vio failed\n",__func__,__LINE__);
	}

	/* get regulator's value*/
	excep->vdd_mv = regulator_get_voltage(data->vdd)/1000;
	if(excep->vdd_mv < 0){
		TXC_PA2240_ERR("%s,line %d:regulator_get_voltage vdd failed\n",__func__,__LINE__);
	}

	excep->vio_mv = regulator_get_voltage(data->vio)/1000;
	if(excep->vio_mv < 0){
		TXC_PA2240_ERR("%s,line %d:regulator_get_voltage vio failed\n",__func__,__LINE__);
	}

	/* report i2c err info */
	ret = txc_pa2240_dsm_report_err(DSM_LPS_I2C_ERROR,data);

	TXC_PA2240_INFO("%s,line %d:i2c_scl_val=%d,i2c_sda_val=%d,vdd = %d, vdd_status = %d\n"
			"vio=%d, vio_status=%d, excep_num=%d, i2c_err_num=%d",__func__,__LINE__
			,excep->i2c_scl_val,excep->i2c_sda_val,excep->vdd_mv,excep->vdd_status
			,excep->vio_mv,excep->vio_status,excep->excep_num,excep->i2c_err_num);

	excep->i2c_err_num = 0;

	return ret;

}


static void txc_pa2240_report_i2c_info(struct txc_pa2240_data* data, int ret)
{
	data->ls_test_exception.i2c_err_num = ret;
	txc_pa2240_dump_i2c_exception_status(data);
}

/*************************************************
  Function:        txc_pa2240_dsm_irq_excep_work
  Description:    this func is to report dsm err, no irq occured
                       after ps enabled
  Input:            work
  Output:          none
  Return:          0
*************************************************/
static void txc_pa2240_dsm_irq_excep_work(struct work_struct *work)
{
	struct txc_pa2240_data *data =
		container_of((struct delayed_work *)work, struct txc_pa2240_data, dsm_irq_work);

	struct ls_test_excep *excep = &data->ls_test_exception;

			if(!excep->ps_report_flag){
				/*
				* report dsm err, no irq occured after ps enabled.
				*/
				txc_pa2240_dsm_report_err(DSM_LPS_ENABLED_IRQ_ERROR,data);

			}
}

static void txc_pa2240_excep_work(struct work_struct *work)
{
	struct txc_pa2240_data *data =
		container_of((struct delayed_work *)work, struct txc_pa2240_data, dsm_work);
	struct i2c_client *client = data->client;
	int high_threshold;
	int low_threshold;

	/*
	*	read high and low threshold, save them.
	*/
	//low_threshold = txc_pa2240_i2c_read(client,APDS993X_PILTL_REG,APDS993X_I2C_WORD);
	//high_threshold = txc_pa2240_i2c_read(client,APDS993X_PIHTL_REG,APDS993X_I2C_WORD);
	low_threshold = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_TL,TXC_PA2240_I2C_BYTE);
	high_threshold = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_TH,TXC_PA2240_I2C_BYTE);
	txc_pa2240_dsm_save_threshold(data, high_threshold, low_threshold);

	/*
	* report dsm err, high and low threshold don't changed after ps irq.
	*/
	txc_pa2240_dsm_report_err(DSM_LPS_THRESHOLD_ERROR,data);


}


static int txc_pa2240_dsm_init(struct txc_pa2240_data *data)
{
	txc_pa2240_ps_dclient = dsm_register_client(&dsm_ps_pa2240);
	if (!txc_pa2240_ps_dclient) {
		TXC_PA2240_ERR("%s@%d register dsm txc_pa2240_ps_dclient failed!\n",__func__,__LINE__);
		return -ENOMEM;
	}
	/*for dmd */
	//txc_pa2240_ps_dclient->driver_data = data;

	data->ls_test_exception.reg_buf = kzalloc(512, GFP_KERNEL);
	if(!data->ls_test_exception.reg_buf){
		TXC_PA2240_ERR("%s@%d alloc dsm reg_buf failed!\n",__func__,__LINE__);
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&data->dsm_work, txc_pa2240_excep_work);
	INIT_DELAYED_WORK(&data->dsm_irq_work, txc_pa2240_dsm_irq_excep_work);
	mutex_init(&data->ls_test_exception.dsm_lock);

	return 0;
}

static void txc_pa2240_dsm_exit(void)
{
	dsm_unregister_client(txc_pa2240_ps_dclient,&dsm_ps_pa2240);
}

static ssize_t txc_pa2240_sysfs_dsm_test(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	long mode;
	int ret = 0;

	if (strict_strtol(buf, 10, &mode))
			return -EINVAL;

	switch(mode){
		case DSM_LPS_I2C_ERROR:
			ret = txc_pa2240_dump_i2c_exception_status(data);
			break;
		case DSM_LPS_WRONG_IRQ_ERROR:
		case DSM_LPS_THRESHOLD_ERROR:
		case DSM_LPS_ENABLED_IRQ_ERROR:
			ret = txc_pa2240_dsm_report_err(mode, data);
			break;

		default:
			TXC_PA2240_ERR("%s unsupport err_no = %ld \n", __func__, mode);
			break;

	}

	return ret;
}

static DEVICE_ATTR(dsm_excep,S_IWUSR|S_IWGRP, NULL, txc_pa2240_sysfs_dsm_test);

#endif


static ssize_t txc_pa2240_write_file(char *filename, char* param)
{
	struct file  *fop;
	mm_segment_t old_fs;

	if(NULL == filename)
	{
		TXC_PA2240_ERR("filename is empty\n");
		return -1;
	}
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	fop = filp_open(filename,O_CREAT | O_RDWR,0660);
	if (IS_ERR_OR_NULL(fop))
	{
		set_fs(old_fs);
		TXC_PA2240_ERR("Create file error!! Path = %s\n",filename);
		return -1;
	}
	
	vfs_write(fop, (char *)param, strlen(param), &fop->f_pos);

	filp_close(fop,NULL);
	set_fs(old_fs);
	return 0;
}

/*we use the unified the function for i2c write and read operation*/
static int txc_pa2240_i2c_write(struct i2c_client*client, u8 reg, u16 value,bool flag)
{
	int err,loop;

	struct txc_pa2240_data *data = i2c_get_clientdata(client);

	loop = TXC_PA2240_I2C_RETRY_COUNT;
	/*we give three times to repeat the i2c operation if i2c errors happen*/
	while(loop) {
		mutex_lock(&data->update_lock);
		/*0 is i2c_smbus_write_byte_data,1 is i2c_smbus_write_word_data*/
		if(flag == TXC_PA2240_I2C_BYTE)
		{
			err = i2c_smbus_write_byte_data(client, reg, (u8)value);
		}
		else if(flag == TXC_PA2240_I2C_WORD)
		{
			err = i2c_smbus_write_word_data(client, reg, value);
		}
		mutex_unlock(&data->update_lock);
		if(err < 0){
			loop--;
			msleep(TXC_PA2240_I2C_RETRY_TIMEOUT);
		}
		else
			break;
	}
	/*after three times,we print the register and regulator value*/
	if(loop == 0){
		TXC_PA2240_ERR("%s,line %d:attention:i2c write err = %d\n",__func__,__LINE__,err);
#ifdef CONFIG_HUAWEI_DSM
		if (data->device_exist){
			txc_pa2240_report_i2c_info(data,err);
		}
#endif
	}

	return err;
}

static int txc_pa2240_i2c_read(struct i2c_client*client, u8 reg,bool flag)
{
	int err,loop;

	struct txc_pa2240_data *data = i2c_get_clientdata(client);

	loop = TXC_PA2240_I2C_RETRY_COUNT;
	/*we give three times to repeat the i2c operation if i2c errors happen*/
	while(loop) {
		mutex_lock(&data->update_lock);
		/*0 is i2c_smbus_read_byte_data,1 is i2c_smbus_read_word_data*/
		if(flag == TXC_PA2240_I2C_BYTE)
		{
			err = i2c_smbus_read_byte_data(client, reg);
		}
		else if(flag == TXC_PA2240_I2C_WORD)
		{
			err = i2c_smbus_read_word_data(client, reg);
		}
		mutex_unlock(&data->update_lock);
		if(err < 0){
			loop--;
			msleep(TXC_PA2240_I2C_RETRY_TIMEOUT);
		}
		else
			break;
	}
	/*after three times,we print the register and regulator value*/
	if(loop == 0){
		TXC_PA2240_ERR("%s,line %d:attention: i2c read err = %d,reg=0x%x\n",__func__,__LINE__,err,reg);
#ifdef CONFIG_HUAWEI_DSM
		if (data->device_exist){
			txc_pa2240_report_i2c_info(data,err);
		}
#endif
	}

	return err;
}


static int txc_pa2240_get_pscrosstalk(struct i2c_client *client)
{
	int ret = 0;
	
	ret = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_OFFSET, TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		TXC_PA2240_ERR("%s,line %d:read TXC OFFSET reg failed\n",__func__,__LINE__);
	}

	TXC_PA2240_FLOW("%s,line %d: read TXC OFFSET reg : %d\n", __func__, __LINE__, ret);
	return ret;
}

/*
*	print the registers value with proper format
*/
static int dump_reg_buf(struct txc_pa2240_data *data,char *buf, int size,int enable)
{
	int i=0;

	mutex_lock(&data->update_lock);

	if(enable)
		TXC_PA2240_INFO("[enable]");
	else
		TXC_PA2240_INFO("[disable]");
	TXC_PA2240_INFO(" reg_buf= ");
	for(i = 0;i < size; i++){
		TXC_PA2240_INFO("0x%2x  ",buf[i]);
	}
	mutex_unlock(&data->update_lock);

	TXC_PA2240_INFO("\n");
	return 0;
}
static int txc_pa2240_regs_debug_print(struct txc_pa2240_data *data,int enable)
{
	int i=0;
	char reg_buf[TXC_PA2240_REG_LEN];
	struct i2c_client *client = data->client;

	/* read registers[0x0~0x1a] value*/
	for(i = 0; i < TXC_PA2240_REG_LEN; i++ )
	{

		reg_buf[i] = txc_pa2240_i2c_read(client,i,TXC_PA2240_I2C_BYTE);

		if(reg_buf[i] <0){
			TXC_PA2240_ERR("%s,line %d:read %d reg failed\n",__func__,__LINE__,i);
			return reg_buf[i] ;
		}
	}

	/* print the registers[0x0~0x1a] value in proper format*/
	dump_reg_buf(data,reg_buf,TXC_PA2240_REG_LEN,enable);

	return 0;
}

static int txc_pa2240_set_cfg0(struct i2c_client *client, int enable)
{
	int ret;

	ret = txc_pa2240_i2c_write(client, TXC_PA2240_REG_CFG0, enable,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error,enable = %d\n",__func__,__LINE__,enable);
		return ret;
	}
	TXC_PA2240_FLOW("%s,line %d:txc_pa2240 enable = %d\n",__func__,__LINE__,enable);
	return ret;
}

static int txc_pa2240_set_cfg1(struct i2c_client *client, int cfg)
{
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG1, cfg,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error,cfg1 = %d\n",__func__,__LINE__,cfg);
		return ret;
	}

	TXC_PA2240_FLOW("%s,line %d:txc_pa2240 cfg1 = %d\n",__func__,__LINE__,cfg);
	return ret;
}

static int txc_pa2240_set_cfg2(struct i2c_client *client, int cfg)
{
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG2, cfg,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error,cfg2 = %d\n",__func__,__LINE__,cfg);
		return ret;
	}

	TXC_PA2240_FLOW("%s,line %d:txc_pa2240 cfg2 = %d\n",__func__,__LINE__,cfg);
	return ret;
}

static int txc_pa2240_set_pilt(struct i2c_client *client, int threshold)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TL, threshold,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error,threshold = %d\n",__func__,__LINE__,threshold);
		return ret;
	}

	data->pilt = threshold;
	TXC_PA2240_INFO("%s,line %d:set txc_pa2240 pilt =%d\n", __func__, __LINE__,threshold);

	return ret;
}

static int txc_pa2240_set_piht(struct i2c_client *client, int threshold)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TH, threshold,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error,threshold = %d\n",__func__,__LINE__,threshold);
		return ret;
	}

	data->piht = threshold;
	TXC_PA2240_INFO("%s,line %d:set txc_pa2240 piht =%d\n", __func__,__LINE__,threshold);

	return ret;
}

static int txc_pa2240_set_cfg3(struct i2c_client *client, int config)
{
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG3, config,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error,config3 = %d\n",__func__,__LINE__,config);
		return ret;
	}

	TXC_PA2240_FLOW("%s,line %d:txc_pa2240 config3 = %d\n",__func__,__LINE__,config);
	return ret;
}

static int txc_pa2240_set_ps(struct i2c_client *client,int config)
{
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_SET, config,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error, set ps set err\n",__func__,__LINE__);
		return ret;
	}

	TXC_PA2240_FLOW("%s,line %d:txc_pa2240, set ps  set success\n",__func__,__LINE__);
	return ret;
}

static int txc_pa2240_get_ps(struct i2c_client *client)
{
	int ret;

	ret = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_SET, TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error, get ps err\n",__func__,__LINE__);
		return ret;
	}

	TXC_PA2240_FLOW("%s,line %d:txc_pa2240, get ps success\n",__func__,__LINE__);
	return ret;
}

static int txc_pa2240_set_flct(struct i2c_client *client,int config)
{
	int ret;

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_FLCT, config,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:i2c error, set ps flct set err\n",__func__,__LINE__);
		return ret;
	}

	TXC_PA2240_FLOW("%s,line %d:txc_pa2240, set ps flct set success\n",__func__,__LINE__);
	return ret;
}


static void txc_pa2240_dump_register(struct i2c_client *client)
{
	int reg_cfg0,reg_cfg1,reg_cfg2,reg_ps_tl,reg_ps_th,reg_ps_data,reg_ps_offset;
	reg_cfg0 = txc_pa2240_i2c_read(client, TXC_PA2240_REG_CFG0,TXC_PA2240_I2C_BYTE);
	reg_cfg1= txc_pa2240_i2c_read(client, TXC_PA2240_REG_CFG1,TXC_PA2240_I2C_BYTE);
	reg_cfg2 =txc_pa2240_i2c_read(client, TXC_PA2240_REG_CFG2,TXC_PA2240_I2C_BYTE);
	reg_ps_tl=txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_TL,TXC_PA2240_I2C_BYTE);
	reg_ps_th=txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_TH,TXC_PA2240_I2C_BYTE);
	reg_ps_data = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_DATA,TXC_PA2240_I2C_BYTE);
	reg_ps_offset= txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_OFFSET,TXC_PA2240_I2C_BYTE);

	TXC_PA2240_INFO("%s,line %d:reg_cfg0 = 0x%x,reg_cfg1=0x%x,reg_cfg2=0x%x\n",__func__,__LINE__,reg_cfg0,reg_cfg1,reg_cfg2);
	TXC_PA2240_INFO("%s,line %d:reg_ps_tl = 0x%x,reg_ps_th=0x%x,reg_ps_data=0x%x\n",__func__,__LINE__,reg_ps_tl,reg_ps_th,reg_ps_data);
	TXC_PA2240_INFO("%s,line %d:reg_ps_offset = 0x%x;\n",__func__,__LINE__,reg_ps_offset);
}
static void txc_pa2240_ps_report_event(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	int ret;

	ret = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_DATA,TXC_PA2240_I2C_BYTE);
	if( ret < 0 )
	{
	/* the number "200" is a value to make sure there is a valid value */
		data->ps_data = 200 ;
		TXC_PA2240_ERR("%s, line %d: pdate<0, reset to %d\n", __func__, __LINE__, data->ps_data);
	}else{
		data->ps_data = ret ;
	}

	if(data->ps_data < PA2240_PDATA_SATURATION_VAL)
	{
		TXC_PA2240_ERR("%s: pa224 saturation happen\n", __func__);
		data->saturation_flag = true;
		if(data->ps_detection == TXC_PA2240_CLOSE_FLAG)
		{
			input_report_abs(data->input_dev_ps, ABS_DISTANCE, TXC_PA2240_FAR_FLAG);
			input_sync(data->input_dev_ps);
			data->ps_detection= TXC_PA2240_FAR_FLAG;
			TXC_PA2240_ERR("%s: sunlight report far event, data->ps_data:%d\n", __func__,data->ps_data);
			/*Change high threshld to detect close event also report far event*/
			data->piht = min(far_ps_min + oil_effect, PA2240_PDATA_MAX_HIGH_TH - oil_effect);
			ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TH, data->piht, TXC_PA2240_I2C_BYTE);
			if (ret < 0){
				TXC_PA2240_ERR("%s,line %d:data->pilt = %d,data->piht=%d, i2c wrong\n",__func__,__LINE__,data->pilt,data->piht);
			}
			TXC_PA2240_INFO("%s,line %d:change threshoid,data->ps_data =%d,data->pilt=%d,data->piht=%d,far_ps_min=%d\n",
				__func__,__LINE__,data->ps_data, data->pilt, data->piht,far_ps_min);
		}
		/*Prevent too many interruptions under sunlight*/
		msleep(50);
		return;
	}

	TXC_PA2240_FLOW("%s,line %d:TXC PA2240 ps_data=%d middle_threshold=%d calibration_threshold=%d\n",__func__,__LINE__,data->ps_data,middle_threshold,calibration_threshold);
	TXC_PA2240_FLOW("%s,line %d:TXC PA2240 low_threshold=%d high_threshold=%d oil_effect=%d pdata->ir_current=%d\n",__func__,__LINE__,low_threshold,high_threshold,oil_effect,pdata->ir_current);
	/*
	 *	Status is far and object is moving away or crosstalk is getting smaller
	 */
	TXC_PA2240_INFO("%s:%d data->ps_data =%d,data->pilt=%d,data->piht=%d,far_ps_min=%d, pdata_min=%d, data->ps_detection=%d, oil_occur=%d.\n",
		__func__,__LINE__,data->ps_data, data->pilt, data->piht,far_ps_min, data->pdata_min, data->ps_detection, data->oil_occur);
	/*To avoid middle value of ps data under sunlight*/
	if(data->saturation_flag){
		data->saturation_flag = false;
		msleep(20);
		return;
	}

	if(data->ps_data <= data->pilt){

		if (data->ps_detection == TXC_PA2240_CLOSE_FLAG){
			data->ps_detection = TXC_PA2240_FAR_FLAG;
			input_report_abs(data->input_dev_ps, ABS_DISTANCE, TXC_PA2240_FAR_FLAG);
			input_sync(data->input_dev_ps);
			TXC_PA2240_INFO("%s,line %d:PROXIMITY far event\n", __func__,__LINE__);
		}
		data->pdata_min = min(data->pdata_min, data->ps_data);

#ifdef CONFIG_HUAWEI_DSM
		txc_pa2240_dsm_change_ps_enable_status(data);
#endif

		/*If user touch TP or oil occure or user continously move away, update far_ps_min*/
		if((data->oil_occur) || (far_ps_min > data->ps_data)){
			far_ps_min = data->ps_data;
			data->oil_occur = false;
		}
		data->pilt = far_ps_min -low_threshold;
		data->piht = far_ps_min + high_threshold;
	}else if (data->ps_data >= data->piht){
		/*
		*	Status is far and object is moving close or status is near and object is moving closer
		*	It is very important to detect object moving closer in both near adn far state
		*/
		if (data->ps_detection == TXC_PA2240_FAR_FLAG){
			data->ps_detection = TXC_PA2240_CLOSE_FLAG;
			input_report_abs(data->input_dev_ps, ABS_DISTANCE, TXC_PA2240_CLOSE_FLAG);
			input_sync(data->input_dev_ps);
			TXC_PA2240_INFO("%s,line %d:PROXIMITY close event\n", __func__,__LINE__);
		}

#ifdef CONFIG_HUAWEI_DSM
		txc_pa2240_dsm_change_ps_enable_status(data);
#endif
         /*User is not close to TP*/
		if (data->ps_data < PA2240_PDATA_TOUCH_VAL){
			data->pilt = ((far_ps_min + middle_threshold) < data->ps_data)  ?  (far_ps_min + middle_threshold) : (data->ps_data -oil_effect);
			data->piht = PA2240_PDATA_TOUCH_VAL;
		/*User is very close to TP, oil may occured*/
		}else{
			data->pilt = min(data->pdata_min + oil_effect, (PA2240_PDATA_MAX_HIGH_TH -oil_effect));
			data->piht = PA2240_PDATA_MAX_HIGH_TH;
			data->oil_occur = true;
		}
	}else {
#ifdef CONFIG_HUAWEI_DSM
		txc_pa2240_dsm_no_update_threhold_check(data);
#endif

		TXC_PA2240_ERR("%s:%d read pdata Not within reasonable limits,data->pilt=%d,data->piht=%d data->ps_data=%d\n",
			__FUNCTION__,__LINE__,data->pilt,data->piht,data->ps_data);
	}

	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TH, data->piht, TXC_PA2240_I2C_BYTE);
	ret += txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TL, data->pilt, TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		TXC_PA2240_ERR("%s,line %d:data->pilt = %d,data->piht=%d, i2c wrong\n",__func__,__LINE__,data->pilt,data->piht);
		goto exit;
	}

	TXC_PA2240_INFO("%s,line %d:change threshoid,data->ps_data =%d,data->pilt=%d,data->piht=%d,far_ps_min=%d, pdata_min=%d, oil_occur=%d\n",
		__func__,__LINE__,data->ps_data, data->pilt, data->piht,far_ps_min, data->pdata_min, data->oil_occur);
	
	return ;
exit:
	/*if i2c error happens,we report far event*/
	if(data->ps_detection == TXC_PA2240_CLOSE_FLAG){
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, TXC_PA2240_FAR_FLAG);
		input_sync(data->input_dev_ps);
		data->ps_detection= TXC_PA2240_FAR_FLAG;
		TXC_PA2240_ERR("%s:i2c error happens, report far event, data->ps_data:%d\n", __func__,data->ps_data);
		return ;
	}
}

/* PS interrupt routine */

static void txc_pa2240_work_handler(struct work_struct *work)
{
	struct txc_pa2240_data *data = container_of(work, struct txc_pa2240_data, dwork);
	struct i2c_client *client=data->client;
	int reg_cfg2;
	int ret;
	int pdata;
	int status;

	status = wait_event_timeout(data->notify_i2c_ready_event,
		(data->i2c_ready_flag == true),
		msecs_to_jiffies(TXC_PA2240_I2C_RESUME_TIMEOUT));
	if (status == 0) {
		data->i2c_ready_flag = true;
		TXC_PA2240_ERR("%s: Failed to wait for I2C bus ready\n", __func__);
	}

	wake_lock_timeout(&data->ps_report_wk, PS_WAKEUP_TIME);
	mutex_lock(&data->single_lock);
	ret = txc_pa2240_get_ps(client);
	if (ret != 0x82) {
		TXC_PA2240_ERR("%s line %d: instant power off, REG_PS_SET=0x%02x\n", __func__, __LINE__, ret);
		ret = txc_pa2240_power_off_init_client(client);
		if (ret) {
			TXC_PA2240_ERR("%s: Failed to reinit txc_pa2240 client\n", __func__);
		}
	}
	pdata = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_DATA,TXC_PA2240_I2C_BYTE);
	reg_cfg2 = txc_pa2240_i2c_read(client, TXC_PA2240_REG_CFG2,TXC_PA2240_I2C_BYTE);
	if(pdata == (TXC_PA2240_PROX_MAX_ADC_VALUE + 1))
	{
		txc_pa2240_dump_register(client);
	}

	if (reg_cfg2 & TXC_PA2240_PS_INT_ACTIVE) {
		txc_pa2240_ps_report_event(client);		
	} else{
		TXC_PA2240_ERR("%s,line %d:wrong interrupts,TXC_PA2240_REG_CFG2 is 0X%x\n",__func__,__LINE__,reg_cfg2);
	}

	/*  CLR PS INT */
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG2, (reg_cfg2 & (~TXC_PA2240_PS_INT_ACTIVE)),TXC_PA2240_I2C_BYTE);
	mutex_unlock(&data->single_lock);
	if (ret < 0)
	{
		TXC_PA2240_ERR("%s,%d:i2c error happens, pls clear irq flag ,ret = %d\n", __func__,__LINE__,ret);
		return ;
	}
	if (data->irq)
	{
		operate_irq(data,1,true);
	}
#ifdef CONFIG_SENSOR_DEVELOP_TEST
	if(sensorDT_mode)
	{
		ps_data_count++;
	}
#endif
}

void operate_irq(struct txc_pa2240_data *data, int enable, bool sync)
{
	if (data->irq)
	{
		if(enable)
		{
			/*Avoid competitive access problems*/
			data->count++;
			enable_irq(data->irq);
		}
		else
		{
			if(data->count > 0)
			{
				if(sync)
				{
					disable_irq(data->irq);
				}
				else
				{
					disable_irq_nosync(data->irq);
				}
				data->count--;
			}
		}
	}
}

/* assume this is ISR */
static irqreturn_t txc_pa2240_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	/*in 400ms,system keeps in wakeup state to avoid the sleeling system lose the pls event*/
	operate_irq(data,0,false);
	wake_lock_timeout(&data->ps_report_wk, PS_WAKEUP_TIME);
	queue_work(txc_pa2240_workqueue, &data->dwork);

	return IRQ_HANDLED;
}

static void txc_pa2240_swap(int *x, int *y)
{
        int temp = *x;
        *x = *y;
        *y = temp;
}

static int pa224_run_calibration(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	int i, j;	
	int ret;
	u16 sum_of_pdata = 0;
	u8  cfg0data = 0, cfg2data = 0;
	int temp_pdata[20];
	unsigned int ArySize = 20;
	unsigned int cal_check_flag = 0;
	char buftemp[20] = "\0";
	TXC_PA2240_INFO("%s: START proximity sensor calibration\n", __func__);
	pdata->flag = PA2240_CALIBRATION_SUCCESS;
	if (data->platform_data->power_on)
		if(0==data->enable_ps_sensor)
			data->platform_data->power_on(true,data);
RECALIBRATION:
	sum_of_pdata = 0;
	/* Prevent interrput */
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TH,0xFF,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TL,0x00,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	/*Offset mode & disable intr from ps*/
	cfg2data = txc_pa2240_i2c_read(client,TXC_PA2240_REG_CFG2,TXC_PA2240_I2C_BYTE);
	if(cfg2data < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG2,0x08,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	/*Set crosstalk = 0*/	
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_OFFSET,0x00,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	/*PS On*/
	cfg0data = txc_pa2240_i2c_read(client,TXC_PA2240_REG_CFG0,TXC_PA2240_I2C_BYTE);	
	if(cfg0data < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG0,cfg0data | 0x02,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	for (i = 0; i < 20; i++)
	{
		msleep(50);
		temp_pdata[i] = txc_pa2240_i2c_read(client,TXC_PA2240_REG_PS_DATA,TXC_PA2240_I2C_BYTE);
		if(temp_pdata[i] < 0)
		{
			pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
			TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
			return -EINVAL;
		}
		TXC_PA2240_INFO("temp_data = %d\n", temp_pdata[i]);	
	}	
	
	/* pdata sorting */
	for (i = 0; i < ArySize - 1; i++)
	for (j = i+1; j < ArySize; j++)
		if (temp_pdata[i] > temp_pdata[j])
			txc_pa2240_swap(temp_pdata + i, temp_pdata + j);	
	
	/* calculate the cross-talk using central 10 data */
	for (i = 5; i < 15; i++) 
	{
		TXC_PA2240_INFO("%s: temp_pdata = %d\n", __func__, temp_pdata[i]);
		sum_of_pdata = sum_of_pdata + temp_pdata[i];
	}

	pdata->crosstalk = sum_of_pdata/10;
	TXC_PA2240_INFO("%s: sum_of_pdata = %d   cross_talk = %d\n",
                        __func__, sum_of_pdata, pdata->crosstalk);
	
	/* Restore CFG2  and Measure base x-talk */
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG2,cfg2data,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	if (pdata->crosstalk > calibration_threshold)
	{
		TXC_PA2240_INFO("%s: invalid calibrated data,pdata->crosstalk = %d\n", __func__,pdata->crosstalk);
		if(cal_check_flag == 0)
		{
			TXC_PA2240_INFO("%s: RECALIBRATION start\n", __func__);
			cal_check_flag = 1;
			goto RECALIBRATION;
		}
		else
		{
			TXC_PA2240_INFO("%s: CALIBRATION FAIL -> "
                               "cross_talk is set to DEFAULT\n", __func__);
			pdata->crosstalk = pdata ->defalt_crosstalk;
			ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG0,cfg0data,TXC_PA2240_I2C_BYTE);
			if(ret < 0)
			{
				pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
				TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
				return -EINVAL;
			}
			pdata->flag = PA2240_CALIBRATION_ERROR;
			return -EINVAL;
             }
	}	
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG0,cfg0data,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		pdata->flag = PA2240_CALIBRATION_I2C_ERROR;
		TXC_PA2240_ERR("%s,line %d\n",__func__,__LINE__);
		return -EINVAL;
	}
	/*the file will store two number
	*  want to split two numbers add "  " 
	*  this place store the pdata->crosstalk of calibration 
	*/
	snprintf(buftemp, MAX_BUF_LEN,"%d  ", pdata->crosstalk);
	TXC_PA2240_INFO("%s:%d calibration end buftemp=%s\n",__func__,__LINE__,buftemp);
	
	TXC_PA2240_INFO("%s: FINISH proximity sensor calibration\n", __func__);

	if(txc_pa2240_write_file(TXC_PA2240_PS_CAL_FILE_PATH,buftemp) < 0)
	{
		TXC_PA2240_ERR("Open PS x-talk calibration file error!!");
	}
	else
	{
		TXC_PA2240_INFO("Open PS x-talk calibration file Success!!");
	}
	
	TXC_PA2240_INFO("%s,line %d:CALIBRATION SUCCESS\n",__func__,__LINE__);
	if (data->platform_data->power_on)
		if(0==data->enable_ps_sensor)
			data->platform_data->power_on(false,data);
	return pdata->crosstalk;	
}

static int txc_pa2240_run_fast_calibration(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	int ret =0;
	int i = 0;
	int j = 0;	
	int sum_of_pdata = 0;
	int  xtalk_temp = 0;
    	int temp_pdata[4];
   	unsigned int ArySize = 4;
	u8 cfg0data = 0, cfg2data = 0;
	
   	TXC_PA2240_INFO("func:%s, line:%d,START proximity sensor calibration\n", __func__, __LINE__);

	/*Offset mode & disable intr from ps*/
	cfg2data = txc_pa2240_i2c_read(client,TXC_PA2240_REG_CFG2,TXC_PA2240_I2C_BYTE);
	ret = txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG2,0x08,TXC_PA2240_I2C_BYTE);

	/*Set crosstalk = 0*/
	cfg0data = txc_pa2240_i2c_read(client,TXC_PA2240_REG_CFG0,TXC_PA2240_I2C_BYTE);
	ret +=txc_pa2240_i2c_write(client, TXC_PA2240_REG_PS_OFFSET, 0x00, TXC_PA2240_I2C_BYTE);

	/*PS On*/
	ret += txc_pa2240_i2c_write(client, TXC_PA2240_REG_CFG0,0x02, TXC_PA2240_I2C_BYTE); 
	if(ret < 0)
	{
		TXC_PA2240_ERR("func:%s, line:%d, set TXC CHIP fail( %d)\n", __func__, __LINE__, ret);
		return ret;
	}
	
	for (i = 0; i < 4; i++)
	{
		msleep(50);
		ret = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_DATA,TXC_PA2240_I2C_BYTE);
		if(ret < 0)
		{
			TXC_PA2240_ERR("func:%s, line:%d,read ps data err( %d)\n", __func__, __LINE__, ret);
			return ret;
		}else{
			temp_pdata[i] = ret; 
		}
		TXC_PA2240_FLOW("func:%s, line:%d,temp_data = %d\n", __func__, __LINE__, temp_pdata[i]);	
	}	
	
	
	/* pdata sorting */
	for (i = 0; i < ArySize - 1; i++)
		for (j = i+1; j < ArySize; j++)
			if (temp_pdata[i] > temp_pdata[j])
			txc_pa2240_swap(temp_pdata + i, temp_pdata + j);	
	
	/* calculate the cross-talk using central 10 data */
	for (i = 1; i < 3; i++) 
	{
		TXC_PA2240_FLOW("%s: temp_pdata = %d\n", __func__, temp_pdata[i]);
		sum_of_pdata = sum_of_pdata + temp_pdata[i];
	}

	xtalk_temp = sum_of_pdata/2;

	xtalk_temp += PA2240_PS_OFFSET_EXTRA;
	TXC_PA2240_INFO("%s:%d pdata->defalt_crosstalk=%d pdata->crosstalk=%d xtalk_temp=%d\n",__func__, __LINE__, pdata->defalt_crosstalk,pdata->crosstalk,xtalk_temp);
	if ((xtalk_temp < pdata->crosstalk + 30) && (xtalk_temp > pdata->crosstalk))
	{
		/*calibration success*/
		TXC_PA2240_FLOW("%s:%d Fast calibrated data=%d\n", __func__, __LINE__,xtalk_temp);
	}
	else
	{
		/*calibration failure*/
		TXC_PA2240_ERR("%s:Fast calibration fail, use factory crosstalk = %d \n", __func__, pdata->crosstalk);
		/*dont update crosstalk, used lasted crosstalk val*/
		xtalk_temp = pdata->crosstalk;
	}   

   	TXC_PA2240_INFO("%s: sum_of_pdata = %d   xtalk_temp = %d\n",
                        __func__, sum_of_pdata, xtalk_temp);

	ret = txc_pa2240_i2c_write(client, TXC_PA2240_REG_PS_OFFSET, xtalk_temp, TXC_PA2240_I2C_BYTE);
	ret += txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG2,cfg2data | 0x40,TXC_PA2240_I2C_BYTE);
	ret += txc_pa2240_i2c_write(client,TXC_PA2240_REG_CFG0,cfg0data,TXC_PA2240_I2C_BYTE);
	if(ret < 0)
	{
		TXC_PA2240_ERR("func:%s, line:%d,set PS calibration fail\n", __func__, __LINE__);
		return ret;
	}
	
	TXC_PA2240_INFO("func:%s, line:%d,FINISH PS calibration\n", __func__, __LINE__);
		
	return 0;

}

static int txc_pa2240_open_ps_sensor(struct txc_pa2240_data *data, struct i2c_client *client)
{
	int ret = 0;
	/* turn on p sensor */
	if (data->enable_ps_sensor==0) {
		/* Power on and initalize the device */
		if (data->platform_data->power_on)
			data->platform_data->power_on(true,data);

		/*must calibration ps offset before use the device */
		/*delete this because this algorithm can not resolve the oil problem*/
		/*
		ret = txc_pa2240_run_fast_calibration(client);
		if(ret < 0)
		{
			TXC_PA2240_ERR("%s:line:%d,Failed to set txc_pa2240 ps offset\n", __func__, __LINE__);
			return ret;
		}

		ret = txc_pa2240_init_client(client);
		if (ret) {
			TXC_PA2240_ERR("%s:line:%d,Failed to init txc_pa2240\n", __func__, __LINE__);
			return ret;
		}
		*/
		data->enable_ps_sensor= 1;
		data->saturation_flag = false;
		data->oil_occur = false;
		/*initialize the ps_min_threshold,to update data->piht and data->pilt*/
#ifdef CONFIG_HUAWEI_DSM
		txc_pa2240_dsm_save_threshold(data, far_init, near_init);
#endif
		ret = txc_pa2240_get_ps(client);
		if (ret != 0x82) {
		TXC_PA2240_ERR("%s line %d: instant power off, REG_PS_SET=0x%02x\n", __func__, __LINE__, ret);
		ret = txc_pa2240_init_client(client);
		if (ret) {
			TXC_PA2240_ERR("%s: Failed to reinit txc_pa2240 client\n", __func__);
		}
	}

//		txc_pa2240_set_flct(client,TXC_PA2240_FILTER);
		data->enable = TXC_PA2240_PS_ACTIVE;
		txc_pa2240_set_cfg0(client, data->enable);
		msleep(PA24_PS_ENABLE_DELAY);
//		txc_pa2240_set_flct(client,(TXC_PA2240_FILTER | (PA2240_PS_FLCT<<4)));
		TXC_PA2240_INFO("%s: line:%d,enable pls sensor.data->enable = 0x%x\n", __func__, __LINE__,data->enable);

		data->ps_detection = TXC_PA2240_FAR_FLAG;
		/*first report event  0 is close, 1 is far */
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, TXC_PA2240_FAR_FLAG);
		input_sync(data->input_dev_ps);
		TXC_PA2240_INFO("%s,line %d: enable ps, report ABS_DISTANCE, far event\n", __func__,__LINE__);

		far_ps_min = 255 - oil_effect;
		data->pdata_min = far_ps_min;
		ret = txc_pa2240_set_pilt(client, far_init);
		ret += txc_pa2240_set_piht(client, near_init);
		if (ret < 0){
			return ret;
		}
		TXC_PA2240_INFO("%s,line %d:data->pilt=%d,data->piht=%d,\n",
			__func__,__LINE__,data->pilt,data->piht);

		if (data->irq)
		{
			operate_irq(data,1,true);
			/*set the property of pls irq,so the pls irq can wake up the sleeping system */
			irq_set_irq_wake(data->irq, 1);
		}
	}
	return ret;
}
static int txc_pa2240_enable_ps_sensor(struct i2c_client *client,unsigned int val)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	int ret;

	TXC_PA2240_INFO("%s,line %d:val=%d\n",__func__,__LINE__,val);
	if ((val != 0) && (val != 1)) {
		TXC_PA2240_ERR("%s: invalid value=%d\n", __func__, val);
		return -EINVAL;
	}
	if (val == 1) {
		mutex_lock(&data->single_lock);
		ret = txc_pa2240_open_ps_sensor(data, client);
		mutex_unlock(&data->single_lock);
		if(ret)
		{
			TXC_PA2240_ERR("%s,line %d:read power_value failed,open ps fail\n",__func__,__LINE__);
			return ret;
		}
#ifdef CONFIG_HUAWEI_DSM
        txc_pa2240_dsm_no_irq_check(data);
#endif

		power_key_ps = false;
		schedule_delayed_work(&data->powerkey_work, msecs_to_jiffies(100));
	} else {

		if (data->enable_ps_sensor==1) {

			mutex_lock(&data->single_lock);
			
			data->enable_ps_sensor = 0;
			data->enable= 0x00;
			txc_pa2240_set_cfg0(client, data->enable);

			mutex_unlock(&data->single_lock);
			TXC_PA2240_INFO("%s: line:%d,disable pls sensor,data->enable = 0x%x\n", __func__, __LINE__,data->enable);
			cancel_work_sync(&data->dwork);
			cancel_delayed_work(&data->powerkey_work);
			if (data->irq)
			{
				/*when close the pls,make the wakeup property diabled*/
				irq_set_irq_wake(data->irq, 0);
				operate_irq(data,0,true);
			}

#ifdef CONFIG_HUAWEI_DSM
		txc_pa2240_dsm_change_ps_enable_status(data);
#endif

		}
	}

	txc_pa2240_regs_debug_print(data, data->enable_ps_sensor);
	/* Vote off  regulators if both light and prox sensor are off */
	if ((data->enable_ps_sensor == 0) &&(data->platform_data->power_on)){
		data->platform_data->power_on(false,data);
	}
	return 0;
}


static int txc_pa2240_ps_set_enable(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	struct txc_pa2240_data *data = container_of(sensors_cdev,
			struct txc_pa2240_data, ps_cdev);
	if ((enable != 0) && (enable != 1)) {
		TXC_PA2240_ERR("%s: invalid value(%d)\n", __func__, enable);
		return -EINVAL;
	}
	return txc_pa2240_enable_ps_sensor(data->client, enable);
}

static ssize_t txc_pa2240_show_pdata(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int pdata;

	pdata = txc_pa2240_i2c_read(client, TXC_PA2240_REG_PS_DATA,TXC_PA2240_I2C_BYTE);
	if(pdata <0){
		TXC_PA2240_ERR("%s,line %d:read pdata failed\n",__func__,__LINE__);
	}

	return snprintf(buf,32, "%d\n", pdata);
}

static DEVICE_ATTR(pdata, S_IRUGO, txc_pa2240_show_pdata, NULL);


/*
* set the register's value from userspace
* Usage: echo "0x08|0x12" > dump_reg
*			"reg_address|reg_value"
*/
static ssize_t txc_pa2240_write_reg(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	int val_len_max = 4;
	char* input_str =NULL;
	char reg_addr_str[10]={'\0'};
	char reg_val_str[10]={'\0'};
	long reg_addr,reg_val;
	int addr_lenth=0,value_lenth=0,buf_len=0,ret = -1;
	char* strtok=NULL;

	buf_len = strlen(buf);
	input_str = kzalloc(buf_len, GFP_KERNEL);
	if (!input_str)
	{
		TXC_PA2240_ERR("%s:kmalloc fail!\n",__func__);
		return -ENOMEM;
	}

	snprintf(input_str, 10,"%s", buf);
	/*Split the string when encounter "|", for example "0x08|0x12" will be splited "0x18" "0x12" */
	strtok=strsep(&input_str, "|");
	if(strtok!=NULL)
	{
		addr_lenth = strlen(strtok);
		memcpy(reg_addr_str,strtok,((addr_lenth > (val_len_max))?(val_len_max):addr_lenth));
	}
	else
	{
		TXC_PA2240_ERR("%s: buf name Invalid:%s", __func__,buf);
		goto parse_fail_exit;
	}
	strtok=strsep(&input_str, "|");
	if(strtok!=NULL)
	{
		value_lenth = strlen(strtok);
		memcpy(reg_val_str,strtok,((value_lenth > (val_len_max))?(val_len_max):value_lenth));
	}
	else
	{
		TXC_PA2240_ERR("%s: buf value Invalid:%s", __func__,buf);
		goto parse_fail_exit;
	}
	/* transform string to long int */
	ret = kstrtol(reg_addr_str,16,&reg_addr);
	if(ret)
		goto parse_fail_exit;

	ret = kstrtol(reg_val_str,16,&reg_val);
	if(ret)
		goto parse_fail_exit;

	/* write the parsed value in the register*/
	ret = txc_pa2240_i2c_write(client,(char)reg_addr,(char)reg_val,TXC_PA2240_I2C_BYTE);
	if (ret < 0){
		goto parse_fail_exit;
	}
	if(input_str)
		kfree(input_str);
	return count;

parse_fail_exit:
	if (input_str)
		kfree(input_str);

	return ret;
}

/*
* show all registers' value to userspace
*/
static ssize_t txc_pa2240_print_reg_buf(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int i;
	char reg[TXC_PA2240_REG_LEN];
	struct i2c_client *client = to_i2c_client(dev);

	/* read all register value and print to user*/
	for(i = 0; i < TXC_PA2240_REG_LEN; i++ )
	{
		reg[i] = txc_pa2240_i2c_read(client,i,TXC_PA2240_I2C_BYTE);
		if(reg[i] <0){
			TXC_PA2240_ERR("%s,line %d:read %d reg failed\n",__func__,__LINE__,i);
			return reg[i] ;
		}
	}

	return snprintf(buf,512,"reg[0x0~0x8]=0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x\n"
			"reg[0x09~0x12]0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x\n",
			reg[0x00],reg[0x01],reg[0x02],reg[0x03],reg[0x04],reg[0x05],reg[0x06],reg[0x07],reg[0x08],
			reg[0x09],reg[0x0a],reg[0x0b],reg[0x0c],reg[0x0d],reg[0x0e],reg[0x0f],reg[0x10],reg[0x11],reg[0x12]);
}

static DEVICE_ATTR(dump_reg ,S_IRUGO | S_IWUSR, txc_pa2240_print_reg_buf, txc_pa2240_write_reg);

static ssize_t txc_pa2240_show_ps_calibration(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int ps_cal_offset =0;
	struct i2c_client *client = to_i2c_client(dev);
	
	ps_cal_offset =  txc_pa2240_get_pscrosstalk(client);
	
	return snprintf(buf, MAX_BUF_LEN, "x-talk = %d\n", ps_cal_offset);
	
}

/* PS Calibration */
static ssize_t txc_pa2240_store_ps_calibration(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	static int ret = 0;
	struct i2c_client *client = to_i2c_client(dev);	
	struct txc_pa2240_data *data = i2c_get_clientdata(client);

	
	ret = txc_pa2240_run_fast_calibration(client);
	if(ret)
	{
		TXC_PA2240_ERR("%s,line%d:set ps_cal_offset fail:%d\n ", __func__, __LINE__, ret);
	}
	
	TXC_PA2240_INFO("%s,line%d:set ps_cal_offset to %d\n ", __func__, __LINE__, data->platform_data->crosstalk);
	
	return count;
}

static ssize_t pa224_store_ps_calibration(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);	
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	int err;
	u32 val;

	err = kstrtoint(buf, 0, &val);
	if (err < 0) {
		TXC_PA2240_ERR("%s:%d kstrtoint failed\n", __func__,__LINE__);
		return err;
	}
	TXC_PA2240_INFO("%s,line%d:val =%d\n ", __func__, __LINE__, val);
	if(val == 1)
	{
		mutex_lock(&data->single_lock);
		pa224_run_calibration(client);
		mutex_unlock(&data->single_lock);
	}
	return count;
}

static ssize_t pa224_show_ps_calibration(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);	
	return snprintf(buf, MAX_BUF_LEN, "x-talk = %d\n", txc_pa2240_get_pscrosstalk(client));
	
}

static ssize_t pa224_show_ps_calibration_result(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);	
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	if(pdata->flag < 0)
	{
		return snprintf(buf, MAX_BUF_LEN,"%d\n", pdata->flag);
	}
	
	return snprintf(buf, MAX_BUF_LEN,"%d\n", pdata->crosstalk);
}

static ssize_t pa224_store_ps_calibration_result(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	int err;
	u32 val;
	
	err = kstrtoint(buf, 0, &val);
	if (err < 0) {
		TXC_PA2240_ERR("%s:%d kstrtoint failed\n", __func__,__LINE__);
		return err;
	}
	TXC_PA2240_INFO("%s,line%d:val =%d\n ", __func__, __LINE__, val);
	pdata->crosstalk = val;
	return count;
}

static DEVICE_ATTR(ps_calibration, S_IRUGO | S_IWUSR, txc_pa2240_show_ps_calibration, txc_pa2240_store_ps_calibration);
static DEVICE_ATTR(txc_pa2240_calibration, S_IRUGO | S_IWUSR, pa224_show_ps_calibration, pa224_store_ps_calibration);
static DEVICE_ATTR(txc_pa2240_calibration_result, S_IRUGO | S_IWUSR, pa224_show_ps_calibration_result,pa224_store_ps_calibration_result);

static struct attribute *txc_pa2240_attributes[] = {
	&dev_attr_pdata.attr,
	&dev_attr_dump_reg.attr,
	&dev_attr_ps_calibration.attr,
	&dev_attr_txc_pa2240_calibration.attr,
	&dev_attr_txc_pa2240_calibration_result.attr,
#ifdef CONFIG_HUAWEI_DSM
	&dev_attr_dsm_excep.attr,
#endif
	NULL
};

static const struct attribute_group txc_pa2240_attr_group = {
	.attrs = txc_pa2240_attributes,
};

/*
 * Initialization function
 */
static int txc_pa2240_init_client(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	int err;
	data->enable = 0x00;
	err = txc_pa2240_set_cfg0(client, data->enable);
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg0 FAIL ",__func__,__LINE__);
		return err;
	}

	err = txc_pa2240_set_cfg1(client, (pdata->ir_current << 4) | ( PA2240_PS_PRST << 2));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg1 FAIL ",__func__,__LINE__);
		return err;
	}

	/* no long wait */
	err = txc_pa2240_set_cfg3(client, (PA2240_INT_TYPE << 6) | (PA2240_PS_PERIOD << 3));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg3 FAIL ",__func__,__LINE__);
		return err;
	}
	err = txc_pa2240_set_ps(client,0x82);
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_ps FAIL ",__func__,__LINE__);
		return err;
	}
	err = txc_pa2240_set_flct(client,(0x0C | (PA2240_PS_FLCT<<4)));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_flct FAIL ",__func__,__LINE__);
		return err;
	}	
	/* init threshold for proximity */
	err = txc_pa2240_set_pilt(client, 0);
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_pilt FAIL ",__func__,__LINE__);
		return err;
	}

	err = txc_pa2240_set_piht(client, 0xFF);
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_piht FAIL ",__func__,__LINE__);
		return err;
	}
	data->ps_detection = TXC_PA2240_FAR_FLAG; /* initial value = far*/

	/*set ps nomal mode, and clear chip irq*/
	err = txc_pa2240_set_cfg2(client, (PA2240_PS_MODE << 6) | (1 << 2));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg2 FAIL ",__func__,__LINE__);
		return err;
	}

	return 0;
}
/*power off init client */
static int txc_pa2240_power_off_init_client(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;
	int err;
	TXC_PA2240_INFO("%s,line%d:txc_pa2240_power_off_init_client\n",__func__,__LINE__);
	err = txc_pa2240_set_cfg0(client, data->enable);
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg0 FAIL ",__func__,__LINE__);
		return err;
	}

	err = txc_pa2240_set_cfg1(client, (pdata->ir_current << 4) | ( PA2240_PS_PRST << 2));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg1 FAIL ",__func__,__LINE__);
		return err;
	}

	/* no long wait */
	err = txc_pa2240_set_cfg3(client, (PA2240_INT_TYPE << 6) | (PA2240_PS_PERIOD << 3));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_cfg3 FAIL ",__func__,__LINE__);
		return err;
	}
	err = txc_pa2240_set_ps(client,0x82);
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_ps FAIL ",__func__,__LINE__);
		return err;
	}
	err = txc_pa2240_set_flct(client,(0x0C | (PA2240_PS_FLCT<<4)));
	if (err < 0)
	{
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_flct FAIL ",__func__,__LINE__);
		return err;
	}	
	/* init threshold for proximity */
	err = txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TH, data->piht, TXC_PA2240_I2C_BYTE); 
	err += txc_pa2240_i2c_write(client,TXC_PA2240_REG_PS_TL, data->pilt, TXC_PA2240_I2C_BYTE);
	if (err < 0) 
	{ 
		TXC_PA2240_ERR("%s,line%d:txc_pa2240_set_piht FAIL ",__func__,__LINE__); 
		return err; 
	} 

	return 0;
}
/*qualcom updated the regulator configure functions and we add them all*/
static int sensor_regulator_configure(struct txc_pa2240_data *data, bool on)
{
	int rc;

	if (!on) {
		if (regulator_count_voltages(data->vdd) > 0){
			rc = regulator_set_voltage(data->vdd, 0, TXC_PA2240_VDD_MAX_UV);
			if(rc){
				TXC_PA2240_ERR("%s,line%d:Regulator set vdd failed rc=%d\n",__func__,__LINE__,rc);
			}
		}

		regulator_put(data->vdd);

		if (regulator_count_voltages(data->vio) > 0){
			rc = regulator_set_voltage(data->vio, 0, TXC_PA2240_VIO_MAX_UV);
			if(rc){
				TXC_PA2240_ERR("%s,line%d:Regulator set vio failed rc=%d\n",__func__,__LINE__,rc);
			}
		}

		regulator_put(data->vio);

	} else {
		data->vdd = regulator_get(&data->client->dev, "vdd");
		if (IS_ERR(data->vdd)) {
			rc = PTR_ERR(data->vdd);
			TXC_PA2240_ERR("%s,line%d:Regulator get failed vdd rc=%d\n",__func__,__LINE__, rc);
			return rc;
		}

		if (regulator_count_voltages(data->vdd) > 0) {
			rc = regulator_set_voltage(data->vdd,
				TXC_PA2240_VDD_MIN_UV, TXC_PA2240_VDD_MAX_UV);
			if (rc) {
				TXC_PA2240_ERR("%s,line%d:Regulator set failed vdd rc=%d\n",__func__,__LINE__,rc);
				goto reg_vdd_put;
			}
		}

		data->vio = regulator_get(&data->client->dev, "vio");
		if (IS_ERR(data->vio)) {
			rc = PTR_ERR(data->vio);
			TXC_PA2240_ERR("%s,line%d:Regulator get failed vio rc=%d\n",__func__,__LINE__, rc);
			goto reg_vdd_set;
		}

		if (regulator_count_voltages(data->vio) > 0) {
			rc = regulator_set_voltage(data->vio,
				TXC_PA2240_VIO_MIN_UV, TXC_PA2240_VIO_MAX_UV);
			if (rc) {
				TXC_PA2240_ERR("%s,line%d:Regulator set failed vio rc=%d\n",__func__,__LINE__, rc);
				goto reg_vio_put;
			}
		}

	}

	return 0;
reg_vio_put:
	regulator_put(data->vio);

reg_vdd_set:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, TXC_PA2240_VDD_MAX_UV);
reg_vdd_put:
	regulator_put(data->vdd);
	return rc;
}
/*In suspend and resume function,we only control the als,leave pls alone*/
static int txc_pa2240_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	if (NULL == data) {
		TXC_PA2240_ERR("%s: i2c_get_clientdata is null\n", __func__);
		return -1;
	}

	data->i2c_ready_flag = false;
	TXC_PA2240_INFO("%s: data->i2c_ready_flag: %d\n", __func__, data->i2c_ready_flag);
	return 0;
}

static int txc_pa2240_resume(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	if (NULL == data) {
		TXC_PA2240_ERR("%s: i2c_get_clientdata is null\n", __func__);
		return -1;
	}

	data->i2c_ready_flag = true;
	wake_up(&data->notify_i2c_ready_event);
	TXC_PA2240_INFO("%s: data->i2c_ready_flag: %d\n", __func__, data->i2c_ready_flag);
	return 0;
}
/*pamameter subfunction of probe to reduce the complexity of probe function*/
static int txc_pa2240_sensorclass_init(struct txc_pa2240_data *data,struct i2c_client* client)
{
	int err;
	/* Register to sensors class */
	data->ps_cdev = sensors_proximity_cdev;
	data->ps_cdev.sensors_enable = txc_pa2240_ps_set_enable;
	data->ps_cdev.sensors_poll_delay = NULL;

	err = sensors_classdev_register(&data ->input_dev_ps ->dev, &data->ps_cdev);
	if (err) {
		TXC_PA2240_ERR("%s: Unable to register to sensors class: %d\n",__func__, err);
	}

	return err;
}
static void txc_pa2240_parameter_init(struct txc_pa2240_data *data)
{
	/* Set the default parameters */
	data->enable = 0x00;	/* default mode is standard */
	data->enable_ps_sensor = 0;	// default to 0
	data->count = 1;	// disable_irq is before enable_irq, so the initial value should more than zero
}
/*input init subfunction of probe to reduce the complexity of probe function*/
static int txc_pa2240_input_init(struct txc_pa2240_data *data)
{
	int err = 0;

	data->input_dev_ps = input_allocate_device();
	if (!data->input_dev_ps) {
		err = -ENOMEM;
		TXC_PA2240_ERR("%s: Failed to allocate input device ps\n", __func__);
		goto exit;
	}

	set_bit(EV_ABS, data->input_dev_ps->evbit);

	input_set_abs_params(data->input_dev_ps, ABS_DISTANCE, 0, 5, 0, 0);

	data->input_dev_ps->name = "proximity";

	err = input_register_device(data->input_dev_ps);
	if (err) {
		err = -ENOMEM;
		TXC_PA2240_ERR("%s: Unable to register input device ps: %s\n",
				__func__, data->input_dev_ps->name);
		goto unregister_als;
	}
	goto exit;
unregister_als:
	input_free_device(data->input_dev_ps);
exit:
	return err;
}
/*irq request subfunction of probe to reduce the complexity of probe function*/
static int txc_pa2240_irq_init(struct txc_pa2240_data *data,struct i2c_client *client)
{
	int ret = 0;
	if (data->platform_data->irq_gpio)
	{
		ret = gpio_request(data->platform_data->irq_gpio,"pa2240_irq_gpio");
		if (ret)
		{
			TXC_PA2240_ERR("%s, line %d:unable to request gpio [%d]\n", __func__, __LINE__,data->platform_data->irq_gpio);
			return ret;
		}
		else
		{
			ret = gpio_direction_input(data->platform_data->irq_gpio);
			if(ret)
			{
				TXC_PA2240_ERR("%s, line %d: Failed to set gpio %d direction\n", __func__, __LINE__,data->platform_data->irq_gpio);
				return ret;
			}
		}
	}
	client->irq = gpio_to_irq(data->platform_data->irq_gpio);
	if (client->irq < 0) {
		ret = -EINVAL;
		TXC_PA2240_ERR("%s, line %d:gpio_to_irq FAIL! IRQ=%d\n", __func__, __LINE__,data->platform_data->irq_gpio);
		return ret;
	}
	data->irq = client->irq;
	if (client->irq)
	{
		/*AP examination of low level to prevent lost interrupt*/
		if (request_irq(data->irq, txc_pa2240_interrupt,IRQF_TRIGGER_LOW|IRQF_ONESHOT|IRQF_NO_SUSPEND, TXC_PA2240_DRV_NAME, (void *)client) >= 0)
		{
			TXC_PA2240_FLOW("%s, line %d:Received IRQ!\n", __func__, __LINE__);
			operate_irq(data,0,true);
		}
		else
		{
			TXC_PA2240_ERR("%s, line %d:Failed to request IRQ!\n", __func__, __LINE__);
			ret = -EINVAL;
			return ret;
		}
	}
	return ret;
}
static int sensor_regulator_power_on(struct txc_pa2240_data *data, bool on)
{
	int rc = 0;

	if (!on) {
		rc = regulator_disable(data->vdd);
		if (rc) {
			TXC_PA2240_ERR("%s: Regulator vdd disable failed rc=%d\n", __func__, rc);
			return rc;
		}

		rc = regulator_disable(data->vio);
		if (rc) {
			TXC_PA2240_ERR("%s: Regulator vdd disable failed rc=%d\n", __func__, rc);
			rc = regulator_enable(data->vdd);
			TXC_PA2240_ERR("%s:Regulator vio re-enabled rc=%d\n",__func__, rc);
			/*
			 * Successfully re-enable regulator.
			 * Enter poweron delay and returns error.
			 */
			if (!rc) {
				rc = -EBUSY;
				goto enable_delay;
			}
		}
		return rc;
	} else {
		rc = regulator_enable(data->vdd);
		if (rc) {
			TXC_PA2240_ERR("%s:Regulator vdd enable failed rc=%d\n",__func__, rc);
			return rc;
		}

		rc = regulator_enable(data->vio);
		if (rc) {
			TXC_PA2240_ERR("%s:Regulator vio enable failed rc=%d\n", __func__,rc);
			rc = regulator_disable(data->vdd);
			return rc;
		}
	}
enable_delay:
	if(1 == txc_power_delay_flag)
	{
		msleep(10);
	}
	TXC_PA2240_FLOW("%s:Sensor regulator power on =%d\n",__func__, on);
	return rc;
}

static int sensor_platform_hw_power_on(bool on,struct txc_pa2240_data *data)
{
	int err = 0;

	if (data->power_on != on) {
		if (!IS_ERR_OR_NULL(data->pinctrl)) {
			if (on)
				/*after poweron,set the INT pin the default state*/
				err = pinctrl_select_state(data->pinctrl,
					data->pin_default);
			if (err)
				TXC_PA2240_ERR("%s,line%d:Can't select pinctrl state\n", __func__, __LINE__);
		}

		err = sensor_regulator_power_on(data, on);
		if (err)
			TXC_PA2240_ERR("%s,line%d:Can't configure regulator!\n", __func__, __LINE__);
		else
			data->power_on = on;
	}

	return err;
}
static int sensor_platform_hw_init(struct txc_pa2240_data *data)
{
	int error;

	error = sensor_regulator_configure(data, true);
	if (error < 0) {
		TXC_PA2240_ERR("%s,line %d:unable to configure regulator\n",__func__,__LINE__);
		return error;
	}

	return 0;
}

static void sensor_platform_hw_exit(struct txc_pa2240_data *data)
{
	int error;
	error = sensor_regulator_configure(data, false);
	if (error < 0) {
		TXC_PA2240_ERR("%s,line %d:unable to configure regulator\n",__func__,__LINE__);
	}
}
static int txc_pa2240_pinctrl_init(struct txc_pa2240_data *data)
{
	struct i2c_client *client = data->client;

	data->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR_OR_NULL(data->pinctrl)) {
		TXC_PA2240_ERR("%s,line %d:Failed to get pinctrl\n",__func__,__LINE__);
		return PTR_ERR(data->pinctrl);
	}
	/*we have not set the sleep state of INT pin*/
	data->pin_default =
		pinctrl_lookup_state(data->pinctrl, "default");
	if (IS_ERR_OR_NULL(data->pin_default)) {
		TXC_PA2240_ERR("%s,line %d:Failed to look up default state\n",__func__,__LINE__);
		return PTR_ERR(data->pin_default);
	}

	return 0;
}

static int sensor_parse_dt(struct device *dev,
		struct txc_pa2240_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	unsigned int tmp = 0;
	int rc = 0;

	/* set functions of platform data */
	pdata->init = sensor_platform_hw_init;
	pdata->exit = sensor_platform_hw_exit;
	pdata->power_on = sensor_platform_hw_power_on;

	/* irq gpio */
	rc = of_get_named_gpio_flags(dev->of_node,
			"txc,irq-gpio", 0, NULL);
	if (rc < 0) {
		TXC_PA2240_ERR("%s,line %d:Unable to read irq gpio\n",__func__,__LINE__);
		return rc;
	}
	pdata->irq_gpio = rc;

	rc = of_property_read_u32(np, "txc,ps_wave", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read pwave_value\n",__func__,__LINE__);
		return rc;
	}
	pdata ->pwave= tmp;

	rc = of_property_read_u32(np, "txc,ps_window", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read pwindow_value\n",__func__,__LINE__);
		return rc;
	}
	pdata ->pwindow= tmp;

	rc = of_property_read_u32(np, "txc,ps_defalt_crosstalk", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read ps_defalt_crosstalk\n",__func__,__LINE__);
		return rc;
	}
	pdata ->defalt_crosstalk = tmp;
	pdata->crosstalk = pdata ->defalt_crosstalk;

	rc = of_property_read_u32(np, "txc,ir_current", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read ir_current\n",__func__,__LINE__);
		return rc;
	}
	pdata->ir_current = tmp;

	rc = of_property_read_u32(np, "txc,oil_effect", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read oil_effect\n",__func__,__LINE__);
		return rc;
	}
	oil_effect = tmp;

	rc = of_property_read_u32(np, "txc,high_threshold", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read high_threshold\n",__func__,__LINE__);
		return rc;
	}
	high_threshold = tmp;

	rc = of_property_read_u32(np, "txc,low_threshold", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read low_threshold\n",__func__,__LINE__);
		return rc;
	}
	low_threshold = tmp;

	rc = of_property_read_u32(np, "txc,middle_threshold", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read middle_threshold\n",__func__,__LINE__);
		return rc;
	}
	middle_threshold = tmp;

	rc = of_property_read_u32(np, "txc,calibration_threshold", &tmp);
	if (rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read middle_threshold\n",__func__,__LINE__);
		return rc;
	}
	calibration_threshold = tmp;

	rc = of_property_read_u32(np,"txc,power_delay_flag",&tmp);
	if(rc) {
		TXC_PA2240_ERR("%s,line %d:Unable to read power delay flag\n",__func__,__LINE__);
		txc_power_delay_flag = 1;
	}
	else{
		txc_power_delay_flag = tmp;
	}
	pdata->i2c_scl_gpio = of_get_named_gpio_flags(np, "txc,i2c-scl-gpio", 0, NULL);
	if (!gpio_is_valid(pdata->i2c_scl_gpio)) {
		TXC_PA2240_ERR("gpio i2c-scl pin %d is invalid\n", pdata->i2c_scl_gpio);
		return -EINVAL;
	}

	pdata->i2c_sda_gpio = of_get_named_gpio_flags(np, "txc,i2c-sda-gpio", 0, NULL);
	if (!gpio_is_valid(pdata->i2c_sda_gpio)) {
		TXC_PA2240_ERR("gpio i2c-sda pin %d is invalid\n", pdata->i2c_sda_gpio);
		return -EINVAL;
	}

	return 0;
}



static void txc_pa2240_powerkey_screen_handler(struct work_struct *work)
{
	struct txc_pa2240_data *data = container_of((struct delayed_work *)work, struct txc_pa2240_data, powerkey_work);
	if(power_key_ps)
	{
		TXC_PA2240_INFO("%s : power_key_ps (%d) press\n",__func__, power_key_ps);
		power_key_ps=false;
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, TXC_PA2240_FAR_FLAG);
		input_sync(data->input_dev_ps);
	}
	schedule_delayed_work(&data->powerkey_work, msecs_to_jiffies(500));
}

/*
 * I2C init/probing/exit functions
 */
static struct i2c_driver txc_pa2240_driver;
static int txc_pa2240_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct txc_pa2240_data *data;
	struct txc_pa2240_platform_data *pdata;
	int err = 0;

	TXC_PA2240_INFO("%s,line %d:PROBE START.\n",__func__,__LINE__);

	if (client->dev.of_node) {
		/*Memory allocated with this function is automatically freed on driver detach.*/
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct txc_pa2240_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			TXC_PA2240_ERR("%s,line %d:Failed to allocate memory\n",__func__,__LINE__);
			err =-ENOMEM;
			goto exit;
		}

		client->dev.platform_data = pdata;
		err = sensor_parse_dt(&client->dev, pdata);
		if (err) {
			TXC_PA2240_ERR("%s: sensor_parse_dt() err\n", __func__);
			goto exit_parse_dt;
		}
	} else {
		pdata = client->dev.platform_data;
		if (!pdata) {
			TXC_PA2240_ERR("%s,line %d:No platform data\n",__func__,__LINE__);
			err = -ENODEV;
			goto exit;
		}
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		TXC_PA2240_ERR("%s,line %d:Failed to i2c_check_functionality\n",__func__,__LINE__);
		err = -EIO;
		goto exit_parse_dt;
	}

	data = kzalloc(sizeof(struct txc_pa2240_data), GFP_KERNEL);
	if (!data) {
		TXC_PA2240_ERR("%s,line %d:Failed to allocate memory\n",__func__,__LINE__);
		err = -ENOMEM;
		goto exit_parse_dt;
	}

	data->platform_data = pdata;
	data->client = client;
	data->device_exist = false;

	/* h/w initialization */
	if (pdata->init)
		err = pdata->init(data);

	if (pdata->power_on)
		err = pdata->power_on(true,data);
#ifdef CONFIG_HUAWEI_DSM
	err = txc_pa2240_dsm_init(data);
	if(err < 0)
		goto exit_uninit;
#endif

	i2c_set_clientdata(client, data);
	txc_pa2240_parameter_init(data);
	/* initialize pinctrl */
	err = txc_pa2240_pinctrl_init(data);
	if (err) {
		TXC_PA2240_ERR("%s,line %d:Can't initialize pinctrl\n",__func__,__LINE__);
			goto exit_unregister_dsm;
	}
	err = pinctrl_select_state(data->pinctrl, data->pin_default);
	if (err) {
		TXC_PA2240_ERR("%s,line %d:Can't select pinctrl default state\n",__func__,__LINE__);
		goto exit_unregister_dsm;
	}

	data->i2c_ready_flag = true;
	mutex_init(&data->update_lock);
	mutex_init(&data->single_lock);
	INIT_WORK(&data->dwork, txc_pa2240_work_handler);
	init_waitqueue_head(&data->notify_i2c_ready_event);

	INIT_DELAYED_WORK(&data->powerkey_work, txc_pa2240_powerkey_screen_handler);

	err = txc_pa2240_init_client(client);
	if (err) {
		TXC_PA2240_ERR("%s: Failed to init txc_pa2240\n", __func__);
		goto exit_unregister_dsm;
	}

	err = txc_pa2240_input_init(data);
	if(err)
		goto exit_unregister_dsm;

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &txc_pa2240_attr_group);
	if (err)
		goto exit_unregister_dev_ps;

	wake_lock_init(&data->ps_report_wk, WAKE_LOCK_SUSPEND, "psensor_wakelock");

	txc_pa2240_workqueue = create_workqueue("txc_pa2240_work_queue");
	if (!txc_pa2240_workqueue)
	{
		TXC_PA2240_ERR("%s: Create ps_workqueue fail.\n", __func__);
		goto exit_remove_sysfs_group;
	}

	device_init_wakeup(&(client->dev), true);

	err = txc_pa2240_sensorclass_init(data,client);
	if (err) {
		TXC_PA2240_ERR("%s: Unable to register to sensors class: %d\n",
	__func__, err);
		goto exit_free_irq;
	}

	err=txc_pa2240_irq_init(data,client);
	if(err)
		goto exit_unregister_sensorclass;

	err = app_info_set("P-Sensor", "TXC PA2240");
	if (err < 0)/*failed to add app_info*/
	{
	    TXC_PA2240_ERR("%s %d:failed to add app_info\n", __func__, __LINE__);
	}

	//set_sensors_list(P_SENSOR);
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_APS);
#endif
	/*
	err = set_sensor_input(PS, data->input_dev_ps->dev.kobj.name);
	if (err) {
		TXC_PA2240_ERR("%s set_sensor_input PS failed\n", __func__);
	}
	*/

	if (pdata->power_on)
		err = pdata->power_on(false,data);
	TXC_PA2240_INFO("%s: Support ver. %s enabled\n", __func__, DRIVER_VERSION);
	data->device_exist = true;
	return 0;
exit_unregister_sensorclass:
	sensors_classdev_unregister(&data->ps_cdev);
exit_free_irq:
	if (gpio_is_valid(data->platform_data->irq_gpio))
		gpio_free(data->platform_data->irq_gpio);
	free_irq(data->irq, client);
exit_remove_sysfs_group:
	wake_lock_destroy(&data->ps_report_wk);
	sysfs_remove_group(&client->dev.kobj, &txc_pa2240_attr_group);
exit_unregister_dev_ps:
	input_unregister_device(data->input_dev_ps);
#ifdef CONFIG_HUAWEI_DSM
exit_unregister_dsm:
	txc_pa2240_dsm_exit();
exit_uninit:
#else
exit_unregister_dsm:
#endif
	if (pdata->power_on)
		pdata->power_on(false,data);
	if (pdata->exit)
		pdata->exit(data);
	kfree(data);

exit_parse_dt:
exit:
	return err;
}

static int txc_pa2240_remove(struct i2c_client *client)
{
	struct txc_pa2240_data *data = i2c_get_clientdata(client);
	struct txc_pa2240_platform_data *pdata = data->platform_data;

	/* Power down the device */
	data->enable = 0x00;
	txc_pa2240_set_cfg0(client, data->enable);
	wake_lock_destroy(&data->ps_report_wk);
	sysfs_remove_group(&client->dev.kobj, &txc_pa2240_attr_group);

	input_unregister_device(data->input_dev_ps);

	free_irq(client->irq, data);
#ifdef CONFIG_HUAWEI_DSM
	txc_pa2240_dsm_exit();
#endif

	if (pdata->power_on)
		pdata->power_on(false,data);

	if (pdata->exit)
		pdata->exit(data);

	kfree(data);

	return 0;
}

static const struct i2c_device_id txc_pa2240_id[] = {
	{ "txc_pa2240", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, txc_pa2240_id);

static struct of_device_id txc_pa2240_match_table[] = {
	{ .compatible = "txc,pa224",},
	{ },
};

static struct i2c_driver txc_pa2240_driver = {
	.driver = {
		.name   = TXC_PA2240_DRV_NAME,
		.owner  = THIS_MODULE,
		.of_match_table = txc_pa2240_match_table,
	},
	.probe  = txc_pa2240_probe,
	.remove = txc_pa2240_remove,
	.suspend = txc_pa2240_suspend,
	.resume = txc_pa2240_resume,
	.id_table = txc_pa2240_id,
};
static int __init txc_pa2240_init(void)
{
	return i2c_add_driver(&txc_pa2240_driver);
}

static void __exit txc_pa2240_exit(void)
{
	if (txc_pa2240_workqueue) {
		destroy_workqueue(txc_pa2240_workqueue);
		txc_pa2240_workqueue = NULL;
	}

	i2c_del_driver(&txc_pa2240_driver);
}

MODULE_DESCRIPTION("TXC_PA2240 proximity sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(txc_pa2240_init);
module_exit(txc_pa2240_exit);
