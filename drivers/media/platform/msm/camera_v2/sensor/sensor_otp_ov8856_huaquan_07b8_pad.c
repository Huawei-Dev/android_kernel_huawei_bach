
//#define HW_CMR_LOGSWC 0   //file log switch set 0 off,default is 1 on
#define HW_CMR_LOG_TAG "sensor_otp_ov8856_huaquan_07b8_pad"

#include <linux/hw_camera_common.h>
#include "msm_sensor.h"
#include "sensor_otp_common_if.h"

/* if support OTP or open OTP function, please define it */

#define GROUP_OTP_EMPTY 0  //group is empty
#define GROUP_OTP_INVALID 1  //group is invalid
#define GROUP_OTP_VALID 2  //group is vaild

#define OV8856_HUAQUAN_07B8_PAD_OTP_GROUP1_FLAG 0x40
#define OV8856_HUAQUAN_07B8_PAD_OTP_GROUP2_FLAG 0xD0

/*module info & lens shading reg address from otp doc.*/
#define OV8856_HUAQUAN_07B8_PAD_OTP_MODULE_INFO_FLAG_ADDR 0x7010
#define OV8856_HUAQUAN_07B8_PAD_OTP_LENS_FLAG_ADDR 0x7028

/*lens shading num is 240*/
#define OV8856_HUAQUAN_07B8_PAD_MAX_OTP_LENS_NUM 240

//#define OV8856_HUAQUAN_07B8_PAD_DEBUG_OTP_DATA
//OV8856_HUAQUAN_07B8_PAD has three groups: [1,2,3]
typedef enum ov8856_huaquan_07b8_pad_groups_count{
	GROUP_1 = 0,
	GROUP_2,
	GROUP_MAX
}enum_ov8856_huaquan_07b8_pad_groups;
/*OV8856_HUAQUAN_07B8_PAD have four types otp*/
typedef enum ov8856_huaquan_07b8_pad_otp_type_e{
	MODULE_INFO_OTP,
	LENS_OTP
}enum_ov8856_huaquan_07b8_pad_otp_type;

typedef struct ov8856_huaquan_07b8_pad_otp_reg_addr {
	uint16_t start_address;
	uint16_t end_address;
}ST_ED_REG_ADDR;

//OTP info struct
typedef struct ov8856_huaquan_07b8_pad_otp_info {
	uint16_t module_integrator_id;
	uint16_t lens_id;
	uint16_t production_year;
	uint16_t production_month;
	uint16_t production_day;
	uint16_t rg_ratio;
	uint16_t bg_ratio;
	uint8_t lenc[OV8856_HUAQUAN_07B8_PAD_MAX_OTP_LENS_NUM];
    uint16_t checksum;
}st_ov8856_huaquan_07b8_pad_otp_info;

//Golden sensor typical ratio
static int RG_Ratio_Typical = 0x253;
static int BG_Ratio_Typical = 0x27F;

#define OV8856_HUAQUAN_07B8_PAD_MMI_OTP_MODULE_INFO_FLAG            ( 1 << 0 )
#define OV8856_HUAQUAN_07B8_PAD_MMI_OTP_LSC_FLAG                    ( 1 << 1 )
#define OV8856_HUAQUAN_07B8_PAD_OTP_CHECKED_FAILED                  ( 1 << 2 )

static ST_ED_REG_ADDR ov8856_huaquan_07b8_pad_module_info_otp_read_addr[GROUP_MAX] = {
	{0x7011,0x7018},
	{0x7019,0x7020},
};
static ST_ED_REG_ADDR ov8856_huaquan_07b8_pad_lens_otp_read_addr[GROUP_MAX] = {
	{0x7029,0x7119},
	{0x711A,0x720A},
};


