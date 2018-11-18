
#define HW_CMR_LOG_TAG "sensor_otp_hi843_qtech_fh843_pad"

#include <linux/hw_camera_common.h>
#include "msm_sensor.h"
#include "sensor_otp_common_if.h"
//Golden sensor typical ratio
static int RG_Ratio_Typical = 0x010C;
static int BG_Ratio_Typical = 0x0130;
#define HI843_QTECH_FH843_PAD_MODULE_REG_NO   10
#define HI843_QTECH_FH843_PAD_AWB_REG_NO      28
#define HI843_QTECH_FH843_PAD_LSC_REG_NO      858
#define HI843_QTECH_FH843_PAD_AF_REG_NO      5
#define HI843_I2C_RETRY_TIMES 3
//#define HI843_QTECH_FH843_PAD_DEBUG_OTP_DATA
#define loge_if_ret(x) \
{\
	if (x<0) \
	{\
		CMR_LOGE("'%s' failed", #x); \
		return -1; \
	} \
}
//OTP info struct
typedef struct hi843_qtech_fh843_pad_otp_info {
       uint8_t  vendor_id;
	uint8_t  year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  module_code;
	uint8_t  supplier_code;
	uint8_t  version;
	uint16_t rg_ratio;
	uint16_t bg_ratio;
	uint16_t gb_gr_ratio;
	uint16_t starting_dac;
	uint16_t infinity_dac;
	uint16_t macro_dac;
	//uint8_t  checksum;
}st_hi843_qtech_fh843_pad_otp_info;

typedef struct hi843_qtech_fh843_pad_otp_reg_addr {
	uint16_t start_addr;
	uint16_t checksum_module_addr;
	uint16_t checksum_lsc_addr;
	uint16_t checksum_awb_addr;
	uint16_t checksum_af_addr;
}st_reg_addr;

//hi843_qtech_fh843_pad has three groups: [1,2,3]
typedef enum hi843_qtech_fh843_pad_groups_count{
	GROUP_1 = 0,
	GROUP_2,
	GROUP_3,
	GROUP_MAX
}enum_hi843_qtech_fh843_pad_groups;

static uint16_t group_checksum_module = 0;
static uint16_t group_checksum_lsc = 0;
static uint16_t group_checksum_awb = 0;
static uint16_t group_checksum_af = 0;

static st_reg_addr hi843_qtech_fh843_pad_module_info_otp_read_addr[] = {
	{0x0202,0x0212,0x0598,0x0C7D,0x0CC0},
	{0x0213,0x0223,0x08FB,0x0C9B,0x0CC6},
	{0x0224,0x0234,0x0C5E,0x0CB9,0x0CCC},
};

static st_reg_addr hi843_qtech_fh843_pad_awb_otp_read_addr[] = {
	{0x0C60,0x0212,0x0598,0x0C7D,0x0CC0},
	{0x0C7E,0x0223,0x08FB,0x0C9B,0x0CC6},
	{0x0C9C,0x0234,0x0C5E,0x0CB9,0x0CCC},
};
static st_reg_addr hi843_qtech_fh843_pad_lsc_otp_read_addr[] = {
	{0x0236,0x0212,0x0598,0x0C7D,0x0CC0},
	{0x0599,0x0223,0x08FB,0x0C9B,0x0CC6},
	{0x08FC,0x0234,0x0C5E,0x0CB9,0x0CCC},
};
static st_reg_addr hi843_qtech_fh843_pad_af_otp_read_addr[] = {
	{0x0CBB,0x0212,0x0598,0x0C7D,0x0CC0},
	{0x0CC1,0x0223,0x08FB,0x0C9B,0x0CC6},
	{0x0CC7,0x0234,0x0C5E,0x0CB9,0x0CCC},
};
static struct msm_camera_i2c_reg_array hi843_qtech_fh843_pad_otp_init_setting[]=
{
//Sensor Information////////////////////////////
//Sensor		  : Hi-843B
//Date		  : 2015-11-24
//Customer          : Huawei
////////////////////////////////////////////////

//{0x0118, 0x0000}, //sleep On
  //{0x0000, 0x0300}, // image_orient / null

  {0x0a00, 0x0000},	//sleep On

  {0x0e00, 0x0002}, //tg_pmem_sckpw/sdly
  {0x0e02, 0x0002}, //tg_dmem_sckpw/sdly
  {0x2000, 0x4031},
  {0x2002, 0x8400},
  {0x2004, 0x40f2},
  {0x2006, 0x000a},
  {0x2008, 0x0f90},
  {0x200a, 0x43c2},
  {0x200c, 0x0f82},
  {0x200e, 0x40b2},
  {0x2010, 0x1807},
  {0x2012, 0x82a8},
  {0x2014, 0x40b2},
  {0x2016, 0x3540},
  {0x2018, 0x82aa},
  {0x201a, 0x40b2},
  {0x201c, 0x3540},
  {0x201e, 0x82ac},
  {0x2020, 0x4382},
  {0x2022, 0x82ae},
  {0x2024, 0x4382},
  {0x2026, 0x82b0},
  {0x2028, 0x4382},
  {0x202a, 0x82b2},
  {0x202c, 0x40b2},
  {0x202e, 0x0600},
  {0x2030, 0x82b4},
  {0x2032, 0x4382},
  {0x2034, 0x82b6},
  {0x2036, 0x12b0},
  {0x2038, 0xea48},
  {0x203a, 0x40b2},
  {0x203c, 0x2d0b},
  {0x203e, 0x0b96},
  {0x2040, 0x0c0a},
  {0x2042, 0x40b2},
  {0x2044, 0xc009},
  {0x2046, 0x0b98},
  {0x2048, 0x0c0a},
  {0x204a, 0x0900},
  {0x204c, 0x7312},
  {0x204e, 0x43d2},
  {0x2050, 0x0f82},
  {0x2052, 0x0cff},
  {0x2054, 0x0cff},
  {0x2056, 0x0cff},
  {0x2058, 0x0cff},
  {0x205a, 0x0cff},
  {0x205c, 0x0cff},
  {0x205e, 0x0cff},
  {0x2060, 0x0cff},
  {0x2062, 0x0cff},
  {0x2064, 0x0cff},
  {0x2066, 0x0cff},
  {0x2068, 0x0cff},
  {0x206a, 0x0cff},
  {0x206c, 0x0cff},
  {0x206e, 0x0cff},
  {0x2070, 0x0cff},
  {0x2072, 0x0cff},
  {0x2074, 0x0cff},
  {0x2076, 0x40f2},
  {0x2078, 0x000e},
  {0x207a, 0x0f90},
  {0x207c, 0x43d2},
  {0x207e, 0x0780},
  {0x2080, 0x4392},
  {0x2082, 0x760e},
  {0x2084, 0x9382},
  {0x2086, 0x760c},
  {0x2088, 0x2002},
  {0x208a, 0x0c64},
  {0x208c, 0x3ffb},
  {0x208e, 0x421f},
  {0x2090, 0x760a},
  {0x2092, 0x931f},
  {0x2094, 0x2013},
  {0x2096, 0x421b},
  {0x2098, 0x078a},
  {0x209a, 0x4b82},
  {0x209c, 0x7600},
  {0x209e, 0x0260},
  {0x20a0, 0x0000},
  {0x20a2, 0x0c56},
  {0x20a4, 0x0240},
  {0x20a6, 0x0000},
  {0x20a8, 0x0260},
  {0x20aa, 0x0000},
  {0x20ac, 0x0c1a},
  {0x20ae, 0x4b0f},
  {0x20b0, 0x12b0},
  {0x20b2, 0xe84c},
  {0x20b4, 0x4fc2},
  {0x20b6, 0x0788},
  {0x20b8, 0x4b0a},
  {0x20ba, 0x3fe2},
  {0x20bc, 0x903f},
  {0x20be, 0x0201},
  {0x20c0, 0x23df},
  {0x20c2, 0x531b},
  {0x20c4, 0x4b0e},
  {0x20c6, 0x108e},
  {0x20c8, 0xf37e},
  {0x20ca, 0xc312},
  {0x20cc, 0x100e},
  {0x20ce, 0x110e},
  {0x20d0, 0x110e},
  {0x20d2, 0x110e},
  {0x20d4, 0x4a0f},
  {0x20d6, 0x108f},
  {0x20d8, 0xf37f},
  {0x20da, 0xc312},
  {0x20dc, 0x100f},
  {0x20de, 0x110f},
  {0x20e0, 0x110f},
  {0x20e2, 0x110f},
  {0x20e4, 0x9f0e},
  {0x20e6, 0x27e3},
  {0x20e8, 0x0261},
  {0x20ea, 0x0000},
  {0x20ec, 0x4b82},
  {0x20ee, 0x7600},
  {0x20f0, 0x0260},
  {0x20f2, 0x0000},
  {0x20f4, 0x0c56},
  {0x20f6, 0x0240},
  {0x20f8, 0x0000},
  {0x20fa, 0x0260},
  {0x20fc, 0x0000},
  {0x20fe, 0x0c1a},
  {0x2100, 0x3fd6},
  {0x2102, 0x4030},
  {0x2104, 0xf106},
  {0x2106, 0xdf02},
  {0x2108, 0x3ffe},
  {0x210a, 0x4c82},
  {0x210c, 0x0130},
  {0x210e, 0x4a82},
  {0x2110, 0x0138},
  {0x2112, 0x4c82},
  {0x2114, 0x0134},
  {0x2116, 0x421e},
  {0x2118, 0x013a},
  {0x211a, 0x4292},
  {0x211c, 0x013c},
  {0x211e, 0x013a},
  {0x2120, 0x4b82},
  {0x2122, 0x0138},
  {0x2124, 0x4d82},
  {0x2126, 0x0134},
  {0x2128, 0x4a82},
  {0x212a, 0x0138},
  {0x212c, 0x421f},
  {0x212e, 0x013a},
  {0x2130, 0x4130},
  {0x2132, 0xee0e},
  {0x2134, 0x403b},
  {0x2136, 0x0011},
  {0x2138, 0x3c05},
  {0x213a, 0x100d},
  {0x213c, 0x6e0e},
  {0x213e, 0x9a0e},
  {0x2140, 0x2801},
  {0x2142, 0x8a0e},
  {0x2144, 0x6c0c},
  {0x2146, 0x6d0d},
  {0x2148, 0x831b},
  {0x214a, 0x23f7},
  {0x214c, 0x4130},
  {0x214e, 0xef0f},
  {0x2150, 0xee0e},
  {0x2152, 0x4039},
  {0x2154, 0x0021},
  {0x2156, 0x3c0a},
  {0x2158, 0x1008},
  {0x215a, 0x6e0e},
  {0x215c, 0x6f0f},
  {0x215e, 0x9b0f},
  {0x2160, 0x2805},
  {0x2162, 0x2002},
  {0x2164, 0x9a0e},
  {0x2166, 0x2802},
  {0x2168, 0x8a0e},
  {0x216a, 0x7b0f},
  {0x216c, 0x6c0c},
  {0x216e, 0x6d0d},
  {0x2170, 0x6808},
  {0x2172, 0x8319},
  {0x2174, 0x23f1},
  {0x2176, 0x4130},
  {0x2ffe, 0xf000},
  {0x0d00, 0x0707},
  {0x0d04, 0x0100},
  {0x004c, 0x0100}, //tg enable,hdr off

  // Sleep Off (Streaming On)
  {0x0a00, 0x0100},	//sleep Off

};
/*OTP READ STATUS*/
#define HI843_QTECH_FH843_PAD_OTP_MODULE_INFO_READ   (1 << 0)
#define HI843_QTECH_FH843_PAD_OTP_AWB_READ           (1 << 1)
#define HI843_QTECH_FH843_PAD_OTP_LSC_READ           (1 << 2)
#define HI843_QTECH_FH843_PAD_OTP_AF_READ           (1 << 3)
#define HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG          (1 << 4)

#define HI843_QTECH_FH843_PAD_OTP_SUCCESS (HI843_QTECH_FH843_PAD_OTP_MODULE_INFO_READ|HI843_QTECH_FH843_PAD_OTP_AWB_READ|HI843_QTECH_FH843_PAD_OTP_LSC_READ|HI843_QTECH_FH843_PAD_OTP_AF_READ)

#define HI843_QTECH_FH843_PAD_MMI_OTP_MODULE_INFO_FLAG  (1 << 0)
#define HI843_QTECH_FH843_PAD_MMI_OTP_AWB_FLAG          (1 << 1)
#define HI843_QTECH_FH843_PAD_MMI_OTP_LSC_FLAG          (1 << 2)
#define HI843_QTECH_FH843_PAD_MMI_OTP_AF_FLAG          (1 << 3)
#define HI843_QTECH_FH843_PAD_MMI_OTP_FAIL (HI843_QTECH_FH843_PAD_MMI_OTP_MODULE_INFO_FLAG|HI843_QTECH_FH843_PAD_MMI_OTP_AWB_FLAG|HI843_QTECH_FH843_PAD_MMI_OTP_LSC_FLAG|HI843_QTECH_FH843_PAD_MMI_OTP_AF_FLAG)

#define MODULE_FLAG_ADDR 0x0201
#define AWB_FLAG_ADDR 0x0C5F
#define LSC_FLAG_ADDR 0x0235
#define AF_FLAG_ADDR 0x0CBA

static st_hi843_qtech_fh843_pad_otp_info hi843_qtech_fh843_pad_otp_info = {0};
static uint16_t  hi843_qtech_fh843_pad_otp_flag   = 0;
static struct msm_camera_i2c_reg_setting st_hi843_qtech_fh843_pad_otp_init;
/****************************************************************************
 * FunctionName: hi843_qtech_fh843_pad_otp_write_i2c;
 * Description : write otp info via i2c;
 ***************************************************************************/
static int32_t hi843_qtech_fh843_pad_otp_write_i2c(struct msm_sensor_ctrl_t *s_ctrl, int32_t addr, uint16_t data)
{
	int32_t rc = 0;

	rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client,
		addr,
		data,
		MSM_CAMERA_I2C_BYTE_DATA);

	if ( rc < 0 ){
		CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, data);
#ifdef CONFIG_HUAWEI_DSM
		camera_report_dsm_err_otp(s_ctrl, DSM_CAMERA_OTP_I2C_ERR, addr, OTP_WRITE_I2C_ERR);
#endif
	}

	return rc;
}

