/*
 *  apds9110.c - Linux kernel modules for ambient proximity sensor
 *
 *  Copyright (C) 2015 Lee Kai Koon <kai-koon.lee@avagotech.com>
 *  Copyright (C) 2015 Avago Technologies
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
#include <huawei_platform/sensor/apds9110.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/sensors.h>
#include <linux/wakelock.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#include <misc/app_info.h>

#include <linux/debugfs.h>

#undef CONFIG_HUAWEI_DSM



#ifdef CONFIG_HUAWEI_DSM
#include 	<dsm/dsm_pub.h>
#endif


/* Change History 
 *
 * 1.0.0   23-Nov-2015   Fundamental Functions of APDS-9110
 *
 */

#define APDS9110_DRV_NAME "apds9110"
#define DRIVER_VERSION "1.0.0"

/*dynamic debug mask to control log print,you can echo value to apds9110_debug to control*/
static int apds9110_debug_mask= 1;
module_param_named(apds9110_debug, apds9110_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
#define APDS9110_ERR(x...) do {\
	if (apds9110_debug_mask >=0) \
        printk(KERN_ERR x);\
    } while (0)
/*KERNEL_HWFLOW is for radar using to control all the log of devices*/
#define APDS9110_INFO(x...) do {\
	if (apds9110_debug_mask >0) \
        printk(KERN_ERR x);\
    } while (0)
#define APDS9110_FLOW(x...) do {\
    if ( apds9110_debug_mask >1) \
        printk(KERN_ERR x);\
    } while (0)


struct apds9110_data {
	struct i2c_client *client;
	/*to protect the i2c read and write operation*/
	struct mutex update_lock;
	/*to protect only one thread to control the device register*/
	struct mutex single_lock;
	struct work_struct	dwork;		/* for PS interrupt */
	struct delayed_work    powerkey_work;
	struct input_dev *input_dev_ps;

	/* regulator data */
	bool power_on;
	struct regulator *vdd;
	struct regulator *vio;

	/* pinctrl data*/
	struct pinctrl *pinctrl;
	struct pinctrl_state *pin_default;
	/*sensor class for	ps*/
	struct sensors_classdev ps_cdev;
	struct apds9110_platform_data *platform_data;

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
	unsigned int ps_threshold;
	unsigned int ps_hysteresis_threshold; /* always lower than ps_threshold */
	unsigned int ps_min_threshold;	/*it is the min_proximity_value */
	unsigned int ps_detection;		/* 5 = near-to-far; 0 = far-to-near */
	unsigned int ps_data;/* to store PS data,it is pdata */
	unsigned int pdata_min; /*the value of Stay away from the phone's infrared hole*/
	bool saturation_flag;
	bool oil_occur;
	bool device_exist;
	unsigned int cal_flag;
	int ps_gain_index;
};
/*
 * Global data
 */