//OTP info
static st_ov8856_huaquan_07b8_pad_otp_info g_ov8856_huaquan_07b8_pad_otp = {0};

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_otp_write_i2c;
 * Description : write otp info via i2c;
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_otp_write_i2c(struct msm_sensor_ctrl_t *s_ctrl, int32_t addr, uint16_t data)
{
	int32_t rc = 0;

	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client,
			addr,
			data,
			MSM_CAMERA_I2C_BYTE_DATA);

	if ( rc < 0 )
	{
		CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, data);
	}

	return rc;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_otp_read_i2c;
 * Description : read otp info via i2c;
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_otp_read_i2c(struct msm_sensor_ctrl_t *s_ctrl,uint32_t addr, uint16_t *data)
{
	int32_t rc = 0;

	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(s_ctrl->sensor_i2c_client,
			addr,
			data,
			MSM_CAMERA_I2C_BYTE_DATA);
	if ( rc < 0 )
	{
		CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, *data);
	}

	return rc;
}

/********************************************************************************************
* To avoid OTP memory access timing conflict,before doing OTP read/write,register 0x5002[3]
* must be set to 0 After OTP memory access,set register 0x5002[3] back to 1
********************************************************************************************/
int32_t ov8856_huaquan_07b8_pad_otp_enable_DPC(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint16_t addr = 0x5001;
	uint16_t data = 0;

	rc = ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl, addr, &data);

	pr_info("%s change 0x%x value from 0x%x to 0x%x\n", __func__,addr, data,  data | 0x08);

	/* set 0x5001[3] to 1 */
	rc = ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, addr, data | 0x08);

	return rc;
}

int32_t ov8856_huaquan_07b8_pad_otp_disable_DPC(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint16_t addr = 0x5001;
	uint16_t data = 0;

	rc = ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl, addr, &data);

	pr_info("%s change 0x%x value from 0x%x to 0x%x\n",__func__, addr, data,  data & 0xF7);

	/* set 0x5001[3] to 0 */
	rc = ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, addr, data & 0xF7);

	return rc;
}

