/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)��All Rights Reserved.
*
* File Name: Global.c
*
* Author: Software Development Team, AE
*
* Created: 2015-07-14
*
* Abstract: global function for test
*
************************************************************************/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/printk.h>

#include <linux/delay.h>//msleep

#include "ini.h"
#include "Global.h"

#include "test_lib.h"

#define DEVIDE_MODE_ADDR	0x00
#define STR_BUF_LEN			128

struct StruScreenSeting g_ScreenSetParam; //��Ļ���ò���
struct stTestItem g_stTestItem[1][MAX_TEST_ITEM];
struct structSCapConfEx g_stSCapConfEx;
struct structDtsConfig g_stDtsCapaconfig;
struct stEnableTestItems_DTS g_stEnableTestItems_DTS = {true};
//struct structSCapConf g_stSCapConf;
int g_TestItemNum = 0;
char g_strIcName[MAX_IC_NAME_LEN] ={0};

int GetPrivateProfileString(char *section, char *ItemName, char *defaultvalue, char *returnValue, char *IniFile){
	char value[512] = {0};
	int len = 0;
	
	if(NULL == returnValue)
	{
		printk("[FTS] returnValue==NULL in function %s.", __func__);
		return 0;
	}
	
	if(ini_get_key(IniFile, section, ItemName, value) < 0) 
	{
		if(NULL != defaultvalue) memcpy(value, defaultvalue, strlen(defaultvalue));
		snprintf(returnValue, STR_BUF_LEN, "%s", value);
		return 0;
	} else {
		len = snprintf(returnValue, STR_BUF_LEN, "%s", value);
	}
		
	return len;	
}