static struct sensors_classdev sensors_proximity_cdev = {
	.name = "apds9110-proximity",
	.vendor = "avago",
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

static unsigned char psGainValueArray[32]={0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
                                           0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
                                           0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
                                           0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10};
/* Proximity sensor use this work queue to report data */
static struct workqueue_struct *apds9110_workqueue = NULL;
extern bool power_key_ps ;    //the value is true means powerkey is pressed, false means not pressed



/*we use the unified the function for i2c write and read operation*/
static int apds9110_i2c_write(struct i2c_client*client, u8 reg, u16 value,bool flag)
{
	int err,loop;

	struct apds9110_data *data = i2c_get_clientdata(client);

	loop = APDS9110_I2C_RETRY_COUNT;
	/*we give three times to repeat the i2c operation if i2c errors happen*/
	while(loop) {
		mutex_lock(&data->update_lock);
		/*0 is i2c_smbus_write_byte_data,1 is i2c_smbus_write_word_data*/
		if(flag == APDS9110_I2C_BYTE)
		{
			err = i2c_smbus_write_byte_data(client, reg, (u8)value);
		}
		else if(flag == APDS9110_I2C_WORD)
		{
			err = i2c_smbus_write_word_data(client, reg, value);
		}
		mutex_unlock(&data->update_lock);
		if(err < 0){
			loop--;
			msleep(APDS9110_I2C_RETRY_TIMEOUT);
		}
		else
			break;
	}
	/*after three times,we print the register and regulator value*/
	if(loop == 0){
		APDS9110_ERR("%s,line %d:attention:i2c write err = %d\n",__func__,__LINE__,err);
#ifdef CONFIG_HUAWEI_DSM
		if (data->device_exist == true){
			apds9110_report_i2c_info(data,err);
		}
#endif
	}

	return err;
}


static int apds9110_i2c_read(struct i2c_client*client, u8 reg,bool flag)
{
	int err,loop;

	struct apds9110_data *data = i2c_get_clientdata(client);

	loop = APDS9110_I2C_RETRY_COUNT;
	/*we give three times to repeat the i2c operation if i2c errors happen*/
	while(loop) {
		mutex_lock(&data->update_lock);
		/*0 is i2c_smbus_read_byte_data,1 is i2c_smbus_read_word_data*/
		if(flag == APDS9110_I2C_BYTE)
		{
			err = i2c_smbus_read_byte_data(client, reg);
		}
		else if(flag == APDS9110_I2C_WORD)
		{
			err = i2c_smbus_read_word_data(client, reg);
		}
		mutex_unlock(&data->update_lock);
		if(err < 0){
			loop--;
			msleep(APDS9110_I2C_RETRY_TIMEOUT);
		}
		else
			break;
	}
	/*after three times,we print the register and regulator value*/
	if(loop == 0){
		APDS9110_ERR("%s,line %d:attention: i2c read err = %d,reg=0x%x\n",__func__,__LINE__,err,reg);
#ifdef CONFIG_HUAWEI_DSM
		if (data->device_exist == true){
			apds9110_report_i2c_info(data,err);
		}
#endif
	}

	return err;
}

static int apds9110_dd_set_main_ctrl(struct i2c_client *client, int main_ctrl)
{
	return i2c_smbus_write_byte_data(client, APDS9110_DD_MAIN_CTRL_ADDR, main_ctrl);
}

static int apds9110_dd_set_prx_meas_rate(struct i2c_client *client, int prx_meas)
{	
	return i2c_smbus_write_byte_data(client, APDS9110_DD_PRX_MEAS_RATE_ADDR, prx_meas);
}
static int apds9110_dd_set_prx_gain(struct i2c_client *client, int prx_gain)
{	
	return i2c_smbus_write_byte_data(client, APDS9110_DD_PRX_GAIN_ADDR, prx_gain);
}
static int apds9110_dd_set_prx_thresh(struct i2c_client *client, int thres_low, int thres_up)
{
	int ret;
	ret = i2c_smbus_write_word_data(client, APDS9110_DD_PRX_THRES_UP_ADDR, thres_up);
	if(ret)
		APDS9110_ERR("%s, line:%d i2c write data fail, ret=%d\n", __func__, __LINE__, ret);
	
	return i2c_smbus_write_word_data(client, APDS9110_DD_PRX_THRES_LOW_ADDR, thres_low);
}

static int apds9110_set_pilt(struct i2c_client *client, int threshold)
{
	struct apds9110_data *data = i2c_get_clientdata(client);
	int ret;

	ret = apds9110_i2c_write(client,APDS9110_DD_PRX_THRES_LOW_ADDR, threshold,APDS9110_I2C_WORD);
	if (ret < 0){
		APDS9110_ERR("%s,line %d:i2c error,threshold = %d\n",__func__,__LINE__,threshold);
		return ret;
	}

	data->pilt = threshold;
	APDS9110_INFO("%s,line %d:set apds9110 pilt =%d\n", __func__, __LINE__,threshold);

	return ret;
}

static int apds9110_set_piht(struct i2c_client *client, int threshold)
{
	struct apds9110_data *data = i2c_get_clientdata(client);
	int ret;

	ret = apds9110_i2c_write(client,APDS9110_DD_PRX_THRES_UP_ADDR, threshold,APDS9110_I2C_WORD);
	if (ret < 0){
		APDS9110_ERR("%s,line %d:i2c error,threshold = %d\n",__func__,__LINE__,threshold);
		return ret;
	}

	data->piht = threshold;
	APDS9110_INFO("%s,line %d:set apds9110 piht =%d\n", __func__,__LINE__,threshold);

	return ret;
}

static void apds9110_ps_report_event(struct i2c_client *client)
{
	struct apds9110_data *data = i2c_get_clientdata(client);
	struct apds9110_platform_data *pdata = data->platform_data;
	int ret=0;
	bool change_threshold = false;

	 /* To calculate and set the minimum threshold for PS */
	if((data->ps_data + pdata->pwave) < data->ps_min_threshold){
		data->ps_min_threshold = data->ps_data + pdata->pwave;		
		change_threshold = true;
		APDS9110_INFO("%s:Changed ps_min_threshold=%d\n",__func__,data->ps_min_threshold);
	}

	APDS9110_INFO("%s:ps_data =%d,data->piht=%d,data->pilt=%d\n",__func__,data->ps_data,data->piht,data->pilt);
	/*far event*/
	 if (data->ps_data <=  data->pilt && data->ps_detection == APDS9110_CLOSE_FLAG){
		data->ps_detection =APDS9110_FAR_FLAG ;
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, APDS9110_FAR_FLAG);
		input_sync(data->input_dev_ps);
		APDS9110_INFO("%s:PROXIMITY far event,ps_data =%d,data->piht=%d,data->pilt=%d\n",__func__,data->ps_data,data->piht,data->pilt);
	}
	/*near event*/
	else if (data->ps_data >= data->piht && data->ps_detection ==  APDS9110_FAR_FLAG){
		data->ps_detection = APDS9110_CLOSE_FLAG;
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, APDS9110_CLOSE_FLAG);
		input_sync(data->input_dev_ps);
		APDS9110_INFO("%s:PROXIMITY close event,ps_data =%d,data->piht=%d,data->pilt=%d\n",__func__,data->ps_data,data->piht,data->pilt);
	}
	else{
		APDS9110_ERR("%s:%d read pdata Not within reasonable limits,data->pilt=%d,data->piht=%d data->ps_data=%d\n",
			__FUNCTION__,__LINE__,data->pilt,data->piht,data->ps_data);
	}

	if(change_threshold){
		data->pilt = FAR_THRESHOLD(pdata->threshold_value);
		data->piht = NEAR_THRESHOLD(pdata->threshold_value);
		APDS9110_INFO("%s:Changed threshold,ps_data = %d,pilt = %d,piht = %d\n",__func__,data->ps_data,data->pilt,data->piht);
		ret = apds9110_set_piht(client,data->piht);
		ret += apds9110_set_pilt(client, data->pilt);
		if (ret < 0){
			APDS9110_ERR("%s,line %d:data->pilt = %d,data->piht=%d, i2c wrong\n",__func__,__LINE__,data->pilt,data->piht);
			goto exit;
		}
	}

	return;

	exit:
	/*if i2c error happens,we report far event*/
	if(data->ps_detection == APDS9110_CLOSE_FLAG){
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, APDS9110_FAR_FLAG);
		input_sync(data->input_dev_ps);
		data->ps_detection= APDS9110_FAR_FLAG;
		APDS9110_ERR("%s:i2c error happens, report far event, data->ps_data:%d\n", __func__,data->ps_data);
	}

	
}

void operate_irq(struct apds9110_data *data, int enable, bool sync)
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


