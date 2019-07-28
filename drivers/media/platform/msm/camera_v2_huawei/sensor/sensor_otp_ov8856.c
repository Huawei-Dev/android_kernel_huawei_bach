#define HW_CMR_LOG_TAG "sensor_otp_ov8856"

#include <linux/hw_camera_common.h>
#include "msm_sensor.h"
#include "sensor_otp_common_if.h"

//Golden sensor typical ratio
static uint32_t RG_Ratio_Typical = 0x268;
static uint32_t BG_Ratio_Typical = 0x2C3;

#define OV8856_MODULE_REG_NO   5
#define OV8856_AWB_REG_NO      6
#define OV8856_LSC_REG_NO      240
#define OV8856_OTP_REG_NO      504
#define OV8856_MODULE_HUAWEI_ID 0xC8 //23060200

#define OV8856_LENC_EN_ADDR          0x5000
#define OV8856_LENC_VSCALE_ADDR      0x59f8
#define OV8856_GAIN_1X_REG           0x400
#define OV8856_STREAM_ONOFF_ADDR     0x0100
#define OV8856_DPC_EN_ADDR           0x5001
#define OV8856_OTP_START_ADDR 0x7010
#define OV8856_CHKSUM_ADDR_GROUP1   0x710B
#define OV8856_CHKSUM_ADDR_GROUP2   0x7207

#define OV8856_LSC_CAL_START_REG 0x5900

#define OV8856_AWB_CAL_RGAIN_H 0x5019
#define OV8856_AWB_CAL_RGAIN_L 0x501a
#define OV8856_AWB_CAL_GGAIN_H 0x501b
#define OV8856_AWB_CAL_GGAIN_L 0x501c
#define OV8856_AWB_CAL_BGAIN_H 0x501d
#define OV8856_AWB_CAL_BGAIN_L 0x501e

//OTP info struct
typedef struct ov8856_otp_info {
    uint8_t  year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  module_code;
    uint8_t  supplier_code;
    uint8_t  version;
    uint16_t rg_ratio;
    uint16_t bg_ratio;
    uint16_t gb_gr_ratio;
    uint8_t  lsc_param[OV8856_LSC_REG_NO];
    uint8_t  checksum;
}st_ov8856_otp_info;

typedef struct ov8856_otp_reg_addr {
    uint16_t start_module;
    uint16_t start_awb;
    uint16_t start_lsc;
}st_reg_addr;

//OV8856 has two groups: [1,2]
typedef enum ov8856_groups_count{
    GROUP_1 = 0,
    GROUP_2,
    GROUP_MAX
}enum_ov8856_groups;

/*OTP READ STATUS*/
#define OV8856_OTP_MODULE_INFO_READ   (1 << 0)
#define OV8856_OTP_AWB_READ           (1 << 1)
#define OV8856_OTP_LSC_READ           (1 << 2)
#define OV8856_OTP_CHECKSUM_READ      (1 << 3)
#define OV8856_OTP_CHKSUM_SUCC        (1 << 4)
#define OV8856_OTP_FAIL_FLAG          (1 << 5)
#define OV8856_OTP_SUCCESS (OV8856_OTP_MODULE_INFO_READ|OV8856_OTP_AWB_READ| \
                            OV8856_OTP_LSC_READ|OV8856_OTP_CHECKSUM_READ|OV8856_OTP_CHKSUM_SUCC)

#define OV8856_MMI_OTP_MODULE_INFO_FLAG  (1 << 0)
#define OV8856_MMI_OTP_AWB_FLAG          (1 << 1)
#define OV8856_MMI_OTP_LSC_FLAG          (1 << 2)
#define OV8856_MMI_OTP_CHECKSUM_FLAG     (1 << 3)
#define OV8856_MMI_OTP_SUMVAL_FLAG       (1 << 4)
#define OV8856_MMI_OTP_FAIL (OV8856_MMI_OTP_MODULE_INFO_FLAG|OV8856_MMI_OTP_AWB_FLAG| \
                            OV8856_MMI_OTP_LSC_FLAG|OV8856_MMI_OTP_CHECKSUM_FLAG|OV8856_MMI_OTP_SUMVAL_FLAG)

