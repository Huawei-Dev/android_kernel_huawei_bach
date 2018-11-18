#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>

#include <linux/i2c.h>//iic
#include <linux/delay.h>//msleep
#include "test_lib.h"
#include "Global.h"
#include "focaltech_core.h"

//#define MAX_TEST_DATA_SIZE		(1024*8)
//20160621
#define MAX_TEST_DATA_SIZE		(1024*20)
#define MAX_BUF_LEN 128
#define INI_SIZE (1024*8-5)
extern struct i2c_client *fts_i2c_client;
#define FTS_INI_FILEPATH "/mnt/sdcard/"  

//获取配置文件大小, 用于分配内存读取配置
/*
static int ft8736_GetInISize(char *config_name)
{
	struct file *pfile = NULL;
	struct inode *inode = NULL;
	//unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));

	sprintf(filepath, "%s%s", FTS_INI_FILEPATH, config_name);
	printk("[FTS] %s. file path:%s.\n", __func__, filepath);
	
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);

	if (IS_ERR(pfile)) {
		pr_err("[FTS] error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	//magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	filp_close(pfile, NULL);

	return fsize;
}
*/
//读取配置到内存
/*
static int ft8736_ReadInIData(char *config_name, char *config_buf)
{
	struct file *pfile = NULL;
	struct inode *inode = NULL;
	//unsigned long magic;
	off_t fsize = 0;
	char filepath[128];
	loff_t pos = 0;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "%s%s", FTS_INI_FILEPATH, config_name);
	if (NULL == pfile)
		pfile = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(pfile)) {
		pr_err("[FTS] error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	inode = pfile->f_dentry->d_inode;
	//magic = inode->i_sb->s_magic;
	fsize = inode->i_size;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_read(pfile, config_buf, fsize, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}
*/

int gnFtsTestResult;

char result_reason_flag = 1;

