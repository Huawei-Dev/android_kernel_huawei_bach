
#define HW_CMR_LOG_TAG "sensor_otp_imx219"

#include <linux/hw_camera_common.h>
#include "msm_sensor.h"
#include "sensor_otp_common_if.h"
//Golden sensor typical ratio
static int RG_Ratio_Typical = 0x0220;
static int BG_Ratio_Typical = 0x0267;
static char otp_tmp_buf[291]={0};
#define IMX219_LCS_REG_NO      280
#define IMX219_MODULE_HUAWEI_ID 0xC8 //23060200
#define IMX219_I2C_RETRY_TIMES 3

//OTP info struct
typedef struct imx219_otp_info {
	uint8_t  year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  module_code;
	uint8_t  supplier_code;
	uint8_t  version;
	uint8_t  vendor_id;
	uint16_t rg_ratio;
	uint16_t bg_ratio;
	uint16_t gb_gr_ratio;
	uint8_t  lsc_param[IMX219_LCS_REG_NO];
	uint8_t  checksum;
}st_imx219_otp_info;

typedef enum imx219_groups_count{
	GROUP_1 = 0,
	GROUP_2,
	GROUP_MAX
}enum_imx219_groups;

#define IMX219_SUNNY_ID   1
#define IMX219_LITEON_ID  3

/*OTP READ STATUS*/
#define IMX219_OTP_MODULE_INFO_READ   (1 << 0)
#define IMX219_OTP_AWB_READ           (1 << 1)
#define IMX219_OTP_LSC_READ           (1 << 2)
#define IMX219_OTP_CHECKSUM_READ      (1 << 3)
#define IMX219_OTP_CHKSUM_SUCC        (1 << 4)
#define IMX219_OTP_FAIL_FLAG          (1 << 5)
#define IMX219_OTP_SUCCESS (IMX219_OTP_MODULE_INFO_READ|IMX219_OTP_AWB_READ| \
                            IMX219_OTP_LSC_READ|IMX219_OTP_CHECKSUM_READ|IMX219_OTP_CHKSUM_SUCC)

#define IMX219_MMI_OTP_MODULE_INFO_FLAG  (1 << 0)
#define IMX219_MMI_OTP_AWB_FLAG          (1 << 1)
#define IMX219_MMI_OTP_LSC_FLAG          (1 << 2)
#define IMX219_MMI_OTP_CHECKSUM_FLAG     (1 << 3)
#define IMX219_MMI_OTP_SUMVAL_FLAG       (1 << 4)
#define IMX219_MMI_OTP_FAIL (IMX219_MMI_OTP_MODULE_INFO_FLAG|IMX219_MMI_OTP_AWB_FLAG| \
                            IMX219_MMI_OTP_LSC_FLAG|IMX219_MMI_OTP_CHECKSUM_FLAG|IMX219_MMI_OTP_SUMVAL_FLAG)

static st_imx219_otp_info imx219_otp = {0};
static uint16_t  imx219_otp_flag   = 0;
static struct msm_camera_i2c_reg_array imx219_otp_lsc[IMX219_LCS_REG_NO];
static struct msm_camera_i2c_reg_setting st_imx219_otp_lsc;

/****************************************************************************
 * FunctionName: imx219_otp_write_i2c;
 * Description : write otp info via i2c;
 ***************************************************************************/
static int32_t imx219_otp_write_i2c(struct msm_sensor_ctrl_t *s_ctrl, int32_t addr, uint16_t data)
{
	int32_t rc = 0;
	int32_t i = 0;

	for(i = 0; i < IMX219_I2C_RETRY_TIMES; i++){
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(
 		s_ctrl->sensor_i2c_client,
 		addr,
 		data,
 		MSM_CAMERA_I2C_BYTE_DATA);

		if (rc < 0) {
			CMR_LOGE("%s, failed i = %d",__func__,i);
			continue;
		}
		break;
	}

	if ( rc < 0 ){
		CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, data);
#ifdef CONFIG_HUAWEI_DSM
		camera_report_dsm_err_otp(s_ctrl, DSM_CAMERA_OTP_I2C_ERR, addr, OTP_WRITE_I2C_ERR);
#endif
	}

	return rc;
}
/****************************************************************************
 * FunctionName: imx219_otp_write_i2c_table;
 * Description : write otp info via i2c;
 ***************************************************************************/