static st_ov8856_otp_info ov8856_otp = {0};
static int OTPSUMVAL         = 0;
static uint16_t  ov8856_otp_flag   = 0;
static st_reg_addr ov8856_otp_read_addr[] = {
    {0x7010,0x7015,0x701B},
    {0x710C,0x7111,0x7117},
};

static struct msm_camera_i2c_reg_array ov8856_otp_bufclear[OV8856_OTP_REG_NO];
static struct msm_camera_i2c_reg_array ov8856_otp_lsc[OV8856_LSC_REG_NO];
static struct msm_camera_i2c_reg_setting st_ov8856_otp_bufclear;
static struct msm_camera_i2c_reg_setting st_ov8856_otp_lsc;

/****************************************************************************
 * FunctionName: ov8856_otp_write_i2c;
 * Description : write otp info via i2c;
 ***************************************************************************/
static int32_t ov8856_otp_write_i2c(struct msm_sensor_ctrl_t *s_ctrl, int32_t addr, uint16_t data)
{
    int32_t rc = 0;

    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write(s_ctrl->sensor_i2c_client,
        addr,
        data,
        MSM_CAMERA_I2C_BYTE_DATA);

    if ( rc < 0 ){
        CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, data);
    }

    return rc;
}

/****************************************************************************
 * FunctionName: ov8856_otp_write_i2c_table;
 * Description : write otp info via i2c;
 ***************************************************************************/
static int32_t ov8856_otp_write_i2c_table(struct msm_sensor_ctrl_t *s_ctrl, struct msm_camera_i2c_reg_setting *write_setting)
{
    int32_t rc = 0;

    if(NULL == write_setting){
        CMR_LOGE("%s fail,noting to write i2c \n", __func__);
        return -1;
    }
    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table(s_ctrl->sensor_i2c_client,write_setting);

    if ( rc < 0 ){
        CMR_LOGE("%s fail, rc = %d! \n", __func__, rc);
    }

    return rc;
}

/****************************************************************************
 * FunctionName: ov8856_otp_read_i2c;
 * Description : read otp info via i2c;
 ***************************************************************************/
static int32_t ov8856_otp_read_i2c(struct msm_sensor_ctrl_t *s_ctrl,uint32_t addr, uint16_t *data)
{
    int32_t rc = 0;
    if(NULL == data){
        CMR_LOGE("%s fail,NULL == data",__func__);
        return -1;
    }

    rc = s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read(s_ctrl->sensor_i2c_client,
                addr,
                data,
                MSM_CAMERA_I2C_BYTE_DATA);
    if ( rc < 0 ){
        CMR_LOGE("%s fail, rc = %d! addr = 0x%x, data = 0x%x\n", __func__, rc, addr, *data);
    }

    return rc;
}
/********************************************************************************************
* To avoid OTP memory access timing conflict,before doing OTP read/write,register 0x5001[3]
* must be set to 0. After OTP memory access,set register 0x5001[3] back to 1.
********************************************************************************************/
static int32_t ov8856_otp_enable_DPC(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    uint16_t addr = OV8856_DPC_EN_ADDR;
    uint16_t data = 0;

    rc = ov8856_otp_read_i2c(s_ctrl, addr, &data);
    if(rc < 0){
        CMR_LOGE("%s fail change 0x%x value from 0x%x to 0x%x\n", __func__,addr, data,  data | 0x08);
        return -1;
    }

    /* set 0x5001[3] to 1 */
    rc = ov8856_otp_write_i2c(s_ctrl, addr, data | 0x08);
    if(rc < 0){
        CMR_LOGE("%s  addr:0x%x, data:0x%x\n", __func__,addr, data);
    }

    return rc;
}