#if 0
static int fts_SaveTestData(char * file_name, char * data_buf, int iLen)
{
	struct file *pfile = NULL;
	
	char filepath[MAX_BUF_LEN];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	snprintf(filepath, MAX_BUF_LEN, "%s%s", FTS_INI_FILEPATH, file_name);
	printk("[FTS] %s. file path:%s.\n", __func__, filepath);
		
	if (NULL == pfile)
		pfile = filp_open(filepath, O_CREAT|O_RDWR, 0);
	if (IS_ERR(pfile)) {
		pr_err("[FTS] error occured while opening file %s.\n", filepath);
		return -EIO;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
	vfs_write(pfile, data_buf, iLen, &pos);
	filp_close(pfile, NULL);
	set_fs(old_fs);

	return 0;
}
#endif
//读取,解析配置文件,初始化测试变量
/*
static int ft8736_get_testparam_from_ini(char *config_name)
{
	char *filedata = NULL;

	int inisize = ft8736_GetInISize(config_name);

	pr_info("[FTS]  inisize = %d \n ", inisize);
	if (inisize <= 0) {
		pr_err("[FTS] %s ERROR:Get firmware size failed\n", __func__);
		return -EIO;
	}

	filedata = kmalloc(inisize + 1, GFP_ATOMIC);
		
	if (ft8736_ReadInIData(config_name, filedata)) {
		pr_err("[FTS] %s() - ERROR: request_firmware failed\n", __func__);
		kfree(filedata);
		return -EIO;
	} else {
		pr_info("[FTS] ft8736_ReadInIData successful\n");
	}

	set_param_data(filedata);
	return 0;
}
*/
static void fts_add_str_carrige(char* pcdest, char* pcadd)
{
	strncat(pcdest, pcadd, INI_SIZE);
	strncat(pcdest, "\r\n", 2);
}

#if 0
// frank add.
// get test param direct. Not  from ini file.
static int ft8736_get_testparam_direct(void)
{
	char *filedata = NULL;

	int inisize = INI_SIZE;
	pr_info("[FTS]  inisize = %d \n ", inisize);
	filedata = vmalloc(inisize + 1);
	if(NULL == filedata)
	{
		printk("[FTS] vmalloc failed in function:%s.  ERROR 	!!!\n", __func__);
		return -1;
	}

	// input test param
	strncpy(filedata, "[Valid_File]\r\n", INI_SIZE);

	fts_add_str_carrige(filedata, "OnlyMultipleTest=1");
	
	fts_add_str_carrige(filedata, "[Interface]");
	fts_add_str_carrige(filedata, "IC_Type=FT8736");
	fts_add_str_carrige(filedata, "Normalize_Type=0");
	fts_add_str_carrige(filedata, "Interface_Type=0");
	fts_add_str_carrige(filedata, "Slave_Addr=0x70");
	fts_add_str_carrige(filedata, "Freq_Index=2");
	fts_add_str_carrige(filedata, "Phase_Pola=-1");
	fts_add_str_carrige(filedata, "Max_Points=5");
	fts_add_str_carrige(filedata, "iRotationDegree=0");
	fts_add_str_carrige(filedata, "isReversed=0");
	fts_add_str_carrige(filedata, "ixMaxPixel=480");
	fts_add_str_carrige(filedata, "iyMaxPixel=800");

	//
	fts_add_str_carrige(filedata, "[Config]");
	fts_add_str_carrige(filedata, "Run_Mode=0");
	fts_add_str_carrige(filedata, "Read_Bytes=256");
	fts_add_str_carrige(filedata, "Write_Bytes=128");
	fts_add_str_carrige(filedata, "Test_Way=0");
	fts_add_str_carrige(filedata, "Handle_SN=0");
	fts_add_str_carrige(filedata, "SN_Length=18");
	fts_add_str_carrige(filedata, "SN_AutoTest=1");
	fts_add_str_carrige(filedata, "SKey_Index=0");
	fts_add_str_carrige(filedata, "SKeyValue=13");
	fts_add_str_carrige(filedata, "TP_AutoTest=0");
	fts_add_str_carrige(filedata, "TP_AutoTest_Time=0");
	fts_add_str_carrige(filedata, "TP_ReplaceTP=0");
	fts_add_str_carrige(filedata, "TP_ReplaceTP_Time=0");
	fts_add_str_carrige(filedata, "TP_Always_Replace=0");
	fts_add_str_carrige(filedata, "TP_Always_Replace_Time=0");
	fts_add_str_carrige(filedata, "Store_Result=1");
	fts_add_str_carrige(filedata, "Auto_Switch=0");
	fts_add_str_carrige(filedata, "Continue_Test_After_Fail=0");
	fts_add_str_carrige(filedata, "CB_Test_Mode=1");
	fts_add_str_carrige(filedata, "Tip_After_TestNG=0");
	fts_add_str_carrige(filedata, "Show_Res=0");
	fts_add_str_carrige(filedata, "Result_Type=0");
	fts_add_str_carrige(filedata, "Result_Format=0");
	fts_add_str_carrige(filedata, "Result_Path_Enable=0");
	fts_add_str_carrige(filedata, "Result_OP_Num=0");
	fts_add_str_carrige(filedata, "Result_Use_IcId=0");
	fts_add_str_carrige(filedata, "Result_ReplaceOldLog=0");
	fts_add_str_carrige(filedata, "Result_SaveTestMsg=1");
	fts_add_str_carrige(filedata, "Result_SimpleDirectory=0");
	fts_add_str_carrige(filedata, "Output_LevelSignal=0");
	fts_add_str_carrige(filedata, "Output_NgSignal=0");
	fts_add_str_carrige(filedata, "Input_LevelSignal=0");
	fts_add_str_carrige(filedata, "Reverse_Time=0");
	fts_add_str_carrige(filedata, "Switch_Protocol=0");
	fts_add_str_carrige(filedata, "CLB_Other_Return=0");
	fts_add_str_carrige(filedata, "Count_Result=0");
	fts_add_str_carrige(filedata, "Count_Result_Type=0");
	fts_add_str_carrige(filedata, "Full_Screen=0");

	//
	fts_add_str_carrige(filedata, "[Android_Terminal]");
	fts_add_str_carrige(filedata, "I2C_Interface=1");
	fts_add_str_carrige(filedata, "I2C_Index=0");
	fts_add_str_carrige(filedata, "RW_Byte=0");
	fts_add_str_carrige(filedata, "CustomConfPath=0");
	fts_add_str_carrige(filedata, "AutoSave=1");
	fts_add_str_carrige(filedata, "ResultPath=");

	//
	fts_add_str_carrige(filedata, "[TestItem]");
	fts_add_str_carrige(filedata, "FW_VERSION_TEST=0");
	fts_add_str_carrige(filedata, "FACTORY_ID_TEST=0");
	fts_add_str_carrige(filedata, "IC_VERSION_TEST=0");
	fts_add_str_carrige(filedata, "PROJECT_CODE_TEST=0");
	fts_add_str_carrige(filedata, "RAWDATA_TEST=0");
	fts_add_str_carrige(filedata, "CB_TEST=1");
	fts_add_str_carrige(filedata, "SHORT_CIRCUIT_TEST=0");
	fts_add_str_carrige(filedata, "CHANNEL_NUM_TEST=0");
	fts_add_str_carrige(filedata, "INT_PIN_TEST=0");
	fts_add_str_carrige(filedata, "RESET_PIN_TEST=0");
	fts_add_str_carrige(filedata, "NOISE_TEST=0");
	fts_add_str_carrige(filedata, "OPEN_TEST=0");
	fts_add_str_carrige(filedata, "CB_UNIFORMITY_TEST=0");
	fts_add_str_carrige(filedata, "DIFFER_UNIFORMITY_TEST=0");
	fts_add_str_carrige(filedata, "DIFFER2_UNIFORMITY_TEST=0");
	fts_add_str_carrige(filedata, "LCD_NOISE_TEST=0");
	fts_add_str_carrige(filedata, "VIRTUAL_BUTTON_TEST=0");
	fts_add_str_carrige(filedata, "ONELINE_TEST=0");
	fts_add_str_carrige(filedata, "DIAGONAL_TEST=0");
	fts_add_str_carrige(filedata, "FREEPAINT_TEST=0");
	fts_add_str_carrige(filedata, "SPECIAL_BUTTON_TEST=0");
	fts_add_str_carrige(filedata, "LINEARITY_TEST=0");
	fts_add_str_carrige(filedata, "CIRCLE_TEST=0");
	fts_add_str_carrige(filedata, "SQUARE_TEST=0");
	fts_add_str_carrige(filedata, "KEY_TEST=0");

	//	[Basic_Threshold]
	fts_add_str_carrige(filedata, "[Basic_Threshold]");
	fts_add_str_carrige(filedata, "FW_VER_VALUE=255");
	fts_add_str_carrige(filedata, "Factory_ID_Number=255");
	fts_add_str_carrige(filedata, "IC_Version=0");
	fts_add_str_carrige(filedata, "Project_Code=");
	fts_add_str_carrige(filedata, "RawDataTest_Min=5000");
	fts_add_str_carrige(filedata, "RawDataTest_Max=11000");
	fts_add_str_carrige(filedata, "CBTest_VA_Check=1");
	fts_add_str_carrige(filedata, "CBTest_Min=3");
	fts_add_str_carrige(filedata, "CBTest_Max=100");
	fts_add_str_carrige(filedata, "CBTest_VKey_Check=0");
	fts_add_str_carrige(filedata, "CBTest_Min_Vkey=3");
	fts_add_str_carrige(filedata, "CBTest_Max_Vkey=100");
	fts_add_str_carrige(filedata, "Distance_Diagonal=50");
	fts_add_str_carrige(filedata, "Type_Diagonal=2");
	fts_add_str_carrige(filedata, "MaxNG_Diagonal=0");
	fts_add_str_carrige(filedata, "LimitTime_Diagonal=0");
	fts_add_str_carrige(filedata, "LinearityCheck_Diagonal=1");
	fts_add_str_carrige(filedata, "SET_TOUCH_THRESHOLD_INCELL=0");
	fts_add_str_carrige(filedata, "Key_Div_Number_Incell=2");
	fts_add_str_carrige(filedata, "Preserved_key_threshold_Incell=800");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_MAX_NG=0");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_LIMITE_TIME=0");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_BOARDER=360");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_EDGE=20");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_TRACK=10");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_CENTER1=270");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_CENTER2=120");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_LINEARITY=35");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_SPLITS=10");
	fts_add_str_carrige(filedata, "SET_TOUCH_THRESHOLD=0");
	fts_add_str_carrige(filedata, "Key_Div_Number=1");
	fts_add_str_carrige(filedata, "Preserved_key_threshold=800");
	fts_add_str_carrige(filedata, "1_key_threshold=800");
	fts_add_str_carrige(filedata, "2_key_threshold=800");
	fts_add_str_carrige(filedata, "3_key_threshold=800");
	fts_add_str_carrige(filedata, "4_key_threshold=800");
	fts_add_str_carrige(filedata, "Key_Threshold=800");
	fts_add_str_carrige(filedata, "KEY_TEST_MAX_NG=0");
	fts_add_str_carrige(filedata, "KEY_TEST_LIMITE_TIME=0");
	fts_add_str_carrige(filedata, "KEY_TEST_KEY_NUM=21");
	fts_add_str_carrige(filedata, "[SiuParam]");
	fts_add_str_carrige(filedata, "Check_Siu_Version=0");
	fts_add_str_carrige(filedata, "Siu_MainVersion=0");
	fts_add_str_carrige(filedata, "Siu_SubVersion=0");
	fts_add_str_carrige(filedata, "Check_Set_IICVol=0");
	fts_add_str_carrige(filedata, "IIC_Vol_Type=0");
	fts_add_str_carrige(filedata, "IIC_Vdd_Type=0");
	fts_add_str_carrige(filedata, "Check_Iovcc=0");
	fts_add_str_carrige(filedata, "Iovcc_Vol_Type=0");
	fts_add_str_carrige(filedata, "Iovcc_Current_Type=0");
	fts_add_str_carrige(filedata, "Iovcc_Min_Hole=0");
	fts_add_str_carrige(filedata, "Iovcc_Max_Hole=50");
	fts_add_str_carrige(filedata, "Check_Vdd=0");
	fts_add_str_carrige(filedata, "Vdd_Vol_Type=0");
	fts_add_str_carrige(filedata, "Check_Normal=0");
	fts_add_str_carrige(filedata, "Check_Sleep=0");
	fts_add_str_carrige(filedata, "Vdd_Normal_Min=0");
	fts_add_str_carrige(filedata, "Vdd_Normal_Max=500");
	fts_add_str_carrige(filedata, "Vdd_Sleep_Min=0");
	fts_add_str_carrige(filedata, "Vdd_Sleep_Max=150");

	set_param_data(filedata);
	return 0;
}
#endif