static int32_t imx219_otp_write_i2c_table(struct msm_sensor_ctrl_t *s_ctrl, struct msm_camera_i2c_reg_setting *write_setting)
{
	int32_t rc = 0;
	int32_t i = 0;

	if(NULL == write_setting){
		CMR_LOGE("%s fail,noting to write i2c \n", __func__);
		return -1;
	}

	for(i = 0; i < IMX219_I2C_RETRY_TIMES; i++){
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
 		s_ctrl->sensor_i2c_client,write_setting);

		if (rc < 0) {
			CMR_LOGE("%s, failed i = %d",__func__,i);
			continue;
		}
		break;
	}

	if ( rc < 0 ){
		CMR_LOGE("%s fail, rc = %d! \n", __func__, rc);
#ifdef CONFIG_HUAWEI_DSM
		camera_report_dsm_err_otp(s_ctrl, DSM_CAMERA_OTP_I2C_ERR, 0, OTP_WRITE_I2C_ERR);
#endif
	}

	return rc;
}

/****************************************************************************
 * FunctionName: imx219_otp_read_i2c;
 * Description : read otp info via i2c;
 ***************************************************************************/
static int32_t imx219_otp_read_i2c(struct msm_sensor_ctrl_t *s_ctrl,uint32_t addr, uint16_t *data)
{
	int32_t rc = 0;

	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(s_ctrl->sensor_i2c_client,
				addr,
				data,
				MSM_CAMERA_I2C_BYTE_DATA);
	if ( rc < 0 ){
		CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, *data);
#ifdef CONFIG_HUAWEI_DSM
		camera_report_dsm_err_otp(s_ctrl, DSM_CAMERA_OTP_I2C_ERR, addr, OTP_READ_I2C_ERR);
#endif
	}

	return rc;
}

static int imx219_check_status(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	uint16_t check_status =0;
	int delay_time = 5;
	do{
	    msleep(5);
        rc = imx219_otp_read_i2c(s_ctrl,0x3201, &check_status);
	    if(rc < 0){
			CMR_LOGE("%s: failed read 0x3201.\n", __func__);
			return rc;
	    }
	}while(((check_status & 0x01) == 0)&&(--delay_time));
	if(0 == delay_time){
		CMR_LOGE("imx219_read_otp_page error timeout %d",delay_time);
		return -1;
    }
	return rc;
}
static int imx219_otp_get_chksum(struct msm_sensor_ctrl_t* s_ctrl,enum_imx219_groups * group_sel)
{
	uint16_t check_val = 0;
	int rc = 0;
	
	if( imx219_check_status(s_ctrl)<0){
		CMR_LOGE("%s, read status error\n",__func__);
		return -1;
	}
	
	if( imx219_otp_write_i2c(s_ctrl, 0x3202, 0x09) < 0 ){
		CMR_LOGE("%s, otp stream off fail\n",__func__);
		return -1;
	}

	rc = imx219_otp_read_i2c(s_ctrl, 0x320B, &check_val);   //0x320B the second otp group checksum reg
	if(rc < 0){
		CMR_LOGE("%s fail read group 2 checksum reg 0x320B\n",__func__);
		return rc;
	}
		
	if( imx219_check_status(s_ctrl)<0){
		CMR_LOGE("%s, read status error\n",__func__);
		return -1;
	}
	if(0 == check_val){
		CMR_LOGW("read first otp value\n");		
		if( imx219_check_status(s_ctrl)<0){
			CMR_LOGE("%s, read status error\n",__func__);
			return -1;
		}
		if( imx219_otp_write_i2c(s_ctrl, 0x3202, 0x04) < 0 ){
			CMR_LOGE("%s, otp stream off fail\n",__func__);
			return -1;
		}

		rc = imx219_otp_read_i2c(s_ctrl, 0x3227, &check_val);   //0x3227 the first otp group checksum reg
		if(rc < 0){
			CMR_LOGE("%s fail read group 1 checksum reg 0x3227\n",__func__);
			return rc;
		}
		if( imx219_check_status(s_ctrl)<0){
			CMR_LOGE("%s, read status error\n",__func__);
			return -1;
		}
		if(0 != check_val){
			*group_sel = GROUP_1;
		}else{
			CMR_LOGE("this imx219 module burn otp bad.\n");
			return -1;
		}
	}else{
		CMR_LOGW("read second otp value\n");
		*group_sel = GROUP_2;
	}
	imx219_otp.checksum = check_val;
	return rc;
}