static int32_t ov8856_otp_disable_DPC(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    uint16_t addr = OV8856_DPC_EN_ADDR;
    uint16_t data = 0;

    rc = ov8856_otp_read_i2c(s_ctrl, addr, &data);
    if( rc < 0){
        CMR_LOGE("%s fail change 0x%x value from 0x%x to 0x%x\n",__func__, addr, data,  data & 0xF7);
        return rc;
    }

    /* set 0x5001[3] to 0 */
    rc = ov8856_otp_write_i2c(s_ctrl, addr, data & 0xF7);
    if(rc < 0){
        CMR_LOGE("%s  addr:0x%x, data:0x%x\n", __func__,addr, data);
    }

    return rc;
}
static int ov8856_otp_read_block(struct msm_sensor_ctrl_t* s_ctrl, uint16_t  reg, uint8_t* buf, uint16_t count)
{
    int i = 0;
    int rc = 0;
    uint16_t val = 0;
    if(NULL == buf){
        CMR_LOGE("%s fail,NULL == buf \n", __func__);
        return -1;
    }
    for (i = 0; i < count; i++){
        rc = ov8856_otp_read_i2c(s_ctrl, reg + i , &val);

        if (rc < 0){
            CMR_LOGE("%s fail to read otp with error code %d,reg_addr=0x%x\n", __func__, rc, reg + i);
            return rc;
        }
        buf[i] = (val & 0xff);
        OTPSUMVAL += buf[i];
        CMR_LOGD("%s:reg_addr=0x%x, reg_val:0x%x, otpsum:0x%x\n",__func__, reg + i, val, OTPSUMVAL);
    }
    return rc;
}
static int ov8856_otp_get_chksum(struct msm_sensor_ctrl_t* s_ctrl,enum_ov8856_groups * group_sel)
{
    uint16_t check_val = 0;
    int rc = 0;
    if(NULL == group_sel){
        CMR_LOGE("%s fail,NULL == group_sel\n",__func__);
        return -1;
    }
    rc = ov8856_otp_read_i2c(s_ctrl, OV8856_CHKSUM_ADDR_GROUP2, &check_val);
    if(rc < 0){
        CMR_LOGE("%s fail read group 2 checksum reg 0x7207\n",__func__);
        return rc;
    }
    
    if(0 == check_val){
        CMR_LOGW("read first otp value\n");
        rc = ov8856_otp_read_i2c(s_ctrl, OV8856_CHKSUM_ADDR_GROUP1, &check_val);
        if(rc < 0){
            CMR_LOGE("%s fail read group 1 checksum reg 0x710B\n",__func__);
            return rc;
        }

        if(0 != check_val){
            *group_sel = GROUP_1;
        }else{
            CMR_LOGE("this ov8856 module burn otp bad.\n");
            return -1;
        }
    }else{
        CMR_LOGW("read second otp value\n");
        *group_sel = GROUP_2;
    }
    ov8856_otp.checksum = check_val;
    return 0;
}