//frank add. 20160620
//get test param direct. Not  from ini file.
static int ft8716_get_testparam_direct(void)
{
	char *filedata = NULL;

	int inisize = INI_SIZE;
	pr_info("[FTS]  inisize = %d \n ", inisize);
	filedata = vmalloc(inisize + 1);
	if(NULL == filedata)
	{
		printk("[FTS] vmalloc failed in function:%s.  ERROR 	!!!\n", __func__);
		return -1;
	}

	// input test param
	strncpy(filedata, "[Valid_File]\r\n", INI_SIZE);
	fts_add_str_carrige(filedata, "OnlyMultipleTest=1");
	fts_add_str_carrige(filedata, "[Interface]");
	fts_add_str_carrige(filedata, "IC_Type=FT8716");
	fts_add_str_carrige(filedata, "Normalize_Type=0");
	fts_add_str_carrige(filedata, "Interface_Type=0");
	fts_add_str_carrige(filedata, "Slave_Addr=0x70");
	fts_add_str_carrige(filedata, "Freq_Index=2");
	fts_add_str_carrige(filedata, "Phase_Pola=-1");
	fts_add_str_carrige(filedata, "Max_Points=5");
	fts_add_str_carrige(filedata, "iRotationDegree=0");
	fts_add_str_carrige(filedata, "isReversed=0");
	fts_add_str_carrige(filedata, "ixMaxPixel=1080");
	fts_add_str_carrige(filedata, "iyMaxPixel=1920");
	fts_add_str_carrige(filedata, "[Config]");
	fts_add_str_carrige(filedata, "Run_Mode=0");
	fts_add_str_carrige(filedata, "Test_Way=0");
	fts_add_str_carrige(filedata, "Handle_SN=0");
	fts_add_str_carrige(filedata, "SN_Length=4");
	fts_add_str_carrige(filedata, "SN_AutoTest=1");
	fts_add_str_carrige(filedata, "SKey_Index=0");
	fts_add_str_carrige(filedata, "SKeyValue=13");
	fts_add_str_carrige(filedata, "TP_AutoTest=0");
	fts_add_str_carrige(filedata, "TP_AutoTest_Time=0");
	fts_add_str_carrige(filedata, "TP_ReplaceTP=0");
	fts_add_str_carrige(filedata, "TP_ReplaceTP_Time=0");
	fts_add_str_carrige(filedata, "TP_Always_Replace=0");
	fts_add_str_carrige(filedata, "TP_Always_Replace_Time=0");
	fts_add_str_carrige(filedata, "Store_Result=1");
	fts_add_str_carrige(filedata, "Auto_Switch=0");
	fts_add_str_carrige(filedata, "Continue_Test_After_Fail=0");
	fts_add_str_carrige(filedata, "CB_Test_Mode=0");
	fts_add_str_carrige(filedata, "Output_LevelSignal=0");
	fts_add_str_carrige(filedata, "Output_NgSignal=0");
	fts_add_str_carrige(filedata, "Input_LevelSignal=0");
	fts_add_str_carrige(filedata, "Reverse_Time=0");
	fts_add_str_carrige(filedata, "Switch_Protocol=0");
	fts_add_str_carrige(filedata, "CLB_Other_Return=0");
	fts_add_str_carrige(filedata, "Count_Result=1");
	fts_add_str_carrige(filedata, "Count_Result_Type=0");
	fts_add_str_carrige(filedata, "Full_Screen=0");
	fts_add_str_carrige(filedata, "Result_Type=0");
	fts_add_str_carrige(filedata, "Result_Format=0");
	fts_add_str_carrige(filedata, "Result_Path_Enable=0");
	fts_add_str_carrige(filedata, "Result_OP_Num=0");
	fts_add_str_carrige(filedata, "Result_Use_IcId=1");
	fts_add_str_carrige(filedata, "Result_ReplaceOldLog=0");
	fts_add_str_carrige(filedata, "Result_SaveTestMsg=1");
	fts_add_str_carrige(filedata, "Result_SimpleDirectory=0");
	fts_add_str_carrige(filedata, "Tip_After_TestNG=0");
	fts_add_str_carrige(filedata, "Show_Res=1");
	fts_add_str_carrige(filedata, "Read_Bytes=256");
	fts_add_str_carrige(filedata, "Write_Bytes=128");
	fts_add_str_carrige(filedata, "Check_Mode=0");
	fts_add_str_carrige(filedata, "Non_Common_GND=0");
	fts_add_str_carrige(filedata, "[Android_Terminal]");
	fts_add_str_carrige(filedata, "I2C_Interface=1");
	fts_add_str_carrige(filedata, "I2C_Index=0");
	fts_add_str_carrige(filedata, "RW_Byte=0");
	fts_add_str_carrige(filedata, "RawdataMode=0");
	fts_add_str_carrige(filedata, "CustomConfPath=0");
	fts_add_str_carrige(filedata, "AutoSave=0");
	fts_add_str_carrige(filedata, "ResultPath=");
	fts_add_str_carrige(filedata, "[TestItem]");
	fts_add_str_carrige(filedata, "FW_VERSION_TEST=0");
	fts_add_str_carrige(filedata, "FACTORY_ID_TEST=1");
	fts_add_str_carrige(filedata, "IC_VERSION_TEST=0");
	fts_add_str_carrige(filedata, "PROJECT_CODE_TEST=0");
	fts_add_str_carrige(filedata, "RAWDATA_TEST=1");
	fts_add_str_carrige(filedata, "CB_TEST=1");
	fts_add_str_carrige(filedata, "TX2TX_TEST=1");
	fts_add_str_carrige(filedata, "RX2RX_TEST=1");
	fts_add_str_carrige(filedata, "SHORT_CIRCUIT_TEST=1");
	fts_add_str_carrige(filedata, "CHANNEL_NUM_TEST=1");
	fts_add_str_carrige(filedata, "INT_PIN_TEST=0");
	fts_add_str_carrige(filedata, "RESET_PIN_TEST=0");
	fts_add_str_carrige(filedata, "NOISE_TEST=1");
	fts_add_str_carrige(filedata, "VIRTUAL_BUTTON_TEST=0");
	fts_add_str_carrige(filedata, "DIAGONAL_TEST=0");
	fts_add_str_carrige(filedata, "ONELINE_TEST=0");
	fts_add_str_carrige(filedata, "FREEPAINT_TEST=0");
	fts_add_str_carrige(filedata, "SPECIAL_BUTTON_TEST=0");
	fts_add_str_carrige(filedata, "LINEARITY_TEST=0");
	fts_add_str_carrige(filedata, "OPEN_TEST=1");
	fts_add_str_carrige(filedata, "CB_UNIFORMITY_TEST=1");
	fts_add_str_carrige(filedata, "DIFFER_UNIFORMITY_TEST=0");
	fts_add_str_carrige(filedata, "CIRCLE_TEST=0");
	fts_add_str_carrige(filedata, "SQUARE_TEST=0");
	fts_add_str_carrige(filedata, "DIFFER2_UNIFORMITY_TEST=0");
	fts_add_str_carrige(filedata, "KEY_TEST=0");
	fts_add_str_carrige(filedata, "LCD_NOISE_TEST=0");
	fts_add_str_carrige(filedata, "GPIO_TEST=0");
	fts_add_str_carrige(filedata, "ICTYPE_TEST=0");
	fts_add_str_carrige(filedata, "HOME_KEY_TEST=0");
	fts_add_str_carrige(filedata, "PRESS_CHANNEL_TEST=0");
	fts_add_str_carrige(filedata, "RESETDETECTION_TEST=0");
	fts_add_str_carrige(filedata, "[Basic_Threshold]");
	fts_add_str_carrige(filedata, "FW_VER_VALUE=4");
	fts_add_str_carrige(filedata, "Factory_ID_Number=141");
	fts_add_str_carrige(filedata, "IC_Version=0");
	fts_add_str_carrige(filedata, "Project_Code=");
	fts_add_str_carrige(filedata, "RawDataTest_Min=3000");
	fts_add_str_carrige(filedata, "RawDataTest_Max=13000");
	fts_add_str_carrige(filedata, "CBTest_Min=1");
	fts_add_str_carrige(filedata, "CBTest_Max=110");
	fts_add_str_carrige(filedata, "ShortCircuit_CBMax=120");
	fts_add_str_carrige(filedata, "ShortCircuit_K2Value=150");
	fts_add_str_carrige(filedata, "ChannelNumTest_ChannelX=18");
	fts_add_str_carrige(filedata, "ChannelNumTest_ChannelY=30");
	fts_add_str_carrige(filedata, "ChannelNumTest_KeyNum=0");
	fts_add_str_carrige(filedata, "IntPinTest_RegAddr=8");
	fts_add_str_carrige(filedata, "ResetPinTest_RegAddr=136");
	fts_add_str_carrige(filedata, "Distance_Diagonal=50");
	fts_add_str_carrige(filedata, "Type_Diagonal=2");
	fts_add_str_carrige(filedata, "MaxNG_Diagonal=0");
	fts_add_str_carrige(filedata, "LimitTime_Diagonal=0");
	fts_add_str_carrige(filedata, "LinearityCheck_Diagonal=1");
	fts_add_str_carrige(filedata, "SET_TOUCH_THRESHOLD_INCELL=0");
	fts_add_str_carrige(filedata, "Key_Div_Number_Incell=2");
	fts_add_str_carrige(filedata, "Preserved_key_threshold_Incell=-1");
	fts_add_str_carrige(filedata, "SET_TOUCH_THRESHOLD=0");
	fts_add_str_carrige(filedata, "Key_Div_Number=1");
	fts_add_str_carrige(filedata, "Preserved_key_threshold=800");
	fts_add_str_carrige(filedata, "1_key_threshold=800");
	fts_add_str_carrige(filedata, "2_key_threshold=800");
	fts_add_str_carrige(filedata, "3_key_threshold=800");
	fts_add_str_carrige(filedata, "4_key_threshold=800");
	fts_add_str_carrige(filedata, "ShortCircuit_ResMin=200");
	fts_add_str_carrige(filedata, "NoiseTest_Coefficient=50");
	fts_add_str_carrige(filedata, "NoiseTest_Frames=32");
	fts_add_str_carrige(filedata, "NoiseTest_Time=1");
	fts_add_str_carrige(filedata, "NoiseTest_SampeMode=0");
	fts_add_str_carrige(filedata, "NoiseTest_NoiseMode=0");
	fts_add_str_carrige(filedata, "NoiseTest_ShowTip=0");
	fts_add_str_carrige(filedata, "OpenTest_CBMin=40");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_MAX_NG=0");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_LIMITE_TIME=0");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_BOARDER=360");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_EDGE=20");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_TRACK=10");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_CENTER1=270");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_CENTER2=120");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_LINEARITY=35");
	fts_add_str_carrige(filedata, "CIRCLE_TEST_SPLITS=10");
	fts_add_str_carrige(filedata, "CBUniformityTest_CHX_Hole=62");
	fts_add_str_carrige(filedata, "CBUniformityTest_CHY_Hole=62");
	fts_add_str_carrige(filedata, "CBUniformityTest_MinMax_Hole=30");
	fts_add_str_carrige(filedata, "CBUniformityTest_Check_CHX=1");
	fts_add_str_carrige(filedata, "CBUniformityTest_Check_CHY=1");
	fts_add_str_carrige(filedata, "CBUniformityTest_Check_MinMax=1");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_CHX_Hole=10");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_CHY_Hole=10");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_Check_CHX=1");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_Check_CHY=1");
	fts_add_str_carrige(filedata, "Key_Threshold=800");
	fts_add_str_carrige(filedata, "KEY_TEST_MAX_NG=0");
	fts_add_str_carrige(filedata, "KEY_TEST_LIMITE_TIME=0");
	fts_add_str_carrige(filedata, "KEY_TEST_KEY_NUM=21");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_Differ_Min=2700");
	fts_add_str_carrige(filedata, "CBTest_VA_Check=1");
	fts_add_str_carrige(filedata, "CBTest_VKey_Check=0");
	fts_add_str_carrige(filedata, "CBTest_Min_Vkey=1");
	fts_add_str_carrige(filedata, "CBTest_Max_Vkey=110");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_Differ_Max=9000");
	fts_add_str_carrige(filedata, "LCDNoiseTest_FrameNum=400");
	fts_add_str_carrige(filedata, "LCDNoiseTest_Coefficient=50");
	fts_add_str_carrige(filedata, "LCDNoiseTest_NoiseMode=0");
	fts_add_str_carrige(filedata, "CBTest_VKey_Double_Check=0");
	fts_add_str_carrige(filedata, "CBTest_Min_DCheck_Vkey=140");
	fts_add_str_carrige(filedata, "CBTest_Max_DCheck_Vkey=180");
	fts_add_str_carrige(filedata, "Lcd_Busy_Adjust=0");
	fts_add_str_carrige(filedata, "OpenTest_Check_K1=0");
	fts_add_str_carrige(filedata, "OpenTest_K1Threshold=30");
	fts_add_str_carrige(filedata, "OpenTest_Check_K2=0");
	fts_add_str_carrige(filedata, "OpenTest_K2Threshold=5");
	fts_add_str_carrige(filedata, "LCDNoiseTest_Coefficient_Key=60");
	fts_add_str_carrige(filedata, "LimitTime_HomeKey=10");
	fts_add_str_carrige(filedata, "HomeKey_LeftChannel=0");
	fts_add_str_carrige(filedata, "HomeKey_RightChannel=0");
	fts_add_str_carrige(filedata, "HomeKey_TopChannel=0");
	fts_add_str_carrige(filedata, "HomeKey_BottonChannel=0");
	fts_add_str_carrige(filedata, "HomeKey_Hole=0");
	fts_add_str_carrige(filedata, "SET_TOUCH_THRESHOLD_INCELL2=0");
	fts_add_str_carrige(filedata, "Key_Div_Number_Incell2=2");
	fts_add_str_carrige(filedata, "Preserved_key_threshold_Incell2=30");
	fts_add_str_carrige(filedata, "PressChannelTest_Min=5000");
	fts_add_str_carrige(filedata, "Press_Test_LimitedTime=0");
	fts_add_str_carrige(filedata, "RawDataTest_VA_Check=1");
	fts_add_str_carrige(filedata, "RawDataTest_VKey_Check=0");
	fts_add_str_carrige(filedata, "RawDataTest_Min_VKey=3000");
	fts_add_str_carrige(filedata, "RawDataTest_Max_VKey=13000");
	fts_add_str_carrige(filedata, "[SiuParam]");
	fts_add_str_carrige(filedata, "Check_Siu_Version=0");
	fts_add_str_carrige(filedata, "Siu_MainVersion=3");
	fts_add_str_carrige(filedata, "Siu_SubVersion=3");
	fts_add_str_carrige(filedata, "Check_Set_IICVol=0");
	fts_add_str_carrige(filedata, "IIC_Vol_Type=0");
	fts_add_str_carrige(filedata, "IIC_Vdd_Type=0");
	fts_add_str_carrige(filedata, "Check_Iovcc=0");
	fts_add_str_carrige(filedata, "Iovcc_Vol_Type=0");
	fts_add_str_carrige(filedata, "Iovcc_Current_Type=0");
	fts_add_str_carrige(filedata, "Iovcc_Min_Hole=0");
	fts_add_str_carrige(filedata, "Iovcc_Max_Hole=50");
	fts_add_str_carrige(filedata, "Check_Vdd=0");
	fts_add_str_carrige(filedata, "Vdd_Vol_Type=0");
	fts_add_str_carrige(filedata, "Check_Normal=0");
	fts_add_str_carrige(filedata, "Check_Sleep=0");
	fts_add_str_carrige(filedata, "Vdd_Normal_Min=0");
	fts_add_str_carrige(filedata, "Vdd_Normal_Max=500");
	fts_add_str_carrige(filedata, "Vdd_Sleep_Min=0");
	fts_add_str_carrige(filedata, "Vdd_Sleep_Max=150");
	fts_add_str_carrige(filedata, "[INVALID_NODE]");
	fts_add_str_carrige(filedata, "[OnLine_Setting]");
	fts_add_str_carrige(filedata, "IPAddress=127.0.0.1");
	fts_add_str_carrige(filedata, "TCPPort=5555");
	fts_add_str_carrige(filedata, "[SpecialSet]");
	fts_add_str_carrige(filedata, "Differ2UniformityTest_CHY10=15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 10, ");

	set_param_data(filedata);
	return 0;
}