static int imx219_otp_init_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
	int i = 0;
	int32_t addr = 0;
	uint16_t data = 0;
	int rc = 0;
	struct msm_camera_i2c_reg_array imx219_init_setting[]=
	{
		{0x0100, 0x00, 0x00},//sensor sw_standby
		{0x3302, 0x02, 0x00},//otp write clk[15:8]
		{0x3303, 0x58, 0x00},//otp write clk[7:0],  wrcnt=600d
		{0x012A, 0x18, 0x00},//input frequency [15:8]
		{0x012B, 0x00, 0x00},//input frequency [7:0],
		{0x3300, 0x08, 0x00},//ECC off
		{0x3200, 0x01, 0x00},//read mode
	};

	for( i = 0; i < ARRAY_SIZE(imx219_init_setting); i++ ){
		addr = imx219_init_setting[i].reg_addr;
		data = imx219_init_setting[i].reg_data;
		rc = imx219_otp_write_i2c(s_ctrl,addr, data);
		if(rc < 0){
			CMR_LOGE("%s: failed. reg:0x%x, data:0x%x\n", __func__, addr, data);
			return rc;
		}
	}
	return rc;
}

static int imx219_otp_lsc_access_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
	int i = 0;
	int32_t addr = 0;
	uint16_t data = 0;
	int rc = 0;
	struct msm_camera_i2c_reg_array imx219_lsc_access_setting[]=
	{
		{0x30EB, 0x05, 0x00},//access the Specific Registers Area
		{0x30EB, 0x0C, 0x00},
		{0x300A, 0xFF, 0x00},
		{0x300B, 0xFF, 0x00},
		{0x30EB, 0x05, 0x00},
		{0x30EB, 0x09, 0x00},
	};
	for( i = 0; i < ARRAY_SIZE(imx219_lsc_access_setting); i++ ){
		addr = imx219_lsc_access_setting[i].reg_addr;
		data = imx219_lsc_access_setting[i].reg_data;
		rc = imx219_otp_write_i2c(s_ctrl,addr, data);
		if(rc < 0){
			CMR_LOGE("%s: failed. reg:0x%x, data:0x%x\n", __func__, addr, data);
			return rc;
		}
	}
	return rc;
}

static int imx219_otp_lsc_open_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
	int i = 0;
	int32_t addr = 0;
	uint16_t data = 0;
	int rc = 0;
	struct msm_camera_i2c_reg_array imx219_lsc_open_setting[]=
	{
		{0x0190, 0x01, 0x00},//LSC enable A
		{0x0192, 0x00, 0x00},//LSC table 0
		{0x0191, 0x00, 0x00},//LCS color mode:4 color R/Gr/Gb/B
		{0x0193, 0x00, 0x00},//LSC tuning disable
		{0x01a4, 0x03, 0x00},//Knot point format A:u4.8
	};
	for( i = 0; i < ARRAY_SIZE(imx219_lsc_open_setting); i++ ){
		addr = imx219_lsc_open_setting[i].reg_addr;
		data = imx219_lsc_open_setting[i].reg_data;
		rc = imx219_otp_write_i2c(s_ctrl,addr, data);
		if(rc < 0){
			CMR_LOGE("%s: failed. reg:0x%x, data:0x%x\n", __func__, addr, data);
			return rc;
		}
	}
	return rc;
}

