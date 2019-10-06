/* Copyright (c) 2011-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include "msm_sensor.h"
#include "msm_sd.h"
#include "camera.h"
#include "msm_cci.h"
#include "msm_camera_io_util.h"
#include "msm_camera_i2c_mux.h"
#include <linux/regulator/rpm-smd-regulator.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HUAWEI_DSM
#include "msm_camera_dsm.h"
#endif

#include "sensor_otp_common_if.h"
/* optimize camera print mipi packet and frame count log*/
#include "./msm.h"
#include "./csid/msm_csid.h"
#include "./eeprom/msm_eeprom.h"
#undef CDBG
#define CDBG(fmt, args...) pr_debug(fmt, ##args)
#define MSM_CAM_ID_NO_PULL 2

#ifdef CONFIG_HUAWEI_DSM
void  camera_report_dsm_err_msm_sensor(struct msm_sensor_ctrl_t *s_ctrl, int type, int err_num , const char* str);
#endif

static int msm_sensor_check_cam_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc=0,mcam_id=-1;
	const char *sensor_name;
	unsigned gpio;
	struct pinctrl_state *pin_up;
	struct pinctrl_state *pin_down;
	int pullup_read = -ENODEV; //pullup id value
	int pulldown_read = -ENODEV ; //pulldown id value
	int read_id = 0;
	struct msm_camera_slave_info *slave_info = s_ctrl->sensordata->slave_info;
	struct msm_pinctrl_info *sensor_ctrl = NULL;
	sensor_ctrl = &s_ctrl->sensordata->power_info.pinctrl_info;

	if(!s_ctrl->sensordata->power_info.gpio_conf->gpio_num_info->valid[SENSOR_GPIO_CAM_ID])
		return 0;

	sensor_name = s_ctrl->sensordata->sensor_name;

	if(!slave_info->cam_id_info){
		CDBG("%s, %s, no need to check camera id", __func__, sensor_name);
		return 0;
	}

	gpio=s_ctrl->sensordata->power_info.gpio_conf->gpio_num_info->gpio_num[SENSOR_GPIO_CAM_ID];
	pin_up = pinctrl_lookup_state(sensor_ctrl->pinctrl, "cam_up");
	pin_down = pinctrl_lookup_state(sensor_ctrl->pinctrl, "cam_down");


	if (IS_ERR_OR_NULL(pin_up) && IS_ERR_OR_NULL(pin_down)){
		gpio_direction_input(gpio);
		mcam_id=gpio_get_value(gpio);
		if(mcam_id == slave_info->cam_id_info->cam_expected_id){
			rc=0;
			pr_err("%s:%s gpio %d except value:%d match",__func__,sensor_name,gpio,mcam_id);
			return rc;
		}else{
			pr_err("%s:%s gpio %d value:%d not match",__func__,sensor_name,gpio,mcam_id);
			rc = -ENODEV;
			return rc;
		}
	}else if((!IS_ERR_OR_NULL(pin_up)) && (!IS_ERR_OR_NULL(pin_down))){
		pinctrl_select_state(sensor_ctrl->pinctrl,pin_up);
		udelay(10);
		gpio_direction_input(gpio);
		pullup_read = gpio_get_value(gpio);

		pinctrl_select_state(sensor_ctrl->pinctrl,pin_down);
		udelay(10);
		gpio_direction_input(gpio);
		pulldown_read = gpio_get_value(gpio);
		pr_err("%s:the pullup_read is %d and the pulldown_read is %d .\n",__func__,pullup_read,pulldown_read);

		if(pulldown_read != pullup_read){
			read_id = MSM_CAM_ID_NO_PULL;
		}
		else{
			read_id = pullup_read;
		}
		if(slave_info->cam_id_info->cam_expected_id == read_id){
			rc = 0;
			pr_err("%s:%s gpio %d except value:%d match! \n",__func__,sensor_name,gpio,read_id);
			return rc;
		}
		else{
			//detect fail not match
			pr_err("%s %d :  module id from camera module id not match!  \n",  __func__, __LINE__);
			rc = -ENODEV;
			return rc;
		}
	}else{
			rc = -ENODEV;
			pr_err("%s: cam id read error!\n", __func__);
			return rc;
	}
}


static void msm_sensor_adjust_mclk(struct msm_camera_power_ctrl_t *ctrl)
{
	int idx;
	struct msm_sensor_power_setting *power_setting;
	for (idx = 0; idx < ctrl->power_setting_size; idx++) {
		power_setting = &ctrl->power_setting[idx];
		if (power_setting->seq_type == SENSOR_CLK &&
			power_setting->seq_val ==  SENSOR_CAM_MCLK) {
			if (power_setting->config_val == 24000000) {
				power_setting->config_val = 23880000;
				CDBG("%s MCLK request adjusted to 23.88MHz\n"
							, __func__);
			}
			break;
		}
	}

	return;
}

static void msm_sensor_misc_regulator(
	struct msm_sensor_ctrl_t *sctrl, uint32_t enable)
{
	int32_t rc = 0;
	if (enable) {
		sctrl->misc_regulator = (void *)rpm_regulator_get(
			&sctrl->pdev->dev, sctrl->sensordata->misc_regulator);
		if (sctrl->misc_regulator) {
			rc = rpm_regulator_set_mode(sctrl->misc_regulator,
				RPM_REGULATOR_MODE_HPM);
			if (rc < 0) {
				pr_err("%s: Failed to set for rpm regulator on %s: %d\n",
					__func__,
					sctrl->sensordata->misc_regulator, rc);
				rpm_regulator_put(sctrl->misc_regulator);
			}
		} else {
			pr_err("%s: Failed to vote for rpm regulator on %s: %d\n",
				__func__,
				sctrl->sensordata->misc_regulator, rc);
		}
	} else {
		if (sctrl->misc_regulator) {
			rc = rpm_regulator_set_mode(
				(struct rpm_regulator *)sctrl->misc_regulator,
				RPM_REGULATOR_MODE_AUTO);
			if (rc < 0)
				pr_err("%s: Failed to set for rpm regulator on %s: %d\n",
					__func__,
					sctrl->sensordata->misc_regulator, rc);
			rpm_regulator_put(sctrl->misc_regulator);
		}
	}
}

int32_t msm_sensor_free_sensor_data(struct msm_sensor_ctrl_t *s_ctrl)
{
	if (!s_ctrl->pdev && !s_ctrl->sensor_i2c_client->client)
		return 0;
	kfree(s_ctrl->sensordata->slave_info);
	kfree(s_ctrl->sensordata->cam_slave_info);
	kfree(s_ctrl->sensordata->actuator_info);
	kfree(s_ctrl->sensordata->power_info.gpio_conf->gpio_num_info);
	kfree(s_ctrl->sensordata->power_info.gpio_conf->cam_gpio_req_tbl);
	kfree(s_ctrl->sensordata->power_info.gpio_conf);
	kfree(s_ctrl->sensordata->power_info.cam_vreg);
	kfree(s_ctrl->sensordata->power_info.power_setting);
	kfree(s_ctrl->sensordata->power_info.power_down_setting);
	kfree(s_ctrl->sensordata->csi_lane_params);
	kfree(s_ctrl->sensordata->sensor_info);
	if (s_ctrl->sensor_device_type == MSM_CAMERA_I2C_DEVICE) {
		msm_camera_i2c_dev_put_clk_info(
			&s_ctrl->sensor_i2c_client->client->dev,
			&s_ctrl->sensordata->power_info.clk_info,
			&s_ctrl->sensordata->power_info.clk_ptr,
			s_ctrl->sensordata->power_info.clk_info_size);
	} else {
		msm_camera_put_clk_info(s_ctrl->pdev,
			&s_ctrl->sensordata->power_info.clk_info,
			&s_ctrl->sensordata->power_info.clk_ptr,
			s_ctrl->sensordata->power_info.clk_info_size);
	}

	kfree(s_ctrl->sensordata);
	return 0;
}