int focal_i2c_Read(unsigned char *writebuf,
					int writelen, unsigned char *readbuf, int readlen)
{
	int ret = 0;

	ret = fts_i2c_read(fts_i2c_client, writebuf, writelen, readbuf, readlen);
	return ret;
}
/*write data by i2c*/
int focal_i2c_Write(unsigned char *writebuf, int writelen)
{
	int ret = 0;

	ret = fts_i2c_write(fts_i2c_client, writebuf, writelen);
	return ret;
}

/*
  get_of_u32_val() - get the u32 type value fro the DTSI.
  @np: device node
  @name: The DTSI key name
  @default_value: If the name is not matched, return a default value.
*/
u32 fts_get_of_u32_val(struct device_node *np, const char *name,u32 default_val)
{
	u32 tmp = 0;
	int err = 0;

	err = of_property_read_u32(np, name, &tmp);

	if (!err)
	{
		FTS_COMMON_DBG("tmp[%d]", tmp);
		return tmp;
	}
	else 
	{
		FTS_COMMON_DBG("default_val");	
		return default_val;
	}
}

u16 *fts_create_and_get_u16_array(struct device_node *dev_node, const char *name, int *size)
{
	const __be32 *values;
	u16 *val_array;
	int len;
	int sz;
	int rc;
	int i;

	/*To get the property by the property name in the node*/
	values = of_get_property(dev_node, name, &len);
	FTS_COMMON_DBG("values = of_get_property(dev_node, name, &len);\n");
	if (values == NULL)
	{
		FTS_COMMON_DBG(" (values == NULL)\n");
		return NULL;
	}

	sz = len / sizeof(u32);
//tp_log_debug("%s: %s size:%d\n", __func__, name, sz);

	FTS_COMMON_DBG(" %s size:%d\n",  name, sz);

	val_array = kzalloc(sz * sizeof(u16), GFP_KERNEL);
	if (val_array == NULL) {
		rc = -ENOMEM;
		FTS_COMMON_DBG(" (val_array == NULL)\n");
		goto fail;
	}

	for (i = 0; i < sz; i++)
		val_array[i] = (u16)be32_to_cpup(values++);

	*size = sz;

	FTS_COMMON_DBG(" return val_array;\n");
	return val_array;

fail:
	FTS_COMMON_DBG(" fail:\n");
	return ERR_PTR(rc);
}
void fts_ft8716_getrawdata_max_min_thr_from_dts(struct device *dev)
{
	struct device_node  *np = dev->of_node;
	int size = 0;
	int enable_rawdatatest = true;
	int enable_cbtest = true;

	FTS_COMMON_DBG("");
	g_stDtsCapaconfig.channelxnum_dts = fts_get_of_u32_val(np, "focaltech,channel_Xnum", 18);
	FTS_COMMON_DBG("channel_Xnum[%d]\n", g_stDtsCapaconfig.channelxnum_dts);

	g_stDtsCapaconfig.channelynum_dts = fts_get_of_u32_val(np, "focaltech,channel_Ynum", 30);
	FTS_COMMON_DBG("channel_Ynum[%d]\n", g_stDtsCapaconfig.channelynum_dts);

	g_stDtsCapaconfig.cbunif_maxmin = fts_get_of_u32_val(np, "focaltech,cbunif_maxmin", 27);
	FTS_COMMON_DBG("cbunif_maxmin[%d]\n", g_stDtsCapaconfig.cbunif_maxmin);

	g_stDtsCapaconfig.cbunif_chx_linearity = fts_get_of_u32_val(np, "focaltech,cbunif_chx_linearity", 70);
	FTS_COMMON_DBG("cbunif_chx_linearity[%d]\n", g_stDtsCapaconfig.cbunif_chx_linearity);

	g_stDtsCapaconfig.cbunif_chy_linearity = fts_get_of_u32_val(np, "focaltech,cbunif_chy_linearity", 70);
	FTS_COMMON_DBG("cbunif_chy_linearity[%d]\n", g_stDtsCapaconfig.cbunif_chy_linearity);

	g_stDtsCapaconfig.singleraw_upperlimit = fts_get_of_u32_val(np, "focaltech,singleraw_upperlimit", 13000);
	FTS_COMMON_DBG("singleraw_upperlimit[%d]\n", g_stDtsCapaconfig.singleraw_upperlimit);

	g_stDtsCapaconfig.singleraw_lowerlimit = fts_get_of_u32_val(np, "focaltech,singleraw_lowerlimit", 3000);
	FTS_COMMON_DBG("singleraw_lowerlimit[%d]\n", g_stDtsCapaconfig.singleraw_lowerlimit);

	g_stDtsCapaconfig.fts_full_raw_max_cap = fts_create_and_get_u16_array(np,"focaltech,fullraw_upperlimit", &size);
	if (!g_stDtsCapaconfig.fts_full_raw_max_cap || size != g_stDtsCapaconfig.channelxnum_dts*g_stDtsCapaconfig.channelynum_dts) {
		FTS_COMMON_DBG("unable to read focaltech,fullraw_upperlimit size=%d\n", size);
		enable_rawdatatest = false;
	}
	else {
		enable_rawdatatest = true;
	}

	g_stDtsCapaconfig.fts_full_raw_min_cap = fts_create_and_get_u16_array(np,"focaltech,fullraw_lowerlimit", &size);
	if (!g_stDtsCapaconfig.fts_full_raw_min_cap || size != g_stDtsCapaconfig.channelxnum_dts*g_stDtsCapaconfig.channelynum_dts) {
		FTS_COMMON_DBG("unable to read focaltech,fullraw_lowerlimit size=%d\n", size);
		enable_rawdatatest = false;
	}
	else {
		enable_rawdatatest &= true;
	}

	if(enable_rawdatatest == true) {
		g_stEnableTestItems_DTS.Enable_RAWDATATest = true;
	}
	else {
		g_stEnableTestItems_DTS.Enable_RAWDATATest = false;
	}
	g_stDtsCapaconfig.fts_cb_min_limit = fts_create_and_get_u16_array(np,"focaltech,fullcb_lowerlimit", &size);
	if (!g_stDtsCapaconfig.fts_cb_min_limit || size != g_stDtsCapaconfig.channelxnum_dts*g_stDtsCapaconfig.channelynum_dts) {
		FTS_COMMON_DBG("unable to read focaltech,fts_noise_limit size=%d\n", size);
		enable_cbtest = false;
	}
	else {
		enable_cbtest = true;
	}
	g_stDtsCapaconfig.fts_cb_max_limit = fts_create_and_get_u16_array(np,"focaltech,fullcb_upperlimit", &size);
	if (!g_stDtsCapaconfig.fts_cb_max_limit || size != g_stDtsCapaconfig.channelxnum_dts*g_stDtsCapaconfig.channelynum_dts) {
		FTS_COMMON_DBG("unable to read focaltech,fts_noise_limit size=%d\n", size);
		enable_cbtest = false;
	}
	else {
		enable_cbtest &= true;
	}
	if(enable_cbtest == true) {
		g_stEnableTestItems_DTS.Enable_CBTest= true;
	}
	else {
		g_stEnableTestItems_DTS.Enable_CBTest = false;
	}
	g_stDtsCapaconfig.fts_noise_limit = fts_create_and_get_u16_array(np,"focaltech,fts_noise_limit", &size);
	if (!g_stDtsCapaconfig.fts_noise_limit || size != g_stDtsCapaconfig.channelxnum_dts*g_stDtsCapaconfig.channelynum_dts) {
		FTS_COMMON_DBG("unable to read focaltech,fts_noise_limit size=%d\n", size);
		g_stEnableTestItems_DTS.Enable_NOISETest = false;
	}
	else {
		g_stEnableTestItems_DTS.Enable_NOISETest = true;
	}
	g_stDtsCapaconfig.fts_tx2tx_limit = fts_create_and_get_u16_array(np,"focaltech,fts_tx2tx_limit", &size);
	if (!g_stDtsCapaconfig.fts_tx2tx_limit || size != (g_stDtsCapaconfig.channelxnum_dts-1)*g_stDtsCapaconfig.channelynum_dts) {
		FTS_COMMON_DBG("unable to read focaltech,fts_tx2tx_limit size=%d\n", size);
		g_stEnableTestItems_DTS.Enable_TX2TXTest = false;
	}
	else {
		g_stEnableTestItems_DTS.Enable_TX2TXTest = true;
	}
	g_stDtsCapaconfig.fts_rx2rx_limit = fts_create_and_get_u16_array(np,"focaltech,fts_rx2rx_limit", &size);
	if (!g_stDtsCapaconfig.fts_rx2rx_limit || size != g_stDtsCapaconfig.channelxnum_dts*(g_stDtsCapaconfig.channelynum_dts-1)) {
		FTS_COMMON_DBG("unable to read focaltech,fts_rx2rx_limit size=%d\n", size);
		g_stEnableTestItems_DTS.Enable_RX2RXTest = false;
	}
	else {
		g_stEnableTestItems_DTS.Enable_RX2RXTest = true;
	}
	if ((g_stEnableTestItems_DTS.Enable_RAWDATATest & g_stEnableTestItems_DTS.Enable_CBTest & g_stEnableTestItems_DTS.Enable_NOISETest &
		g_stEnableTestItems_DTS.Enable_RX2RXTest & g_stEnableTestItems_DTS.Enable_TX2TXTest) == true) {
		FTS_COMMON_DBG("success^^^^^^^^--fts_ft8716_getrawdata_max_min_thr_from_dts");
	}
	else {
		FTS_COMMON_DBG("failed^^^^^^^^--fts_ft8716_getrawdata_max_min_thr_from_dts");
	}

	return;
}