static int imx219_read_otp_page(struct msm_sensor_ctrl_t *s_ctrl, unsigned int page, unsigned int group_flag)
{
	int i =0;
	static int j=0;
	uint16_t read_data =0;
	int rc =0;

	if( imx219_check_status(s_ctrl)<0){
		CMR_LOGE("%s, read status error\n",__func__);
		return -1;
	}
	
	if( imx219_otp_write_i2c(s_ctrl, 0x3202, page) < 0 ){
		CMR_LOGE("%s, otp stream off fail\n",__func__);
		return -1;
	}

	if(page == 4){
		if(group_flag==0){   //for group one otp data
			for(i=0;i<35;i++){  //group 4 only 35 valid data (0x3226-0x3204+1)
				rc = imx219_otp_read_i2c(s_ctrl,0x3204+i,&read_data);
				if(rc < 0){
						CMR_LOGE("%s: failed read page4 data.\n", __func__);
						return rc;
					}
				otp_tmp_buf[j++]=read_data&0xff;
			}
		}else{   //for group two otp data
			for(i=0;i<28;i++){//group 4 only 28 valid data(0x3243-0x3228+1)
				rc = imx219_otp_read_i2c(s_ctrl,0x3228+i,&read_data);
				if(rc < 0){
					CMR_LOGE("%s: failed read page4 data.\n", __func__);
					return rc;
				}
				otp_tmp_buf[j++]=read_data&0xff;
			}
		}
	}else if(page == 9){  //this page only 7 valid otp data(0x320a-0x3204+1)
		for(i=0;i<7;i++){
			rc = imx219_otp_read_i2c(s_ctrl,0x3204+i,&read_data);
			if(rc < 0){
				CMR_LOGE("%s: failed read page9 data.\n", __func__);
				return rc;
			}
			otp_tmp_buf[j++]=read_data&0xff;
		}
	}else{  // whole valid otp data
		for(i=0;i<64;i++){
            rc = imx219_otp_read_i2c(s_ctrl,0x3204+i,&read_data);
			if(rc < 0){
				CMR_LOGE("%s: failed read data.\n", __func__);
				return rc;
			}
			otp_tmp_buf[j++]=read_data&0xff;
		}
	}

	if( imx219_check_status(s_ctrl)<0){
		CMR_LOGE("%s, read status error\n",__func__);
		return -1;
	}
	return 0;
}