static int ov8856_otp_get_module_id(struct msm_sensor_ctrl_t* s_ctrl,uint16_t  module_addr)
{
    uint8_t buf[OV8856_MODULE_REG_NO] = {0};
    int rc = 0;

    rc = ov8856_otp_read_block(s_ctrl,module_addr , buf, OV8856_MODULE_REG_NO);
    if (rc < 0 ){
        CMR_LOGE("%s, read module id fail\n", __func__);
        return -1;
    }
    CMR_LOGW("%s module info year 20%02d month %d day %d. huawei_id 0x%x,  vendor id&version 0x%x\n", __func__, buf[0], buf[1], buf[2], buf[3], buf[4]);

    if (buf[3] != OV8856_MODULE_HUAWEI_ID){
        CMR_LOGE("%s, huawei_id is err!\n", __func__);
        return -1;
    }
    return 0;
}
static int ov8856_otp_get_awb(struct msm_sensor_ctrl_t* s_ctrl,uint16_t  awb_addr)
{
    uint8_t buf[OV8856_AWB_REG_NO] = {0};
    int rc = 0;

    rc = ov8856_otp_read_block(s_ctrl, awb_addr, buf, OV8856_AWB_REG_NO);
    if ( rc < 0 ){
        CMR_LOGE("%s, read awb fail\n", __func__);
        return -1;
    }
    ov8856_otp.rg_ratio = (buf[0]  << 8) | buf[1];
    ov8856_otp.bg_ratio = (buf[2]  << 8) | buf[3];
    ov8856_otp.gb_gr_ratio =(buf[4] << 8) | buf[5];

    CMR_LOGW("%s OTP data are rg_ratio=0x%x, bg_ratio=0x%x, gb_gr_ratio=0x%x\n", __func__,ov8856_otp.rg_ratio, ov8856_otp.bg_ratio, ov8856_otp.gb_gr_ratio);

    if (0 == ov8856_otp.rg_ratio || 0 == ov8856_otp.bg_ratio || 0 == ov8856_otp.gb_gr_ratio){
        //if awb value read is error for zero, abnormal branch deal
        CMR_LOGE("%s OTP awb is wrong!!!\n", __func__);
        return -1;
    }

    return 0;
}

static int ov8856_otp_get_lsc(struct msm_sensor_ctrl_t* s_ctrl,enum_ov8856_groups  group_tmp)
{
    int i = 0;
    int rc = 0;
    uint16_t addr;

    addr = ov8856_otp_read_addr[group_tmp].start_lsc;
    rc = ov8856_otp_read_block(s_ctrl, addr, ov8856_otp.lsc_param, OV8856_LSC_REG_NO);
    if(rc < 0){
        CMR_LOGE("%s ov8856 read lsc worng!!!\n", __func__);
        return rc;
    }

    for(i = 0;i < OV8856_LSC_REG_NO;i++){
        ov8856_otp_lsc[i].reg_addr = OV8856_LSC_CAL_START_REG + i;//lsc control register start address
        ov8856_otp_lsc[i].reg_data = ov8856_otp.lsc_param[i];
        ov8856_otp_lsc[i].delay = 0x00;
    }
    return rc;
}

static int ov8856_otp_enable_read(struct msm_sensor_ctrl_t *s_ctrl)
{
    unsigned int i = 0;
    int32_t addr = 0;
    uint16_t data = 0;
    int rc = 0;
    struct msm_camera_i2c_reg_array regs[]=
    {
        {0x3D84, 0xC0, 0x00},//enable otp read mode
        {0x3D88, 0x70, 0x00},//set otp start addr
        {0x3D89, 0x10, 0x00},
        {0x3D8A, 0x72, 0x00},//set OTP end addr
        {0x3D8B, 0x07, 0x00},
        {0x3D81, 0x01, 0x00},//load OTP info
    };

    for( i = 0; i < ARRAY_SIZE(regs); i++ ){
        addr = regs[i].reg_addr;
        data = regs[i].reg_data;
        rc = ov8856_otp_write_i2c(s_ctrl,addr, data);
        if(rc < 0){
            CMR_LOGE("%s: failed. reg:0x%x, data:0x%x\n", __func__, addr, data);
            return rc;
        }
    }
    return rc;
}
static int ov8856_otp_init_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
    unsigned int i = 0;
    int32_t addr = 0;
    uint16_t data = 0;
    int rc = 0;
    struct msm_camera_i2c_reg_array ov8856_init_setting[]=
    {
        {0x0103, 0x01, 0x00},//reset
        {0x3D85, 0x17, 0x00},//set otp start addr
    };

    for( i = 0; i < ARRAY_SIZE(ov8856_init_setting); i++ ){
        addr = ov8856_init_setting[i].reg_addr;
        data = ov8856_init_setting[i].reg_data;
        rc = ov8856_otp_write_i2c(s_ctrl,addr, data);
        if(rc < 0){
            CMR_LOGE("%s: failed. reg:0x%x, data:0x%x\n", __func__, addr, data);
            return rc;
        }
    }
    return rc;
}