/****************************************************************************
// index: index of otp group will be checked: (1, 2, 3)
// check_address: otp flag address
// return:
// 0, group index is empty
// 1, group index has invalid data
// 2, group index has valid data
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_check_otp_group_index(struct msm_sensor_ctrl_t *s_ctrl, int* index, uint16_t check_address)
{
	int32_t rc = 0;
	uint16_t data = 0;

	if(check_address != OV8856_HUAQUAN_07B8_PAD_OTP_MODULE_INFO_FLAG_ADDR
			&& check_address != OV8856_HUAQUAN_07B8_PAD_OTP_LENS_FLAG_ADDR)
	{
		CMR_LOGE("%s: check_address = 0x%x  error! \n",__func__,check_address);
		return -1;
	}

	// clear otp data buffer
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,check_address, 0x00);

	// enter otp read mode
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d84, 0xc0); // program disable, manual mode

	//partial mode OTP write start address
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d88, (check_address>>8));
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d89, (check_address & 0xff));

	// partial mode OTP write end address
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d8A, (check_address>>8));
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d8B, (check_address & 0xff));

	// open otp read function
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d81, 0x01); // read otp

	//delay 20ms
	msleep(20);

	//read otp data
	rc = ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,check_address, &data);
	if(rc < 0)
	{
		data = 0;
		CMR_LOGE("%s:%d read otp data fail\n",__func__,__LINE__);
	}
	else
	{
		pr_info("%s:%d data = %d\n",__func__,__LINE__, data);
		//select group
		switch(data)
		{
			case OV8856_HUAQUAN_07B8_PAD_OTP_GROUP1_FLAG:
				*index = GROUP_1;
                rc = GROUP_OTP_VALID;
				break;

			case OV8856_HUAQUAN_07B8_PAD_OTP_GROUP2_FLAG:
				*index = GROUP_2;
                rc = GROUP_OTP_VALID;
				break;

			default:
				rc = GROUP_OTP_INVALID;
				CMR_LOGE("%s:%d read otp data fail\n",__func__,__LINE__);
				break;
		}
	}

	// close otp read function
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d81, 0x00);

	// clear otp data buffer
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,check_address, 0x00);

	return rc;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_read_group_address_index;
 * Description : find group index which real store otp data and convert the group index to address array index.
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_read_group_address_index(struct msm_sensor_ctrl_t *s_ctrl, uint16_t check_addr)
{
	int32_t ret = 0;
	int32_t index = -1;

	//get vaild gruop index

	ret = ov8856_huaquan_07b8_pad_check_otp_group_index(s_ctrl,&index,check_addr);


	if(index >= GROUP_MAX || index < GROUP_1)
	{
		CMR_LOGE("%s: check module info gruop index fail\n",__func__);
		return -1;
	}

	return index;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_read_group_data;
 * Description : read the specific otp data from group1 or group2 or group3. and fill the otp data to g_ov8856_huaquan_07b8_pad_otp
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_read_group_data(struct msm_sensor_ctrl_t *s_ctrl, enum_ov8856_huaquan_07b8_pad_otp_type otp_type)
{
	int32_t i = 0;
	int32_t rc = 0;
	int32_t index = 0;
	uint16_t lens_temp = 0;
	uint16_t check_addr = 0;
	uint16_t start_address = 0;
	uint16_t end_address = 0;
	ST_ED_REG_ADDR *pcur_addr_array = NULL;

	switch(otp_type)
	{
		case MODULE_INFO_OTP:
			check_addr = OV8856_HUAQUAN_07B8_PAD_OTP_MODULE_INFO_FLAG_ADDR;
			pcur_addr_array = ov8856_huaquan_07b8_pad_module_info_otp_read_addr;
			break;

		case LENS_OTP:
			check_addr = OV8856_HUAQUAN_07B8_PAD_OTP_LENS_FLAG_ADDR;
			pcur_addr_array = ov8856_huaquan_07b8_pad_lens_otp_read_addr;
			break;

		default:
			CMR_LOGE("%s: otp type error \n",__func__);
			return -1;
	}

	index = ov8856_huaquan_07b8_pad_read_group_address_index(s_ctrl,check_addr);
	if(index < 0 || index >= GROUP_MAX)
	{
		CMR_LOGE("%s: otp_type=%d fail index = %d\n",__func__,otp_type,index);
		return -1;
	}

	CMR_LOGD("%s: otp_type=%d index:%d \n", __func__,otp_type,index);

	start_address = pcur_addr_array[index].start_address;
	end_address = pcur_addr_array[index].end_address;

	//clear otp data buffer
	for(i = start_address; i <= end_address; i++)
	{
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,i, 0x00);
	}

	// enter otp read mode
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d84, 0xc0); // program disable, manual mode

	//partial mode OTP write start address
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d88, (start_address>>8));
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d89, (start_address & 0xff));

	// partial mode OTP write end address
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d8A, (end_address>>8));
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d8B, (end_address & 0xff));

	// enable otp read function
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d81, 0x01); // read otp

	//delay 20ms
	msleep(20);

	if(otp_type == MODULE_INFO_OTP)
	{
        uint16_t awb_temp1=0,awb_temp2=0,awb_temp3=0;
        ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address, &g_ov8856_huaquan_07b8_pad_otp.module_integrator_id);
		ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+1, &g_ov8856_huaquan_07b8_pad_otp.lens_id);
		ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+2, &g_ov8856_huaquan_07b8_pad_otp.production_year);
		ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+3, &g_ov8856_huaquan_07b8_pad_otp.production_month);
		ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+4, &g_ov8856_huaquan_07b8_pad_otp.production_day);
        ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+5, &awb_temp1);
        ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+6, &awb_temp2);
        ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+7, &awb_temp3);
        g_ov8856_huaquan_07b8_pad_otp.bg_ratio = ((awb_temp3 >> 4) & 0x03)| ((awb_temp2&0xFF) << 2) ;
        g_ov8856_huaquan_07b8_pad_otp.rg_ratio = ((awb_temp3 >> 6) & 0x03)| ((awb_temp1&0xFF) << 2);
		s_ctrl->vendor_otp_info.vendor_id = g_ov8856_huaquan_07b8_pad_otp.module_integrator_id;

        #ifdef OV8856_HUAQUAN_07B8_PAD_DEBUG_OTP_DATA
        CMR_LOGE("%s,MODULE OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address, g_ov8856_huaquan_07b8_pad_otp.module_integrator_id);
	    CMR_LOGE("%s,MODULE OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+1, g_ov8856_huaquan_07b8_pad_otp.lens_id);
	    CMR_LOGE("%s,MODULE OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+2, g_ov8856_huaquan_07b8_pad_otp.production_year);
	    CMR_LOGE("%s,MODULE OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+3, g_ov8856_huaquan_07b8_pad_otp.production_month);
	    CMR_LOGE("%s,MODULE OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+4, g_ov8856_huaquan_07b8_pad_otp.production_day);
	    CMR_LOGE("%s,AWB OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+5, awb_temp1);
	    CMR_LOGE("%s,AWB OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+6, awb_temp2);
	    CMR_LOGE("%s,AWB OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+7, awb_temp3);
	    #endif
	}
	else if(otp_type == LENS_OTP)
	{
		for(i=0;i<OV8856_HUAQUAN_07B8_PAD_MAX_OTP_LENS_NUM;i++) {
			ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+i, &lens_temp);
			g_ov8856_huaquan_07b8_pad_otp.lenc[i] = (uint8_t)(lens_temp & 0xFF);

			#ifdef OV8856_HUAQUAN_07B8_PAD_DEBUG_OTP_DATA
            CMR_LOGE("%s,LSC OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+i,g_ov8856_huaquan_07b8_pad_otp.lenc[i]);
			#endif


		}
        ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,start_address+i,
                &g_ov8856_huaquan_07b8_pad_otp.checksum);

		#ifdef OV8856_HUAQUAN_07B8_PAD_DEBUG_OTP_DATA
        CMR_LOGE("%s,LSC OTP Info:addr = 0x%x, data = 0x%x\n",__func__,start_address+i,g_ov8856_huaquan_07b8_pad_otp.checksum);
		#endif

	}
	else
	{
		CMR_LOGE("%s: otp type error! \n",__func__);
		rc = -1;
	}
	// close otp read function
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x3d81, 0x00);

	//clear otp data buffer
	for(i = start_address; i <= end_address; i++)
	{
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,i, 0x00);
	}

	return rc;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_read_otp_data;
 * Description : read all otp data info;
 * return value:
 * 0 means read otp info succeed.
 * -1 means read otp info failed, should not write otp.
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_read_otp_data(struct msm_sensor_ctrl_t *s_ctrl, uint16_t *hw_otp_check)
{
	int index = 0;
	int ret = 0;
	uint16_t  ov8856_huaquan_07b8_pad_mmi_otp_flag = 0x03;

	if (!s_ctrl)
	{
		CMR_LOGE("%s:%d fail\n",__func__,__LINE__);
		return -1;
	}

	if (!hw_otp_check)
	{
		CMR_LOGE("%s:%d fail\n",__func__,__LINE__);
		return -1;
	}

	*hw_otp_check = 0x03;

	ov8856_huaquan_07b8_pad_otp_disable_DPC(s_ctrl);

       //read module info otp data
	ret = ov8856_huaquan_07b8_pad_read_group_data(s_ctrl,MODULE_INFO_OTP);
	if(ret < 0)
	{
		CMR_LOGE("%s: read module info otp data fail\n",__func__);
		goto OTP_ERROR;
	}
       ov8856_huaquan_07b8_pad_mmi_otp_flag &= ~OV8856_HUAQUAN_07B8_PAD_MMI_OTP_MODULE_INFO_FLAG;

	//read lens otp data
	ret = ov8856_huaquan_07b8_pad_read_group_data(s_ctrl,LENS_OTP);
	if(ret < 0)
	{
		CMR_LOGE("%s: read module info otp data fail\n",__func__);
		goto OTP_ERROR;
	}
	 ov8856_huaquan_07b8_pad_mmi_otp_flag &= ~OV8856_HUAQUAN_07B8_PAD_MMI_OTP_LSC_FLAG;

	ov8856_huaquan_07b8_pad_otp_enable_DPC(s_ctrl);
	CMR_LOGD("%s: year=%d month=%d day=%d product=%d moduleid=%d rg=%d bg=%d \n", __func__,
			g_ov8856_huaquan_07b8_pad_otp.production_year,g_ov8856_huaquan_07b8_pad_otp.production_month,
			g_ov8856_huaquan_07b8_pad_otp.production_day,g_ov8856_huaquan_07b8_pad_otp.lens_id,
			g_ov8856_huaquan_07b8_pad_otp.module_integrator_id,g_ov8856_huaquan_07b8_pad_otp.rg_ratio,
			g_ov8856_huaquan_07b8_pad_otp.bg_ratio);

	//debug info test
	CMR_LOGD("%s: ---------lens shading otp data start----------\n",__func__);
	for(index = 0; index <OV8856_HUAQUAN_07B8_PAD_MAX_OTP_LENS_NUM; index++)
	{
		CMR_LOGD("[%d]=0x%02x \n",index,g_ov8856_huaquan_07b8_pad_otp.lenc[index]);
	}
	CMR_LOGD("%s: ---------lens shading otp data start----------\n",__func__);

OTP_ERROR:
	*hw_otp_check = ov8856_huaquan_07b8_pad_mmi_otp_flag;
	return ret;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_update_awb_gain;
 * Description : write R_gain,G_gain,B_gain to otp;
 * 0x400 =1x Gain
 * 0 means write WB info succeed.
 * -1 means write WB info failed.
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_update_awb_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t R_gain=0, G_gain=0, B_gain=0, Base_gain=0;
	int rg, bg;

	rg = g_ov8856_huaquan_07b8_pad_otp.rg_ratio;
	bg = g_ov8856_huaquan_07b8_pad_otp.bg_ratio;

	if(rg != 0 && bg != 0){
		R_gain = RG_Ratio_Typical * 1000 / rg;
		B_gain = BG_Ratio_Typical * 1000 / bg;
		G_gain = 1000;
	}else{
		CMR_LOGE("%s: rg_ratio=%d bg_ratio=%d fail\n",__func__,g_ov8856_huaquan_07b8_pad_otp.rg_ratio,g_ov8856_huaquan_07b8_pad_otp.bg_ratio);
		return -1;
	}

	if(R_gain < 1000 || B_gain < 1000){
		if(R_gain < B_gain){
			Base_gain = R_gain;
		}else{
			Base_gain = B_gain;
		}
	}else{
		Base_gain = G_gain;
	}

	if(Base_gain != 0){
		R_gain = 0x400 * R_gain/(Base_gain);
		B_gain = 0x400 * B_gain/(Base_gain);
		G_gain = 0x400 * G_gain/(Base_gain);
	}else{
		CMR_LOGE("%s error nBase_gain 0x%x\n", __func__, Base_gain);
	}

	if (R_gain>0x400) {
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x5019, R_gain>>8);
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x501A, R_gain & 0x00ff);
	}
	if (G_gain>0x400) {
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x501B, G_gain>>8);
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x501C, G_gain & 0x00ff);
	}
	if (B_gain>0x400) {
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x501D, B_gain>>8);
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl,0x501E, B_gain & 0x00ff);
	}

	CMR_LOGD("%s: R_gain=%d, G_gain=%d, B_gain=%d \n",__func__,R_gain,G_gain,B_gain);
	return 0;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_update_lenc_otp;
 * Description : write 110 lens shading otp data to sensor;
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_update_lenc_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t i = 0;
	uint16_t temp = 0;

	ov8856_huaquan_07b8_pad_otp_read_i2c(s_ctrl,0x5000, &temp);
	temp = 0x20 | temp;
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, 0x5000, temp);

	for(i=0;i<OV8856_HUAQUAN_07B8_PAD_MAX_OTP_LENS_NUM;i++) {
		ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, 0x5900 + i, g_ov8856_huaquan_07b8_pad_otp.lenc[i]);
	}

	return 0;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_get_otp_info;
 * Description : get otp info from sensor;
 ***************************************************************************/