static int imx219_read_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t tmp_mmi_otp_flag = IMX219_MMI_OTP_FAIL;
	int rc = 0;
	int i= 0;
	uint32_t sum = 0;
	enum_imx219_groups group = GROUP_MAX;
	unsigned char check_sum_tmp = 0;

	CMR_LOGW("%s enter\n", __func__);
	if (IMX219_OTP_FAIL_FLAG == (imx219_otp_flag & IMX219_OTP_FAIL_FLAG)){
		CMR_LOGE("%s, IMX219_OTP_FAIL_FLAG\n", __func__);
		return -1;
	} 
	else if (IMX219_OTP_SUCCESS == imx219_otp_flag){
		CMR_LOGW("%s, IMX219_OTP_COMPLETE\n", __func__);
		return 0;
	}
	//initial global parameters.
	imx219_otp_flag = 0;
	memset(&imx219_otp, 0 , sizeof(imx219_otp));

	/*init setting before read otp*/
	rc =imx219_otp_init_setting(s_ctrl);
	if(rc < 0){
		CMR_LOGE("%s: failed to imx219 otp init setting.\n", __func__);
		goto GET_OTP_FAIL;
	}

	/*fifth read otp reg*/
	rc = imx219_otp_get_chksum(s_ctrl,&group);
	if ( (rc < 0)||(group >= GROUP_MAX) ){
		CMR_LOGE("%s,faild imx219_otp_get_chksum or group %d,group:%d\n", __func__,rc,group);
        goto GET_OTP_FAIL;
	}
	imx219_otp_flag |= IMX219_OTP_CHECKSUM_READ;
	tmp_mmi_otp_flag &= ~IMX219_MMI_OTP_CHECKSUM_FLAG;
	memset(otp_tmp_buf,0,sizeof(otp_tmp_buf));
	//group 2 is invalid
	if(group==GROUP_1){
		//group one otp data from page 0-4
		for(i=0; i<5; i++){
			rc = imx219_read_otp_page(s_ctrl, i, GROUP_1);
			if(rc < 0){
				CMR_LOGE("read otp page error");
				goto GET_OTP_FAIL;
			}
		}
	}else if(group==GROUP_2){
		//group one otp data from page 4-9
		for(i=4; i<10; i++){
			rc = imx219_read_otp_page(s_ctrl, i, GROUP_2);
			if(rc<0){
				CMR_LOGE("read otp page error");
				goto GET_OTP_FAIL;
			}
		}
	}else{
		CMR_LOGE("error group!");
		goto GET_OTP_FAIL;
	}

	for(i=0; i<291; i++){
        sum = otp_tmp_buf[i]+sum;
    }
	check_sum_tmp = sum %255 + 1;
	if(check_sum_tmp == imx219_otp.checksum){
		imx219_otp_flag |= IMX219_OTP_CHKSUM_SUCC;
		tmp_mmi_otp_flag &= ~IMX219_MMI_OTP_SUMVAL_FLAG;
		CMR_LOGE("%s , success verify checksum ,checksum=0x%x,group_checksum=0x%x\n",
					__func__, check_sum_tmp,imx219_otp.checksum);
		
	}else{
		CMR_LOGE("%s , failed verify checksum ,checksum=0x%x,group_checksum=0x%x\n",
					__func__, check_sum_tmp,imx219_otp.checksum);
        rc = -1;
		goto GET_OTP_FAIL;
    }
	/*get module id*/
	imx219_otp.year = otp_tmp_buf[0];
	imx219_otp.month = otp_tmp_buf[1];
	imx219_otp.day = otp_tmp_buf[2];
	imx219_otp.module_code = otp_tmp_buf[3];
	imx219_otp.supplier_code = (otp_tmp_buf[4]>>4)&0x0F;
	imx219_otp.version = (otp_tmp_buf[4]&0x0F);
	imx219_otp.vendor_id= ((otp_tmp_buf[4]>>4)&0x0F);
	CMR_LOGW("%s module info year 20%02d month %d day %d. huawei_id 0x%x,  vendor id&version 0x%x\n",
			__func__, imx219_otp.year, imx219_otp.month, imx219_otp.day, imx219_otp.module_code, otp_tmp_buf[4]);
	if(IMX219_MODULE_HUAWEI_ID != imx219_otp.module_code){
        CMR_LOGE("invalid otp module id: 0x%x",imx219_otp.module_code);
        rc = -1;
        goto GET_OTP_FAIL;
    }
	imx219_otp_flag |= IMX219_OTP_MODULE_INFO_READ;
	tmp_mmi_otp_flag &= ~IMX219_MMI_OTP_MODULE_INFO_FLAG;

	/*get awb info*/
	imx219_otp.rg_ratio = (otp_tmp_buf[5]<<8) | otp_tmp_buf[6];
	imx219_otp.bg_ratio = (otp_tmp_buf[7]<<8) | otp_tmp_buf[8];
	imx219_otp.gb_gr_ratio =(otp_tmp_buf[9]<<8) | otp_tmp_buf[10];
	
	CMR_LOGW("%s OTP data are rg_ratio=0x%x, bg_ratio=0x%x, gb_gr_ratio=0x%x\n", 
		__func__,imx219_otp.rg_ratio, imx219_otp.bg_ratio, imx219_otp.gb_gr_ratio);
	if(imx219_otp.rg_ratio == 0 || imx219_otp.bg_ratio == 0){
		CMR_LOGE("%s: rg_ratio=%d bg_ratio=%d fail\n",__func__,imx219_otp.rg_ratio,imx219_otp.bg_ratio);
		rc = -1;
        goto GET_OTP_FAIL;
	}
	imx219_otp_flag |= IMX219_OTP_AWB_READ;
	tmp_mmi_otp_flag &= ~IMX219_MMI_OTP_AWB_FLAG;
	
	/*get lsc info*/
	for(i = 0;i<IMX219_LCS_REG_NO;i++){
		imx219_otp.lsc_param[i]=otp_tmp_buf[i+11];
	}
	imx219_otp_flag |= IMX219_OTP_LSC_READ;
	tmp_mmi_otp_flag &= ~IMX219_MMI_OTP_LSC_FLAG;
	goto EXIT_OTP_READ;

GET_OTP_FAIL:
	imx219_otp_flag |= IMX219_OTP_FAIL_FLAG;