int msm_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_power_ctrl_t *power_info;
	enum msm_camera_device_type_t sensor_device_type;
	struct msm_camera_i2c_client *sensor_i2c_client;

	if (!s_ctrl) {
		pr_err("%s:%d failed: s_ctrl %pK\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}

	if (s_ctrl->is_csid_tg_mode)
		return 0;

	power_info = &s_ctrl->sensordata->power_info;
	sensor_device_type = s_ctrl->sensor_device_type;
	sensor_i2c_client = s_ctrl->sensor_i2c_client;

	if (!power_info || !sensor_i2c_client) {
		pr_err("%s:%d failed: power_info %pK sensor_i2c_client %pK\n",
			__func__, __LINE__, power_info, sensor_i2c_client);
		return -EINVAL;
	}
	return msm_camera_power_down(power_info, sensor_device_type,
		sensor_i2c_client);
}

#define MAX_EEPROM_NUM 2
static int msm_sensor_check_vendor_id(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    int i = 0;
    struct v4l2_subdev *subdev_eeproms[MAX_EEPROM_NUM] = {NULL};
    uint32_t subdev_id[MAX_EEPROM_NUM] = {0};
    struct  msm_sensor_info_t *sensor_info = s_ctrl->sensordata->sensor_info;
    struct msm_eeprom_get_vendor_t eeprom_vendor = {0};
    struct msm_eeprom_ctrl_t *e_ctrl = NULL;

    if(sensor_info->subdev_id[SUB_MODULE_EEPROM] < 0)
    {
        pr_info("%s: no eeprom subdev sensor: %s\n",__func__,s_ctrl->sensordata->sensor_name);
        return rc;
    }
    if(!s_ctrl->sensordata->slave_info->cam_id_info || !s_ctrl->sensordata->slave_info->cam_id_info->cam_vendor_id)
    {
        pr_info("%s: cam_vendor_id is zero sensor: %s\n",__func__,s_ctrl->sensordata->sensor_name);
        return rc;
    }

    msm_sd_get_subdevs(subdev_eeproms,MAX_EEPROM_NUM,"msm_eeprom");

    for(i = 0; i<MAX_EEPROM_NUM; i++)
    {
        if(!subdev_eeproms[i])
            break;

        v4l2_subdev_call(subdev_eeproms[i], core, ioctl, 
            VIDIOC_MSM_SENSOR_GET_SUBDEV_ID, &subdev_id[i]);

        pr_info("%s: sensor:%s subdev_id[%d]=%d eeprom_subid=%d \n",__func__,s_ctrl->sensordata->sensor_name,i,subdev_id[i],\
            sensor_info->subdev_id[SUB_MODULE_EEPROM]);

        if(sensor_info->subdev_id[SUB_MODULE_EEPROM] != subdev_id[i])
        {
            continue;
        }

        if(subdev_eeproms[i]->dev_priv)
            e_ctrl = subdev_eeproms[i]->dev_priv;

        if(e_ctrl && e_ctrl->msm_eeprom_get_vendor_id)
        {
            eeprom_vendor.vendor_offset = s_ctrl->sensordata->slave_info->cam_id_info->cam_vendor_offset;
            pr_info("%s: from lib vendor_offset = %d\n",__func__, \
                s_ctrl->sensordata->slave_info->cam_id_info->cam_vendor_offset);
            if(!e_ctrl->msm_eeprom_get_vendor_id(e_ctrl,&eeprom_vendor))
            {
                //todo check eeprom_vendor.vendor_id with sensorinfo from .h file
                pr_info("%s: from lib vendor_id = %d\n",__func__, \
                s_ctrl->sensordata->slave_info->cam_id_info->cam_vendor_id);

                if(eeprom_vendor.vendor_id != s_ctrl->sensordata->slave_info->cam_id_info->cam_vendor_id)
                {
                    pr_err("%s: sensor:%s vendor id check fail\n",__func__,s_ctrl->sensordata->sensor_name);
                    return -1;
                }
                //add cam_module_version check here
                pr_err("%s: cam_module_version_support = %d \n",__func__,s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_support);
                if(s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_support && e_ctrl->msm_eeprom_get_module_version)
                {
                    eeprom_vendor.cam_module_version_offset = s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_offset;
                    pr_info("%s: from lib cam_module_version_offset = %d\n",__func__, \
                        s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_offset);
                    if(!e_ctrl->msm_eeprom_get_module_version(e_ctrl,&eeprom_vendor))
                    {
                        //todo check eeprom_vendor.cam_module_version with sensorinfo from .h file
                        pr_info("%s: from lib cam_module_version_start = %d\n",__func__, \
                        s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_start);
                        pr_info("%s: from lib cam_module_version_end = %d\n",__func__, \
                        s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_end);
                        if(eeprom_vendor.cam_module_version < s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_start
                            || eeprom_vendor.cam_module_version > s_ctrl->sensordata->slave_info->cam_id_info->cam_module_version_end)
                        {
                            pr_err("%s: sensor:%s camera module version check fail\n",__func__,s_ctrl->sensordata->sensor_name);
                            return -1;
                        }
                        return 0;
                    }
                    else
                    {
                        pr_err("%s: sensor:%s msm_eeprom_get_module_version fail\n",__func__,s_ctrl->sensordata->sensor_name);
                        return -1;
                    }
                }
                //end of check module version
                return 0;
            }
            else
            {
                pr_err("%s: sensor:%s msm_eeprom_get_vendor_id fail\n",__func__,s_ctrl->sensordata->sensor_name);
                return -1;
            }
        }
    }

    return 0;
}

int msm_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;
	struct msm_camera_power_ctrl_t *power_info;
	struct msm_camera_i2c_client *sensor_i2c_client;
	struct msm_camera_slave_info *slave_info;
	const char *sensor_name;
	uint32_t retry = 0;

	if (!s_ctrl) {
		pr_err("%s:%d failed: %pK\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}

	if (s_ctrl->is_csid_tg_mode)
		return 0;

	power_info = &s_ctrl->sensordata->power_info;
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	slave_info = s_ctrl->sensordata->slave_info;
	sensor_name = s_ctrl->sensordata->sensor_name;

	if (!power_info || !sensor_i2c_client || !slave_info ||
		!sensor_name) {
		pr_err("%s:%d failed: %pK %pK %pK %pK\n",
			__func__, __LINE__, power_info,
			sensor_i2c_client, slave_info, sensor_name);
		return -EINVAL;
	}

	if (s_ctrl->set_mclk_23880000)
		msm_sensor_adjust_mclk(power_info);

	for (retry = 0; retry < 3; retry++) {
		rc = msm_camera_power_up(power_info, s_ctrl->sensor_device_type,
			sensor_i2c_client);
		if (rc < 0)
			return rc;
		rc = msm_sensor_check_id(s_ctrl);
		if (rc < 0) {
			msm_camera_power_down(power_info,
				s_ctrl->sensor_device_type, sensor_i2c_client);
			msleep(20);
			continue;
		}

		rc = msm_sensor_check_cam_id(s_ctrl);
		if(rc < 0){
			msm_camera_power_down(power_info,
				s_ctrl->sensor_device_type, sensor_i2c_client);
            break;
		}
		/*no matter cam id match or not, no need retry, break directly*/
		rc = msm_sensor_check_vendor_id(s_ctrl);
		if(rc < 0){
			msm_camera_power_down(power_info,
				s_ctrl->sensor_device_type, sensor_i2c_client);
		}
		break;
	}

	return rc;
}

static uint16_t msm_sensor_id_by_mask(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t chipid)
{
	uint16_t sensor_id = chipid;
	int16_t sensor_id_mask = s_ctrl->sensordata->slave_info->sensor_id_mask;

	if (!sensor_id_mask)
		sensor_id_mask = ~sensor_id_mask;

	sensor_id &= sensor_id_mask;
	sensor_id_mask &= -sensor_id_mask;
	sensor_id_mask -= 1;
	while (sensor_id_mask) {
		sensor_id_mask >>= 1;
		sensor_id >>= 1;
	}
	return sensor_id;
}

int msm_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint16_t chipid = 0;
	struct msm_camera_i2c_client *sensor_i2c_client;
	struct msm_camera_slave_info *slave_info;
	const char *sensor_name;

	if (!s_ctrl) {
		pr_err("%s:%d failed: %pK\n",
			__func__, __LINE__, s_ctrl);
		return -EINVAL;
	}
	sensor_i2c_client = s_ctrl->sensor_i2c_client;
	slave_info = s_ctrl->sensordata->slave_info;
	sensor_name = s_ctrl->sensordata->sensor_name;

	if (!sensor_i2c_client || !slave_info || !sensor_name) {
		pr_err("%s:%d failed: %pK %pK %pK\n",
			__func__, __LINE__, sensor_i2c_client, slave_info,
			sensor_name);
		return -EINVAL;
	}

	rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, slave_info->sensor_id_reg_addr,
		&chipid, MSM_CAMERA_I2C_WORD_DATA);
	if (rc < 0) {
		pr_err("%s: %s: read id failed\n", __func__, sensor_name);
		return rc;
	}

	pr_debug("%s: read id: 0x%x expected id 0x%x:\n",
			__func__, chipid, slave_info->sensor_id);
	if (msm_sensor_id_by_mask(s_ctrl, chipid) != slave_info->sensor_id) {
		pr_err("%s chip id %x does not match %x\n",
				__func__, chipid, slave_info->sensor_id);
		return -ENODEV;
	}
	return rc;
}

static struct msm_sensor_ctrl_t *get_sctrl(struct v4l2_subdev *sd)
{
	return container_of(container_of(sd, struct msm_sd_subdev, sd),
		struct msm_sensor_ctrl_t, msm_sd);
}

static void msm_sensor_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;

	mutex_lock(s_ctrl->msm_sensor_mutex);
	if (s_ctrl->sensor_state == MSM_SENSOR_POWER_UP) {
		s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &s_ctrl->stop_setting);
		kfree(s_ctrl->stop_setting.reg_setting);
		s_ctrl->stop_setting.reg_setting = NULL;

		if (s_ctrl->func_tbl->sensor_power_down) {
			if (s_ctrl->sensordata->misc_regulator)
				msm_sensor_misc_regulator(s_ctrl, 0);

			rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);
			if (rc < 0) {
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
			}
			s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
			CDBG("%s:%d sensor state %d\n", __func__, __LINE__,
				s_ctrl->sensor_state);
		} else {
			pr_err("s_ctrl->func_tbl NULL\n");
		}
	}
	mutex_unlock(s_ctrl->msm_sensor_mutex);
	return;
}