static int ov8856_clear_otp_buffer(struct msm_sensor_ctrl_t *s_ctrl)
{
    int i = 0;
    int rc =0;

    for(i = 0;i<OV8856_OTP_REG_NO;i++){
        ov8856_otp_bufclear[i].reg_addr = OV8856_OTP_START_ADDR + i;
        ov8856_otp_bufclear[i].reg_data = 0x00;
        ov8856_otp_bufclear[i].delay = 0x00;
    }
    st_ov8856_otp_bufclear.reg_setting =ov8856_otp_bufclear;
    st_ov8856_otp_bufclear.size = ARRAY_SIZE(ov8856_otp_bufclear);
    st_ov8856_otp_bufclear.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
    st_ov8856_otp_bufclear.data_type = MSM_CAMERA_I2C_BYTE_DATA;
    st_ov8856_otp_bufclear.delay = 0;
    rc = ov8856_otp_write_i2c_table(s_ctrl,&st_ov8856_otp_bufclear);

    if(rc < 0){
        CMR_LOGE("%s,%d fail, rc = %d! \n", __func__, __LINE__,rc);
    }
    return rc ;
}

static int ov8856_otp_init_read(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    /*write init setting*/
    rc =ov8856_otp_init_setting(s_ctrl);
    if(rc < 0){
        CMR_LOGE("%s: failed to ov8856 otp init setting.\n", __func__);
        return rc;
    }
    /*stream on before read otp*/
    rc = ov8856_otp_write_i2c(s_ctrl, OV8856_STREAM_ONOFF_ADDR, 0x01);
    if(rc < 0){
        CMR_LOGE("%s: failed to write stream on.\n", __func__);
        return rc;
    }

    /*first clear otp buffer register*/
    rc = ov8856_clear_otp_buffer(s_ctrl);
    if(rc < 0){
        CMR_LOGE("%s: failed to ov8856_clear_otp_buffer\n", __func__);
        return rc;
    }

    /*second disable DPC*/
    rc = ov8856_otp_disable_DPC(s_ctrl);
    if(rc < 0){
        CMR_LOGE("%s: failed to disable DPC.\n", __func__);
        return rc;
    }

    /*Third enable otp read*/
    rc = ov8856_otp_enable_read(s_ctrl);
    if(rc < 0){
        CMR_LOGE("%s: failed to enable otp read.\n", __func__);
        return rc;
    }

    /*Fourth delay 10ms*/
    /*Do be sure delay 10ms after init to read otp*/
    msleep(10);

    return 0;
}
static int ov8856_otp_exit_read(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    /*sixth clear otp buffer register*/
    if(ov8856_clear_otp_buffer(s_ctrl) < 0){
        CMR_LOGE("%s, faild ov8856 clear otp buffer\n",__func__);
        rc = -1;
    }
    //seventh open DPC function.
    if(ov8856_otp_enable_DPC(s_ctrl) < 0){
        CMR_LOGE("%s, enable DPC fail\n",__func__);
        rc = -1;
    }
    //stream off no matter success or fail.
    if( ov8856_otp_write_i2c(s_ctrl, OV8856_STREAM_ONOFF_ADDR, 0x00) < 0 ){
        CMR_LOGE("%s, otp stream off fail\n",__func__);
        rc = -1;
    }
    return rc ;
}