void focal_msleep(int ms)
{
	msleep(ms);
}
void SysDelay(int ms)
{
	msleep(ms);
}
int focal_abs(int value)
{
	if(value < 0)
		value = 0 - value;

	return value;
}
/////////////////////////////////////////////////////////////////////////////
////��ȡIC��Ӧ��
/////////////////////////////////////////////////////////////////////////////
unsigned char get_ic_code(char * strIcName)
{
	printk("[focal] %s get_ic_code == %s \n", __func__,strIcName);
	if(strncmp(strIcName,"FT5X36",6) == 0) return IC_FT5X36;
	if(strncmp(strIcName, "FT5X36i",7) == 0) return IC_FT5X36i;
	if(strncmp(strIcName, "FT3X16",6) == 0) return IC_FT3X16;
	if(strncmp(strIcName, "FT3X26",6) == 0) return IC_FT3X26;

	if(strncmp(strIcName, "FT5X46",6) == 0) return IC_FT5X46;
	if(strncmp(strIcName, "FT5X46i",7) == 0) return IC_FT5X46i;
	if(strncmp(strIcName, "FT5526",6) == 0) return IC_FT5526;
	if(strncmp(strIcName, "FT3X17",6) == 0) return IC_FT3X17;
	if(strncmp(strIcName, "FT5436",6) == 0) return IC_FT5436;
	if(strncmp(strIcName, "FT3X27",6) == 0) return IC_FT3X27;
	if(strncmp(strIcName, "FT5526i",7) == 0) return IC_FT5526I;
	if(strncmp(strIcName, "FT5416",6) == 0) return IC_FT5416;
	if(strncmp(strIcName, "FT5426",6) == 0) return IC_FT5426;
	if(strncmp(strIcName, "FT5435",6) == 0) return IC_FT5435;
	
	if(strncmp(strIcName, "FT6X06",6) == 0) return IC_FT6X06;
	if(strncmp(strIcName, "FT3X06",6) == 0) return IC_FT3X06;

	if(strncmp(strIcName, "FT6X36",6) == 0) return IC_FT6X36;
	if(strncmp(strIcName, "FT3X07",6) == 0) return IC_FT3X07;
	if(strncmp(strIcName, "FT6416",6) == 0) return IC_FT6416;
	if(strncmp(strIcName, "FT6336G/U",9) == 0) return IC_FT6426;

	if(strncmp(strIcName, "FT5X16",6) == 0) return IC_FT5X16;
	if(strncmp(strIcName, "FT5X12",6) == 0) return IC_FT5X12;

	if(strncmp(strIcName, "FT5506",6) == 0) return IC_FT5506;
	if(strncmp(strIcName, "FT5606",6) == 0) return IC_FT5606;
	if(strncmp(strIcName, "FT5816",6) == 0) return IC_FT5816;

	if(strncmp(strIcName, "FT5822",6) == 0) return IC_FT5822;
	if(strncmp(strIcName, "FT5626",6) == 0) return IC_FT5626;
	if(strncmp(strIcName, "FT5726",6) == 0) return IC_FT5726;
	if(strncmp(strIcName, "FT5826B",7) == 0) return IC_FT5826B;
	if(strncmp(strIcName, "FT5826S",7) == 0) return IC_FT5826S;

	if(strncmp(strIcName, "FT5306",6) == 0) return IC_FT5306;
	if(strncmp(strIcName, "FT5406",6) == 0) return IC_FT5406;

	if(strncmp(strIcName, "FT8606",6) == 0) return IC_FT8606;
	
	if(strncmp(strIcName, "FT3C47U",7) == 0) return IC_FT3C47U;

	if(strncmp(strIcName,"FT8716",6) == 0) return IC_FT8716;

	if(strncmp(strIcName,"FT8736",6) == 0) return IC_FT8736;
	return 0xff;
}
/////////////////////////////////////////////////////////////////////////////
////��ȡIC��
/////////////////////////////////////////////////////////////////////////////
void get_fts_ic_name(unsigned char ucIcCode, char * strIcName)
{
	if(NULL == strIcName)return;

	snprintf(strIcName, MAX_IC_NAME_LEN, "%s", "NA");/*if can't find IC , set 'NA'*/
	
	if(ucIcCode == IC_FT5X36)snprintf(strIcName, MAX_IC_NAME_LEN, "%s", "FT5X36");
	if(ucIcCode == IC_FT5X36i)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5X36i");
	if(ucIcCode == IC_FT3X16)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3X16");
	if(ucIcCode == IC_FT3X26)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3X26");

	if(ucIcCode == IC_FT5X46)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5X22");
	if(ucIcCode == IC_FT5X46) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5X46");
	if(ucIcCode == IC_FT5X46i) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5X46i");
	if(ucIcCode == IC_FT5526) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5526");
	if(ucIcCode == IC_FT3X17)  snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3X17");
	if(ucIcCode == IC_FT5436) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5436");
	if(ucIcCode == IC_FT3X27)  snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3X27");
	if(ucIcCode == IC_FT5526I) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5526i");
	if(ucIcCode == IC_FT5416) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5416");
	if(ucIcCode == IC_FT5426) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5426");
	if(ucIcCode == IC_FT5435) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5435");
	
	if(ucIcCode == IC_FT6X06)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT6X06");
	if(ucIcCode == IC_FT3X06)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3X06");

	if(ucIcCode == IC_FT6X36)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT6X36");
	if(ucIcCode == IC_FT3X07)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3X07");
	if(ucIcCode == IC_FT6416)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT6416");
	if(ucIcCode == IC_FT6426)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT6336G/U");

	if(ucIcCode == IC_FT5X16)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5X16");
	if(ucIcCode == IC_FT5X12)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5X12");

	if(ucIcCode == IC_FT5506)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5506");
	if(ucIcCode == IC_FT5606)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5606");
	if(ucIcCode == IC_FT5816)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5816");

	if(ucIcCode == IC_FT5822)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5822");
	if(ucIcCode == IC_FT5626)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5626");
	if(ucIcCode == IC_FT5726)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5726");
	if(ucIcCode == IC_FT5826B)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5826B");
	if(ucIcCode == IC_FT5826S)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5826S");

	if(ucIcCode == IC_FT5306)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5306");
	if(ucIcCode == IC_FT5406)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT5406");

	if(ucIcCode == IC_FT8606)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT8606");
	if(ucIcCode == IC_FT8716)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT8716");

	if(ucIcCode == IC_FT3C47U) snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT3C47U");
	if(ucIcCode == IC_FT8736)snprintf(strIcName, MAX_IC_NAME_LEN, "%s",  "FT8736");
	
	return ;
}
void OnInit_InterfaceCfg(char * strIniFile)
{
	char str[STR_BUF_LEN] = {0};

	FTS_TEST_DBG(".");

	///////////////////////////IC_Type
	GetPrivateProfileString("Interface","IC_Type","FT5X36",str,strIniFile);
	g_ScreenSetParam.iSelectedIC = get_ic_code(str);
	//g_ScreenSetParam.iSelectedIC = IC_FT8736;
	g_ScreenSetParam.iSelectedIC = IC_FT8716;//20160620

	/////////////////////////Normalize Type
	GetPrivateProfileString("Interface","Normalize_Type",0,str,strIniFile);
	g_ScreenSetParam.isNormalize = atoi(str);	

}
/************************************************************************
* Name: ReadReg(Same function name as FT_MultipleTest)
* Brief:  Read Register
* Input: RegAddr
* Output: RegData
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
int ReadReg(unsigned char RegAddr, unsigned char *RegData)
{
	int iRet;

	if(NULL == fts_i2c_read_test)
		{
		printk("[focal] %s fts_i2c_read_test == NULL \n", __func__);
		return (ERROR_CODE_INVALID_COMMAND);
		}
	
	iRet = fts_i2c_read_test(&RegAddr, 1, RegData, 1);

	if(iRet >= 0)
		return (ERROR_CODE_OK);
	else
		return (ERROR_CODE_COMM_ERROR);
}
/************************************************************************
* Name: WriteReg(Same function name as FT_MultipleTest)
* Brief:  Write Register
* Input: RegAddr, RegData
* Output: null
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
int WriteReg(unsigned char RegAddr, unsigned char RegData)
{
	int iRet;
	unsigned char cmd[2] = {0};

	if(NULL == fts_i2c_write_test)
		{
		printk("[focal] %s fts_i2c_write_test == NULL \n", __func__);
		return (ERROR_CODE_INVALID_COMMAND);
		}
	
	cmd[0] = RegAddr;
	cmd[1] = RegData;
	iRet = fts_i2c_write_test(cmd, 2);

	if(iRet >= 0)
		return (ERROR_CODE_OK);
	else
		return (ERROR_CODE_COMM_ERROR);
}
/************************************************************************
* Name: Comm_Base_IIC_IO(Same function name as FT_MultipleTest)
* Brief:  Write/Read Data by IIC
* Input: pWriteBuffer, iBytesToWrite, iBytesToRead
* Output: pReadBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char Comm_Base_IIC_IO(unsigned char *pWriteBuffer, int  iBytesToWrite, unsigned char *pReadBuffer, int iBytesToRead)
{
	int iRet;	

	if(NULL == fts_i2c_read_test)
		{
		printk("[focal] %s fts_i2c_read_test == NULL \n", __func__);
		return (ERROR_CODE_INVALID_COMMAND);
		}
	
	iRet = fts_i2c_read_test(pWriteBuffer, iBytesToWrite, pReadBuffer, iBytesToRead);

	if(iRet >= 0)
		return (ERROR_CODE_OK);
	else
		return (ERROR_CODE_COMM_ERROR);
}
/************************************************************************
* Name: EnterWork(Same function name as FT_MultipleTest)
* Brief:  Enter Work Mode
* Input: null
* Output: null
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char EnterWork(void)
{
	unsigned char RunState = 0;
	unsigned char ReCode = ERROR_CODE_COMM_ERROR;

	ReCode = ReadReg(DEVIDE_MODE_ADDR, &RunState);
	if(ReCode == ERROR_CODE_OK)
	{
		if(((RunState>>4)&0x07) == 0x00)	//work
		{
			ReCode = ERROR_CODE_OK;
		}
		else
		{
			ReCode = WriteReg(DEVIDE_MODE_ADDR, 0);
			if(ReCode == ERROR_CODE_OK)
			{
				ReCode = ReadReg(DEVIDE_MODE_ADDR, &RunState);
				if(ReCode == ERROR_CODE_OK)
				{	
					if(((RunState>>4)&0x07) == 0x00)	ReCode = ERROR_CODE_OK;
					else	ReCode = ERROR_CODE_COMM_ERROR;
				}
			}
		}
	}

	return ReCode;
	
}
/************************************************************************
* Name: EnterFactory
* Brief:  enter Fcatory Mode
* Input: null
* Output: null
* Return: Comm Code. Code = 0 is OK, else fail.
***********************************************************************/
unsigned char EnterFactory(void)
{
	unsigned char RunState = 0;
	unsigned char ReCode = ERROR_CODE_COMM_ERROR;

	ReCode = ReadReg(DEVIDE_MODE_ADDR, &RunState);
	if(ReCode == ERROR_CODE_OK)
	{
		if(((RunState>>4)&0x07) == 0x04)	//factory
		{
			ReCode = ERROR_CODE_OK;
		}
		else
		{
			ReCode = WriteReg(DEVIDE_MODE_ADDR, 0x40);
			if(ReCode == ERROR_CODE_OK)
			{
				ReCode = ReadReg(DEVIDE_MODE_ADDR, &RunState);
				if(ReCode == ERROR_CODE_OK)
				{	
					if(((RunState>>4)&0x07) == 0x04)	ReCode = ERROR_CODE_OK;
					else	ReCode = ERROR_CODE_COMM_ERROR;
				}
				else
					printk("EnterFactory read DEVIDE_MODE_ADDR error 3.\n");
			}
			else
				printk("EnterFactory write DEVIDE_MODE_ADDR error 2.\n");
		}
	}
	else
		printk("EnterFactory read DEVIDE_MODE_ADDR error 1.\n");

	return ReCode;
}