ssize_t fts_fts8716_getcapacitancedata_show(struct kobject *dev,struct kobj_attribute *attr, char *buf)
{
	/* place holder for future use */
	char cfgname[MAX_BUF_LEN] = {0};
	char *testdata = NULL;
	int iTestDataLen=0;
	ssize_t num_read_chars = 0;

	char result_buf[MAX_BUF_LEN] = {0};
	//fts_ft8716_getrawdata_max_min_thr_from_dts(&fts_i2c_client->dev);

	result_reason_flag = 1;
	gnFtsTestResult = 0;

	printk("[FTS] %s \n", __func__);

	fts_auto_reset_suspend();
	FTS_DBG("");
	FTS_COMMON_DBG("1.fts_fts8716_getcapacitancedata_show\n");
	FTS_COMMON_DBG("***************Start FTS CapData Test****************\n");

	memset(buf, 0, PAGE_SIZE);

	testdata = vmalloc(MAX_TEST_DATA_SIZE);
	if(NULL == testdata)
	{
		printk("[FTS] kmalloc failed in function:%s.  ERROR 	!!!\n", __func__);
		return -EPERM;
	}

	memset(cfgname, 0, sizeof(cfgname));
	snprintf(cfgname, MAX_BUF_LEN, "%s", "test.ini");
	cfgname[8] = '\0';

	init_i2c_write_func(focal_i2c_Write);
	init_i2c_read_func(focal_i2c_Read);

	/* config test item */
	if(ft8716_get_testparam_direct() <0)
	{
		FTS_DBG("[FTS] get testparam from ini failure\n");
		num_read_chars=snprintf(buf,128 ,"0F \n");

		FTS_COMMON_DBG("****************END FAIL FTS CapData Test**************\n");			
	}
	else 
	{
		if(true == start_test_tp())
		{
			FTS_DBG("[FTS] tp test pass\n");
		}
		else
		{

			FTS_DBG("[FTS] tp test failure\n");

			if (result_reason_flag == 0)
			{
				goto ERROR;
			}
		}

		iTestDataLen = get_test_data(testdata);
		//fts_SaveTestData("testdata.csv", testdata, iTestDataLen);
		FTS_DBG("[FTS] iTestDataLen:%d  PAGE_SIZE:%ld\n",iTestDataLen,PAGE_SIZE);

		memset(testdata, 0, MAX_TEST_DATA_SIZE);
		memset(result_buf, 0, MAX_BUF_LEN);
		fts_get_test_result(result_buf, testdata, MAX_TEST_DATA_SIZE+MAX_BUF_LEN);
		/* result */
		iTestDataLen = 0;
		strncat(buf, result_buf, (4*PAGE_SIZE)  - iTestDataLen);

		/* rawdata cbdata noisedata */
		iTestDataLen = strnlen(buf, 4*PAGE_SIZE);
		strncat(buf, testdata, (4*PAGE_SIZE) - iTestDataLen -1);

		num_read_chars = strnlen(buf, 4*PAGE_SIZE);

ERROR:
		if (result_reason_flag == 0)
		{
			memset(result_buf, 0, MAX_BUF_LEN);
			strncat(result_buf, "F-software_reason", 100);
			iTestDataLen = 0;
			strncat(buf, result_buf, PAGE_SIZE - iTestDataLen);
			num_read_chars = strnlen(buf, PAGE_SIZE);
		}
		FTS_COMMON_DBG("*************num_read_chars[%zd]", num_read_chars);
		FTS_COMMON_DBG("*************END FTS CapData Test*************\n");	
		free_test_param_data();
	}

	if(NULL != testdata)
	{
		vfree(testdata);
		testdata = NULL;
	}

	FTS_DBG(" END.");

	return num_read_chars;
}