/* PS interrupt routine */
static void apds9110_work_handler(struct work_struct *work)
{
	struct apds9110_data *data = container_of(work, struct apds9110_data, dwork);
	struct i2c_client *client=data->client;
	int status;
	int pdata;

	mutex_lock(&data->single_lock);
	pdata= apds9110_i2c_read(client, APDS9110_DD_PRX_DATA_0_ADDR,APDS9110_I2C_WORD);
	status = apds9110_i2c_read(client, APDS9110_DD_MAIN_STATUS_ADDR,APDS9110_I2C_BYTE);
	if(pdata < 0){
		/* If i2c error in near, ps_data assignment 100 ps reported so far */
		data->ps_data = 100;
		APDS9110_ERR("%s:i2c error,ps_data = 100",__func__);
	}else{
		data->ps_data = pdata & 0x7FF;
	}
	if (status & APDS9110_DD_PRX_INT_STATUS) {
		apds9110_ps_report_event(client);		
	} else{
		APDS9110_ERR("%s,line %d:wrong interrupts,APDS9110_DD_MAIN_STATUS_ADDR is 0X%x\n",__func__,__LINE__,status);
	}
	mutex_unlock(&data->single_lock);
	if (data->irq)
	{
		operate_irq(data,1,true);
	}
}


/* assume this is ISR */
static irqreturn_t apds9110_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct apds9110_data *data = i2c_get_clientdata(client);

	operate_irq(data,0,false);
	/*in 400ms,system keeps in wakeup state to avoid the sleeling system lose the pls event*/
	wake_lock_timeout(&data->ps_report_wk, PS_WAKEUP_TIME);
	queue_work(apds9110_workqueue, &data->dwork);

	return IRQ_HANDLED;
}


/*
 * Initialization function
 */
static int apds9110_init_client(struct i2c_client *client)
{
	struct apds9110_data *data = i2c_get_clientdata(client);
	struct apds9110_platform_data *pdata = data->platform_data;
	int err;
	data->enable = 0x00;
	/*set enable*/
	err = apds9110_i2c_write(client, APDS9110_DD_MAIN_CTRL_ADDR,data->enable,APDS9110_I2C_BYTE);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:set enable FAIL\n",__func__,__LINE__);
		return err;
	}
	/* PS_LED 60khz 10ma*/
	err = apds9110_i2c_write(client, APDS9110_DD_PRX_LED_ADDR,APDS9110_DD_PRX_DEFAULT_LED_FREQ|APDS9110_DD_PRX_DEFAULT_LED_CURRENT,APDS9110_I2C_BYTE);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:PS_LED FAIL\n",__func__,__LINE__);
		return err;
	}

	/* PS_PULSES 32 pulses */
	err = apds9110_i2c_write(client,APDS9110_DD_PRX_PULSES_ADDR,APDS9110_DD_PRX_DEFAULT_PULSE,APDS9110_I2C_BYTE);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:PS_PULSES FAIL\n",__func__,__LINE__);
		return err;
	}
	/* PS_MEAS_RATE  Measurement Rate:6.25ms Resolution:10 bit*/
	err = apds9110_i2c_write(client,APDS9110_DD_PRX_MEAS_RATE_ADDR,0x40|APDS9110_DD_PRX_DEFAULT_RES|APDS9110_DD_PRX_MEAS_RATE_6_25_MS,APDS9110_I2C_BYTE);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:PS_MEAS_RATE FAIL\n",__func__,__LINE__);
		return err;
	}
	/* INT_PERSISTENCE */
	err = apds9110_i2c_write(client,APDS9110_DD_INT_PERSISTENCE_ADDR,APDS9110_DD_PRX_PERS_1,APDS9110_I2C_BYTE);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:INT_PERSISTENCE  FAIL\n",__func__,__LINE__);
		return err;
	}	
	/* init threshold for proximity */
	err = apds9110_set_pilt(client, pdata->pwave+pdata->max_noise);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:apds9110_set_pilt FAIL ",__func__,__LINE__);
		return err;
	}

	err = apds9110_set_piht(client, data->pilt+pdata->pwindow);
	if (err < 0)
	{
		APDS9110_ERR("%s,line%d:apds9110_set_piht FAIL ",__func__,__LINE__);
		return err;
	}
	err = apds9110_dd_set_prx_gain(client, psGainValueArray[data->ps_gain_index]);
	if (err < 0) 
	{
		return err;
	}
	data->ps_detection = APDS9110_FAR_FLAG; /* initial value = far*/
	data->ps_min_threshold = 50;

	return 0;
}


static int apds9110_open_ps_sensor(struct apds9110_data *data, struct i2c_client *client)
{
	int ret = 0;
	/* turn on p sensor */
	if (data->enable_ps_sensor==0) {
		/* Power on and initalize the device */
		if (data->platform_data->power_on)
			data->platform_data->power_on(true,data);
		ret = apds9110_init_client(client);
		if (ret) {
			APDS9110_ERR("%s:line:%d,Failed to init apds9110\n", __func__, __LINE__);
			return ret;
		}

		data->enable_ps_sensor= 1;
#ifdef CONFIG_HUAWEI_DSM
		apds_dsm_save_threshold(data, APDS9110_FAR_INIT, APDS9110_NEAR_INIT);
#endif
		data->enable = APDS9110_DD_PRX_EN;
		/*Enable chip interrupts*/
		ret = apds9110_i2c_write(client,APDS9110_DD_INT_CFG_ADDR,APDS9110_DD_PRX_INT_EN,APDS9110_I2C_BYTE);
		/*Enable the chip*/
		ret += apds9110_i2c_write(client,APDS9110_DD_MAIN_CTRL_ADDR,data->enable,APDS9110_I2C_BYTE);
		if(ret < 0){
			APDS9110_INFO("%s:enable chip error,ret = %d\n", __func__,ret);
			return ret;
		}
		APDS9110_INFO("%s: line:%d,enable ps sensor,report far event,data->enable = 0x%x\n", __func__, __LINE__,data->enable);
		/*first report event  0 is close, 1 is far */
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, APDS9110_FAR_FLAG);
		input_sync(data->input_dev_ps);
		if (data->irq){
			operate_irq(data,1,true);
			/*set the property of pls irq,so the pls irq can wake up the sleeping system */
			irq_set_irq_wake(data->irq, 1);
		}
	}
	return ret;
}