static int ov8856_read_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
    uint16_t tmp_mmi_otp_flag = OV8856_MMI_OTP_FAIL;
    int rc = 0;
    enum_ov8856_groups group = GROUP_1;

    CMR_LOGW("%s enter\n", __func__);
    if (OV8856_OTP_FAIL_FLAG == (ov8856_otp_flag & OV8856_OTP_FAIL_FLAG)){
        CMR_LOGE("%s, OV8856_OTP_FAIL_FLAG\n", __func__);
        return -1;
    } else if (OV8856_OTP_SUCCESS == ov8856_otp_flag){
        CMR_LOGW("%s, OV8856_OTP_COMPLETE\n", __func__);
        return 0;
    }
    //initial global parameters.
    ov8856_otp_flag = 0;
    OTPSUMVAL = 0;
    memset(&ov8856_otp, 0 , sizeof(ov8856_otp));

    /*init before read otp*/
    rc = ov8856_otp_init_read(s_ctrl);
    if(rc < 0){
        CMR_LOGE("%s,faild ov8856_otp_init_read\n", __func__);
        goto GET_OTP_FAIL;
    }

    /*fifth read otp reg*/
    rc = ov8856_otp_get_chksum(s_ctrl,&group);
    if ( (rc < 0)||(group >= GROUP_MAX) ){
        CMR_LOGE("%s,faild ov8856_otp_get_chksum or group %d,group:%d\n", __func__,rc,group);
        goto GET_OTP_FAIL;
    }
    ov8856_otp_flag |= OV8856_OTP_CHECKSUM_READ;
    tmp_mmi_otp_flag &= ~OV8856_MMI_OTP_CHECKSUM_FLAG;

    /*read module id*/
    rc =ov8856_otp_get_module_id(s_ctrl,ov8856_otp_read_addr[group].start_module);
    if ( rc < 0 ){
        CMR_LOGE("%s,faild ov8856_otp_init_read\n", __func__);
        goto GET_OTP_FAIL;
    }
    ov8856_otp_flag |= OV8856_OTP_MODULE_INFO_READ;
    tmp_mmi_otp_flag &= ~OV8856_MMI_OTP_MODULE_INFO_FLAG;

    /*read awb info*/
    rc = ov8856_otp_get_awb(s_ctrl,ov8856_otp_read_addr[group].start_awb);
    if ( rc < 0 ){
        CMR_LOGE("%s,faild ov8856_otp_get_awb\n", __func__);
        goto GET_OTP_FAIL;
    }
    ov8856_otp_flag |= OV8856_OTP_AWB_READ;
    tmp_mmi_otp_flag &= ~OV8856_MMI_OTP_AWB_FLAG;

    /*read lsc info*/
    rc = ov8856_otp_get_lsc(s_ctrl,group);
    if ( rc < 0){
        CMR_LOGE("%s,faild ov8856_otp_get_lsc\n", __func__);
        goto GET_OTP_FAIL;
    }
    ov8856_otp_flag |= OV8856_OTP_LSC_READ;
    tmp_mmi_otp_flag &= ~OV8856_MMI_OTP_LSC_FLAG;

    /*calc checksum*/
    if((uint8_t)(OTPSUMVAL % 255+1) == ov8856_otp.checksum){
        ov8856_otp_flag |= OV8856_OTP_CHKSUM_SUCC;
        tmp_mmi_otp_flag &= ~OV8856_MMI_OTP_SUMVAL_FLAG;
        CMR_LOGW("%s success, OTPSUMVAL: %d, otpCheckSumVal: %d , ov8856_otp_flag=0x%x\n", __func__, OTPSUMVAL,
        ov8856_otp.checksum, ov8856_otp_flag);
        rc = 0;
        goto GET_OTP_SUCCESS;
    } else {
        CMR_LOGE("%s fail, OTPSUMVAL: %d, otpCheckSumVal: %d \n", __func__, OTPSUMVAL,ov8856_otp.checksum);
        rc = -1;
    }

GET_OTP_FAIL:
    ov8856_otp_flag |= OV8856_OTP_FAIL_FLAG;