static int msm_sensor_get_af_status(struct msm_sensor_ctrl_t *s_ctrl,
			void __user *argp)
{
	/* TO-DO: Need to set AF status register address and expected value
	We need to check the AF status in the sensor register and
	set the status in the *status variable accordingly*/
	return 0;
}
static int msm_sensor_get_afc_otp_info(struct msm_sensor_ctrl_t *s_ctrl,
			void __user *argp)
{
	int rc = 0;
#ifdef CONFIG_COMPAT
	struct sensorb_cfg_data32 *cdata = (struct sensorb_cfg_data32 *)argp;
	if (copy_to_user((void *)compat_ptr(cdata->cfg.setting),
		(void *)&s_ctrl->afc_otp_info, sizeof(struct msm_sensor_afc_otp_info))) {
		pr_err("%s:%d CONFIG_COMPAT afc_otp_info copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}
#else
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	if (copy_to_user((void *)cdata->cfg.setting,
		(void *)&s_ctrl->afc_otp_info, sizeof(struct msm_sensor_afc_otp_info))) {
		pr_err("%s:%d afc_otp_info copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}
#endif

	return rc;
}

static int msm_sensor_get_awb_otp_info(struct msm_sensor_ctrl_t *s_ctrl,
			void __user *argp)
{
	int rc = 0;
#ifdef CONFIG_COMPAT
	struct sensorb_cfg_data32 *cdata = (struct sensorb_cfg_data32 *)argp;
	if (copy_to_user( (void *)compat_ptr(cdata->cfg.setting),
		(void*)& s_ctrl->awb_otp_info, sizeof(struct msm_sensor_awb_otp_info))) {
		pr_err("%s:%d awb_otp_info copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}

#else
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	if (copy_to_user((void *)cdata->cfg.setting,
		(void *)& s_ctrl->awb_otp_info, sizeof(struct msm_sensor_awb_otp_info))) {
		pr_err("%s:%d awb_otp_info copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}
#endif
	return rc;
}

static int msm_sensor_get_lsc_otp_info(struct msm_sensor_ctrl_t *s_ctrl,
			void __user *argp)
{
	int rc = 0;
	int i;
#ifdef CONFIG_COMPAT
	struct sensorb_cfg_data32 *cdata = (struct sensorb_cfg_data32 *)argp;
	if (copy_to_user( (void *)compat_ptr(cdata->cfg.setting),
		(void*)& s_ctrl->lsc_otp_info, sizeof(struct msm_sensor_lsc_otp_info))) {
		pr_err("%s:%d awb_otp_info copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}

#else
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	if (copy_to_user((void *)cdata->cfg.setting,
		(void *)& s_ctrl->lsc_otp_info, sizeof(struct msm_sensor_lsc_otp_info))) {
		pr_err("%s:%d awb_otp_info copy failed\n", __func__, __LINE__);
		rc = -EFAULT;
	}
#endif

	return rc;
}

static long msm_sensor_subdev_ioctl(struct v4l2_subdev *sd,
			unsigned int cmd, void *arg)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	void __user *argp = (void __user *)arg;
	if (!s_ctrl) {
		pr_err("%s s_ctrl NULL\n", __func__);
		return -EBADF;
	}
	switch (cmd) {
	case VIDIOC_MSM_SENSOR_CFG:
#ifdef CONFIG_COMPAT
		if (is_compat_task())
			rc = s_ctrl->func_tbl->sensor_config32(s_ctrl, argp);
		else
#endif
			rc = s_ctrl->func_tbl->sensor_config(s_ctrl, argp);
		return rc;
	case VIDIOC_MSM_SENSOR_GET_AF_STATUS:
		return msm_sensor_get_af_status(s_ctrl, argp);
	case VIDIOC_MSM_SENSOR_RELEASE:
	case MSM_SD_SHUTDOWN:
		msm_sensor_stop_stream(s_ctrl);
		return 0;
	case MSM_SD_NOTIFY_FREEZE:
		return 0;
	case MSM_SD_UNNOTIFY_FREEZE:
		return 0;
	default:
		return -ENOIOCTLCMD;
	}
}

/* optimize camera print mipi packet and frame count log*/
static int read_times = 0;
#define HW_PRINT_PACKET_NUM_TIME 5 //print 5 times
#define HW_READ_PACKET_NUM_TIME 100 //100ms
#define MAX_CSID_NUM 3// from dtsi qcom,csid-sd-index
struct hw_sensor_fct {
	char *sensor_name; //sensor name from sensor_init.c
	uint32_t fct_reg_addr; //the frame count reg addr
	enum msm_camera_i2c_data_type type; //frame count reg data type
};


#ifdef CONFIG_HUAWEI_DSM
static int csi_pkg_count = 0;
static struct sensor_frame_count{
	int sensor_frame_read[HW_PRINT_PACKET_NUM_TIME];
	int count;
} sensor_frame_dsm_cnt;
#endif


/*
sensor name      frame count reg addr
imx214       ---0x0005
*/

struct hw_sensor_fct sensor_fct_list[] ={
	{"imx298_liteon",0x0005,MSM_CAMERA_I2C_BYTE_DATA},
	{"imx298_lgit",0x0005,MSM_CAMERA_I2C_BYTE_DATA},
	{"imx298_sunny",0x0005,MSM_CAMERA_I2C_BYTE_DATA},
	{"imx298_foxconn",0x0005,MSM_CAMERA_I2C_BYTE_DATA},
        {"imx486_sunny_trt", 0x0005, MSM_CAMERA_I2C_BYTE_DATA},
        {"imx486_ofilm_trt", 0x0005, MSM_CAMERA_I2C_BYTE_DATA},
        {"ov12a10_liteon_trt", 0x483e, MSM_CAMERA_I2C_WORD_DATA},
};

static int hw_sensor_read_framecount(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = -1;
	int i = 0;
	uint16_t framecount = 0;
	struct msm_camera_i2c_client *sensor_i2c_client = NULL;
	int sensor_list_size = sizeof(sensor_fct_list) / sizeof(sensor_fct_list[0]);

	if(!s_ctrl || !s_ctrl->sensordata || !s_ctrl->sensordata->sensor_name)
	return rc;

	//check current sensor support read frame count reg?
	for(i  = 0; i < sensor_list_size; i++)
	{
		if(0 == strcmp(s_ctrl->sensordata->sensor_name, sensor_fct_list[i].sensor_name))
		{
			break;
		}
	}
	if(i >= sensor_list_size)
	{
		pr_info("%s:%s don't support read frame count \n",__func__,
		s_ctrl->sensordata->sensor_name);
		return rc;
	}

	//pr_info("%s:%s i=%d support read frame count! \n",__func__,
	//s_ctrl->sensordata->sensor_name, i);

	sensor_i2c_client = s_ctrl->sensor_i2c_client;

	if(!sensor_i2c_client)
	{
		pr_err("%s: sensor_i2c_client is NULL \n",__func__);
		return rc;
	}

	rc = sensor_i2c_client->i2c_func_tbl->i2c_read(
		sensor_i2c_client, sensor_fct_list[i].fct_reg_addr,
		&framecount, sensor_fct_list[i].type);

	if(rc < 0)
	{
		pr_err("%s: read framecount failed\n", __func__);
		return rc;
	}

	return framecount;
}

static void read_framecount_work_handler(struct work_struct *work)
{
	struct v4l2_subdev *subdev_csids[MAX_CSID_NUM] = {NULL};
	struct csid_device *csid_dev = NULL;

	int i = 0;
	int frm_cnt = 0;
	uint32_t csid_pkg = 0;
	struct msm_sensor_ctrl_t *s_ctrl = container_of(work, struct msm_sensor_ctrl_t,frm_cnt_work.work);
	if(!s_ctrl || s_ctrl->sensor_state != MSM_SENSOR_POWER_UP || (read_times <= 0))
	{
		read_times = 0;
		pr_err("%s:%d\n",__func__,__LINE__);
		return;
	}

	//read sensor send
	if(s_ctrl->func_tbl->sensor_read_framecount)
	{
		frm_cnt = s_ctrl->func_tbl->sensor_read_framecount(s_ctrl);
		if(frm_cnt > 0)
			pr_info("%s: read_times[%d] framecount = 0x%x \n",__func__,read_times,frm_cnt);
	}
#ifdef CONFIG_HUAWEI_DSM
	if (sensor_frame_dsm_cnt.count < HW_PRINT_PACKET_NUM_TIME)
	{
		sensor_frame_dsm_cnt.sensor_frame_read[sensor_frame_dsm_cnt.count] = frm_cnt;
		sensor_frame_dsm_cnt.count++;
	}
#endif

	//read csid receive 
	msm_sd_get_subdevs(subdev_csids,MAX_CSID_NUM,"msm_csid");

	//we have two csids
	//uint32_t subdev_id[MAX_CSID_NUM] = {0};
	for(i = 0; i<MAX_CSID_NUM; i++)
	{
		if(!subdev_csids[i])
		continue;
		/*
		v4l2_subdev_call(subdev_csids[i], core, ioctl, 
			VIDIOC_MSM_SENSOR_GET_SUBDEV_ID, &subdev_id[i]);
		*/
		if(subdev_csids[i]->dev_priv)
			csid_dev = subdev_csids[i]->dev_priv;

		if(!csid_dev)
		{
			read_times = 0;
			pr_err("%s: csid_dev[%d] is NULL \n",__func__,i);
			continue;
		}

		//back camera use csid0, front camera use csid1
		if(csid_dev->csid_state == CSID_POWER_UP && csid_dev->csid_read_mipi_pkg)
		{
			csid_pkg = csid_dev->csid_read_mipi_pkg(csid_dev);
			pr_info("%s: csid[%d] read_times[%d] total mipi packet = %u\n",__func__,
				csid_dev->pdev->id, read_times,csid_pkg);
#ifdef CONFIG_HUAWEI_DSM
			csi_pkg_count += csid_pkg;
#endif
			break;
		}
	}

	read_times--;
	if(read_times > 0) {
		schedule_delayed_work(&s_ctrl->frm_cnt_work, msecs_to_jiffies(HW_READ_PACKET_NUM_TIME));
	}
	#ifdef CONFIG_HUAWEI_DSM
	else if(csi_pkg_count <= 0)
	{
		int len = 0;
		int rc = 0;
		memset(camera_dsm_log_buff, 0, MSM_CAMERA_DSM_BUFFER_SIZE);
		len += snprintf(camera_dsm_log_buff+len, MSM_CAMERA_DSM_BUFFER_SIZE-len, "camera didn't get any frame.\n 5 frame from sensor at the beginning:\n");
		if ((len < 0) || (len >= MSM_CAMERA_DSM_BUFFER_SIZE -1))
			pr_err("%s %d. write dsm buf error\n",__func__,__LINE__);

		for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
		{
			len += snprintf(camera_dsm_log_buff+len, MSM_CAMERA_DSM_BUFFER_SIZE-len, "  %d", sensor_frame_dsm_cnt.sensor_frame_read[i]);
			if ((len < 0) || (len >= MSM_CAMERA_DSM_BUFFER_SIZE -1))
			pr_err("%s %d. write dsm buf error\n",__func__,__LINE__);
		}
	len += snprintf(camera_dsm_log_buff+len, MSM_CAMERA_DSM_BUFFER_SIZE-len, "\n");
		if ((len < 0) || (len >= MSM_CAMERA_DSM_BUFFER_SIZE -1))
			pr_err("%s %d. write dsm buf error\n",__func__,__LINE__);

		rc = camera_report_dsm_err(DSM_CAMERA_SENSOR_NO_FRAME, csi_pkg_count, camera_dsm_log_buff);
		if (rc < 0)
		{
			pr_err("%s. report dsm err fail.\n", __func__);
		}
	}
#endif


}

#ifdef CONFIG_COMPAT
static long msm_sensor_subdev_do_ioctl(
	struct file *file, unsigned int cmd, void *arg)
{
	struct video_device *vdev = video_devdata(file);
	struct v4l2_subdev *sd = vdev_to_v4l2_subdev(vdev);
	switch (cmd) {
	case VIDIOC_MSM_SENSOR_CFG32:
		cmd = VIDIOC_MSM_SENSOR_CFG;
	default:
		return msm_sensor_subdev_ioctl(sd, cmd, arg);
	}
}

long msm_sensor_subdev_fops_ioctl(struct file *file,
	unsigned int cmd, unsigned long arg)
{
	return video_usercopy(file, cmd, arg, msm_sensor_subdev_do_ioctl);
}

static int msm_sensor_config32(struct msm_sensor_ctrl_t *s_ctrl,
	void __user *argp)
{
	struct sensorb_cfg_data32 *cdata = (struct sensorb_cfg_data32 *)argp;
	int32_t rc = 0;
	int32_t i = 0;
	int32_t otp_index = -1;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++) {
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
			cdata->cfg.sensor_info.subdev_intf[i] =
				s_ctrl->sensordata->sensor_info->subdev_intf[i];
		}
		cdata->cfg.sensor_info.is_mount_angle_valid =
			s_ctrl->sensordata->sensor_info->is_mount_angle_valid;
		cdata->cfg.sensor_info.sensor_mount_angle =
			s_ctrl->sensordata->sensor_info->sensor_mount_angle;
		cdata->cfg.sensor_info.position =
			s_ctrl->sensordata->sensor_info->position;
		cdata->cfg.sensor_info.modes_supported =
			s_ctrl->sensordata->sensor_info->modes_supported;
		CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		CDBG("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++) {
			CDBG("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);
			CDBG("%s:%d subdev_intf[%d] %d\n", __func__, __LINE__,
				i, cdata->cfg.sensor_info.subdev_intf[i]);
		}
		CDBG("%s:%d mount angle valid %d value %d\n", __func__,
			__LINE__, cdata->cfg.sensor_info.is_mount_angle_valid,
			cdata->cfg.sensor_info.sensor_mount_angle);

		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		cdata->cfg.sensor_init_params.modes_supported =
			s_ctrl->sensordata->sensor_info->modes_supported;
		cdata->cfg.sensor_init_params.position =
			s_ctrl->sensordata->sensor_info->position;
		cdata->cfg.sensor_init_params.sensor_mount_angle =
			s_ctrl->sensordata->sensor_info->sensor_mount_angle;
		CDBG("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;
	case CFG_WRITE_I2C_ARRAY:
	case CFG_WRITE_I2C_ARRAY_SYNC:
	case CFG_WRITE_I2C_ARRAY_SYNC_BLOCK:
	case CFG_WRITE_I2C_ARRAY_ASYNC: {
		struct msm_camera_i2c_reg_setting32 conf_array32;
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(&conf_array32,
			(void *)compat_ptr(cdata->cfg.setting),
			sizeof(struct msm_camera_i2c_reg_setting32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		conf_array.addr_type = conf_array32.addr_type;
		conf_array.data_type = conf_array32.data_type;
		conf_array.delay = conf_array32.delay;
		conf_array.size = conf_array32.size;
		conf_array.reg_setting = compat_ptr(conf_array32.reg_setting);

		if (!conf_array.size ||
			conf_array.size > I2C_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting,
			(void *)(conf_array.reg_setting),
			conf_array.size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;

		if (CFG_WRITE_I2C_ARRAY == cdata->cfgtype)
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table(s_ctrl->sensor_i2c_client,
				&conf_array);
		else if (CFG_WRITE_I2C_ARRAY_ASYNC == cdata->cfgtype)
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table_async(s_ctrl->sensor_i2c_client,
				&conf_array);
		else if (CFG_WRITE_I2C_ARRAY_SYNC_BLOCK == cdata->cfgtype)
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table_sync_block(
				s_ctrl->sensor_i2c_client,
				&conf_array);
		else
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table_sync(s_ctrl->sensor_i2c_client,
				&conf_array);

		kfree(reg_setting);
		break;
	}
	case CFG_SLAVE_READ_I2C: {
		struct msm_camera_i2c_read_config read_config;
		struct msm_camera_i2c_read_config *read_config_ptr = NULL;
		uint16_t local_data = 0;
		uint16_t orig_slave_addr = 0, read_slave_addr = 0;
		uint16_t orig_addr_type = 0, read_addr_type = 0;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		read_config_ptr =
			(struct msm_camera_i2c_read_config *)
			compat_ptr(cdata->cfg.setting);

		if (copy_from_user(&read_config, read_config_ptr,
			sizeof(struct msm_camera_i2c_read_config))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		read_slave_addr = read_config.slave_addr;
		read_addr_type = read_config.addr_type;

		CDBG("%s:CFG_SLAVE_READ_I2C:", __func__);
		CDBG("%s:slave_addr=0x%x reg_addr=0x%x, data_type=%d\n",
			__func__, read_config.slave_addr,
			read_config.reg_addr, read_config.data_type);
		if (s_ctrl->sensor_i2c_client->cci_client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->cci_client->sid;
			s_ctrl->sensor_i2c_client->cci_client->sid =
				read_slave_addr >> 1;
		} else if (s_ctrl->sensor_i2c_client->client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->client->addr;
			s_ctrl->sensor_i2c_client->client->addr =
				read_slave_addr >> 1;
		} else {
			pr_err("%s: error: no i2c/cci client found.", __func__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s:orig_slave_addr=0x%x, new_slave_addr=0x%x",
				__func__, orig_slave_addr,
				read_slave_addr >> 1);

		orig_addr_type = s_ctrl->sensor_i2c_client->addr_type;
		s_ctrl->sensor_i2c_client->addr_type = read_addr_type;

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
				s_ctrl->sensor_i2c_client,
				read_config.reg_addr,
				&local_data, read_config.data_type);
		if (s_ctrl->sensor_i2c_client->cci_client) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				orig_slave_addr;
		} else if (s_ctrl->sensor_i2c_client->client) {
			s_ctrl->sensor_i2c_client->client->addr =
				orig_slave_addr;
		}
		s_ctrl->sensor_i2c_client->addr_type = orig_addr_type;

		pr_debug("slave_read %x %x %x\n", read_slave_addr,
			read_config.reg_addr, local_data);

		if (rc < 0) {
			pr_err("%s:%d: i2c_read failed\n", __func__, __LINE__);
			break;
		}
		read_config_ptr->data = local_data;
		break;
	}
	case CFG_SLAVE_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_array_write_config32 write_config32;
		struct msm_camera_i2c_array_write_config write_config;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		uint16_t orig_slave_addr = 0, write_slave_addr = 0;
		uint16_t orig_addr_type = 0, write_addr_type = 0;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (copy_from_user(&write_config32,
				(void *)compat_ptr(cdata->cfg.setting),
				sizeof(
				struct msm_camera_i2c_array_write_config32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		write_config.slave_addr = write_config32.slave_addr;
		write_config.conf_array.addr_type =
			write_config32.conf_array.addr_type;
		write_config.conf_array.data_type =
			write_config32.conf_array.data_type;
		write_config.conf_array.delay =
			write_config32.conf_array.delay;
		write_config.conf_array.size =
			write_config32.conf_array.size;
		write_config.conf_array.reg_setting =
			compat_ptr(write_config32.conf_array.reg_setting);

		pr_debug("%s:CFG_SLAVE_WRITE_I2C_ARRAY:\n", __func__);
		pr_debug("%s:slave_addr=0x%x, array_size=%d addr_type=%d data_type=%d\n",
			__func__,
			write_config.slave_addr,
			write_config.conf_array.size,
			write_config.conf_array.addr_type,
			write_config.conf_array.data_type);

		if (!write_config.conf_array.size ||
			write_config.conf_array.size > I2C_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(write_config.conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting,
				(void *)(write_config.conf_array.reg_setting),
				write_config.conf_array.size *
				sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		write_config.conf_array.reg_setting = reg_setting;
		write_slave_addr = write_config.slave_addr;
		write_addr_type = write_config.conf_array.addr_type;

		if (s_ctrl->sensor_i2c_client->cci_client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->cci_client->sid;
			s_ctrl->sensor_i2c_client->cci_client->sid =
				write_slave_addr >> 1;
		} else if (s_ctrl->sensor_i2c_client->client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->client->addr;
			s_ctrl->sensor_i2c_client->client->addr =
				write_slave_addr >> 1;
		} else {
			pr_err("%s: error: no i2c/cci client found.",
				__func__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		pr_debug("%s:orig_slave_addr=0x%x, new_slave_addr=0x%x\n",
				__func__, orig_slave_addr,
				write_slave_addr >> 1);
		orig_addr_type = s_ctrl->sensor_i2c_client->addr_type;
		s_ctrl->sensor_i2c_client->addr_type = write_addr_type;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &(write_config.conf_array));

		s_ctrl->sensor_i2c_client->addr_type = orig_addr_type;
		if (s_ctrl->sensor_i2c_client->cci_client) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				orig_slave_addr;
		} else if (s_ctrl->sensor_i2c_client->client) {
			s_ctrl->sensor_i2c_client->client->addr =
				orig_slave_addr;
		} else {
			pr_err("%s: error: no i2c/cci client found.\n",
				__func__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting32 conf_array32;
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(&conf_array32,
			(void *)compat_ptr(cdata->cfg.setting),
			sizeof(struct msm_camera_i2c_seq_reg_setting32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		conf_array.addr_type = conf_array32.addr_type;
		conf_array.delay = conf_array32.delay;
		conf_array.size = conf_array32.size;
		conf_array.reg_setting = compat_ptr(conf_array32.reg_setting);

		if (!conf_array.size ||
			conf_array.size > I2C_SEQ_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}

	case CFG_POWER_UP:
		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_DOWN) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}
		if (s_ctrl->func_tbl->sensor_power_up) {
			if (s_ctrl->sensordata->misc_regulator)
				msm_sensor_misc_regulator(s_ctrl, 1);

			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
			if (rc < 0) {
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
				break;
			}
			s_ctrl->sensor_state = MSM_SENSOR_POWER_UP;
			CDBG("%s:%d sensor state %d\n", __func__, __LINE__,
				s_ctrl->sensor_state);
		} else {
			rc = -EFAULT;
		}
		break;
	case CFG_POWER_DOWN:
		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		kfree(s_ctrl->stop_setting.reg_setting);
		s_ctrl->stop_setting.reg_setting = NULL;
		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}
		/* optimize camera print mipi packet and frame count log*/
		if(read_times > 0) {
			pr_info("%s: cancel frm_cnt_work read_times = %d\n",__func__,read_times);
			cancel_delayed_work_sync(&s_ctrl->frm_cnt_work);
			read_times = 0;
#ifdef CONFIG_HUAWEI_DSM
			csi_pkg_count = 0;
			sensor_frame_dsm_cnt.count = 0;
			for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
			{
				sensor_frame_dsm_cnt.sensor_frame_read[i] = 0;
			}
#endif
		}
		if (s_ctrl->func_tbl->sensor_power_down) {
			if (s_ctrl->sensordata->misc_regulator)
				msm_sensor_misc_regulator(s_ctrl, 0);

			rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);
			if (rc < 0) {
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
				break;
			}
			s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
			CDBG("%s:%d sensor state %d\n", __func__, __LINE__,
				s_ctrl->sensor_state);
		} else {
			rc = -EFAULT;
		}
#ifdef CONFIG_HUAWEI_DSM
		camera_is_closing = 0;
#endif
		break;
	/* optimize camera print mipi packet and frame count log*/
	case CFG_START_FRM_CNT:
	{
		cancel_delayed_work_sync(&s_ctrl->frm_cnt_work);
#ifdef CONFIG_HUAWEI_DSM
		csi_pkg_count = 0;
		sensor_frame_dsm_cnt.count = 0;
		for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
		{
			sensor_frame_dsm_cnt.sensor_frame_read[i] = 0;
		}
#endif
		read_times = HW_PRINT_PACKET_NUM_TIME;
		schedule_delayed_work(&s_ctrl->frm_cnt_work, msecs_to_jiffies(HW_READ_PACKET_NUM_TIME));
		pr_info("%s:%s start read frame count work \n",__func__,s_ctrl->sensordata->sensor_name);
	}
	break;

    case CFG_STOP_FRM_CNT:
	{
		cancel_delayed_work_sync(&s_ctrl->frm_cnt_work);
		read_times = 0;
#ifdef CONFIG_HUAWEI_DSM
		csi_pkg_count = 0;
		sensor_frame_dsm_cnt.count = 0;
		for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
		{
			sensor_frame_dsm_cnt.sensor_frame_read[i] = 0;
		}
#endif
		pr_info("%s:%s stop read frame count work \n",__func__,s_ctrl->sensordata->sensor_name);
	}
	break;

	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting32 stop_setting32;
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (copy_from_user(&stop_setting32,
				(void *)compat_ptr((cdata->cfg.setting)),
			sizeof(struct msm_camera_i2c_reg_setting32))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		stop_setting->addr_type = stop_setting32.addr_type;
		stop_setting->data_type = stop_setting32.data_type;
		stop_setting->delay = stop_setting32.delay;
		stop_setting->size = stop_setting32.size;

		reg_setting = compat_ptr(stop_setting32.reg_setting);

		if (!stop_setting->size) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
			(void *)reg_setting,
			stop_setting->size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
	}

	case CFG_SET_I2C_SYNC_PARAM: {
		struct msm_camera_cci_ctrl cci_ctrl;

		s_ctrl->sensor_i2c_client->cci_client->cid =
			cdata->cfg.sensor_i2c_sync_params.cid;
		s_ctrl->sensor_i2c_client->cci_client->id_map =
			cdata->cfg.sensor_i2c_sync_params.csid;

		CDBG("I2C_SYNC_PARAM CID:%d, line:%d delay:%d, cdid:%d\n",
			s_ctrl->sensor_i2c_client->cci_client->cid,
			cdata->cfg.sensor_i2c_sync_params.line,
			cdata->cfg.sensor_i2c_sync_params.delay,
			cdata->cfg.sensor_i2c_sync_params.csid);

		cci_ctrl.cmd = MSM_CCI_SET_SYNC_CID;
		cci_ctrl.cfg.cci_wait_sync_cfg.line =
			cdata->cfg.sensor_i2c_sync_params.line;
		cci_ctrl.cfg.cci_wait_sync_cfg.delay =
			cdata->cfg.sensor_i2c_sync_params.delay;
		cci_ctrl.cfg.cci_wait_sync_cfg.cid =
			cdata->cfg.sensor_i2c_sync_params.cid;
		cci_ctrl.cfg.cci_wait_sync_cfg.csid =
			cdata->cfg.sensor_i2c_sync_params.csid;
		rc = v4l2_subdev_call(s_ctrl->sensor_i2c_client->
				cci_client->cci_subdev,
				core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		if (rc < 0) {
			pr_err("%s: line %d rc = %d\n", __func__, __LINE__, rc);
			rc = -EFAULT;
			break;
		}
	}
		break;
	case CFG_GET_OTP_FLAG: {
		CDBG("%s,%d: CFG_GET_OTP_FLAG\n", __func__, __LINE__);
		//if power up
		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP)
		{
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}
		//set otp info
		if ( is_exist_otp_function(s_ctrl, &otp_index) )
		{
			rc = otp_function_lists[otp_index].sensor_otp_function(s_ctrl, otp_index);
			if (rc < 0)
			{
#ifdef CONFIG_HUAWEI_DSM
			camera_report_dsm_err_msm_sensor(s_ctrl, DSM_CAMERA_OTP_ERR, rc, NULL);
#endif
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
			}
			pr_info("%s,%d s_ctrl->hw_otp_check_flag.mmi_otp_check_flag = 0x%x\n", \
			__func__, __LINE__, s_ctrl->hw_otp_check_flag.mmi_otp_check_flag);
		}
		else
		{
		    /*  unsupport otp operation, set the mmi_otp_check_flag = 0 */
			s_ctrl->hw_otp_check_flag.mmi_otp_check_flag = 0;
			pr_err("%s, %d: %s unsupport otp operation, mmi_otp_check_flag = 0.\n", __func__,
					__LINE__, s_ctrl->sensordata->sensor_name);
		}
		if (copy_to_user((void *)compat_ptr(cdata->cfg.setting),
			(void *)&s_ctrl->hw_otp_check_flag, sizeof(struct msm_sensor_mmi_otp_flag))) {
				pr_err("%s:%d copy failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
			}

		break;
	}
	case CFG_SET_AFC_OTP_INFO:
		rc = msm_sensor_get_afc_otp_info(s_ctrl, argp);
		break;

	case CFG_SET_AWB_OTP_INFO:
		rc = msm_sensor_get_awb_otp_info(s_ctrl, argp);
		break;
	case CFG_SET_LSC_OTP_INFO:
		rc = msm_sensor_get_lsc_otp_info(s_ctrl, argp);
		break;
	default:
		rc = -EFAULT;
		break;
	}

DONE:
	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}
#endif

int msm_sensor_config(struct msm_sensor_ctrl_t *s_ctrl, void __user *argp)
{
	struct sensorb_cfg_data *cdata = (struct sensorb_cfg_data *)argp;
	int32_t rc = 0;
	int32_t i = 0;
	int32_t otp_index = -1;
	mutex_lock(s_ctrl->msm_sensor_mutex);
	CDBG("%s:%d %s cfgtype = %d\n", __func__, __LINE__,
		s_ctrl->sensordata->sensor_name, cdata->cfgtype);
	switch (cdata->cfgtype) {
	case CFG_GET_SENSOR_INFO:
		memcpy(cdata->cfg.sensor_info.sensor_name,
			s_ctrl->sensordata->sensor_name,
			sizeof(cdata->cfg.sensor_info.sensor_name));
		cdata->cfg.sensor_info.session_id =
			s_ctrl->sensordata->sensor_info->session_id;
		for (i = 0; i < SUB_MODULE_MAX; i++) {
			cdata->cfg.sensor_info.subdev_id[i] =
				s_ctrl->sensordata->sensor_info->subdev_id[i];
			cdata->cfg.sensor_info.subdev_intf[i] =
				s_ctrl->sensordata->sensor_info->subdev_intf[i];
		}
		cdata->cfg.sensor_info.is_mount_angle_valid =
			s_ctrl->sensordata->sensor_info->is_mount_angle_valid;
		cdata->cfg.sensor_info.sensor_mount_angle =
			s_ctrl->sensordata->sensor_info->sensor_mount_angle;
		cdata->cfg.sensor_info.position =
			s_ctrl->sensordata->sensor_info->position;
		cdata->cfg.sensor_info.modes_supported =
			s_ctrl->sensordata->sensor_info->modes_supported;
		CDBG("%s:%d sensor name %s\n", __func__, __LINE__,
			cdata->cfg.sensor_info.sensor_name);
		CDBG("%s:%d session id %d\n", __func__, __LINE__,
			cdata->cfg.sensor_info.session_id);
		for (i = 0; i < SUB_MODULE_MAX; i++) {
			CDBG("%s:%d subdev_id[%d] %d\n", __func__, __LINE__, i,
				cdata->cfg.sensor_info.subdev_id[i]);
			CDBG("%s:%d subdev_intf[%d] %d\n", __func__, __LINE__,
				i, cdata->cfg.sensor_info.subdev_intf[i]);
		}
		CDBG("%s:%d mount angle valid %d value %d\n", __func__,
			__LINE__, cdata->cfg.sensor_info.is_mount_angle_valid,
			cdata->cfg.sensor_info.sensor_mount_angle);

		break;
	case CFG_GET_SENSOR_INIT_PARAMS:
		cdata->cfg.sensor_init_params.modes_supported =
			s_ctrl->sensordata->sensor_info->modes_supported;
		cdata->cfg.sensor_init_params.position =
			s_ctrl->sensordata->sensor_info->position;
		cdata->cfg.sensor_init_params.sensor_mount_angle =
			s_ctrl->sensordata->sensor_info->sensor_mount_angle;
		CDBG("%s:%d init params mode %d pos %d mount %d\n", __func__,
			__LINE__,
			cdata->cfg.sensor_init_params.modes_supported,
			cdata->cfg.sensor_init_params.position,
			cdata->cfg.sensor_init_params.sensor_mount_angle);
		break;

	case CFG_WRITE_I2C_ARRAY:
	case CFG_WRITE_I2C_ARRAY_SYNC:
	case CFG_WRITE_I2C_ARRAY_SYNC_BLOCK:
	case CFG_WRITE_I2C_ARRAY_ASYNC: {
		struct msm_camera_i2c_reg_setting conf_array;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		if (!conf_array.size ||
			conf_array.size > I2C_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		if (cdata->cfgtype == CFG_WRITE_I2C_ARRAY)
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table(s_ctrl->sensor_i2c_client,
					&conf_array);
		else if (CFG_WRITE_I2C_ARRAY_ASYNC == cdata->cfgtype)
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table_async(s_ctrl->sensor_i2c_client,
					&conf_array);
		else if (CFG_WRITE_I2C_ARRAY_SYNC_BLOCK == cdata->cfgtype)
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table_sync_block(
					s_ctrl->sensor_i2c_client,
					&conf_array);
		else
			rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
				i2c_write_table_sync(s_ctrl->sensor_i2c_client,
					&conf_array);

		kfree(reg_setting);
		break;
	}
	case CFG_SLAVE_READ_I2C: {
		struct msm_camera_i2c_read_config read_config;
		struct msm_camera_i2c_read_config *read_config_ptr = NULL;
		uint16_t local_data = 0;
		uint16_t orig_slave_addr = 0, read_slave_addr = 0;
		uint16_t orig_addr_type = 0, read_addr_type = 0;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		read_config_ptr =
			(struct msm_camera_i2c_read_config *)cdata->cfg.setting;
		if (copy_from_user(&read_config, read_config_ptr,
			sizeof(struct msm_camera_i2c_read_config))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		read_slave_addr = read_config.slave_addr;
		read_addr_type = read_config.addr_type;
		CDBG("%s:CFG_SLAVE_READ_I2C:", __func__);
		CDBG("%s:slave_addr=0x%x reg_addr=0x%x, data_type=%d\n",
			__func__, read_config.slave_addr,
			read_config.reg_addr, read_config.data_type);
		if (s_ctrl->sensor_i2c_client->cci_client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->cci_client->sid;
			s_ctrl->sensor_i2c_client->cci_client->sid =
				read_slave_addr >> 1;
		} else if (s_ctrl->sensor_i2c_client->client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->client->addr;
			s_ctrl->sensor_i2c_client->client->addr =
				read_slave_addr >> 1;
		} else {
			pr_err("%s: error: no i2c/cci client found.", __func__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s:orig_slave_addr=0x%x, new_slave_addr=0x%x",
				__func__, orig_slave_addr,
				read_slave_addr >> 1);

		orig_addr_type = s_ctrl->sensor_i2c_client->addr_type;
		s_ctrl->sensor_i2c_client->addr_type = read_addr_type;

		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(
				s_ctrl->sensor_i2c_client,
				read_config.reg_addr,
				&local_data, read_config.data_type);
		if (s_ctrl->sensor_i2c_client->cci_client) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				orig_slave_addr;
		} else if (s_ctrl->sensor_i2c_client->client) {
			s_ctrl->sensor_i2c_client->client->addr =
				orig_slave_addr;
		}
		s_ctrl->sensor_i2c_client->addr_type = orig_addr_type;

		if (rc < 0) {
			pr_err("%s:%d: i2c_read failed\n", __func__, __LINE__);
			break;
		}
		read_config_ptr->data = local_data;
		break;
	}
	case CFG_SLAVE_WRITE_I2C_ARRAY: {
		struct msm_camera_i2c_array_write_config write_config;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;
		uint16_t orig_slave_addr = 0,  write_slave_addr = 0;
		uint16_t orig_addr_type = 0, write_addr_type = 0;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (copy_from_user(&write_config,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_array_write_config))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		CDBG("%s:CFG_SLAVE_WRITE_I2C_ARRAY:", __func__);
		CDBG("%s:slave_addr=0x%x, array_size=%d\n", __func__,
			write_config.slave_addr,
			write_config.conf_array.size);

		if (!write_config.conf_array.size ||
			write_config.conf_array.size > I2C_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(write_config.conf_array.size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting,
				(void *)(write_config.conf_array.reg_setting),
				write_config.conf_array.size *
				sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		write_config.conf_array.reg_setting = reg_setting;
		write_slave_addr = write_config.slave_addr;
		write_addr_type = write_config.conf_array.addr_type;
		if (s_ctrl->sensor_i2c_client->cci_client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->cci_client->sid;
			s_ctrl->sensor_i2c_client->cci_client->sid =
				write_slave_addr >> 1;
		} else if (s_ctrl->sensor_i2c_client->client) {
			orig_slave_addr =
				s_ctrl->sensor_i2c_client->client->addr;
			s_ctrl->sensor_i2c_client->client->addr =
				write_slave_addr >> 1;
		} else {
			pr_err("%s: error: no i2c/cci client found.", __func__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		CDBG("%s:orig_slave_addr=0x%x, new_slave_addr=0x%x",
				__func__, orig_slave_addr,
				write_slave_addr >> 1);
		orig_addr_type = s_ctrl->sensor_i2c_client->addr_type;
		s_ctrl->sensor_i2c_client->addr_type = write_addr_type;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
			s_ctrl->sensor_i2c_client, &(write_config.conf_array));
		s_ctrl->sensor_i2c_client->addr_type = orig_addr_type;
		if (s_ctrl->sensor_i2c_client->cci_client) {
			s_ctrl->sensor_i2c_client->cci_client->sid =
				orig_slave_addr;
		} else if (s_ctrl->sensor_i2c_client->client) {
			s_ctrl->sensor_i2c_client->client->addr =
				orig_slave_addr;
		} else {
			pr_err("%s: error: no i2c/cci client found.", __func__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}
		kfree(reg_setting);
		break;
	}
	case CFG_WRITE_I2C_SEQ_ARRAY: {
		struct msm_camera_i2c_seq_reg_setting conf_array;
		struct msm_camera_i2c_seq_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		if (copy_from_user(&conf_array,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_seq_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		if (!conf_array.size ||
			conf_array.size > I2C_SEQ_REG_DATA_MAX) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = kzalloc(conf_array.size *
			(sizeof(struct msm_camera_i2c_seq_reg_array)),
			GFP_KERNEL);
		if (!reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(reg_setting, (void *)conf_array.reg_setting,
			conf_array.size *
			sizeof(struct msm_camera_i2c_seq_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(reg_setting);
			rc = -EFAULT;
			break;
		}

		conf_array.reg_setting = reg_setting;
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->
			i2c_write_seq_table(s_ctrl->sensor_i2c_client,
			&conf_array);
		kfree(reg_setting);
		break;
	}

	case CFG_POWER_UP:
		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_DOWN) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}
		if (s_ctrl->func_tbl->sensor_power_up) {
			if (s_ctrl->sensordata->misc_regulator)
				msm_sensor_misc_regulator(s_ctrl, 1);

			rc = s_ctrl->func_tbl->sensor_power_up(s_ctrl);
			if (rc < 0) {
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
				break;
			}
			s_ctrl->sensor_state = MSM_SENSOR_POWER_UP;
			CDBG("%s:%d sensor state %d\n", __func__, __LINE__,
				s_ctrl->sensor_state);
		} else {
			rc = -EFAULT;
		}
		break;

	case CFG_POWER_DOWN:
		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		kfree(s_ctrl->stop_setting.reg_setting);
		s_ctrl->stop_setting.reg_setting = NULL;
		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP) {
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}

		/* optimize camera print mipi packet and frame count log*/
		if(read_times > 0) {
			pr_info("%s: cancel frm_cnt_work read_times = %d\n",__func__,read_times);
			cancel_delayed_work_sync(&s_ctrl->frm_cnt_work);
			read_times = 0;
#ifdef CONFIG_HUAWEI_DSM
			csi_pkg_count = 0;
			sensor_frame_dsm_cnt.count = 0;
			for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
			{
			  sensor_frame_dsm_cnt.sensor_frame_read[i] = 0;
			}
#endif

		}

		if (s_ctrl->func_tbl->sensor_power_down) {
			if (s_ctrl->sensordata->misc_regulator)
				msm_sensor_misc_regulator(s_ctrl, 0);

			rc = s_ctrl->func_tbl->sensor_power_down(s_ctrl);
			if (rc < 0) {
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
				break;
			}
			s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
			CDBG("%s:%d sensor state %d\n", __func__, __LINE__,
				s_ctrl->sensor_state);
		} else {
			rc = -EFAULT;
		}
#ifdef CONFIG_HUAWEI_DSM
		camera_is_closing = 0;
#endif
		break;
	/* optimize camera print mipi packet and frame count log*/
	case CFG_START_FRM_CNT:
	{
		cancel_delayed_work_sync(&s_ctrl->frm_cnt_work);
#ifdef CONFIG_HUAWEI_DSM
		csi_pkg_count = 0;
		sensor_frame_dsm_cnt.count = 0;
		for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
		{
			sensor_frame_dsm_cnt.sensor_frame_read[i] = 0;
		}
#endif
		read_times = HW_PRINT_PACKET_NUM_TIME;

		schedule_delayed_work(&s_ctrl->frm_cnt_work, msecs_to_jiffies(HW_READ_PACKET_NUM_TIME));
		pr_info("%s:%s start read frame count work \n",__func__,s_ctrl->sensordata->sensor_name);
	}
	break;

	case CFG_STOP_FRM_CNT:
	{
		cancel_delayed_work_sync(&s_ctrl->frm_cnt_work);
		read_times = 0;
#ifdef CONFIG_HUAWEI_DSM
		csi_pkg_count = 0;
		sensor_frame_dsm_cnt.count = 0;
		for (i = 0; i < HW_PRINT_PACKET_NUM_TIME; i++)
		{
			sensor_frame_dsm_cnt.sensor_frame_read[i] = 0;
		}
#endif
		pr_info("%s:%s stop read frame count work \n",__func__,s_ctrl->sensordata->sensor_name);
	}
	break;

	case CFG_SET_STOP_STREAM_SETTING: {
		struct msm_camera_i2c_reg_setting *stop_setting =
			&s_ctrl->stop_setting;
		struct msm_camera_i2c_reg_array *reg_setting = NULL;

		if (s_ctrl->is_csid_tg_mode)
			goto DONE;

		if (copy_from_user(stop_setting,
			(void *)cdata->cfg.setting,
			sizeof(struct msm_camera_i2c_reg_setting))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}

		reg_setting = stop_setting->reg_setting;

		if (!stop_setting->size) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -EFAULT;
			break;
		}
		stop_setting->reg_setting = kzalloc(stop_setting->size *
			(sizeof(struct msm_camera_i2c_reg_array)), GFP_KERNEL);
		if (!stop_setting->reg_setting) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			rc = -ENOMEM;
			break;
		}
		if (copy_from_user(stop_setting->reg_setting,
			(void *)reg_setting,
			stop_setting->size *
			sizeof(struct msm_camera_i2c_reg_array))) {
			pr_err("%s:%d failed\n", __func__, __LINE__);
			kfree(stop_setting->reg_setting);
			stop_setting->reg_setting = NULL;
			stop_setting->size = 0;
			rc = -EFAULT;
			break;
		}
		break;
	}

	case CFG_SET_I2C_SYNC_PARAM: {
		struct msm_camera_cci_ctrl cci_ctrl;

		s_ctrl->sensor_i2c_client->cci_client->cid =
			cdata->cfg.sensor_i2c_sync_params.cid;
		s_ctrl->sensor_i2c_client->cci_client->id_map =
			cdata->cfg.sensor_i2c_sync_params.csid;

		CDBG("I2C_SYNC_PARAM CID:%d, line:%d delay:%d, cdid:%d\n",
			s_ctrl->sensor_i2c_client->cci_client->cid,
			cdata->cfg.sensor_i2c_sync_params.line,
			cdata->cfg.sensor_i2c_sync_params.delay,
			cdata->cfg.sensor_i2c_sync_params.csid);

		cci_ctrl.cmd = MSM_CCI_SET_SYNC_CID;
		cci_ctrl.cfg.cci_wait_sync_cfg.line =
			cdata->cfg.sensor_i2c_sync_params.line;
		cci_ctrl.cfg.cci_wait_sync_cfg.delay =
			cdata->cfg.sensor_i2c_sync_params.delay;
		cci_ctrl.cfg.cci_wait_sync_cfg.cid =
			cdata->cfg.sensor_i2c_sync_params.cid;
		cci_ctrl.cfg.cci_wait_sync_cfg.csid =
			cdata->cfg.sensor_i2c_sync_params.csid;
		rc = v4l2_subdev_call(s_ctrl->sensor_i2c_client->
				cci_client->cci_subdev,
				core, ioctl, VIDIOC_MSM_CCI_CFG, &cci_ctrl);
		if (rc < 0) {
			pr_err("%s: line %d rc = %d\n", __func__, __LINE__, rc);
			rc = -EFAULT;
			break;
		}
	}
		break;
	case CFG_GET_OTP_FLAG: {
		CDBG("%s,%d: CFG_GET_OTP_FLAG\n", __func__, __LINE__);
		//if power up
		if (s_ctrl->sensor_state != MSM_SENSOR_POWER_UP)
		{
			pr_err("%s:%d failed: invalid state %d\n", __func__,
				__LINE__, s_ctrl->sensor_state);
			rc = -EFAULT;
			break;
		}
		//set otp info
		if ( is_exist_otp_function(s_ctrl, &otp_index) )
		{
			rc = otp_function_lists[otp_index].sensor_otp_function(s_ctrl, otp_index);
			if (rc < 0)
			{
#ifdef CONFIG_HUAWEI_DSM
				camera_report_dsm_err_msm_sensor(s_ctrl, DSM_CAMERA_OTP_ERR, rc, NULL);
#endif
				pr_err("%s:%d failed rc %d\n", __func__,
					__LINE__, rc);
			}
			pr_info("%s,%d s_ctrl->hw_otp_check_flag.mmi_otp_check_flag = 0x%x\n", \
			__func__, __LINE__, s_ctrl->hw_otp_check_flag.mmi_otp_check_flag);
		}
		else
		{
		    /*  unsupport otp operation, set the mmi_otp_check_flag = 0 */
			s_ctrl->hw_otp_check_flag.mmi_otp_check_flag = 0;
			pr_err("%s, %d: %s unsupport otp operation, mmi_otp_check_flag = 0.\n", __func__,
					__LINE__, s_ctrl->sensordata->sensor_name);
		}
		if (copy_to_user((void *)cdata->cfg.setting,
			(void *)&s_ctrl->hw_otp_check_flag, sizeof(struct msm_sensor_mmi_otp_flag))) {
				pr_err("%s:%d copy failed\n", __func__, __LINE__);
				rc = -EFAULT;
				break;
		}

		break;
	}

	case CFG_SET_AFC_OTP_INFO:
		rc = msm_sensor_get_afc_otp_info(s_ctrl, argp);
		break;

	case CFG_SET_AWB_OTP_INFO:
		rc = msm_sensor_get_awb_otp_info(s_ctrl, argp);
		break;
	case CFG_SET_LSC_OTP_INFO:
		rc = msm_sensor_get_lsc_otp_info(s_ctrl, argp);
		break;

	default:
		rc = -EFAULT;
		break;
	}

DONE:
	mutex_unlock(s_ctrl->msm_sensor_mutex);

	return rc;
}

int msm_sensor_check_id(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc;

	if (s_ctrl->func_tbl->sensor_match_id)
		rc = s_ctrl->func_tbl->sensor_match_id(s_ctrl);
	else
		rc = msm_sensor_match_id(s_ctrl);
	if (rc < 0)
		pr_err("%s:%d match id failed rc %d\n", __func__, __LINE__, rc);
	return rc;
}

static int msm_sensor_power(struct v4l2_subdev *sd, int on)
{
	int rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);
	mutex_lock(s_ctrl->msm_sensor_mutex);
	if (!on && s_ctrl->sensor_state == MSM_SENSOR_POWER_UP) {
		s_ctrl->func_tbl->sensor_power_down(s_ctrl);
		s_ctrl->sensor_state = MSM_SENSOR_POWER_DOWN;
	}
	mutex_unlock(s_ctrl->msm_sensor_mutex);
	return rc;
}

static int msm_sensor_v4l2_enum_fmt(struct v4l2_subdev *sd,
	unsigned int index, enum v4l2_mbus_pixelcode *code)
{
	struct msm_sensor_ctrl_t *s_ctrl = get_sctrl(sd);

	if ((unsigned int)index >= s_ctrl->sensor_v4l2_subdev_info_size)
		return -EINVAL;

	*code = s_ctrl->sensor_v4l2_subdev_info[index].code;
	return 0;
}

#ifdef CONFIG_HUAWEI_DSM
void camera_report_dsm_err_msm_sensor(struct msm_sensor_ctrl_t *s_ctrl, int type, int err_num , const char* str)
{
	ssize_t len = 0;
	int rc = 0;

	memset(camera_dsm_log_buff, 0, MSM_CAMERA_DSM_BUFFER_SIZE);

	if ( (NULL != s_ctrl) && (NULL != s_ctrl->sensordata) )
	{
		//get module info
		len = snprintf(camera_dsm_log_buff,
				MSM_CAMERA_DSM_BUFFER_SIZE,
				"Sensor name:%s, eeprom name:%s ",
				s_ctrl->sensordata->sensor_name,
				s_ctrl->sensordata->eeprom_name);
	}

	if ( len >= MSM_CAMERA_DSM_BUFFER_SIZE -1 )
	{
		CDBG("write camera_dsm_log_buff overflow.\n");
		return;
	}
	
	/* camera record error info according to err type */
	switch(type)
	{
		case DSM_CAMERA_OTP_ERR:
			/* report otp infomation */
			len += snprintf(camera_dsm_log_buff+len, MSM_CAMERA_DSM_BUFFER_SIZE-len, "[msm_sensor]OTP error.No effective OTP info.\n");
			if (len >= MSM_CAMERA_DSM_BUFFER_SIZE -1)
			{
				CDBG("write camera_dsm_log_buff overflow.\n");
				return ;
			}
			break;

		case DSM_CAMERA_CHIP_ID_NOT_MATCH:
			/* report otp infomation */
			len += snprintf(camera_dsm_log_buff+len, MSM_CAMERA_DSM_BUFFER_SIZE-len, "[msm_sensor]Chip ID DON'T MATCH.\n");
			if (len >= MSM_CAMERA_DSM_BUFFER_SIZE -1)
			{
				CDBG("write camera_dsm_log_buff overflow.\n");
				return ;
			}
			break;

		default:
			break;
	}
	
	rc = camera_report_dsm_err( type, err_num, camera_dsm_log_buff);
	if (rc < 0)
	{
	     pr_err("%s. report dsm err fail.\n", __func__);
	     return ;
	}
	return ;
}
#endif
static struct v4l2_subdev_core_ops msm_sensor_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops msm_sensor_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops msm_sensor_subdev_ops = {
	.core = &msm_sensor_subdev_core_ops,
	.video  = &msm_sensor_subdev_video_ops,
};

static struct msm_sensor_fn_t msm_sensor_func_tbl = {
	.sensor_config = msm_sensor_config,
#ifdef CONFIG_COMPAT
	.sensor_config32 = msm_sensor_config32,
#endif
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_match_id = msm_sensor_match_id,
	/* optimize camera print mipi packet and frame count log*/
	.sensor_read_framecount = hw_sensor_read_framecount,

};

static struct msm_camera_i2c_fn_t msm_sensor_cci_func_tbl = {
	.i2c_read = msm_camera_cci_i2c_read,
	.i2c_read_seq = msm_camera_cci_i2c_read_seq,
	.i2c_write = msm_camera_cci_i2c_write,
	.i2c_write_table = msm_camera_cci_i2c_write_table,
	.i2c_write_seq_table = msm_camera_cci_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_cci_i2c_write_table_w_microdelay,
	.i2c_util = msm_sensor_cci_i2c_util,
	.i2c_write_conf_tbl = msm_camera_cci_i2c_write_conf_tbl,
	.i2c_write_table_async = msm_camera_cci_i2c_write_table_async,
	.i2c_write_table_sync = msm_camera_cci_i2c_write_table_sync,
	.i2c_write_table_sync_block = msm_camera_cci_i2c_write_table_sync_block,

};

static struct msm_camera_i2c_fn_t msm_sensor_qup_func_tbl = {
	.i2c_read = msm_camera_qup_i2c_read,
	.i2c_read_seq = msm_camera_qup_i2c_read_seq,
	.i2c_write = msm_camera_qup_i2c_write,
	.i2c_write_table = msm_camera_qup_i2c_write_table,
	.i2c_write_seq_table = msm_camera_qup_i2c_write_seq_table,
	.i2c_write_table_w_microdelay =
		msm_camera_qup_i2c_write_table_w_microdelay,
	.i2c_write_conf_tbl = msm_camera_qup_i2c_write_conf_tbl,
	.i2c_write_table_async = msm_camera_qup_i2c_write_table,
	.i2c_write_table_sync = msm_camera_qup_i2c_write_table,
	.i2c_write_table_sync_block = msm_camera_qup_i2c_write_table,
};

int32_t msm_sensor_init_default_params(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_cci_client *cci_client = NULL;
	unsigned long mount_pos = 0;

	/* Validate input parameters */
	if (!s_ctrl) {
		pr_err("%s:%d failed: invalid params s_ctrl %pK\n", __func__,
			__LINE__, s_ctrl);
		return -EINVAL;
	}

	if (!s_ctrl->sensor_i2c_client) {
		pr_err("%s:%d failed: invalid params sensor_i2c_client %pK\n",
			__func__, __LINE__, s_ctrl->sensor_i2c_client);
		return -EINVAL;
	}

	/* Initialize cci_client */
	s_ctrl->sensor_i2c_client->cci_client = kzalloc(sizeof(
		struct msm_camera_cci_client), GFP_KERNEL);
	if (!s_ctrl->sensor_i2c_client->cci_client) {
		pr_err("%s:%d failed: no memory cci_client %pK\n", __func__,
			__LINE__, s_ctrl->sensor_i2c_client->cci_client);
		return -ENOMEM;
	}

	if (s_ctrl->sensor_device_type == MSM_CAMERA_PLATFORM_DEVICE) {
		cci_client = s_ctrl->sensor_i2c_client->cci_client;

		/* Get CCI subdev */
		cci_client->cci_subdev = msm_cci_get_subdev();

		/* Update CCI / I2C function table */
		if (!s_ctrl->sensor_i2c_client->i2c_func_tbl)
			s_ctrl->sensor_i2c_client->i2c_func_tbl =
				&msm_sensor_cci_func_tbl;
	} else {
		if (!s_ctrl->sensor_i2c_client->i2c_func_tbl) {
			CDBG("%s:%d\n", __func__, __LINE__);
			s_ctrl->sensor_i2c_client->i2c_func_tbl =
				&msm_sensor_qup_func_tbl;
		}
	}

	/* Update function table driven by ioctl */
	if (!s_ctrl->func_tbl)
		s_ctrl->func_tbl = &msm_sensor_func_tbl;

	/* Update v4l2 subdev ops table */
	if (!s_ctrl->sensor_v4l2_subdev_ops)
		s_ctrl->sensor_v4l2_subdev_ops = &msm_sensor_subdev_ops;

	/* Update sensor mount angle and position in media entity flag */
	mount_pos = s_ctrl->sensordata->sensor_info->position << 16;
	mount_pos = mount_pos | ((s_ctrl->sensordata->sensor_info->
					sensor_mount_angle / 90) << 8);
	s_ctrl->msm_sd.sd.entity.flags = mount_pos | MEDIA_ENT_FL_DEFAULT;

	/* optimize camera print mipi packet and frame count log*/
	INIT_DELAYED_WORK(&s_ctrl->frm_cnt_work, read_framecount_work_handler);
	return 0;
}