static int apds9110_ps_calibration(struct i2c_client *client)
{
	int i;
	int ps_data_avg=0, ps_data_loop=0,ps_data_loop_tmp=0;
	int ps_meas_rate;
	struct apds9110_data *data = i2c_get_clientdata(client);
	APDS9110_INFO("%s\n", __func__);
	
	if (data->enable_ps_sensor != APDS_DISABLE_PS) {
		APDS9110_ERR("%s:sensor is active now\n", __func__);
		data->cal_flag=APDS9110_CALIBRATION_SENSOR_ACTIVE;
		return APDS9110_CALIBRATION_SENSOR_ACTIVE;
	}
	
	ps_meas_rate = i2c_smbus_read_byte_data(client, APDS9110_DD_PRX_MEAS_RATE_ADDR);
	if (ps_meas_rate < 0)
	{
		data->cal_flag=APDS9110_CALIBRATION_READ_RATE_ERROR;
		return APDS9110_CALIBRATION_READ_RATE_ERROR;
	}

	apds9110_dd_set_prx_meas_rate(client, 0x40|APDS9110_DD_PRX_MEAS_RES_8_BIT|APDS9110_DD_PRX_MEAS_RATE_6_25_MS);			
	apds9110_dd_set_main_ctrl(client, APDS9110_DD_PRX_EN);
	data->ps_gain_index = 31;
	while (data->ps_gain_index > 0)
	{
		apds9110_dd_set_prx_gain(client, psGainValueArray[data->ps_gain_index]);

		ps_data_avg=0;
		for (i=0; i<APDS9110_PS_CAL_LOOP; i++) {
			mdelay(10);	// must be greater than prx meas rate
		
			ps_data_loop = i2c_smbus_read_word_data(client, APDS9110_DD_PRX_DATA_ADDR);
			ps_data_loop_tmp=ps_data_loop;
			if (ps_data_loop < 0) 
			{
				data->cal_flag=APDS9110_CALIBRATION_READ_PDARA_ERROR;
				return APDS9110_CALIBRATION_READ_PDARA_ERROR;
			}
			ps_data_loop_tmp &= 0x7FF;
			APDS9110_ERR("pdata [%d] = %d  pdata2 [%d] = %d\n", i, ps_data_loop,i,ps_data_loop_tmp);
			ps_data_avg += ps_data_loop;
		}
		ps_data_avg = ps_data_avg/i;
		if (ps_data_avg > 40) {
			data->ps_gain_index -= 2;
		} else {
			break;
		}
	}
	apds9110_dd_set_prx_meas_rate(client, ps_meas_rate);
	apds9110_dd_set_main_ctrl(client, 0);
			
	APDS9110_INFO("APDS9110_PS_CAL pdata = %d\n", ps_data_avg);

	if ((ps_data_avg <= APDS9110_PS_CAL_CROSSTALK_HIGH) && (data->ps_gain_index > 0)) {                 
		data->ps_threshold = ps_data_avg + APDS9110_PS_DETECTION_THRESHOLD;                                                 
		data->ps_hysteresis_threshold = ps_data_avg + APDS9110_PS_HSYTERESIS_THRESHOLD;                                       

		APDS9110_INFO("new threshold=%d, hysteresis_threshold=%d\n", data->ps_threshold, data->ps_hysteresis_threshold);
		
#if (!APDS9110_PS_POLLING)
		APDS9110_INFO("polling  new threshold=%d, hysteresis_threshold=%d\n", data->ps_threshold, data->ps_hysteresis_threshold);
		apds9110_dd_set_prx_thresh(client, data->ps_hysteresis_threshold, data->ps_threshold);
#endif
	} else{
		data->cal_flag=APDS9110_CALIBRATION_PDATA_BIG_ERROR;
		return APDS9110_CALIBRATION_PDATA_BIG_ERROR;
	}
	data->cal_flag=0;	
	return 0;
}



static int apds9110_enable_ps_sensor(struct i2c_client *client,unsigned int val)
{
	struct apds9110_data *data = i2c_get_clientdata(client);
	int ret;

	APDS9110_INFO("%s,line %d:val=%d\n",__func__,__LINE__,val);
	if ((val != 0) && (val != 1)) {
		APDS9110_ERR("%s: invalid value=%d\n", __func__, val);
		return -EINVAL;
	}
	if (val == 1) {
		mutex_lock(&data->single_lock);
		ret = apds9110_open_ps_sensor(data, client);
		mutex_unlock(&data->single_lock);
		if(ret){
			APDS9110_ERR("%s,line %d:read power_value failed,open ps fail\n",__func__,__LINE__);
			return ret;
		}
#ifdef CONFIG_HUAWEI_DSM
		apds_dsm_no_irq_check(data);
#endif

		power_key_ps = false;
		schedule_delayed_work(&data->powerkey_work, msecs_to_jiffies(100));
	} else {

		if (data->enable_ps_sensor==1) {

			mutex_lock(&data->single_lock);
			
			data->enable_ps_sensor = 0;
			data->enable= 0x00;
			/*disable chip interrupts */
//			apds9110_i2c_write(client,APDS9110_DD_INT_CFG_ADDR,0x00,APDS9110_I2C_BYTE);
			/*disable the chip*/
			apds9110_i2c_write(client,APDS9110_DD_MAIN_CTRL_ADDR,data->enable,APDS9110_I2C_BYTE);

			mutex_unlock(&data->single_lock);
			APDS9110_INFO("%s: line:%d,disable pls sensor,data->enable = 0x%x\n", __func__, __LINE__,data->enable);
			cancel_work_sync(&data->dwork);
			cancel_delayed_work(&data->powerkey_work);
			if (data->irq)
			{
				/*when close the ps,make the wakeup property disabled*/
				irq_set_irq_wake(data->irq, 0);
				operate_irq(data,0,true);
			}
		}
	}

	/* Vote off  regulators if both light and prox sensor are off */
	if ((data->enable_ps_sensor == 0) &&(data->platform_data->power_on)){
		data->platform_data->power_on(false,data);
	}
	return 0;
}