GET_OTP_SUCCESS:
    //exit ov8856 otp read
    rc = ov8856_otp_exit_read(s_ctrl);
    if( rc < 0 ){
        CMR_LOGE("%s, ov8856_otp_exit_read\n",__func__);
        ov8856_otp_flag |= OV8856_OTP_FAIL_FLAG;
        tmp_mmi_otp_flag = OV8856_MMI_OTP_FAIL;
        rc = -1;
    }

    s_ctrl->hw_otp_check_flag.mmi_otp_check_flag  = tmp_mmi_otp_flag;
    CMR_LOGE("%s exit ov8856_mmi_otp_flag = 0x%x\n",__func__, s_ctrl->hw_otp_check_flag.mmi_otp_check_flag);
    return rc;
}

/****************************************************************************
 * FunctionName: ov8856_update_awb_gain;
 * Description : write R_gain,G_gain,B_gain to otp;
 * 0x400 =1x Gain
 * 0 means write AWB info succeed.
 * -1 means write AWB info failed.
 ***************************************************************************/
static int ov8856_update_awb_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
    uint16_t R_gain=0, G_gain=0, B_gain=0, Base_gain=0;

    if((0 == ov8856_otp.rg_ratio) || (0 == ov8856_otp.bg_ratio))
    {
        CMR_LOGE("%s: rg_ratio=%d bg_ratio=%d fail\n",__func__,ov8856_otp.rg_ratio,ov8856_otp.bg_ratio);
        return -1;
    }
    //calculate G gain
    //extend accuracy 1000 times
    R_gain = (RG_Ratio_Typical*1000)/ov8856_otp.rg_ratio;
    B_gain = (BG_Ratio_Typical*1000)/ov8856_otp.bg_ratio;
    G_gain = 1000;
    if(R_gain < 1000 || ( B_gain < 1000 ) )
    {
        if(R_gain < B_gain )
            Base_gain = R_gain;
        else
            Base_gain = B_gain;
    }else{
        Base_gain = G_gain;
    }

    if( 0 == Base_gain){
        CMR_LOGE("%s: RG_Ratio_Typical=%d BG_Ratio_Typical=%d fail\n",__func__,RG_Ratio_Typical,BG_Ratio_Typical);
        return -1;
    }
    R_gain = OV8856_GAIN_1X_REG*R_gain/Base_gain;
    B_gain = OV8856_GAIN_1X_REG*B_gain/Base_gain;
    G_gain = OV8856_GAIN_1X_REG*G_gain/Base_gain;

    if (R_gain>OV8856_GAIN_1X_REG) {
        ov8856_otp_write_i2c(s_ctrl,OV8856_AWB_CAL_RGAIN_H, R_gain>>8);//write R_gain high 8bit
        ov8856_otp_write_i2c(s_ctrl,OV8856_AWB_CAL_RGAIN_L, R_gain & 0x00ff);//write R_gain low 8bit
    }

    if (G_gain>OV8856_GAIN_1X_REG) {
        ov8856_otp_write_i2c(s_ctrl,OV8856_AWB_CAL_GGAIN_H, G_gain>>8);//write G_gain high 8bit
        ov8856_otp_write_i2c(s_ctrl,OV8856_AWB_CAL_GGAIN_L, G_gain & 0x00ff);//write G_gain low 8bit
    }

    if (B_gain>OV8856_GAIN_1X_REG) {
        ov8856_otp_write_i2c(s_ctrl,OV8856_AWB_CAL_BGAIN_H, B_gain>>8);//write B_gain high 8bit
        ov8856_otp_write_i2c(s_ctrl,OV8856_AWB_CAL_BGAIN_L, B_gain & 0x00ff);//write B_gain low 8bit
    }

    CMR_LOGW("%s: R_gain=0x%x, G_gain=0x%x, B_gain=0x%x \n",__func__,R_gain,G_gain,B_gain);
    return 0;
}

/****************************************************************************
 * FunctionName: ov8856_update_lenc_otp;
 * Description : write 240 lens shading otp data to sensor;
 ***************************************************************************/