int32_t ov8856_huaquan_07b8_pad_get_otp_info(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	uint16_t ov8856_huaquan_07b8_pad_mmi_otp_flag = 0x03;
	CMR_LOGD("Get ov8856_huaquan_07b8_pad OTP info Enter\n");
	//set sensor mode:Active
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, 0x100, 0x1);

	rc = ov8856_huaquan_07b8_pad_read_otp_data(s_ctrl, &ov8856_huaquan_07b8_pad_mmi_otp_flag);
	s_ctrl->hw_otp_check_flag.mmi_otp_check_flag =  ov8856_huaquan_07b8_pad_mmi_otp_flag;
	CMR_LOGI("%s ov8856_huaquan_07b8_pad_mmi_otp_flag = 0x%x\n",__func__, s_ctrl->hw_otp_check_flag.mmi_otp_check_flag);
	usleep_range(5000, 6000);

	//set sensor mode:Standby
	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, 0x100, 0x0);

	CMR_LOGD("Get ov8856_huaquan_07b8_pad OTP info Exit\n");
	return rc;
}

/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_checksum;
 * Description : checksum;
 ***************************************************************************/
bool ov8856_huaquan_07b8_pad_checksum(void)
{
	int sum = 0;
    int i;
	for(i=0;i<OV8856_HUAQUAN_07B8_PAD_MAX_OTP_LENS_NUM;i++){
        sum += g_ov8856_huaquan_07b8_pad_otp.lenc[i];
    }

    if(g_ov8856_huaquan_07b8_pad_otp.checksum == (sum%255 + 1)){
        CMR_LOGE("ov8856_huaquan_07b8_pad OTP checksum sucessed!\n");
        return true;
    }

	return false;
}