static int apds9110_ps_set_enable(struct sensors_classdev *sensors_cdev,
		unsigned int enable)
{
	struct apds9110_data *data = container_of(sensors_cdev,
			struct apds9110_data, ps_cdev);
	if ((enable != 0) && (enable != 1)) {
		APDS9110_ERR("%s: invalid value(%d)\n", __func__, enable);
		return -EINVAL;
	}
	return apds9110_enable_ps_sensor(data->client, enable);
}

static ssize_t apds9110_show_pdata(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	int pdata;
	
	pdata=apds9110_i2c_read(client, APDS9110_DD_PRX_DATA_ADDR,APDS9110_I2C_WORD);
	if(pdata <0){
		APDS9110_ERR("%s,line %d:read pdata failed\n",__func__,__LINE__);
	}else{
		pdata &= 0x7FF;
	}

	return snprintf(buf,32, "%d\n", pdata);
}

static DEVICE_ATTR(pdata, S_IRUGO, apds9110_show_pdata, NULL);

/*
* show all registers' value to userspace
*/
static ssize_t apds9110_print_reg_buf(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int i=0;
	int j=0;
	char reg[APDS9110_REG_LEN];
	struct i2c_client *client = to_i2c_client(dev);

	/* read all register value and print to user*/
	for(i = 0,j=0; i < APDS9110_REG_MAX; j++ )
	{
		reg[j] = apds9110_i2c_read(client,i,APDS9110_I2C_BYTE);
		if(reg[j] <0){
			APDS9110_ERR("%s,line %d:read %d reg failed\n",__func__,__LINE__,i);
			return reg[j] ;
		}
		if( 0x03== i){
			i += 0x3;
		}else if(0x09==i){
			i += 0x10;
		}else{
			i += 1;
		}
	}

	return snprintf(buf,512,"reg[0x0~0x3]=0x%2x, 0x%2x, 0x%2x, 0x%2x\n"
			"reg[0x6~0x9]=0x%2x, 0x%2x, 0x%2x, 0x%2x\n"
			"reg[0x19~0x20]0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x, 0x%2x\n",
			reg[0x00],reg[0x01],reg[0x02],reg[0x03],
			reg[0x04],reg[0x05],reg[0x06],reg[0x07],
			reg[0x08],reg[0x09],reg[0x0a],reg[0x0b],reg[0x0c],reg[0x0d],reg[0x0e],reg[0x0f]);
}

static DEVICE_ATTR(dump_reg, S_IRUGO, apds9110_print_reg_buf, NULL);

static ssize_t apds9110_show_calibration(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9110_data *data = i2c_get_clientdata(client);
	if(NULL==data)
	{
		return snprintf(buf, 32, "pdata is null\n");
	}
	return snprintf(buf, 32,"%d\n", data->cal_flag);
}