static int ov8856_update_lenc_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
    int rc = 0;
    uint16_t temp = 0;

    rc = ov8856_otp_read_i2c(s_ctrl,OV8856_LENC_EN_ADDR, &temp);
    if(rc < 0){
        CMR_LOGE("%s: failed read ov8856 lenc_en register.\n", __func__);
        return rc;
    }
    temp = 0x20 | temp;
    rc = ov8856_otp_write_i2c(s_ctrl, OV8856_LENC_EN_ADDR, temp);
    if(rc < 0){
        CMR_LOGE("%s: failed.write ov8856 lsc_en register\n", __func__);
        return rc;
    }
    /* lens correction vscale setting register write 0x3d,cause horizontal band*/
    rc = ov8856_otp_write_i2c(s_ctrl, OV8856_LENC_VSCALE_ADDR, 0x3d);
    if(rc < 0){
        CMR_LOGE("%s: failed.write ov8856 lenc_vscale register\n", __func__);
        return rc;
    }
    st_ov8856_otp_lsc.reg_setting =ov8856_otp_lsc;
    st_ov8856_otp_lsc.size = ARRAY_SIZE(ov8856_otp_lsc);
    st_ov8856_otp_lsc.addr_type = MSM_CAMERA_I2C_WORD_ADDR;
    st_ov8856_otp_lsc.data_type = MSM_CAMERA_I2C_BYTE_DATA;
    st_ov8856_otp_lsc.delay = 0;
    rc = ov8856_otp_write_i2c_table(s_ctrl,&st_ov8856_otp_lsc);
    if(rc < 0){
        CMR_LOGE("%s:%d failed write lsc.\n", __func__, __LINE__);
        return rc;
    }
    CMR_LOGW("set OTP LSC to sensor OK\n");
    return rc;
}
/****************************************************************************
 * FunctionName: ov8856_set_otp_info;
 * Description : set otp data to sensor;
 * call this function after ov8856 initialization
 ***************************************************************************/
int ov8856_otp_func(struct msm_sensor_ctrl_t *s_ctrl,int index)
{
    int rc = 0;
    /*Validate the input params.*/
    if((NULL == s_ctrl)||
       (NULL == s_ctrl->sensor_i2c_client)||
       (NULL == s_ctrl->sensor_i2c_client->i2c_func_tbl)||
       (NULL == s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write)||
       (NULL == s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_write_table)||
       (NULL == s_ctrl->sensor_i2c_client->i2c_func_tbl->i2c_read)){
        CMR_LOGE("%s:%d, NULL pointer.\n", __func__, __LINE__);
        return -1;
    }
    /*If the typical value is defined in lists, get it. Or use the initial value*/
    if(otp_function_lists[index].rg_ratio_typical){
        RG_Ratio_Typical = otp_function_lists[index].rg_ratio_typical;
    }

    if(otp_function_lists[index].bg_ratio_typical){
        BG_Ratio_Typical = otp_function_lists[index].bg_ratio_typical;
    }
    CMR_LOGW("%s, rg_ratio_typical=0x%04x,bg_ratio_typical=0x%04x\n", __func__,RG_Ratio_Typical,BG_Ratio_Typical );

    //Get otp info on the first time
    rc = ov8856_read_otp(s_ctrl);

    if ( rc < 0 ){
        CMR_LOGE("%s:%d otp read failed.\n", __func__, __LINE__);
        return -1;
    }
    /*Do nothing if the otp read failed.*/
    CMR_LOGW("Set ov8856 OTP info Enter\n");
    if (OV8856_OTP_FAIL_FLAG == (ov8856_otp_flag & OV8856_OTP_FAIL_FLAG)){
        CMR_LOGE("%s invalid otp info!\n", __func__);
        return -1;
    }
    /*Update the awb otp calibration*/
    ov8856_update_awb_otp(s_ctrl);
    /*Update the lsc otp calibration*/
    ov8856_update_lenc_otp(s_ctrl);

    CMR_LOGW("%s exit\n", __func__);
    return rc;
}