EXIT_OTP_READ:

	s_ctrl->hw_otp_check_flag.mmi_otp_check_flag  = tmp_mmi_otp_flag;
	CMR_LOGW("%s exit imx219_mmi_otp_flag = 0x%x\n",__func__, s_ctrl->hw_otp_check_flag.mmi_otp_check_flag);
	/*if otp read faild, set vendor_id to liteon default */
	if(IMX219_SUNNY_ID == imx219_otp.vendor_id)
	{
		s_ctrl->vendor_otp_info.vendor_id = IMX219_SUNNY_ID;
	}
	else
	{
		s_ctrl->vendor_otp_info.vendor_id = IMX219_LITEON_ID;
	}
	CMR_LOGW("%s vendor_id = %d\n", __func__, imx219_otp.vendor_id);
	return rc;
}

/****************************************************************************
 * FunctionName: imx219_update_awb_gain;
 * Description : write R_gain,G_gain,B_gain to otp;
 * 0 means write AWB info succeed.
 * -1 means write AWB info failed.
 ***************************************************************************/
static int imx219_update_awb_otp(struct msm_sensor_ctrl_t *s_ctrl)
{

	s_ctrl->awb_otp_info.RG = imx219_otp.rg_ratio;
	s_ctrl->awb_otp_info.BG = imx219_otp.bg_ratio;
	s_ctrl->awb_otp_info.typical_RG = RG_Ratio_Typical;
	s_ctrl->awb_otp_info.typical_BG = BG_Ratio_Typical;

	return 0;
}

/****************************************************************************
 * FunctionName: imx219_update_lenc_otp;
 * Description : write 240 lens shading otp data to sensor;
 ***************************************************************************/
static int imx219_update_lenc_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	int k = 0;
	int j = 0;
	int i = 0;
	for(i=0; i < 4; i++){
		for(j=0; j<70; j++){
			imx219_otp_lsc[k].reg_addr = 0xD200+j+i*72;
			imx219_otp_lsc[k].reg_data = imx219_otp.lsc_param[j+i*70];
			imx219_otp_lsc[k].delay = 0x00;
			k++;
		}
	}
	st_imx219_otp_lsc.reg_setting =imx219_otp_lsc;
	st_imx219_otp_lsc.size = ARRAY_SIZE(imx219_otp_lsc);
	st_imx219_otp_lsc.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
	st_imx219_otp_lsc.data_type = MSM_CAMERA_I2C_BYTE_DATA;
	st_imx219_otp_lsc.delay = 0;

	rc =imx219_otp_lsc_access_setting(s_ctrl);
	if(rc < 0){
		CMR_LOGE("%s: failed to imx219 otp lsc access setting.\n", __func__);
		return rc;
	}
	rc = imx219_otp_write_i2c_table(s_ctrl,&st_imx219_otp_lsc);
	if(rc < 0){
		CMR_LOGE("%s:%d failed write lsc.\n", __func__, __LINE__);
		return rc;
	}
	rc =imx219_otp_lsc_open_setting(s_ctrl);
	if(rc < 0){
		CMR_LOGE("%s: failed to imx219 otp lsc open setting.\n", __func__);
		return rc;
	}
	CMR_LOGW("set OTP LSC to sensor OK\n");
	return rc;
}
/****************************************************************************
 * FunctionName: imx219_set_otp_info;
 * Description : set otp data to sensor;
 * call this function after imx219 initialization
 ***************************************************************************/
int imx219_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index)
{
	int rc = 0;
	if(otp_function_lists[index].rg_ratio_typical){
		RG_Ratio_Typical = otp_function_lists[index].rg_ratio_typical;
	}

	if(otp_function_lists[index].bg_ratio_typical){
		BG_Ratio_Typical = otp_function_lists[index].bg_ratio_typical;
	}
	CMR_LOGW("%s, rg_ratio_typical=0x%04x,bg_ratio_typical=0x%04x\n", __func__,RG_Ratio_Typical,BG_Ratio_Typical );

	rc = imx219_read_otp(s_ctrl);
	if ( rc < 0 ){
		CMR_LOGE("%s:%d otp read failed.\n", __func__, __LINE__);
		return -1;
	}

	CMR_LOGW("Set imx219 OTP info Enter\n");
	if (IMX219_OTP_FAIL_FLAG == (imx219_otp_flag & IMX219_OTP_FAIL_FLAG)){
		CMR_LOGE("%s invalid otp info!\n", __func__);
		return -1;
	}
	imx219_update_awb_otp(s_ctrl);

	imx219_update_lenc_otp(s_ctrl);

	CMR_LOGW("%s exit\n", __func__);
	return rc;
}