static ssize_t apds9110_store_calibration(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{

	struct i2c_client *client = to_i2c_client(dev);	
	struct apds9110_data *data = i2c_get_clientdata(client);
	int err;
	u32 val;
    APDS9110_INFO("apds9110_store_calibration\n");
	err = kstrtoint(buf, 0, &val);
	if (err < 0) {
		APDS9110_ERR("%s:%d kstrtoint failed\n", __func__,__LINE__);
		return err;
	}
	APDS9110_INFO("%s,line%d:val =%d\n ", __func__, __LINE__, val);
	if(val == 1)
	{
		APDS9110_INFO("apds9110_store_calibration val = 1");
		mutex_lock(&data->single_lock);
		apds9110_ps_calibration(client);
		mutex_unlock(&data->single_lock);
	}
	return count;
}


static DEVICE_ATTR(avago_apds9110_calibration, S_IRUGO | S_IWUSR,apds9110_show_calibration,apds9110_store_calibration);

static ssize_t apds9110_show_calibration_result(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9110_data *data = i2c_get_clientdata(client);
	
	if (data->cal_flag)
	{
		APDS9110_INFO(" data->cal_flag<0\n");
		return snprintf(buf, sizeof(int),"%d\n", data->cal_flag);
	}
	return snprintf(buf, sizeof(int),"%d\n", data->ps_gain_index);
}

static ssize_t apds9110_store_calibration_result(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds9110_data *data = i2c_get_clientdata(client);
	int err;
	u32 val;
	
	err = kstrtoint(buf, 0, &val);
	if (err < 0) {
		APDS9110_ERR("%s:%d kstrtoint failed\n", __func__,__LINE__);
		return err;
	}
	APDS9110_INFO("%s,line%d:val =%d\n ", __func__, __LINE__, val);
	data->ps_gain_index = val;       //load gain
	return count;
}
static DEVICE_ATTR(avago_apds9110_calibration_result, S_IRUGO | S_IWUSR,apds9110_show_calibration_result,apds9110_store_calibration_result);

static struct attribute *apds9110_attributes[] = {
	&dev_attr_pdata.attr,
	&dev_attr_dump_reg.attr,
	&dev_attr_avago_apds9110_calibration.attr,
	&dev_attr_avago_apds9110_calibration_result.attr,
	NULL
};


static const struct attribute_group apds9110_attr_group = {
	.attrs = apds9110_attributes,
};


/*qualcom updated the regulator configure functions and we add them all*/
static int sensor_regulator_configure(struct apds9110_data *data, bool on)
{
	int rc;

	if (!on) {
		if (regulator_count_voltages(data->vdd) > 0){
			rc = regulator_set_voltage(data->vdd, 0, APDS9110_VDD_MAX_UV);
			if(rc)
				APDS9110_ERR("%s,line%d:Regulator set vdd failed rc=%d\n",__func__,__LINE__,rc);
		}
		regulator_put(data->vdd);

		if (regulator_count_voltages(data->vio) > 0){
			rc = regulator_set_voltage(data->vio, 0, APDS9110_VIO_MAX_UV);
			if(rc)
				APDS9110_ERR("%s,line%d:Regulator set vio failed rc=%d\n",__func__,__LINE__,rc);
		}
		regulator_put(data->vio);

	} else {
		data->vdd = regulator_get(&data->client->dev, "vdd");
		if (IS_ERR(data->vdd)) {
			rc = PTR_ERR(data->vdd);
			APDS9110_ERR("%s,line%d:Regulator get failed vdd rc=%d\n",__func__,__LINE__, rc);
			return rc;
		}

		if (regulator_count_voltages(data->vdd) > 0) {
			rc = regulator_set_voltage(data->vdd,
				APDS9110_VDD_MIN_UV, APDS9110_VDD_MAX_UV);
			if (rc) {
				APDS9110_ERR("%s,line%d:Regulator set failed vdd rc=%d\n",__func__,__LINE__,rc);
				goto reg_vdd_put;
			}
		}

		data->vio = regulator_get(&data->client->dev, "vio");
		if (IS_ERR(data->vio)) {
			rc = PTR_ERR(data->vio);
			APDS9110_ERR("%s,line%d:Regulator get failed vio rc=%d\n",__func__,__LINE__, rc);
			goto reg_vdd_set;
		}

		if (regulator_count_voltages(data->vio) > 0) {
			rc = regulator_set_voltage(data->vio,
				APDS9110_VIO_MIN_UV, APDS9110_VIO_MAX_UV);
			if (rc) {
				APDS9110_ERR("%s,line%d:Regulator set failed vio rc=%d\n",__func__,__LINE__, rc);
				goto reg_vio_put;
			}
		}

	}

	return 0;
reg_vio_put:
	regulator_put(data->vio);

reg_vdd_set:
	if (regulator_count_voltages(data->vdd) > 0)
		regulator_set_voltage(data->vdd, 0, APDS9110_VDD_MAX_UV);
reg_vdd_put:
	regulator_put(data->vdd);
	return rc;
}


/*In suspend and resume function,we only control the als,leave pls alone*/
static int apds9110_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int apds9110_resume(struct i2c_client *client)
{
	return 0;
}


/*pamameter subfunction of probe to reduce the complexity of probe function*/
static int apds9110_sensorclass_init(struct apds9110_data *data,struct i2c_client* client)
{
	int err;
	/* Register to sensors class */
	data->ps_cdev = sensors_proximity_cdev;
	data->ps_cdev.sensors_enable = apds9110_ps_set_enable;
	data->ps_cdev.sensors_poll_delay = NULL;

	err = sensors_classdev_register(&data ->input_dev_ps ->dev, &data->ps_cdev);
	if (err) {
		APDS9110_ERR("%s: Unable to register to sensors class: %d\n",__func__, err);
	}

	return err;
}


static void apds9110_parameter_init(struct apds9110_data *data)
{
	/* Set the default parameters */
	data->enable = 0x00;	/* default mode is standard */
	data->enable_ps_sensor = 0;	// default to 0
	data->count = 1;	// disable_irq is before enable_irq, so the initial value should more than zero
	data->ps_gain_index = 31;      //default gain
}

static int apds9110_input_init(struct apds9110_data *data)
{
	int err = 0;

	data->input_dev_ps = input_allocate_device();
	if (!data->input_dev_ps) {
		err = -ENOMEM;
		APDS9110_ERR("%s: Failed to allocate input device ps\n", __func__);
		goto exit;
	}

	set_bit(EV_ABS, data->input_dev_ps->evbit);

	input_set_abs_params(data->input_dev_ps, ABS_DISTANCE, 0, 5, 0, 0);

	data->input_dev_ps->name = "proximity";

	err = input_register_device(data->input_dev_ps);
	if (err) {
		err = -ENOMEM;
		APDS9110_ERR("%s: Unable to register input device ps: %s\n",
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
static int apds9110_irq_init(struct apds9110_data *data,struct i2c_client *client)
{
	int ret = 0;
	if (data->platform_data->irq_gpio)
	{
		ret = gpio_request(data->platform_data->irq_gpio,"apds9110_irq_gpio");
		if (ret)
		{
			APDS9110_ERR("%s, line %d:unable to request gpio [%d]\n", __func__, __LINE__,data->platform_data->irq_gpio);
			return ret;
		}
		else
		{
			ret = gpio_direction_input(data->platform_data->irq_gpio);
			if(ret)
			{
				APDS9110_ERR("%s, line %d: Failed to set gpio %d direction\n", __func__, __LINE__,data->platform_data->irq_gpio);
				return ret;
			}
		}
	}
	client->irq = gpio_to_irq(data->platform_data->irq_gpio);
	if (client->irq < 0) {
		ret = -EINVAL;
		APDS9110_ERR("%s, line %d:gpio_to_irq FAIL! IRQ=%d\n", __func__, __LINE__,data->platform_data->irq_gpio);
		return ret;
	}
	data->irq = client->irq;
	if (client->irq)
	{
		/*AP examination of low level to prevent lost interrupt*/
		if (request_irq(data->irq, apds9110_interrupt,IRQF_TRIGGER_LOW|IRQF_ONESHOT|IRQF_NO_SUSPEND, APDS9110_DRV_NAME, (void *)client) >= 0)
		{
			APDS9110_FLOW("%s, line %d:Received IRQ!\n", __func__, __LINE__);
			operate_irq(data,0,true);
		}
		else
		{
			APDS9110_ERR("%s, line %d:Failed to request IRQ!\n", __func__, __LINE__);
			ret = -EINVAL;
			return ret;
		}
	}
	return ret;
}


static int sensor_regulator_power_on(struct apds9110_data *data, bool on)
{
	int rc = 0;

	if (!on) {
		rc = regulator_disable(data->vdd);
		if (rc) {
			APDS9110_ERR("%s: Regulator vdd disable failed rc=%d\n", __func__, rc);
			return rc;
		}

		rc = regulator_disable(data->vio);
		if (rc) {
			APDS9110_ERR("%s: Regulator vdd disable failed rc=%d\n", __func__, rc);
			rc = regulator_enable(data->vdd);
			APDS9110_ERR("%s:Regulator vio re-enabled rc=%d\n",__func__, rc);
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
			APDS9110_ERR("%s:Regulator vdd enable failed rc=%d\n",__func__, rc);
			return rc;
		}

		rc = regulator_enable(data->vio);
		if (rc) {
			APDS9110_ERR("%s:Regulator vio enable failed rc=%d\n", __func__,rc);
			rc = regulator_disable(data->vdd);
			return rc;
		}
	}
enable_delay:
	msleep(10);
	APDS9110_FLOW("%s:Sensor regulator power on =%d\n",__func__, on);
	return rc;
}


static int sensor_platform_hw_power_on(bool on,struct apds9110_data *data)
{
	int err = 0;

	if (data->power_on != on) {
		if (!IS_ERR_OR_NULL(data->pinctrl)) {
			if (on)
				/*after poweron,set the INT pin the default state*/
				err = pinctrl_select_state(data->pinctrl,
					data->pin_default);
			if (err)
				APDS9110_ERR("%s,line%d:Can't select pinctrl state\n", __func__, __LINE__);
		}

		err = sensor_regulator_power_on(data, on);
		if (err)
			APDS9110_ERR("%s,line%d:Can't configure regulator!\n", __func__, __LINE__);
		else
			data->power_on = on;
	}

	return err;
}


static int sensor_platform_hw_init(struct apds9110_data *data)
{
	int error;

	error = sensor_regulator_configure(data, true);
	if (error < 0) {
		APDS9110_ERR("%s,line %d:unable to configure regulator\n",__func__,__LINE__);
		return error;
	}

	return 0;
}


static void sensor_platform_hw_exit(struct apds9110_data *data)
{
	int error;
	error = sensor_regulator_configure(data, false);
	if (error < 0) {
		APDS9110_ERR("%s,line %d:unable to configure regulator\n",__func__,__LINE__);
	}
}

static int apds9110_pinctrl_init(struct apds9110_data *data)
{
	struct i2c_client *client = data->client;

	data->pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR_OR_NULL(data->pinctrl)) {
		APDS9110_ERR("%s,line %d:Failed to get pinctrl\n",__func__,__LINE__);
		return PTR_ERR(data->pinctrl);
	}
	/*we have not set the sleep state of INT pin*/
	data->pin_default =
		pinctrl_lookup_state(data->pinctrl, "default");
	if (IS_ERR_OR_NULL(data->pin_default)) {
		APDS9110_ERR("%s,line %d:Failed to look up default state\n",__func__,__LINE__);
		return PTR_ERR(data->pin_default);
	}

	return 0;
}

static int sensor_parse_dt(struct device *dev,
		struct apds9110_platform_data *pdata)
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
			"apds9110,irq-gpio", 0, NULL);
	if (rc < 0) {
		APDS9110_ERR("%s,line %d:Unable to read irq gpio\n",__func__,__LINE__);
		return rc;
	}
	pdata->irq_gpio = rc;

	rc = of_property_read_u32(np, "apds9110,ps_wave", &tmp);
	if (rc) {
		APDS9110_ERR("%s,line %d:Unable to read pwave_value\n",__func__,__LINE__);
		return rc;
	}
	pdata ->pwave= tmp;

	rc = of_property_read_u32(np, "apds9110,ps_window", &tmp);
	if (rc) {
		APDS9110_ERR("%s,line %d:Unable to read pwindow_value\n",__func__,__LINE__);
		return rc;
	}
	pdata ->pwindow= tmp;

	rc = of_property_read_u32(np, "apds9110,ir_current", &tmp);
	if (rc) {
		APDS9110_ERR("%s,line %d:Unable to read ir_current\n",__func__,__LINE__);
		return rc;
	}
	pdata->ir_current = tmp;

	rc = of_property_read_u32(np, "apds9110,threshold_value", &tmp);
	if (rc) {
		APDS9110_ERR("%s,line %d:Unable to read threshold_value\n",__func__,__LINE__);
		return rc;
	}
	pdata->threshold_value = tmp;

	rc = of_property_read_u32(np, "apds9110,max_noise", &tmp);
	if (rc) {
		APDS9110_ERR("%s,line %d:Unable to read max_noise\n",__func__,__LINE__);
		return rc;
	}
	pdata->max_noise = tmp;

	return 0;
}

static void apds9110_powerkey_screen_handler(struct work_struct *work)
{
	struct apds9110_data *data = container_of((struct delayed_work *)work, struct apds9110_data, powerkey_work);
	if(power_key_ps)
	{
		APDS9110_INFO("%s : power_key_ps (%d) press\n",__func__, power_key_ps);
		power_key_ps=false;
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, APDS9110_FAR_FLAG);
		input_sync(data->input_dev_ps);
	}
		schedule_delayed_work(&data->powerkey_work, msecs_to_jiffies(500));
}