/****************************************************************************
 * FunctionName: hi843_qtech_fh843_pad_otp_write_i2c_table;
 * Description : write otp info via i2c;
 ***************************************************************************/
static int32_t hi843_qtech_fh843_pad_otp_write_i2c_table(struct msm_sensor_ctrl_t *s_ctrl, struct msm_camera_i2c_reg_setting *write_setting)
{
	int32_t rc = 0;
        int32_t i = 0;

	if(NULL == write_setting){
	CMR_LOGE("%s fail,noting to write i2c \n", __func__);
	return -1;
	}
	for(i = 0; i < HI843_I2C_RETRY_TIMES; i++){
		rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(
		s_ctrl->sensor_i2c_client,write_setting);

		if (rc < 0){
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
 * FunctionName: hi843_qtech_fh843_pad_otp_read_i2c;
 * Description : read otp info via i2c;
 ***************************************************************************/
static int32_t hi843_qtech_fh843_pad_otp_read_i2c(struct msm_sensor_ctrl_t *s_ctrl,uint32_t addr, uint16_t *data)
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

static int32_t hi843_qtech_fh843_pad_otp_single_read(struct msm_sensor_ctrl_t *s_ctrl,uint32_t addr, uint16_t *data)
{
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl, 0x070A, (addr>>8)&0xFF));
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl, 0x070B, addr&0xFF));
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl, 0x0702, 0x01));
	loge_if_ret(hi843_qtech_fh843_pad_otp_read_i2c(s_ctrl,0x0708,data));
	return 0;
}