/****************************************************************************
 * FunctionName: ov8856_huaquan_07b8_pad_set_otp_info;
 * Description : set otp data to sensor;
 * call this function after OV8856_HUAQUAN_07B8_PAD initialization
 ***************************************************************************/
int ov8856_huaquan_07b8_pad_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index)
{
	int32_t rc = 0;
	static int i_read_otp = 0;

	if(otp_function_lists[index].rg_ratio_typical)
	{
		RG_Ratio_Typical = otp_function_lists[index].rg_ratio_typical;
	}

	if(otp_function_lists[index].bg_ratio_typical)
	{
		BG_Ratio_Typical = otp_function_lists[index].bg_ratio_typical;
	}
	CMR_LOGD("%s, rg_ratio_typical=%04x,bg_ratio_typical=%04x\n", __func__,RG_Ratio_Typical,BG_Ratio_Typical );


	//Get otp info on the first time
	if ( 0 == i_read_otp )
	{
		rc = ov8856_huaquan_07b8_pad_get_otp_info(s_ctrl);

		if ( rc < 0 )
		{
			CMR_LOGE("%s:%d otp read failed.\n", __func__, __LINE__);
			return -1;
		}
		else
		{
			i_read_otp = 1;
		}
	}

    if(!ov8856_huaquan_07b8_pad_checksum()){
        CMR_LOGE("ov8856_huaquan_07b8_pad OTP checksum Failed!\n");
        s_ctrl->hw_otp_check_flag.mmi_otp_check_flag |= OV8856_HUAQUAN_07B8_PAD_OTP_CHECKED_FAILED;
        return -1;
    }

	CMR_LOGI("Set ov8856_huaquan_07b8_pad OTP info Enter\n");

	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, 0x100, 0x1);

	ov8856_huaquan_07b8_pad_update_awb_otp(s_ctrl);

	ov8856_huaquan_07b8_pad_update_lenc_otp(s_ctrl);

	usleep_range(5000, 6000);

	ov8856_huaquan_07b8_pad_otp_write_i2c(s_ctrl, 0x100, 0x0);

	CMR_LOGI("Set ov8856_huaquan_07b8_pad OTP info Exit\n");

	return rc;
}