static int apds9110_read_device_id(struct i2c_client *client)
{
	int id;
	int err;
	id = apds9110_i2c_read(client, APDS9110_DD_PART_ID_ADDR,APDS9110_I2C_BYTE);
	if (id == 0xb1) {
		err = app_info_set("P-Sensor", "APDS9110");
		if (err < 0)/*failed to add app_info*/
		{
			APDS9110_ERR("%s %d:failed to add app_info\n", __func__, __LINE__);
		}
	} else {
		APDS9110_INFO("%s:No apds9110 id(%d)\n", __func__,id);
		return -ENODEV;
	}
	return 0;
}

/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver apds9110_driver;
static int apds9110_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct apds9110_data *data;
	struct apds9110_platform_data *pdata;
	int err = 0;

	
	APDS9110_INFO("%s,line %d:PROBE START.\n",__func__,__LINE__);

	if (client->dev.of_node) {
		/*Memory allocated with this function is automatically freed on driver detach.*/
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct apds9110_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			APDS9110_ERR("%s,line %d:Failed to allocate memory\n",__func__,__LINE__);
			err =-ENOMEM;
			goto exit;
		}
	
		client->dev.platform_data = pdata;
		err = sensor_parse_dt(&client->dev, pdata);
		if (err) {
			APDS9110_ERR("%s: sensor_parse_dt() err\n", __func__);
			goto exit_parse_dt;
		}
	}else{
		pdata = client->dev.platform_data;
		if (!pdata) {
			APDS9110_ERR("%s,line %d:No platform data\n",__func__,__LINE__);
			err = -ENODEV;
			goto exit;
		}
	}
	

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		APDS9110_ERR("%s,line %d:Failed to i2c_check_functionality\n",__func__,__LINE__);
		err = -EIO;
		goto exit_parse_dt;
	}


	data = kzalloc(sizeof(struct apds9110_data), GFP_KERNEL);
	if (!data) {
		APDS9110_ERR("%s,line %d:Failed to allocate memory\n",__func__,__LINE__);
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
		err = apds9110_dsm_init(data);
		if(err < 0)
			goto exit_uninit;
#endif

	i2c_set_clientdata(client, data);
	apds9110_parameter_init(data);

	/* initialize pinctrl */
	err = apds9110_pinctrl_init(data);
	if (err) {
		APDS9110_ERR("%s,line %d:Can't initialize pinctrl\n",__func__,__LINE__);
		goto exit_unregister_dsm;
	}

	err = pinctrl_select_state(data->pinctrl, data->pin_default);
	if (err) {
		APDS9110_ERR("%s,line %d:Can't select pinctrl default state\n",__func__,__LINE__);
		goto exit_unregister_dsm;
	}

	mutex_init(&data->update_lock);
	mutex_init(&data->single_lock);
	INIT_WORK(&data->dwork, apds9110_work_handler);

	INIT_DELAYED_WORK(&data->powerkey_work, apds9110_powerkey_screen_handler);

	err = apds9110_read_device_id(client);
	if (err) {
		APDS9110_ERR("%s: Failed to read apds993x\n", __func__);
		goto exit_unregister_dsm;
	}
	
	err = apds9110_init_client(client);
	if (err) {
		APDS9110_ERR("%s: Failed to init apds9110\n", __func__);
		goto exit_unregister_dsm;
	}

	err = apds9110_input_init(data);
	if(err)
		goto exit_unregister_dsm;

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &apds9110_attr_group);
	if (err)
		goto exit_unregister_dev_ps;

	wake_lock_init(&data->ps_report_wk, WAKE_LOCK_SUSPEND, "psensor_wakelock");

	apds9110_workqueue = create_workqueue("apds9110_work_queue");
	if (!apds9110_workqueue)
	{
		APDS9110_ERR("%s: Create ps_workqueue fail.\n", __func__);
		goto exit_unregister_sensorclass;
	}

	err = apds9110_sensorclass_init(data,client);
	if (err) {
		APDS9110_ERR("%s: Unable to register to sensors class: %d\n",
	__func__, err);
		goto exit_free_irq;
	}

	err=apds9110_irq_init(data,client);
	if(err)
		goto exit_remove_sysfs_group;

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_APS);
#endif

	if (pdata->power_on)
		err = pdata->power_on(false,data);
	APDS9110_INFO("%s: Support ver. %s enabled\n", __func__, DRIVER_VERSION);
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
	sysfs_remove_group(&client->dev.kobj, &apds9110_attr_group);