static int hi843_qtech_fh843_pad_otp_get_group(struct msm_sensor_ctrl_t* s_ctrl,int32_t addr,enum_hi843_qtech_fh843_pad_groups * group_sel)
{
	uint16_t group_num = 0xff;
	int rc = 0;

	rc = hi843_qtech_fh843_pad_otp_single_read(s_ctrl,addr,&group_num);
	CMR_LOGW("%s read group addr=0x%x, data=0x%x\n",__func__,addr,group_num);
	if(rc < 0){
		CMR_LOGE("%s read group addr=0x%x\n",__func__,addr);
	}
	switch(group_num)
	{
		case 0x37://group 3
		{
			*group_sel = GROUP_3;
			break;
		}
		case 0x13://group 2
		{
			*group_sel = GROUP_2;
			break;
		}
		case 0x01://group 1
		{
			*group_sel = GROUP_1;
			break;
		}
		default:
		{
			CMR_LOGE("%s hi843_qtech_fh843_pad module burn otp bad\n",__func__);
			rc = -1;
			break;
		}
	}
	return rc;
}

static int hi843_qtech_fh843_pad_otp_continuous_read(struct msm_sensor_ctrl_t* s_ctrl,st_reg_addr addr,int rd_num,uint16_t * rd_buf,int *sum)
{
	int rc = 0;
	int i=0;
	int sum1=0;
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl, 0x070A, (addr.start_addr)>>8&0xFF));//continus single read mode
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl, 0x070B, (addr.start_addr)&0xFF));
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl, 0x0702, 0x01));
    for(i = 0; i < rd_num; i++)//id part
    {
        rc = hi843_qtech_fh843_pad_otp_read_i2c(s_ctrl, 0x0708, &rd_buf[i]);
		if(rc < 0){
			CMR_LOGE("%s ,%d,fail hi843_qtech_fh843_pad_otp_read_i2c,i=%d,rc=%d\n", __func__,__LINE__,i,rc);
			return -1;
		}
		sum1 += rd_buf[i];
		#ifdef HI843_QTECH_FH843_PAD_DEBUG_OTP_DATA
		CMR_LOGE("%s,OTP Info:addr = 0x%x, data = 0x%x",__func__,addr.start_addr+i,rd_buf[i]);
		#endif
    }

	if(sum != NULL){
		*sum= sum1;
	}

	return rc;
}
static int hi843_qtech_fh843_pad_otp_get_module_info(struct msm_sensor_ctrl_t* s_ctrl,st_reg_addr module_addr)
{
	uint16_t buf[HI843_QTECH_FH843_PAD_MODULE_REG_NO] = {0};
	int rc = 0;
	int sum_module=0;
    uint16_t checksum= 0;
	rc = hi843_qtech_fh843_pad_otp_continuous_read(s_ctrl,module_addr,HI843_QTECH_FH843_PAD_MODULE_REG_NO, buf,&sum_module);
	if(rc < 0){
		CMR_LOGE("%s ,fail read hi843_qtech_fh843_pad module info \n", __func__);
		return -1;
	}
	CMR_LOGW("%s module info module house id %d AF/FF flag %d year 20%02d month %d day %d. sensor id 0x%x,  lens id 0x%x vcm id 0x%x,  driver ic  id 0x%x F number 0x%x\n",
			__func__, buf[0], buf[1], buf[2], buf[3], buf[4],buf[5], buf[6], buf[7], buf[8], buf[9]);

	checksum = sum_module%255+1;//just use the last part checksum value to set final checksum flag validation
	if(checksum != group_checksum_module){
		CMR_LOGE("%s ,failed verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,module_addr.checksum_module_addr,group_checksum_module);
		return -1;
    }
     s_ctrl->vendor_otp_info.vendor_id = buf[0];
    CMR_LOGW("%s ,module_info success verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,module_addr.checksum_module_addr,group_checksum_module);

	return rc;
}
static int hi843_qtech_fh843_pad_otp_get_awb(struct msm_sensor_ctrl_t* s_ctrl,st_reg_addr  awb_addr)
{
	uint16_t buf[HI843_QTECH_FH843_PAD_AWB_REG_NO] = {0};
	int rc = 0;
    int sum_awb=0;
	uint16_t checksum= 0;
	rc = hi843_qtech_fh843_pad_otp_continuous_read(s_ctrl,awb_addr,HI843_QTECH_FH843_PAD_AWB_REG_NO, buf,&sum_awb);
	if(rc < 0){
		CMR_LOGE("%s ,fail hi843_qtech_fh843_pad_otp_read_i2c\n", __func__);
		return rc;
	}
	checksum =sum_awb%255+1;//just use the last part checksum value to set final checksum flag validation
	if(checksum != group_checksum_awb){
		CMR_LOGE("%s ,awb failed verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,awb_addr.checksum_awb_addr,group_checksum_awb);
		return -1;
    }
    CMR_LOGW("%s ,awb success verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,awb_addr.checksum_awb_addr,group_checksum_awb);
	hi843_qtech_fh843_pad_otp_info.rg_ratio = (buf[0]  << 8) | buf[1];
	hi843_qtech_fh843_pad_otp_info.bg_ratio = (buf[2]  << 8) | buf[3];
	hi843_qtech_fh843_pad_otp_info.gb_gr_ratio = (buf[4] << 8) | buf[5];

	//RG_Ratio_Typical = (buf[6] << 8) | buf[7];
	//BG_Ratio_Typical = (buf[8] << 8) | buf[9];

	CMR_LOGW("%s OTP data are rg_ratio=0x%x, bg_ratio=0x%x, gb_gr_ratio=0x%x\n", __func__,hi843_qtech_fh843_pad_otp_info.rg_ratio, hi843_qtech_fh843_pad_otp_info.bg_ratio, hi843_qtech_fh843_pad_otp_info.gb_gr_ratio);

	if (0 == hi843_qtech_fh843_pad_otp_info.rg_ratio || 0 == hi843_qtech_fh843_pad_otp_info.bg_ratio || 0 == hi843_qtech_fh843_pad_otp_info.gb_gr_ratio){
		//if awb value read is error for zero, abnormal branch deal
		CMR_LOGE("%s OTP awb is wrong!!!\n", __func__);
		return -1;
	}
	return 0;
}

static int hi843_qtech_fh843_pad_otp_get_lsc(struct msm_sensor_ctrl_t* s_ctrl,st_reg_addr  lsc_addr)
{
	uint16_t buf[HI843_QTECH_FH843_PAD_LSC_REG_NO] = {0};
	int rc = 0;
	int sum_lsc=0;
	uint16_t checksum= 0;
	rc = hi843_qtech_fh843_pad_otp_continuous_read(s_ctrl,lsc_addr,HI843_QTECH_FH843_PAD_LSC_REG_NO, buf,&sum_lsc);
	if(rc < 0){
		CMR_LOGE("%s ,fail hi843_qtech_fh843_pad_otp_read_i2c\n", __func__);
		return rc;
	}
	checksum = sum_lsc%255+1;//just use the last part checksum value to set final checksum flag validation
	if(checksum != group_checksum_lsc){
		CMR_LOGE("%s ,lsc failed verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,lsc_addr.checksum_lsc_addr,group_checksum_lsc);
		return -1;
    }
	CMR_LOGW("%s ,lsc success verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,lsc_addr.checksum_lsc_addr,group_checksum_lsc);
	return 0;
}

static int hi843_qtech_fh843_pad_otp_get_af(struct msm_sensor_ctrl_t* s_ctrl,st_reg_addr  af_addr)
{
	uint16_t buf[HI843_QTECH_FH843_PAD_AF_REG_NO] = {0};
	int rc = 0;
	int sum_af=0;
	uint16_t checksum= 0;
	rc = hi843_qtech_fh843_pad_otp_continuous_read(s_ctrl,af_addr,HI843_QTECH_FH843_PAD_AF_REG_NO, buf,&sum_af);
	if(rc < 0){
		CMR_LOGE("%s ,fail hi843_qtech_fh843_pad_otp_read_i2c\n", __func__);
		return rc;
	}
	checksum = sum_af%255+1;//just use the last part checksum value to set final checksum flag validation
	if(checksum != group_checksum_af){
		CMR_LOGE("%s ,af failed verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,af_addr.checksum_af_addr,group_checksum_af);
		return -1;
        }
	CMR_LOGW("%s ,af success verify checksum ,checksum=0x%x,group_checksum_addr=0x%x,group_checksum=0x%x\n",
					__func__, checksum,af_addr.checksum_lsc_addr,group_checksum_af);
	hi843_qtech_fh843_pad_otp_info.infinity_dac = (buf[0]  << 8) | buf[1];
	hi843_qtech_fh843_pad_otp_info.macro_dac = (buf[2]  << 8) | buf[3];
	hi843_qtech_fh843_pad_otp_info.starting_dac = hi843_qtech_fh843_pad_otp_info.infinity_dac;

	return 0;
}

static int hi843_qtech_fh843_pad_otp_init_set(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	st_hi843_qtech_fh843_pad_otp_init.reg_setting =hi843_qtech_fh843_pad_otp_init_setting;
	st_hi843_qtech_fh843_pad_otp_init.size = ARRAY_SIZE(hi843_qtech_fh843_pad_otp_init_setting);
	st_hi843_qtech_fh843_pad_otp_init.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
	st_hi843_qtech_fh843_pad_otp_init.data_type = MSM_CAMERA_I2C_WORD_DATA;
	st_hi843_qtech_fh843_pad_otp_init.delay = 0;
	rc = hi843_qtech_fh843_pad_otp_write_i2c_table(s_ctrl,&st_hi843_qtech_fh843_pad_otp_init);
	if(rc < 0){
		CMR_LOGE("%s:%d failed set otp init setting.\n", __func__, __LINE__);
		return rc;
	}

	CMR_LOGW("set  otp init setting to sensor OK\n");
	return rc;
}

static int hi843_qtech_fh843_pad_otp_display_mode(struct msm_sensor_ctrl_t *s_ctrl)
{
	int i = 0;
	int32_t addr = 0;
	uint16_t data = 0;
	int rc = 0;

	struct msm_camera_i2c_reg_array hi843_qtech_fh843_pad_otp_enable_setting[]=
	{
		{0x0f02, 0x00, 0x00},/*pll diszble*/
		{0x071a, 0x01, 0x00},/*CP TRIM_H*/
		{0x071b, 0x09, 0x00},/*IPGM TRIM_H*/
		{0x0d04, 0x01, 0x00},/*fync(otp busy)output enable*/
		{0x0d00, 0x07, 0x00},/*fync(otp busy)output drivability*/
		{0x003e, 0x10, 0x00},/*otp r/w mode*/
		{0x0a00, 0x01, 0x00},/*stand by off*/
	};

	/*fast sleep on*/
	rc = hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x0a02, 0x01);
	if(rc < 0){
		CMR_LOGE("%s: failed write 0x0a02\n", __func__);
		return rc;
	}
	/*stand by on*/
	rc = hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x0a00, 0x00);
	if(rc < 0){
		CMR_LOGE("%s: failed write 0x0a02\n", __func__);
		return rc;
	}
	/* delay 10ms*/
	msleep(10);
	for( i = 0; i < ARRAY_SIZE(hi843_qtech_fh843_pad_otp_enable_setting); i++ ){
		addr = hi843_qtech_fh843_pad_otp_enable_setting[i].reg_addr;
		data = hi843_qtech_fh843_pad_otp_enable_setting[i].reg_data;
		rc = hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,addr, data);
		if(rc < 0){
			CMR_LOGE("%s: failed. reg:0x%x, data:0x%x\n", __func__, addr, data);
			return rc;
		}
	}
	return rc;
}
static int hi843_qtech_fh843_pad_otp_init(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;

	/*write init setting*/
	rc = hi843_qtech_fh843_pad_otp_init_set(s_ctrl);
	if(rc < 0){
		CMR_LOGE("%s: failed to hi843_qtech_fh843_pad otp init setting.\n", __func__);
		return rc;
	}

	/*enter display mode*/
	rc = hi843_qtech_fh843_pad_otp_display_mode(s_ctrl);
	if(rc < 0){
		CMR_LOGE("%s: failed hi843_qtech_fh843_pad_otp_display_mode.\n", __func__);
	}

	return rc;
}
static int hi843_qtech_fh843_pad_otp_exit_read(struct msm_sensor_ctrl_t *s_ctrl)
{
	int rc = 0;
	/*stand by on*/
	rc = hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x0a00, 0x00);
	if(rc < 0){
		CMR_LOGE("%s: failed write 0x0a00 stream off\n", __func__);
		return rc;
	}
	/*Fourth delay 10ms*/
	msleep(10);
	/*display mode*/
	rc = hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x003e, 0x00);
	if(rc < 0){
		CMR_LOGE("%s: failed write 0x003e display mode\n", __func__);
		return rc;
	}
	/*stand by off*/
	rc = hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x0a00, 0x01);
	if(rc < 0){
		CMR_LOGE("%s: failed write 0x0a00 stream on\n", __func__);
		return rc;
	}

	return rc;
}

static int hi843_qtech_fh843_pad_read_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t tmp_mmi_otp_flag = HI843_QTECH_FH843_PAD_MMI_OTP_FAIL;
	int rc = 0;
	enum_hi843_qtech_fh843_pad_groups group = GROUP_1;

	CMR_LOGW("%s enter\n", __func__);
	if (HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG == (hi843_qtech_fh843_pad_otp_flag & HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG)){
		CMR_LOGE("%s, HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG\n", __func__);
		return -1;
	} else if (HI843_QTECH_FH843_PAD_OTP_SUCCESS == hi843_qtech_fh843_pad_otp_flag){
		CMR_LOGW("%s, HI843_QTECH_FH843_PAD_OTP_COMPLETE\n", __func__);
		return 0;
	}
	//initial global parameters.
	hi843_qtech_fh843_pad_otp_flag = 0;
	memset(&hi843_qtech_fh843_pad_otp_info, 0 , sizeof(hi843_qtech_fh843_pad_otp_info));

	/*init before read otp*/
	rc = hi843_qtech_fh843_pad_otp_init(s_ctrl);
	if(rc < 0){
		CMR_LOGE("faild hi843_qtech_fh843_pad_otp_init\n");
		goto GET_OTP_FAIL;
	}

	/*read module info group*/
	rc = hi843_qtech_fh843_pad_otp_get_group(s_ctrl,MODULE_FLAG_ADDR,&group);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get MODULE group,group:%d,\n", __func__,group);
		goto GET_OTP_FAIL;
	}
	/*read checksum of module info*/
	rc = hi843_qtech_fh843_pad_otp_single_read(s_ctrl,hi843_qtech_fh843_pad_module_info_otp_read_addr[group].checksum_module_addr,&group_checksum_module);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get module checksum. \n", __func__);
		goto GET_OTP_FAIL;
	}
	/*read module info*/
	rc =hi843_qtech_fh843_pad_otp_get_module_info(s_ctrl,hi843_qtech_fh843_pad_module_info_otp_read_addr[group]);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad_otp_init_read\n", __func__);
		goto GET_OTP_FAIL;
	}
	hi843_qtech_fh843_pad_otp_flag |= HI843_QTECH_FH843_PAD_OTP_MODULE_INFO_READ;
	tmp_mmi_otp_flag &= ~HI843_QTECH_FH843_PAD_MMI_OTP_MODULE_INFO_FLAG;


	/*read lsc  group*/
	rc = hi843_qtech_fh843_pad_otp_get_group(s_ctrl,LSC_FLAG_ADDR,&group);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get lsc group,group:%d,\n", __func__,group);
		goto GET_OTP_FAIL;
	}
	/*read checksum of lsc*/
	rc = hi843_qtech_fh843_pad_otp_single_read(s_ctrl,hi843_qtech_fh843_pad_module_info_otp_read_addr[group].checksum_lsc_addr,&group_checksum_lsc);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get lsc checksum. \n", __func__);
		goto GET_OTP_FAIL;
	}
	/*read lsc info*/
	rc = hi843_qtech_fh843_pad_otp_get_lsc(s_ctrl,hi843_qtech_fh843_pad_lsc_otp_read_addr[group]);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad_otp_get_lsc\n", __func__);
		goto GET_OTP_FAIL;
	}
	hi843_qtech_fh843_pad_otp_flag |= HI843_QTECH_FH843_PAD_OTP_LSC_READ;
	tmp_mmi_otp_flag &= ~HI843_QTECH_FH843_PAD_MMI_OTP_LSC_FLAG;


	/*read awb  group*/
	rc = hi843_qtech_fh843_pad_otp_get_group(s_ctrl,AWB_FLAG_ADDR,&group);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get awb group,group:%d,\n", __func__,group);
		goto GET_OTP_FAIL;
	}
	/*read checksum of awb*/
	rc = hi843_qtech_fh843_pad_otp_single_read(s_ctrl,hi843_qtech_fh843_pad_module_info_otp_read_addr[group].checksum_awb_addr,&group_checksum_awb);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get awb checksum. \n", __func__);
		goto GET_OTP_FAIL;
	}
	/*read awb info*/
	rc = hi843_qtech_fh843_pad_otp_get_awb(s_ctrl,hi843_qtech_fh843_pad_awb_otp_read_addr[group]);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad_otp_get_awb\n", __func__);
		goto GET_OTP_FAIL;
	}
	hi843_qtech_fh843_pad_otp_flag |= HI843_QTECH_FH843_PAD_OTP_AWB_READ;
	tmp_mmi_otp_flag &= ~HI843_QTECH_FH843_PAD_MMI_OTP_AWB_FLAG;


	/*read af  group*/
	rc = hi843_qtech_fh843_pad_otp_get_group(s_ctrl,AF_FLAG_ADDR,&group);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get af group,group:%d,\n", __func__,group);
		goto GET_OTP_FAIL;
	}
	/*read checksum of af*/
	rc = hi843_qtech_fh843_pad_otp_single_read(s_ctrl,hi843_qtech_fh843_pad_module_info_otp_read_addr[group].checksum_af_addr,&group_checksum_af);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad otp get af checksum. \n", __func__);
		goto GET_OTP_FAIL;
	}

	/*read af info*/
	rc = hi843_qtech_fh843_pad_otp_get_af(s_ctrl,hi843_qtech_fh843_pad_af_otp_read_addr[group]);
	if ( rc < 0 ){
		CMR_LOGE("%s,faild hi843_qtech_fh843_pad_otp_get_af\n", __func__);
		goto GET_OTP_FAIL;
	}
	hi843_qtech_fh843_pad_otp_flag |= HI843_QTECH_FH843_PAD_OTP_AF_READ;
	tmp_mmi_otp_flag &= ~HI843_QTECH_FH843_PAD_MMI_OTP_AF_FLAG;
        goto EXIT_OTP_READ;

