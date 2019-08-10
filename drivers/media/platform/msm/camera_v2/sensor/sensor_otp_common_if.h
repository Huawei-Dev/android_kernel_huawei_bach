


#ifndef _SENSOR_OTP_COMMON_IF_H
#define _SENSOR_OTP_COMMON_IF_H

#ifdef CONFIG_HUAWEI_DSM
#include "msm_camera_dsm.h"
#define OTP_WRITE_I2C_ERR 0
#define OTP_READ_I2C_ERR 1
#endif

struct otp_function_t {
	char sensor_name[32];
	int (*sensor_otp_function) (struct msm_sensor_ctrl_t *s_ctrl, int index);
	uint32_t rg_ratio_typical;//r/g ratio
	uint32_t bg_ratio_typical;//b/g ratio

	bool is_boot_load; //whether load otp info when booting.
};

#ifdef CONFIG_HUAWEI_DSM
void camera_report_dsm_err_otp(struct msm_sensor_ctrl_t *s_ctrl, int type, uint32_t addr , int flag);
#endif

extern bool is_exist_otp_function(struct msm_sensor_ctrl_t *sctl, int32_t *index);

extern int hi553_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index);

extern int hi843s_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index);

extern int hi843_bah_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index);

extern int imx219_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int ags_hi553_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int ags_s5k5e8_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int ov8856_huaquan_07b8_pad_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int ov8856_sunny_f8v07n_pad_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int hi843_qtech_fh843_pad_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int hi843_synology_1p0j0_pad_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern int ov8856_otp_func(struct msm_sensor_ctrl_t * s_ctrl, int index);

extern struct otp_function_t otp_function_lists [];


#endif