exit_unregister_dev_ps:
	input_unregister_device(data->input_dev_ps);
#ifdef CONFIG_HUAWEI_DSM
exit_unregister_dsm:
	apds_dsm_exit();
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

static int apds9110_remove(struct i2c_client *client)
{
	struct apds9110_data *data = i2c_get_clientdata(client);
	struct apds9110_platform_data *pdata = data->platform_data;

	/* Power down the device */
	data->enable = 0x00;
	apds9110_enable_ps_sensor(client, data->enable);
	wake_lock_destroy(&data->ps_report_wk);
	sysfs_remove_group(&client->dev.kobj, &apds9110_attr_group);

	input_unregister_device(data->input_dev_ps);

	free_irq(client->irq, data);
#ifdef CONFIG_HUAWEI_DSM
	apds9110_dsm_exit();
#endif

	if (pdata->power_on)
		pdata->power_on(false,data);

	if (pdata->exit)
		pdata->exit(data);

	kfree(data);

	return 0;

}

static const struct i2c_device_id apds9110_id[] = {
	{ "avgo_apds9110", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, apds9110_id);

static struct of_device_id apds9110_match_table[] = {
	{ .compatible = "avago,apds9110",},
	{ }
};

static struct i2c_driver apds9110_driver = {
	.driver = {
		.name	= APDS9110_DRV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = apds9110_match_table,
	},
	.suspend = apds9110_suspend,
	.resume	= apds9110_resume,
	.probe	= apds9110_probe,
	.remove	= apds9110_remove,
	.id_table = apds9110_id,
};

static int __init apds9110_init(void)
{
	return i2c_add_driver(&apds9110_driver);
}

static void __exit apds9110_exit(void)
{
	if (apds9110_workqueue){
		destroy_workqueue(apds9110_workqueue);
		apds9110_workqueue = NULL;
	}

	i2c_del_driver(&apds9110_driver);
}

MODULE_AUTHOR("Lee Kai Koon <kai-koon.lee@avagotech.com>");
MODULE_DESCRIPTION("APDS9110 proximity sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(apds9110_init);
module_exit(apds9110_exit);