GET_OTP_FAIL:
	hi843_qtech_fh843_pad_otp_flag |= HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG;

EXIT_OTP_READ:
	//exit hi843_qtech_fh843_pad otp read
	rc = hi843_qtech_fh843_pad_otp_exit_read(s_ctrl);
	if( rc < 0 ){
		CMR_LOGE("%s, failed hi843_qtech_fh843_pad_otp_exit_read\n",__func__);
		hi843_qtech_fh843_pad_otp_flag |= HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG;
		tmp_mmi_otp_flag = HI843_QTECH_FH843_PAD_MMI_OTP_FAIL;
		rc = -1;
	}

	s_ctrl->hw_otp_check_flag.mmi_otp_check_flag  = tmp_mmi_otp_flag;
	CMR_LOGW("%s exit hi843_qtech_fh843_pad_mmi_otp_flag = 0x%x\n",__func__, s_ctrl->hw_otp_check_flag.mmi_otp_check_flag);
	return rc;
}

/****************************************************************************
 * FunctionName: hi843_qtech_fh843_pad_update_awb_gain;
 * Description : write R_gain,G_gain,B_gain to otp;
 * 0x200 =1x Gain
 * 0 means write AWB info succeed.
 * -1 means write AWB info failed.
 ***************************************************************************/
static int hi843_qtech_fh843_pad_update_awb_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
	uint16_t R_gain=1, G_gain=1 ,B_gain=1, Base_gain=1;
	if(hi843_qtech_fh843_pad_otp_info.rg_ratio == 0 || hi843_qtech_fh843_pad_otp_info.bg_ratio == 0){
		CMR_LOGE("%s: rg_ratio=%d bg_ratio=%d fail\n",__func__,hi843_qtech_fh843_pad_otp_info.rg_ratio,hi843_qtech_fh843_pad_otp_info.bg_ratio);
		return -1;
	}
	//calculate G gain
	//0x200 = 1x gain
	R_gain = (RG_Ratio_Typical*0x200)/hi843_qtech_fh843_pad_otp_info.rg_ratio;
	B_gain = (BG_Ratio_Typical*0x200)/hi843_qtech_fh843_pad_otp_info.bg_ratio;
	G_gain = 0x200;
	if(R_gain < 0x200 || ( B_gain < 0x200 ) ){
		if(R_gain < B_gain )
			Base_gain = R_gain;
		else
			Base_gain = B_gain;
	}
	else
	{
	Base_gain = G_gain;
	}
	R_gain = 0x200*R_gain/Base_gain;
	B_gain = 0x200*B_gain/Base_gain;
	G_gain = 0x200*G_gain/Base_gain;
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x0508,G_gain>>8));       /*otp_gain_gr_h*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x0509,G_gain & 0x00ff)); /*otp_gain_gr_l*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x050a,G_gain>>8));       /*otp_gain_gb_h*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x050b,G_gain & 0x00ff)); /*otp_gain_gb_l*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x050c,R_gain>>8));       /*otp_gain_r_h*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x050d,R_gain & 0x00ff)); /*otp_gain_r_l*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x050e,B_gain>>8));       /*otp_gain_b_h*/
	loge_if_ret(hi843_qtech_fh843_pad_otp_write_i2c(s_ctrl,0x050f,B_gain & 0x00ff)); /*otp_gain_b_l*/
	CMR_LOGW("%s: R_gain=0x%x, G_gain=0x%x, B_gain=0x%x \n",__func__,R_gain,G_gain,B_gain);
	return 0;
}
/*
**************************************************************************
* FunctionName: hi843_qtech_fh843_pad_set_af_to_platform;
* Description : set the otp info into sctrl;
* Input         : s_ctrl:the struct of sensor controller;
* Output       : NA;
* ReturnValue:NA;
* Other         : NA;
**************************************************************************
*/
static void hi843_qtech_fh843_pad_set_af_to_platform(struct msm_sensor_ctrl_t* s_ctrl)
{

	if (HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG == (hi843_qtech_fh843_pad_otp_flag & HI843_QTECH_FH843_PAD_OTP_FAIL_FLAG))
	{
		CMR_LOGE("%s invalid otp info!\n", __func__);
		return;
	}

	/*set VCM*/
	s_ctrl->afc_otp_info.starting_dac = hi843_qtech_fh843_pad_otp_info.starting_dac;
	s_ctrl->afc_otp_info.infinity_dac = hi843_qtech_fh843_pad_otp_info.infinity_dac;
	s_ctrl->afc_otp_info.macro_dac = hi843_qtech_fh843_pad_otp_info.macro_dac;

	return;
}

/****************************************************************************
 * FunctionName: hi843_qtech_fh843_pad_set_otp_info;
 * Description : set otp data to sensor;
 ***************************************************************************/
int hi843_qtech_fh843_pad_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index)
{
	int rc = 0;
	if(otp_function_lists[index].rg_ratio_typical){
		RG_Ratio_Typical = otp_function_lists[index].rg_ratio_typical;
	}

	if(otp_function_lists[index].bg_ratio_typical){
		BG_Ratio_Typical = otp_function_lists[index].bg_ratio_typical;
	}
	CMR_LOGW("%s, rg_ratio_typical=0x%04x,bg_ratio_typical=0x%04x\n", __func__,RG_Ratio_Typical,BG_Ratio_Typical );
	//Get otp info on the first time
	rc = hi843_qtech_fh843_pad_read_otp(s_ctrl);
	if ( rc < 0 ){
		CMR_LOGE("%s:%d otp read failed.\n", __func__, __LINE__);
	return -1;
	}
	hi843_qtech_fh843_pad_update_awb_otp(s_ctrl);
	hi843_qtech_fh843_pad_set_af_to_platform(s_ctrl);
	CMR_LOGW("%s exit\n", __func__);
	return rc;
}

