/************************************************************************
* Copyright (C) 2012-2015, Focaltech Systems (R)，All Rights Reserved.
*
* File Name: Test_FT8716.c
*
* Author: Software Development 
*
* Created: 2015-12-24
*
* Abstract: test item for FT8716
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "Global.h"
#include "Test_FT8716.h"
#include "DetailThreshold.h"
#include "Config_FT8716.h"
//#include "Comm_FT8716.h"
#include "focaltech_core.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define IC_TEST_VERSION  "Test version: V1.0.0--2015-12-24, (sync version of FT_MultipleTest: V2.9.0.1--2015-12-22)"

#define MAX_NOISE_FRAMES		2       //The real test frame is 4*MAX_NOISE_FRAMES
#define MAX_TMPBUFF_SIZE		(1024*4)
//#define MAX_STORE_AREA_SIZE	(1024*32)
#define MAX_BUFF_SIZE_2KB		(1024*2)
#define MAX_BUFF_SIZE_4KB		(1024*4)
#define MAX_BUFF_SIZE_8KB		(1024*8)
#define MAX_BUFF_SIZE_16KB		(1024*16)
#define MAX_BUFF_SIZE_32KB		(1024*32)

/////////////////////////////////////////////////Reg FT8736
#define DEVIDE_MODE_ADDR	0x00
#define REG_LINE_NUM	0x01
#define REG_TX_NUM	0x02
#define REG_RX_NUM	0x03
#define FT8716_LEFT_KEY_REG    0X1E
#define FT8716_RIGHT_KEY_REG   0X1F

#define REG_CbAddrH  		0x18	 
#define REG_CbAddrL			0x19	
#define REG_OrderAddrH		0x1A	
#define REG_OrderAddrL		0x1B	

#define REG_RawBuf0			0x6A	
#define REG_RawBuf1			0x6B	
#define REG_OrderBuf0		0x6C	
#define REG_CbBuf0			0x6E	

#define REG_K1Delay			0x31	
#define REG_K2Delay			0x32	
#define REG_SCChannelCf		0x34

#define pre 1

#define REG_CLB				0x04
#define FTS_LEN_MAX_STR		80


/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
/*
struct structSCapConfEx 
{
	unsigned char ChannelXNum;
	unsigned char ChannelYNum;
	unsigned char KeyNum;
	unsigned char KeyNumTotal;
	bool bLeftKey1;
	bool bLeftKey2;
	bool bLeftKey3;
	bool bRightKey1;
	bool bRightKey2;
	bool bRightKey3;	
};
struct structSCapConfEx g_stSCapConfEx;
*/
enum NOISE_TYPE
{
	NT_AvgData = 0,
	NT_MaxData = 1,
	NT_MaxDevication = 2,
	NT_DifferData = 3,
};

/*******************************************************************************
* Static variables
*******************************************************************************/

static int m_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int Tx2tx_delta_buffer[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int Rx2rx_delta_buffer[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
//static int m_NoiseRawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
//static int m_NoiseData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_CBData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_CBData_Ori[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
//static int m_AvgData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_iTempData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};//Two-dimensional
static BYTE m_ucTempData[TX_NUM_MAX * RX_NUM_MAX*2] = {0};//One-dimensional
static int m_iTempRawData[TX_NUM_MAX * RX_NUM_MAX] = {0};
//static int m_TempNoiseData[MAX_NOISE_FRAMES][RX_NUM_MAX * TX_NUM_MAX] = {{0,0}};
//static int m_Lcd_Noise_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};
static int m_Lcd_Noise_RawData[TX_NUM_MAX * RX_NUM_MAX] = {0};
static int tmp_Noise_RawData[TX_NUM_MAX][RX_NUM_MAX] = {{0,0}};

//---------------------About Store Test Dat
//static char g_pStoreAllData[MAX_BUFF_SIZE_8KB] = {0};
//20160621
static char g_pStoreAllData[MAX_BUFF_SIZE_32KB] = {0};
static char *g_pTmpBuff = NULL;
static char *g_pStoreMsgArea = NULL;
static int g_lenStoreMsgArea = 0;
static char *g_pMsgAreaLine2 = NULL;
static int g_lenMsgAreaLine2 = 0;
static char *g_pStoreDataArea = NULL;
static int g_lenStoreDataArea = 0;
static unsigned char m_ucTestItemCode = 0;
static int m_iStartLine = 0;
static int m_iTestDataCount = 0;

static int m_iAdcData[TX_NUM_MAX * RX_NUM_MAX] =  {0};

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/


/*******************************************************************************
* Static function prototypes
*******************************************************************************/

/////////////////////////////////////////////////////////////
static int StartScan(void);
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer);
static unsigned char GetPanelRows(unsigned char *pPanelRows);
static unsigned char GetPanelCols(unsigned char *pPanelCols);
static unsigned char GetTxRxCB(unsigned short StartNodeNo, unsigned short ReadNum, unsigned char *pReadBuffer);
/////////////////////////////////////////////////////////////
static unsigned char GetRawData(void);
static unsigned char GetChannelNum(void);
////////////////////////////////////////////////////////////
//////////////////////////////////////////////
static int InitTest(void);
static void FinishTest(void);
//static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount);
static void InitStoreParamOfTestData(void);
static void MergeAllTestData(void);
//////////////////////////////////////////////Others 
static int AllocateMemory(void);
static void FreeMemory(void);
//static unsigned int SqrtNew(unsigned int n) ;

/************************************************************************
* Name: FT8716_StartTest
* Brief:  Test entry. Determine which test item to test
* Input: none
* Output: none
* Return: Test Result, PASS or FAIL
***********************************************************************/
boolean FT8716_StartTest()
{
	bool bTestResult = true;
	bool bTempResult = true;
	unsigned char ReCode;
	unsigned char ucDevice = 0;
	int iItemCount=0;
	int ret = 0;

//	gnFtsTestResult = 0;

//	fts_auto_reset_suspend();
	//--------------1. Init part
	ret = InitTest();
	if(ret < 0)
	{
		return false;
	}

	//--------------2. test item
	if(0 == g_TestItemNum)
		bTestResult = false;

	////////测试过程，即是顺序执行g_stTestItem结构体的测试项
	for(iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
	{
		m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;

		g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
		///////////////////////////////////////////////////////FT8716_ENTER_FACTORY_MODE
		if(Code_FT8716_ENTER_FACTORY_MODE == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			FTS_COMMON_DBG("Code_FT8716_ENTER_FACTORY_MODE\n");
			ReCode = FT8716_TestItem_EnterFactoryMode();
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
				break;//if this item FAIL, no longer test.				
			}
			else
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
		}

		///////////////////////////////////////////////////////FT8716_CHANNEL_NUM_TEST
		if(Code_FT8716_CHANNEL_NUM_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			FTS_COMMON_DBG("Code_FT8716_CHANNEL_NUM_TEST\n");
			ReCode = FT8716_TestItem_ChannelsTest(&bTempResult);
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
				//gnFtsTestResult &= ~(0x01);
				break;//if this item FAIL, no longer test.
			}
			else
			{
				//gnFtsTestResult |= 0x01;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}

		}

		///////////////////////////////////////////////////////FT8716_RAWDATA_TEST
		//FTS_COMMON_DBG("Code_FT8716_RAWDATA_TEST\n");
		if(Code_FT8716_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
			&& g_stEnableTestItems_DTS.Enable_RAWDATATest == true)
		{
			FTS_COMMON_DBG("Code_FT8716_RAWDATA_TEST\n");
			ReCode = FT8716_TestItem_RawDataTest(&bTempResult);
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<1);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
				FTS_COMMON_DBG("g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;\n");
			}
			else
			{
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
				//gnFtsTestResult |= (0x01<<1);
			}
		}

		///////////////////////////////////////////////////////FT8716_CB_TEST

		if(Code_FT8716_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
			&& g_stEnableTestItems_DTS.Enable_CBTest)
		{
			FTS_COMMON_DBG("Code_FT8716_CB_TEST\n");
			ReCode = FT8716_TestItem_CbTest(&bTempResult);
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<2);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<2);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}
		}

		///////////////////////////////////////////////////////FT8716_CB_UNIFORMITY_TEST
		if(Code_FT8716_CB_UNIFORMITY_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
			&& g_stEnableTestItems_DTS.Enable_CBTest)
		{
			FTS_COMMON_DBG("Code_FT8716_CB_UNIFORMITY_TEST\n");
			
			ReCode = Fts_TestItem_CBUniformityTest(&bTempResult);

			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<2);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<2);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}
		}

		///////////////////////////////////////////////////////FT8716_OPEN_TEST

		if(Code_FT8716_OPEN_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			FTS_COMMON_DBG("Code_FT8716_OPEN_TEST\n");
			//fts_ts_HW_reset();
			//ReCode = FT8716_TestItem_EnterFactoryMode();
			ReCode = FT8716_TestItem_OpenTest(&bTempResult); //
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<3);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<3);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}
		}

		///////////////////////////////////////////////////////FT8716_SHORT_TEST

		if(Code_FT8716_SHORT_CIRCUIT_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			FTS_COMMON_DBG("Code_FT8716_SHORT_CIRCUIT_TEST\n");
			//fts_ts_HW_reset();
			//ReCode = FT8716_TestItem_EnterFactoryMode();
			
			ReCode = FT8716_TestItem_ShortCircuitTest(&bTempResult); //
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<4);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<4);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}

		}

		///////////////////////////////////////////////////////FT8716_NOISE_TEST

		if(Code_FT8716_NOISE_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
			&& g_stEnableTestItems_DTS.Enable_NOISETest == true)
		{
			//fts_ts_HW_reset();
			//ReCode = FT8716_TestItem_EnterFactoryMode();
			
			ReCode = FT8716_TestItem_NoiseTest(&bTempResult); //
			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<5);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<5);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}
		}

		//tx2tx test
		if(Code_FT8716_TX2TX_DELTA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
			&& g_stEnableTestItems_DTS.Enable_TX2TXTest == true
			&& g_stEnableTestItems_DTS.Enable_RAWDATATest == true)
		{
			ReCode = Fts_8716_Tx2Tx_Test(&bTempResult); //

			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<6);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<6);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}

		}
		//rx2rx test
		if(Code_FT8716_RX2RX_DELTA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode
			&& g_stEnableTestItems_DTS.Enable_RX2RXTest == true
			&& g_stEnableTestItems_DTS.Enable_RAWDATATest == true)
		{
			ReCode = Fts_8716_Rx2Rx_Test(&bTempResult); //

			if(ERROR_CODE_OK != ReCode || (!bTempResult))
			{
				result_reason_flag = 2;
				//gnFtsTestResult &= ~(0x01<<7);
				bTestResult = false;
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_NG;
			}
			else
			{
				//gnFtsTestResult |= (0x01<<7);
				g_stTestItem[ucDevice][iItemCount].TestResult = RESULT_PASS;
			}

		}

	}

	//--------------3. End Part
	FTS_COMMON_DBG("FinishTest\n");
	FinishTest();

	fts_auto_reset_resume();
	//--------------4. return result

	return bTestResult;

}
/************************************************************************
* Name: InitTest
* Brief:  Init all param before test
* Input: none
* Output: none
* Return: none
***********************************************************************/
static int InitTest(void)
{
	int ret = 0;
	ret = AllocateMemory();//Allocate pointer Memory
	if(ret < 0)
		return -1;
	
	InitStoreParamOfTestData();

	g_stSCapConfEx.ChannelXNum = 0;
	g_stSCapConfEx.ChannelYNum = 0;
	g_stSCapConfEx.KeyNum = 0;
	g_stSCapConfEx.KeyNumTotal = 6;

	FTS_TEST_DBG("[focal] %s \n", IC_TEST_VERSION);	//show lib version

	return 0;
}
/************************************************************************
* Name: FinishTest
* Brief:  Init all param before test
* Input: none
* Output: none
* Return: none
***********************************************************************/
static void FinishTest(void)
{
	MergeAllTestData();//Merge Test Result
	FreeMemory();//Release pointer memory
}
/************************************************************************
* Name: FT8716_get_test_data
* Brief:  get data of test result
* Input: none
* Output: pTestData, the returned buff
* Return: the length of test data. if length > 0, got data;else ERR.
***********************************************************************/
int FT8716_get_test_data(char *pTestData)
{
	FTS_TEST_DBG("");

	if(NULL == pTestData)
	{
		FTS_TEST_DBG("[focal] %s pTestData == NULL \n", __func__);	
		return -1;
	}
	memcpy(pTestData, g_pStoreAllData, (g_lenStoreMsgArea+g_lenStoreDataArea));
	return (g_lenStoreMsgArea+g_lenStoreDataArea);	
}

//////////////////////////////////////////////
/************************************************************************
* Name: AllocateMemory
* Brief:  Allocate pointer Memory
* Input: none
* Output: none
* Return: none
***********************************************************************/
static int AllocateMemory(void)
{
	//New buff
	g_pStoreMsgArea =NULL;	
	if(NULL == g_pStoreMsgArea)
		g_pStoreMsgArea = vmalloc(MAX_BUFF_SIZE_4KB);
	if(NULL == g_pStoreMsgArea)
		goto ERR;

	g_pMsgAreaLine2 =NULL;	
	if(NULL == g_pMsgAreaLine2)
		g_pMsgAreaLine2 = vmalloc(MAX_BUFF_SIZE_2KB);
	if(NULL == g_pMsgAreaLine2)
		goto ERR;

	g_pStoreDataArea =NULL;	
	if(NULL == g_pStoreDataArea)
		g_pStoreDataArea = vmalloc(MAX_BUFF_SIZE_32KB);	// 20160622		g_pStoreDataArea = vmalloc(MAX_BUFF_SIZE_8KB);
	if(NULL == g_pStoreDataArea)
		goto ERR;

	/*g_pStoreAllData =NULL;	
	if(NULL == g_pStoreAllData)
	g_pStoreAllData = kmalloc(1024*8, GFP_ATOMIC);*/
	g_pTmpBuff =NULL;	
	if(NULL == g_pTmpBuff)
		g_pTmpBuff = vmalloc(MAX_TMPBUFF_SIZE);
	if(NULL == g_pTmpBuff)
		goto ERR;

	return 0;

ERR:
	FTS_TEST_DBG("[focal] %s kmalloc err. \n", __func__);
	return -1;

}
/************************************************************************
* Name: FreeMemory
* Brief:  Release pointer memory
* Input: none
* Output: none
* Return: none
***********************************************************************/
static void FreeMemory(void)
{
	//Release buff
	if(NULL != g_pStoreMsgArea)
		vfree(g_pStoreMsgArea);

	if(NULL != g_pMsgAreaLine2)
		vfree(g_pMsgAreaLine2);

	if(NULL != g_pStoreDataArea)
		vfree(g_pStoreDataArea);

	/*if(NULL == g_pStoreAllData)
	kfree(g_pStoreAllData);*/

	if(NULL != g_pTmpBuff)
		vfree(g_pTmpBuff);
}

/************************************************************************
* Name: InitStoreParamOfTestData
* Brief:  Init store param of test data
* Input: none
* Output: none
* Return: none
***********************************************************************/
static void InitStoreParamOfTestData(void)
{
	g_lenStoreMsgArea = 0;
	//Msg Area, Add Line1
	g_lenStoreMsgArea += snprintf(g_pStoreMsgArea, MAX_BUFF_SIZE_4KB, "ECC, 85, 170, IC Name, %s, IC Code, %x\n", g_strIcName,  g_ScreenSetParam.iSelectedIC);

	//Line2
	//g_pMsgAreaLine2 = NULL;
	g_lenMsgAreaLine2 = 0;

	//Data Area
	//g_pStoreDataArea = NULL;
	g_lenStoreDataArea = 0;
	m_iStartLine = 11;//The Start Line of Data Area is 11

	m_iTestDataCount = 0;	
}
/************************************************************************
* Name: MergeAllTestData
* Brief:  Merge All Data of test result
* Input: none
* Output: none
* Return: none
***********************************************************************/
static void MergeAllTestData(void)
{
	int iLen = 0;

	FTS_TEST_DBG("");

	//Add the head part of Line2
	iLen= snprintf(g_pTmpBuff, MAX_TMPBUFF_SIZE, "TestItem, %d, ", m_iTestDataCount);
	memcpy(g_pStoreMsgArea+g_lenStoreMsgArea, g_pTmpBuff, iLen);
	g_lenStoreMsgArea+=iLen;

	//Add other part of Line2, except for "\n"
	memcpy(g_pStoreMsgArea+g_lenStoreMsgArea, g_pMsgAreaLine2, g_lenMsgAreaLine2);
	g_lenStoreMsgArea+=g_lenMsgAreaLine2;	

	//Add Line3 ~ Line10
	iLen= snprintf(g_pTmpBuff, MAX_TMPBUFF_SIZE, "\n");
	memcpy(g_pStoreMsgArea+g_lenStoreMsgArea, g_pTmpBuff, iLen);
	g_lenStoreMsgArea+=iLen;

	///1.Add Msg Area
	memcpy(g_pStoreAllData, g_pStoreMsgArea, g_lenStoreMsgArea);

	///2.Add Data Area
	if(0!= g_lenStoreDataArea)
	{
		memcpy(g_pStoreAllData+g_lenStoreMsgArea, g_pStoreDataArea, g_lenStoreDataArea);
	}

	FTS_TEST_DBG("[focal] %s lenStoreMsgArea=%d,  lenStoreDataArea = %d\n", __func__, g_lenStoreMsgArea, g_lenStoreDataArea);
}

#if 0
/************************************************************************
* Name: Save_Test_Data
* Brief:  Storage format of test data
* Input: int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount
* Output: none
* Return: none
***********************************************************************/
static void Save_Test_Data(int iData[TX_NUM_MAX][RX_NUM_MAX], int iArrayIndex, unsigned char Row, unsigned char Col, unsigned char ItemCount)
{
	int iLen = 0;
	int i = 0, j = 0;

	FTS_TEST_DBG("");

	//Save  Msg (ItemCode is enough, ItemName is not necessary, so set it to "NA".)
	iLen= snprintf(g_pTmpBuff, MAX_TMPBUFF_SIZE, "NA, %d, %d, %d, %d, %d, ", \
		m_ucTestItemCode, Row, Col, m_iStartLine, ItemCount);
	memcpy(g_pMsgAreaLine2+g_lenMsgAreaLine2, g_pTmpBuff, iLen);
	g_lenMsgAreaLine2 += iLen;

	m_iStartLine += Row;
	m_iTestDataCount++;

	//Save Data 
	for(i = 0+iArrayIndex; i < Row+iArrayIndex; i++)
	{
		for(j = 0; j < Col; j++)
		{
			if(j == (Col -1))//The Last Data of the Row, add "\n"
				iLen= snprintf(g_pTmpBuff, MAX_TMPBUFF_SIZE, "%d, \n", iData[i][j]);	
			else
				iLen= snprintf(g_pTmpBuff, MAX_TMPBUFF_SIZE, "%d, ", iData[i][j]);	

			if(g_lenStoreDataArea+iLen> MAX_BUFF_SIZE_32KB)
				break;
			memcpy(g_pStoreDataArea+g_lenStoreDataArea, g_pTmpBuff, iLen);
			g_lenStoreDataArea += iLen;		
		}
		if(g_lenStoreDataArea+iLen> MAX_BUFF_SIZE_32KB)
				break;
	}

}
#endif
////////////////////////////////////////////////////////////
/************************************************************************
* Name: StartScan(Same function name as FT_MultipleTest)
* Brief:  Scan TP, do it before read Raw Data
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static int StartScan(void)
{
	unsigned char RegVal = 0x00;
	unsigned char times = 0;
	const unsigned char MaxTimes = 60;	//最长等待160ms
	unsigned char ReCode = ERROR_CODE_COMM_ERROR;

	//if(hDevice == NULL)		return ERROR_CODE_NO_DEVICE;

	ReCode = ReadReg(DEVIDE_MODE_ADDR,&RegVal);

	FTS_COMMON_DBG("RegVal[%d]\n", RegVal);

	if(ReCode == ERROR_CODE_OK)
	{
		RegVal |= 0x80;		//最高位置1，启动扫描
		ReCode = WriteReg(DEVIDE_MODE_ADDR,RegVal);
		times = 0;

		while(times++ < MaxTimes)		
		{
			ReCode = ReadReg(DEVIDE_MODE_ADDR,&RegVal);

			if(ReCode == ERROR_CODE_OK)
			{
				RegVal |= 0x80;		
				ReCode = WriteReg(DEVIDE_MODE_ADDR,RegVal);

				ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);

				FTS_COMMON_DBG("RegVal[%d], ReCode[%d]\n", RegVal, ReCode);

				if(ReCode == ERROR_CODE_OK)
				{
					if((RegVal>>7) == 1)
					{
						FTS_COMMON_DBG("*****break****RegVal[%d], ReCode[%d]\n", RegVal, ReCode);
						break;
					}
				}
			}
		}

		FTS_COMMON_DBG("RegVal[%d], ReCode[%d]\n", RegVal, ReCode);

		if(ReCode == ERROR_CODE_OK)
		{
			times = 0;
			while(times++ < MaxTimes)		//等待扫描完成
			{
				SysDelay(8);	//8ms
				ReCode = ReadReg(DEVIDE_MODE_ADDR, &RegVal);

				FTS_COMMON_DBG("RegVal[%d], ReCode[%d], times[%d]\n\n", RegVal, ReCode, times);

				if(ReCode == ERROR_CODE_OK)
				{
					if((RegVal>>7) == 0)
					{

						FTS_COMMON_DBG("*****break****RegVal[%d], ReCode[%d], times[%d]\n", RegVal, ReCode, times);
						break;
					}
				}
				else
				{
					FTS_COMMON_DBG("ReadReg Fail, RegVal[%d], ReCode[%d], times[%d]", RegVal, ReCode, times);
					//break;
				}
			}
			if(times < MaxTimes)	
			{
				FTS_COMMON_DBG("ERROR_CODE_OK, RegVal[%d], ReCode[%d], times[%d]\n",  RegVal, ReCode, times);
				ReCode = ERROR_CODE_OK;
			}
			else 
			{
				FTS_COMMON_DBG("ERROR_CODE_COMM_ERROR,RegVal[%d], ReCode[%d], times[%d]\n", RegVal, ReCode, times);
				ReCode = ERROR_CODE_COMM_ERROR;
			}
		}
	}

	FTS_COMMON_DBG("ReCode[%d]", ReCode);
	FTS_COMMON_DBG("******************RegVal[%d], ReCode[%d], times[%d]\n", RegVal, ReCode, times);
	return ReCode;

}	
/************************************************************************
* Name: ReadRawData(Same function name as FT_MultipleTest)
* Brief:  read Raw Data
* Input: Freq(No longer used, reserved), LineNum, ByteNum
* Output: pRevBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char ReadRawData(unsigned char Freq, unsigned char LineNum, int ByteNum, int *pRevBuffer)
{
	unsigned char ReCode=ERROR_CODE_COMM_ERROR;
	unsigned char I2C_wBuffer[3] = {0};
	unsigned char pReadData[ByteNum];
	//unsigned char pReadDataTmp[ByteNum*2];
	int i, iReadNum;
	unsigned short BytesNumInTestMode1=0;

	iReadNum=ByteNum/BYTES_PER_TIME;

	if(0 != (ByteNum%BYTES_PER_TIME)) iReadNum++;

	if(ByteNum <= BYTES_PER_TIME)
	{
		BytesNumInTestMode1 = ByteNum;		
	}
	else
	{
		BytesNumInTestMode1 = BYTES_PER_TIME;
	}

	ReCode = WriteReg(REG_LINE_NUM, LineNum);//Set row addr;


	//***********************************************************Read raw data in test mode1		
	I2C_wBuffer[0] = REG_RawBuf0;	//set begin address
	if(ReCode == ERROR_CODE_OK)
	{
		focal_msleep(10);
		ReCode = Comm_Base_IIC_IO(I2C_wBuffer, 1, pReadData, BytesNumInTestMode1);
	}

	for(i=1; i<iReadNum; i++)
	{
		if(ReCode != ERROR_CODE_OK) break;

		if(i==iReadNum-1)//last packet
		{
			focal_msleep(10);
			ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, ByteNum-BYTES_PER_TIME*i);
		}
		else
		{
			focal_msleep(10);
			ReCode = Comm_Base_IIC_IO(NULL, 0, pReadData+BYTES_PER_TIME*i, BYTES_PER_TIME);	
		}

	}

	if(ReCode == ERROR_CODE_OK)
	{
		for(i=0; i<(ByteNum>>1); i++)
		{
			pRevBuffer[i] = (pReadData[i<<1]<<8)+pReadData[(i<<1)+1];
			//if(pRevBuffer[i] & 0x8000)//有符号位
			//{
			//	pRevBuffer[i] -= 0xffff + 1;
			//}
		}
	}


	return ReCode;

}
/************************************************************************
* Name: GetTxRxCB(Same function name as FT_MultipleTest)
* Brief:  get CB of Tx/Rx
* Input: StartNodeNo, ReadNum
* Output: pReadBuffer
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetTxRxCB(unsigned short StartNodeNo, unsigned short ReadNum, unsigned char *pReadBuffer)
{
	unsigned char ReCode = ERROR_CODE_OK;
	unsigned short usReturnNum = 0;//每次要返回的个数
	unsigned short usTotalReturnNum = 0;//总返回个数
	unsigned char wBuffer[4];	
	int i, iReadNum;

	iReadNum = ReadNum/BYTES_PER_TIME;

	if(0 != (ReadNum%BYTES_PER_TIME)) iReadNum++;

	wBuffer[0] = REG_CbBuf0;

	usTotalReturnNum = 0;

	for(i = 1; i <= iReadNum; i++)
	{
		if(i*BYTES_PER_TIME > ReadNum)
			usReturnNum = ReadNum - (i-1)*BYTES_PER_TIME;
		else
			usReturnNum = BYTES_PER_TIME;	

		wBuffer[1] = (StartNodeNo+usTotalReturnNum) >>8;//地址偏移量高8位
		wBuffer[2] = (StartNodeNo+usTotalReturnNum)&0xff;//地址偏移量低8位

		ReCode = WriteReg(REG_CbAddrH, wBuffer[1]);
		ReCode = WriteReg(REG_CbAddrL, wBuffer[2]);
		//ReCode = fts_i2c_read(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);
		ReCode = Comm_Base_IIC_IO(wBuffer, 1, pReadBuffer+usTotalReturnNum, usReturnNum);

		usTotalReturnNum += usReturnNum;

		if(ReCode != ERROR_CODE_OK)return ReCode;

		//if(ReCode < 0) return ReCode;
	}

	return ReCode;
}

//***********************************************
//获取PanelRows
//***********************************************
static unsigned char GetPanelRows(unsigned char *pPanelRows)
{
	return ReadReg(REG_TX_NUM, pPanelRows);
}

//***********************************************
//获取PanelCols
//***********************************************
static unsigned char GetPanelCols(unsigned char *pPanelCols)
{
	return ReadReg(REG_RX_NUM, pPanelCols);
}



/////////////////////////////////////////////////////////////


/************************************************************************
* Name: FT8716_TestItem_EnterFactoryMode
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_EnterFactoryMode(void)
{	

	unsigned char ReCode = ERROR_CODE_INVALID_PARAM;
	int iRedo = 5;	//如果不成功，重复进入5次
	int i ;
	//SysDelay(150);
	FTS_TEST_DBG("Enter factory mode...\n");
	for(i = 1; i <= iRedo; i++)
	{
		ReCode = EnterFactory();
		if(ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Enter factory mode...\n");
			if(i < iRedo)
			{
				SysDelay(50);
				continue;
			}
		}
		else
		{
			//break;
		}

	}
	//SysDelay(300);
	SysDelay(30);

	if(ReCode == ERROR_CODE_OK)	//进工厂模式成功后，就读出通道数
	{	
		ReCode = GetChannelNum();
	}
	else
	{
		result_reason_flag = 0;
	}
	return ReCode;
}
/************************************************************************
* Name: GetChannelNum
* Brief:  Get Num of Ch_X, Ch_Y and key
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetChannelNum(void)
{
	unsigned char ReCode;
	//int TxNum, RxNum;
	int i ;
	unsigned char rBuffer[1] = {0}; //= new unsigned char;

	FTS_TEST_DBG("Enter GetChannelNum...\n");
	//--------------------------------------------"Get Channel X Num...";
	for(i = 0; i < 3; i++)
	{
		ReCode = GetPanelRows(rBuffer);
		if(ReCode == ERROR_CODE_OK)
		{
			if(0 < rBuffer[0] && rBuffer[0] < 80)
			{
				g_stSCapConfEx.ChannelXNum = rBuffer[0];
				if(g_stSCapConfEx.ChannelXNum > g_ScreenSetParam.iUsedMaxTxNum)
				{
					FTS_TEST_DBG("Failed to get Channel X number, Get num = %d, UsedMaxNum = %d\n",
						g_stSCapConfEx.ChannelXNum, g_ScreenSetParam.iUsedMaxTxNum);
					return ERROR_CODE_INVALID_PARAM;
				}			
				break;
			}
			else
			{
				//SysDelay(150);
				SysDelay(50);
				continue;
			}
		}
		else
		{
			FTS_TEST_DBG("Failed to get Channel X number\n");
			//SysDelay(150);
			SysDelay(50);
		}
	}

	//--------------------------------------------"Get Channel Y Num...";
	for(i = 0; i < 3; i++)
	{
		ReCode = GetPanelCols(rBuffer);
		if(ReCode == ERROR_CODE_OK)
		{
			if(0 < rBuffer[0] && rBuffer[0] < 80)
			{
				g_stSCapConfEx.ChannelYNum = rBuffer[0];
				if(g_stSCapConfEx.ChannelYNum > g_ScreenSetParam.iUsedMaxRxNum)
				{
					FTS_TEST_DBG("Failed to get Channel Y number, Get num = %d, UsedMaxNum = %d\n",
						g_stSCapConfEx.ChannelYNum, g_ScreenSetParam.iUsedMaxRxNum);
					return ERROR_CODE_INVALID_PARAM;
				}				
				break;
			}
			else
			{
				//SysDelay(150);
				SysDelay(50);
				continue;
			}
		}
		else
		{
			FTS_TEST_DBG("Failed to get Channel Y number\n");
			//SysDelay(150);
			SysDelay(50);
		}
	}

	//--------------------------------------------"Get Key Num...";
#if 0
	for(i = 0; i < 3; i++)
	{
		unsigned char regData = 0;
		g_stSCapConfEx.KeyNum = 0;
		ReCode = ReadReg( FT8716_LEFT_KEY_REG, &regData );
		if(ReCode == ERROR_CODE_OK)
		{
			if(((regData >> 0) & 0x01)) { g_stSCapConfEx.bLeftKey1 = true; ++g_stSCapConfEx.KeyNum;}
			if(((regData >> 1) & 0x01)) { g_stSCapConfEx.bLeftKey2 = true; ++g_stSCapConfEx.KeyNum;}
			if(((regData >> 2) & 0x01)) { g_stSCapConfEx.bLeftKey3 = true; ++g_stSCapConfEx.KeyNum;}
		}
		else
		{
			FTS_TEST_DBG("Failed to get Key number\n");
			SysDelay(150);
			continue;
		}
		ReCode = ReadReg( FT8716_RIGHT_KEY_REG, &regData );
		if(ReCode == ERROR_CODE_OK)
		{
			if(((regData >> 0) & 0x01)) {g_stSCapConfEx.bRightKey1 = true; ++g_stSCapConfEx.KeyNum;}
			if(((regData >> 1) & 0x01)) {g_stSCapConfEx.bRightKey2 = true; ++g_stSCapConfEx.KeyNum;}
			if(((regData >> 2) & 0x01)) {g_stSCapConfEx.bRightKey3 = true; ++g_stSCapConfEx.KeyNum;}
			break;
		}
		else
		{
			FTS_TEST_DBG("Failed to get Key number\n");
			SysDelay(150);
			continue;
		}
	}
#endif
	//g_stSCapConfEx.KeyNumTotal = g_stSCapConfEx.KeyNum;

	FTS_TEST_DBG("CH_X = %d, CH_Y = %d, Key = %d\n", g_stSCapConfEx.ChannelXNum ,g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum );
	return ReCode;
}
/************************************************************************
* Name: FT8716_TestItem_ChannelsTest
* Brief:  Check whether TP can enter Factory Mode, and do some thing
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_ChannelsTest(bool * bTestResult)
{
	unsigned char ReCode;

	FTS_TEST_DBG("\n\n==============================Test Item: -------- Channel Test \n");

	ReCode = GetChannelNum();
	if(ReCode == ERROR_CODE_OK)
	{
		if((g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelXNum == g_stSCapConfEx.ChannelXNum)
			&& (g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelYNum == g_stSCapConfEx.ChannelYNum)
			&& (g_stCfg_FT8716_BasicThreshold.ChannelNumTest_KeyNum == g_stSCapConfEx.KeyNum))
		{
			* bTestResult = true;
			FTS_TEST_DBG("\n\nGet channels: (CHx: %d, CHy: %d, Key: %d), Set channels: (CHx: %d, CHy: %d, Key: %d)",
				g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum, 
				g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelXNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelYNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_KeyNum);

			FTS_TEST_DBG("\n//Channel Test is OK!");
		}
		else
		{
			* bTestResult = false;
			FTS_TEST_DBG("\n\nGet channels: (CHx: %d, CHy: %d, Key: %d), Set channels: (CHx: %d, CHy: %d, Key: %d)",
				g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum, 
				g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelXNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_ChannelYNum, g_stCfg_FT8716_BasicThreshold.ChannelNumTest_KeyNum);

			FTS_TEST_DBG("\n//Channel Test is NG!");
		}
	}
	return ReCode;
}
/************************************************************************
* Name: GetRawData
* Brief:  Get Raw Data of VA area and Key area
* Input: none
* Output: none
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
static unsigned char GetRawData(void)
{
	int ReCode = ERROR_CODE_OK;
	int iRow, iCol;

	memset(m_RawData, 0, TX_NUM_MAX * RX_NUM_MAX);
	memset(m_iTempRawData, 0, sizeof(m_iTempRawData));


	//--------------------------------------------Enter Factory Mode
	/*
	ReCode = EnterFactory();	
	if( ERROR_CODE_OK != ReCode ) 
	{
		FTS_TEST_DBG("Failed to Enter Factory Mode...\n");
		return ReCode;
	}

	*/

	//--------------------------------------------Check Num of Channel 
	if(0 == (g_stSCapConfEx.ChannelXNum + g_stSCapConfEx.ChannelYNum)) 
	{
		ReCode = GetChannelNum();
		if( ERROR_CODE_OK != ReCode ) 
		{
			FTS_TEST_DBG("Error Channel Num...\n");
			return ERROR_CODE_INVALID_PARAM;
		}
	}

	//--------------------------------------------Start Scanning
	FTS_TEST_DBG("Start Scan ...\n");
	ReCode = StartScan();
	if(ERROR_CODE_OK != ReCode) 
	{
		FTS_COMMON_DBG("ReCode[%d]\n", ReCode);
		FTS_TEST_DBG("Failed to Scan ...\n");
		return ReCode;
	}


	//--------------------------------------------Read RawData for Channel Area
	FTS_TEST_DBG("Read RawData...\n");
//memset(m_RawData, 0, TX_NUM_MAX * RX_NUM_MAX);
//memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
	ReCode = ReadRawData(0, 0xAD, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum * 2, m_iTempRawData);
	if( ERROR_CODE_OK != ReCode ) 
	{
		FTS_TEST_DBG("Failed to Get RawData\n");
		return ReCode;
	}

	for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow)
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol)
		{
			m_RawData[iRow][iCol] = m_iTempRawData[iRow * g_stSCapConfEx.ChannelYNum + iCol];
		}
	}

	//--------------------------------------------Read RawData for Key Area
	memset(m_iTempRawData, 0, sizeof(m_iTempRawData));
	ReCode = ReadRawData( 0, 0xAE, g_stSCapConfEx.KeyNum * 2, m_iTempRawData );
	if(ERROR_CODE_OK != ReCode) 
	{
		FTS_TEST_DBG("Failed to Get RawData\n");
		return ReCode;
	}

	for (iCol = 0; iCol < g_stSCapConfEx.KeyNum; ++iCol)
	{
		m_RawData[g_stSCapConfEx.ChannelXNum][iCol] = m_iTempRawData[iCol];
	}

	return ReCode;

}
/************************************************************************
* Name: FT8716_TestItem_RawDataTest
* Brief:  TestItem: RawDataTest. Check if MCAP RawData is within the range.
* Input: bTestResult
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_RawDataTest(bool * bTestResult)
{
	unsigned char ReCode=ERROR_CODE_OK;
	bool btmpresult = true;
	//int iMax, iMin, iAvg;
	int RawDataMin;
	int RawDataMax;
	int iValue = 0;
	int i=0;
	int iRow, iCol;

	FTS_TEST_DBG("\n\n==============================Test Item: -------- Raw Data Test\n\n");

	ReCode = WriteReg(0x06, 0x00);

	if(ReCode != ERROR_CODE_OK)
	{
		FTS_TEST_DBG("WriteReg(0x06, 00)");
		return ReCode;
	}

	//----------------------------------------------------------Read RawData
	for(i = 0 ; i < 3; i++)//Lost 3 Frames, In order to obtain stable data
		ReCode = GetRawData();
	if( ERROR_CODE_OK != ReCode ) 
	{
		FTS_TEST_DBG("Failed to get Raw Data!! Error Code: %d\n", ReCode);
		return ReCode;
	}
	//----------------------------------------------------------Show RawData

	
	FTS_PRINT("\nVA Channels: ");
	for(iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
	{
		FTS_PRINT("\nCh_%02d:  ", iRow+1);
		for(iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
		{
			FTS_PRINT("%5d, ", m_RawData[iRow][iCol]);
		}
	}
	FTS_PRINT("\nKeys:  ");
	for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; iCol++ )
	{
		FTS_PRINT("%5d, ",  m_RawData[g_stSCapConfEx.ChannelXNum][iCol]);
	}
	
	//----------------------------------------------------------To Determine RawData if in Range or not
	for(iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
	{
		for(iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
		{
			if(g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue;//Invalid Node

			RawDataMin = *(g_stDtsCapaconfig.fts_full_raw_min_cap+iRow*g_stSCapConfEx.ChannelXNum+iCol); //fullraw_lowerlimit;
			RawDataMax = *(g_stDtsCapaconfig.fts_full_raw_max_cap+iRow*g_stSCapConfEx.ChannelXNum+iCol); //fullraw_upperlimit;

			iValue = m_RawData[iRow][iCol];

			if(iValue < RawDataMin || iValue > RawDataMax)
			{
				btmpresult = false;
				FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
					iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
			}
		}
	}

	iRow = g_stSCapConfEx.ChannelXNum;
	for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; iCol++ )
	{
		if(g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol] == 0)continue;//Invalid Node
		
		RawDataMin = g_stDtsCapaconfig.singleraw_lowerlimit;
		RawDataMax = g_stDtsCapaconfig.singleraw_upperlimit;

		iValue = m_RawData[iRow][iCol];

		if(iValue < RawDataMin || iValue > RawDataMax)
		{
			btmpresult = false;
			FTS_TEST_DBG("rawdata test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
				iRow+1, iCol+1, iValue, RawDataMin, RawDataMax);
		}
	}

	FTS_COMMON_DBG("FT8716_TestItem_RawDataTest*Key*RawDataMin[%d],RawDataMax[%d]\n", RawDataMin, RawDataMax);

	//----------------------------------------------------------Return Result
	if(btmpresult)
	{
		* bTestResult = true;		
		FTS_TEST_DBG("\n\n//RawData Test is OK!\n");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("\n\n//RawData Test is NG!\n");
	}
	return ReCode;
}

/* TX2TX delta test*/
unsigned char Fts_8716_Tx2Tx_Test(bool * bTestResult)
{
	int iRow, iCol;
	bool btmpresult = true;
	unsigned char ReCode=ERROR_CODE_OK;

	for(iRow = 0; iRow<g_stSCapConfEx.ChannelXNum-1; iRow++)
	{
		for(iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
		{
			//FTS_PRINT("%5d, ", m_RawData[iRow][iCol]);
			Tx2tx_delta_buffer[iRow][iCol] = abs(m_RawData[iRow+1][iCol]-m_RawData[iRow][iCol]);
			if (g_stDtsCapaconfig.fts_tx2tx_limit[iRow*g_stSCapConfEx.ChannelXNum+iCol] < Tx2tx_delta_buffer[iRow][iCol]) {
				FTS_TEST_DBG("[%d][%d] %d Tx2tX delta is out of range", iRow, iCol, Tx2tx_delta_buffer[iRow][iCol]);
				btmpresult = false;
			}
		}
	}
	if(btmpresult)
	{
		* bTestResult = true;
		FTS_TEST_DBG("Fts_8716_Tx2Tx_Test is OK!\n");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("Fts_8716_Tx2Tx_Test is NG!\n");
	}
	return ReCode;
}

/* RX2RX delta test*/
unsigned char Fts_8716_Rx2Rx_Test(bool * bTestResult)
{
	int iRow, iCol;
	bool btmpresult = true;
	unsigned char ReCode=ERROR_CODE_OK;

	for(iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
	{
		for(iCol = 0; iCol < g_stSCapConfEx.ChannelYNum-1; iCol++)
		{
			//FTS_PRINT("%5d, ", m_RawData[iRow][iCol]);
			Rx2rx_delta_buffer[iRow][iCol] = abs(m_RawData[iRow][iCol+1]-m_RawData[iRow][iCol]);
			if (g_stDtsCapaconfig.fts_rx2rx_limit[iRow*g_stSCapConfEx.ChannelXNum+iCol] < Rx2rx_delta_buffer[iRow][iCol]) {
				FTS_TEST_DBG("[%d][%d] %d Rx2rX delta is out of range", iRow, iCol, Rx2rx_delta_buffer[iRow][iCol]);
				btmpresult = false;
			}
		}
	}
	if(btmpresult)
	{
		* bTestResult = true;
		FTS_TEST_DBG("Fts_8716_Rx2Rx_Test is OK!\n");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("Fts_8716_Rx2Rx_Test is NG!\n");
	}
	return ReCode;
}

/* CB Uniformity Test */
unsigned char Fts_TestItem_CBUniformityTest(bool * bTestResult)
{
	bool btmpresult = true;
	bool bResult = true;
	unsigned char ReCode = ERROR_CODE_OK;
	int iRow, iCol;
	int iUniform;
	int iMin = 65535, iMax = -65535;	
	int CHXLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0}, {0}};
	int CHYLinearity[TX_NUM_MAX][RX_NUM_MAX] = {{0}, {0}};

	FTS_COMMON_DBG("Start CBUniformityTest");

	/* get cb data */
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
			FTS_PRINT("%6d ", m_CBData[iRow][iCol]);
		}

		FTS_PRINT("\n");
	}

	////////////////////////////////////////////////////////////////////////
	/* Min/Max  */
	{


		for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
		{
			for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
			{				
				iMin = min( iMin, m_CBData[iRow][iCol] );
				iMax = max( iMax, m_CBData[iRow][iCol] );
			}
		}
		
		iMax = !iMax ? 1 : iMax;
		iUniform = 100 * abs(iMin) / abs(iMax);

		FTS_COMMON_DBG("Min/Max[%d] --------------------------------------------------\n\n", iUniform);

		/* 茫脨脰碌脜脨露篓 */
		if(iUniform < g_stDtsCapaconfig.cbunif_maxmin)
		{
			btmpresult = false;
		}
	}
	////////////////////////////////////////////////////////////////////////	

	FTS_COMMON_DBG("CBUniformityTest_Check_CHX*************************************************************\n\n");

	////////////////////////////////////////////////////////////////////////
	/* Check X */
	//if (g_stCfg_FT8716_BasicThreshold.CBUniformityTest_Check_CHX)
	{
		for (iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
		{
			for (iCol = 1; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
			{
				int iDeviation = abs( m_CBData[iRow][iCol] - m_CBData[iRow][iCol-1] );
				int iMax = max( abs(m_CBData[iRow][iCol]), abs(m_CBData[iRow][iCol-1]) );
				iMax = iMax ? iMax : 1;
				CHXLinearity[iRow][iCol] = 100 * iDeviation / iMax;

				FTS_PRINT("%4d", CHXLinearity[iRow][iCol]);

				/* 茫脨脰碌脜脨露篓 */
				if (CHXLinearity[iRow][iCol] > g_stDtsCapaconfig.cbunif_chx_linearity)
				{
					FTS_COMMON_DBG("CHX Fail CHXLinearity[%d], iRow[%d], iCol[%d]", CHXLinearity[iRow][iCol], iRow, iCol);
					bResult = false;
				}
			}
			
			FTS_PRINT("\n");
		}

		if( !bResult )
		{
			FTS_COMMON_DBG("CHXLinearity Fail");
			btmpresult = false;
		}
	}

	////////////////////////////////////////////////////////////////////////
	/*CheckY*/
	//if (g_stCfg_FT8716_BasicThreshold.CBUniformityTest_Check_CHY)
	{
		for (iRow = 1; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
		{
			for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
			{
				int iDeviation = abs( m_CBData[iRow][iCol] - m_CBData[iRow-1][iCol] );
				int iMax = max( abs(m_CBData[iRow][iCol]), abs(m_CBData[iRow-1][iCol]) );
				iMax = iMax ? iMax : 1;
				CHYLinearity[iRow][iCol] = 100 * iDeviation / iMax;

				FTS_PRINT("%4d", CHYLinearity[iRow][iCol]);

				/* 茫脨脰碌脜脨露篓 */
				if (CHYLinearity[iRow][iCol] > g_stDtsCapaconfig.cbunif_chy_linearity)
				{
					FTS_COMMON_DBG("CHX Fail CHXLinearity[%d], iRow[%d], iCol[%d]", CHYLinearity[iRow][iCol], iRow, iCol);
					bResult = false;
				}
			}

			FTS_PRINT("\n");

		}

		if( !bResult )
		{
			FTS_COMMON_DBG("CHYLinearity Fail");
			btmpresult = false;
		}
	}
	////////////////////////////////////////////////////////////////////////
	
	if(btmpresult)
	{
		* bTestResult = true;
		ReCode = ERROR_CODE_OK;
	}
	else
	{
		* bTestResult = false;
		ReCode = ERROR_CODE_COMM_ERROR;
	}

	return ReCode;
}

#if 0
/************************************************************************
* Name: SqrtNew
* Brief:  calculate sqrt of input.
* Input: unsigned int n
* Output: none
* Return: sqrt of n.
***********************************************************************/
static unsigned int SqrtNew(unsigned int n) 
{        
    unsigned int  val = 0, last = 0; 
    unsigned char i = 0;;
    
    if (n < 6)
    {
        if (n < 2)
        {
            return n;
        }
        return n/2;
    }   
    val = n;
    i = 0;
    while (val > 1)
    {
        val >>= 1;
        i++;
    }
    val <<= (i >> 1);
    val = (val + val + val) >> 1;
    do
    {
      last = val;
      val = ((val + n/val) >> 1);
    }while(focal_abs(val-last) > pre);
    return val; 
}
#endif


/************************************************************************
* Name: FT8716_TestItem_NoiseTest
* Brief:  TestItem: NoiseTest. Check if MCAP Noise is within the range.
* Input: bTestResult
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_NoiseTest(bool* bTestResult)
{
	unsigned char ucNoiseTestRegVal;
	unsigned int retry_Times = 0;
	unsigned int MAX_RETRY_TIMES = 50;
	unsigned char ReCode;
	bool btmpresult = true;
	int iRow,iCol;

	FTS_COMMON_DBG("\n\n==============================Test Item: -------- Noise Test  \n\n");

	//Get RawData
	memset(m_Lcd_Noise_RawData, 0, RX_NUM_MAX * TX_NUM_MAX * sizeof(int));

	/*
	ReCode = EnterFactory();
	if(ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Enter factory mode...");
		goto TEST_ERR;
	}
	*/


	ReCode = WriteReg(0x06, 1);
	
	if(ReCode != ERROR_CODE_OK)
	{
		FTS_COMMON_DBG("WriteReg(0x06, 1)");
		goto TEST_ERR;
	}
	else
	{
		FTS_COMMON_DBG("WriteReg(0x06, 1)--OK");
	}

	/* 脟氓露脕RawData buffer Cnt */
	ReCode = WriteReg(0x01, 0xad);
	
	if(ReCode != ERROR_CODE_OK)
	{
		FTS_COMMON_DBG("WriteReg(0x01, ad)");
		goto TEST_ERR;
	}
	else
	{
		FTS_COMMON_DBG("WriteReg(0x01, ad)--OK");
	}

	
	ReCode = WriteReg(0X12, MAX_NOISE_FRAMES);
	
	if(ReCode != ERROR_CODE_OK)
	{
		FTS_COMMON_DBG("WriteReg(0X12, %d)",MAX_NOISE_FRAMES);
		goto TEST_ERR;
	}
	else
	{
		FTS_COMMON_DBG("WriteReg(0X12, %d)--OK",MAX_NOISE_FRAMES);
	}

	
	ReCode = WriteReg(0X11, 1);
	
	if(ReCode != ERROR_CODE_OK)
	{
		FTS_COMMON_DBG("WriteReg(0X11, 1)");
		goto TEST_ERR;
	}
	else
	{
		FTS_COMMON_DBG("WriteReg(0X11, 1)--OK");
	}

	
	//while(ucNoiseTestRegVal != 0)
	for(retry_Times = 0; retry_Times < MAX_RETRY_TIMES; retry_Times++)
	{
		ReCode = ReadReg(0x11, &ucNoiseTestRegVal);
		
		if(ReCode != ERROR_CODE_OK)
		{	
			goto TEST_ERR;
		}

		FTS_TEST_DBG("Waiting for noise test end, get finish status retry %d times ",retry_Times);

		if(ucNoiseTestRegVal == 0)
			break;
		
		SysDelay(8);	//8ms
	}

	
	ReCode = ReadRawData(0, 0xAD, g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum * 2, m_Lcd_Noise_RawData);
	
	if( ERROR_CODE_OK != ReCode ) 
	{
		FTS_COMMON_DBG("Failed to Get NoiseData\n");
		goto TEST_ERR;
	}
	
	for(iRow = 0;iRow < (g_stSCapConfEx.ChannelXNum);iRow++)
	{
		for(iCol = 0;iCol < g_stSCapConfEx.ChannelYNum;iCol++)
		{
			FTS_PRINT("%6d", m_Lcd_Noise_RawData[iRow * g_stSCapConfEx.ChannelYNum + iCol]);
			tmp_Noise_RawData[iRow][iCol] = m_Lcd_Noise_RawData[iRow * g_stSCapConfEx.ChannelYNum + iCol] ;
			if(m_Lcd_Noise_RawData[iRow * g_stSCapConfEx.ChannelYNum + iCol] > g_stDtsCapaconfig.fts_noise_limit[iRow*g_stSCapConfEx.ChannelXNum+iCol])
			{
				FTS_TEST_DBG("[%d][%d] %d noisedata is out of range", iRow, iCol, m_Lcd_Noise_RawData[iRow * g_stSCapConfEx.ChannelYNum + iCol]);
				btmpresult = false;
			}
		}
		FTS_PRINT("\n");
	}

	if(btmpresult)
	{
		* bTestResult = true;
		FTS_TEST_DBG("\n\n//Noise Test is OK!\n");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("\n\n//Noise Test is NG!\n");
	}

	ReCode = WriteReg(0x06, 0x00);
	if(ReCode != ERROR_CODE_OK)
	{
		FTS_COMMON_DBG("WriteReg(0x06, 0)");
		goto TEST_ERR;
	}

	return ReCode;
TEST_ERR:

	* bTestResult = false;
	FTS_TEST_DBG("\n\n//Noise Test is NG!\n");	
	return ReCode;
}
/************************************************************************
* Name: FT8716_TestItem_CbTest
* Brief:  TestItem: Cb Test. Check if Cb is within the range.
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_CbTest(bool* bTestResult)
{
	bool btmpresult = true;
	unsigned char ReCode = ERROR_CODE_OK;
	int iRow = 0;
	int iCol = 0;
	int iMaxValue = 0;
	int iMinValue = 0;

	FTS_TEST_DBG("\n\n==============================Test Item: --------  CB Test\n\n");

//return ReCode;//test.

	ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNum), m_ucTempData );


//	ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNum), m_ucTempData );
	if( ERROR_CODE_OK != ReCode )
	{
		btmpresult = false;
		FTS_TEST_DBG("Failed to get CB value...\n");
		goto TEST_ERR;
	}

	memset(m_CBData, 0, TX_NUM_MAX * RX_NUM_MAX* sizeof(int));
	///VA area
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
			m_CBData_Ori[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
		}
	}
	///key
	for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; ++iCol )
	{
		m_CBData[g_stSCapConfEx.ChannelXNum][iCol] = m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ];
	}

	//------------------------------------------------Show CbData

	
	FTS_PRINT("\nVA Channels: ");
	for(iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
	{
		FTS_PRINT("\nCh_%02d:  ", iRow+1);
		for(iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
		{
			FTS_PRINT("%3d, ", m_CBData[iRow][iCol]);
		}
	}
	FTS_PRINT("\nKeys:  ");
	for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; iCol++ )
	{
		FTS_PRINT("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);
	}

	//iMinValue = g_stCfg_FT8716_BasicThreshold.CbTest_Min;
	//iMaxValue = g_stCfg_FT8716_BasicThreshold.CbTest_Max;	
	for(iRow = 0;iRow < (g_stSCapConfEx.ChannelXNum + 1);iRow++)
	{
		for(iCol = 0;iCol < g_stSCapConfEx.ChannelYNum;iCol++)
		{
			iMinValue = *(g_stDtsCapaconfig.fts_cb_min_limit+iRow*g_stSCapConfEx.ChannelXNum+iCol); //fullcb_lowerlimit;
			iMaxValue = *(g_stDtsCapaconfig.fts_cb_max_limit+iRow*g_stSCapConfEx.ChannelXNum+iCol); //fullcb_uperlimit;

			if( (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) )  
			{
				continue;
			}
			if( iRow >= g_stSCapConfEx.ChannelXNum && iCol >= g_stSCapConfEx.KeyNum ) 
			{
				continue;
			}

			if(focal_abs(m_CBData[iRow][iCol]) < iMinValue || focal_abs(m_CBData[iRow][iCol]) > iMaxValue)
			{
				btmpresult = false;
				FTS_PRINT("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) \n", \
					iRow+1, iCol+1, m_CBData[iRow][iCol], iMinValue, iMaxValue);
			}
		}
	}

	if(btmpresult)
	{
		* bTestResult = true;
		FTS_TEST_DBG("\n\n//CB Test is OK!\n");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("\n\n//CB Test is NG!\n");
	}

	return ReCode;

TEST_ERR:

	* bTestResult = false;
	FTS_TEST_DBG("\n\n//CB Test is NG!\n");
	return ReCode;	
}

void *fts_malloc(size_t size)
{
	if(FTS_MALLOC_TYPE == kmalloc_mode)
	{
		return kmalloc(size, GFP_ATOMIC);
	}
	else if (FTS_MALLOC_TYPE == vmalloc_mode)
	{
		return vmalloc(size);
	}
	else
	{
		FTS_TEST_DBG("[FTS]invalid malloc. \n");
		return NULL;
	}

	return NULL;
}
void fts_free(void *p)
{
	if(FTS_MALLOC_TYPE == kmalloc_mode)
	{
		return kfree(p);
	}
	else if (FTS_MALLOC_TYPE == vmalloc_mode)
	{
		return vfree(p);
	}
	else
	{
		FTS_TEST_DBG("[FTS]invalid free. \n");
		return ;
	}

	return ;
}


//Auto clb
static unsigned char ChipClb(unsigned char *pClbResult)
{
	unsigned char RegData=0;
	unsigned char TimeOutTimes = 100;//5s
	unsigned char ReCode = ERROR_CODE_OK;

	FTS_TEST_DBG(" ChipClb");

	ReCode = WriteReg(REG_CLB, 4);//start auto clb
	SysDelay(80);
	if(ReCode == ERROR_CODE_OK)
	{
		while(TimeOutTimes--)
		{
			//SysDelay(100);//delay 500ms
			ReCode = WriteReg(DEVIDE_MODE_ADDR, 0x04<<4);
			ReCode = ReadReg(0x04, &RegData);
			if(ReCode == ERROR_CODE_OK)
			{
				if(RegData == 0x02)
				{
					*pClbResult = 1;
					break;
				}
			}
			else
			{
				continue;
			}
			SysDelay(50);
		}

		if(TimeOutTimes == 0)
		{
			*pClbResult = 0;
		}
	}
	return ReCode;
}



/************************************************************************
* Name: FT8716_TestItem_OpenTest
* Brief:  
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_OpenTest(bool* bTestResult)
{
	bool btmpresult = true;
	unsigned char ReCode = ERROR_CODE_OK;
	unsigned char chValue=0xff;
	unsigned char chK1Value=0xff,chK2Value=0xff;
	unsigned char get_0x20_value = 0xff;
	unsigned char get_0x20_value_times = 0;
	//unsigned char chClbValue=0x04;
	int iRow = 0;
	int iCol = 0;
	int iMaxValue = 0;
	int iMinValue = 0;
	unsigned char bClbResult = 0;

	FTS_TEST_DBG("\n\n==============================Test Item: --------  Open Test\n");

	/*
	ReCode = EnterFactory();
	if(ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Enter factory mode...");
		goto TEST_ERR;
	}
	*/

	//set gip to connect VGH02/VGL02 (write 0x02 to Register 0x20)

	ReCode = ReadReg(0x20, &chValue);
	if(ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Read Reg...");
		goto TEST_ERR;
	}
	//SysDelay(50);
	SysDelay(5);

	ReCode = WriteReg(0x20, 0x02);
	if(ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Write Reg...0x20");
		goto TEST_ERR;
	}

	for(get_0x20_value_times = 0; get_0x20_value_times < 3; get_0x20_value_times++)
	{
		ReCode = ReadReg(0x20, &get_0x20_value);
		if(ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Read Reg...0x20");
			goto TEST_ERR;
		}
		FTS_TEST_DBG("Write Reg0x20 times: %d",get_0x20_value_times);
		if(get_0x20_value == 0x02)
			break;
		else
		{
			ReCode = WriteReg(0x20, 0x02);
			if(ERROR_CODE_OK != ReCode)
			{
				FTS_TEST_DBG("Failed to Write Reg...0x20");
				goto TEST_ERR;
			}
			SysDelay(5);
		}
	}

	//FTS_TEST_DBG(" k1 cycle");
	//K1 cycle(0x31)
	if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K1)
	{
		FTS_TEST_DBG(" k1 cycle");
		ReCode = ReadReg(0x31, &chK1Value);
		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Read Reg...");
			goto TEST_ERR;
		}

		SysDelay(5);
		ReCode = WriteReg(0x31, g_stCfg_FT8716_BasicThreshold.OpenTest_K1Threshold);
		//SysDelay(50);

		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Write Reg...");
			goto TEST_ERR;
		}
	}

	//FTS_TEST_DBG(" k2 cycle");
	//K2 cycle(0x32)
	if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K2)
	{
		FTS_TEST_DBG(" k2 cycle");
		ReCode = ReadReg(0x32, &chK2Value);
		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Read Reg...");
			goto TEST_ERR;
		}
		SysDelay(5);
		ReCode = WriteReg(0x32, g_stCfg_FT8716_BasicThreshold.OpenTest_K2Threshold);
		//SysDelay(50);
		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Write Reg...");
			goto TEST_ERR;
		}
	}

	//auto clb
	FTS_TEST_DBG(" ChipClb");
	SysDelay(20);
	ReCode = ChipClb( &bClbResult );
	if (ERROR_CODE_OK != ReCode )
	{
		FTS_TEST_DBG("Failed to auto clb...");
		goto TEST_ERR;
	}
	if( 0 == bClbResult)
	{
		FTS_TEST_DBG("ChipClb timeout...");
		goto TEST_ERR;
	}

	//Get CB Data
	FTS_TEST_DBG(" Get CB Data, g_stSCapConfEx.ChannelXNum:%d, g_stSCapConfEx.ChannelYNum:%d, g_stSCapConfEx.KeyNum:%d.", g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum, g_stSCapConfEx.KeyNum);
	FTS_TEST_DBG(" Get CB Data");
	ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNum), m_ucTempData );

	//ReCode = GetTxRxCB( 0, (short)(g_stSCapConfEx.ChannelXNum * g_stSCapConfEx.ChannelYNum + g_stSCapConfEx.KeyNum), m_ucTempData );
	
	if( ERROR_CODE_OK != ReCode )
	{
		btmpresult = false;
		FTS_TEST_DBG("Failed to get CB value...");
		goto TEST_ERR;
	}
	FTS_TEST_DBG(" Get CB Data end.");

	memset(m_CBData, 0, TX_NUM_MAX * RX_NUM_MAX* sizeof(int));
	///VA area
	FTS_PRINT(" set m_CBData value. ");
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for ( iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			m_CBData[iRow][iCol] = m_ucTempData[ iRow * g_stSCapConfEx.ChannelYNum + iCol ];
		}
	}
	FTS_PRINT(" key. ");
	///key
	for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; ++iCol )
	{
		m_CBData[g_stSCapConfEx.ChannelXNum][iCol] = m_ucTempData[ g_stSCapConfEx.ChannelXNum*g_stSCapConfEx.ChannelYNum + iCol ];
	}

	//------------------------------------------------Show CbData
	
	FTS_PRINT("\nVA Channels: ");
	for(iRow = 0; iRow<g_stSCapConfEx.ChannelXNum; iRow++)
	{
		FTS_PRINT("\nCh_%02d:  ", iRow+1);
		for(iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; iCol++)
		{
			FTS_PRINT("%3d, ", m_CBData[iRow][iCol]);
		}
	}
	FTS_PRINT("\nKeys:  ");
	for ( iCol = 0; iCol < g_stSCapConfEx.KeyNum; iCol++ )
	{
		FTS_PRINT("%3d, ",  m_CBData[g_stSCapConfEx.ChannelXNum][iCol]);
	}

	//
	iMinValue = g_stCfg_FT8716_BasicThreshold.OpenTest_CBMin;
	iMaxValue = 200;	
	for(iRow = 0;iRow < (g_stSCapConfEx.ChannelXNum + 1);iRow++)
	{
		for(iCol = 0;iCol < g_stSCapConfEx.ChannelYNum;iCol++)
		{
			if( (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) )  
			{
				continue;
			}
			if( iRow >= g_stSCapConfEx.ChannelXNum && iCol >= g_stSCapConfEx.KeyNum ) 
			{
				continue;
			}

			if(focal_abs(m_CBData[iRow][iCol]) < iMinValue || focal_abs(m_CBData[iRow][iCol]) > iMaxValue)
			{
				btmpresult = false;
				FTS_TEST_DBG("CB test failure. Node=(%d,  %d), Get_value=%d,  Set_Range=(%d, %d) ",\
					iRow+1, iCol+1, m_CBData[iRow][iCol], iMinValue, iMaxValue);
			}
		}
	}
	//reset reg
	ReCode = WriteReg(0x20, chValue);
	//SysDelay(50);
	if (ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Write Reg...");
		goto TEST_ERR;
	}

	FTS_TEST_DBG(" OpenTest_Check_K1.");
	if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K1)
	{
		ReCode = WriteReg(0x31, chK1Value);
		SysDelay(10);
		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Write Reg...");
			goto TEST_ERR;
		}
	}
	FTS_TEST_DBG(" OpenTest_Check_K2.");	
	if (g_stCfg_FT8716_BasicThreshold.OpenTest_Check_K2)
	{
		ReCode = WriteReg(0x32, chK2Value);
		SysDelay(10);
		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to Write Reg...");
			goto TEST_ERR;
		}
	}
	if(btmpresult)
	{
		* bTestResult = true;
		FTS_TEST_DBG("\n\n//Open Test is OK!");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("\n\n//Open Test is NG!");
	}

	return ReCode;

TEST_ERR:
  	ReCode = WriteReg(0x20, chValue);
	* bTestResult = false;
	FTS_TEST_DBG("\n\n//Open Test is NG!");
	return ReCode;	
}

static unsigned char WeakShort_GetAdcData( int AllAdcDataLen, int *pRevBuffer  )
{
	#if 0
	unsigned char ReCode = ERROR_CODE_COMM_ERROR;
	int iReadDataLen = AllAdcDataLen;//Offset*2 + (ClbData + TxNum + RxNum)*2*2
	unsigned char *pDataSend = NULL;

	unsigned char Data = 0xff;
	int i = 0;
	bool bAdcOK = false;

	FTS_TEST_DBG("\n");

	pDataSend = fts_malloc(iReadDataLen + 1);
	if(pDataSend == NULL)	return ERROR_CODE_ALLOCATE_BUFFER_ERROR;
	memset( pDataSend, 0, iReadDataLen + 1 );

	ReCode = WriteReg(0x07, 0x01);
	if(ReCode != ERROR_CODE_OK)
	{
		FTS_TEST_DBG("WriteReg error. \n");
		return ReCode;
	}

	SysDelay(100);

	for(i = 0; i < 100*5; i++)
	{
		SysDelay(10);
		ReCode = ReadReg(0x07, &Data);
		if(ReCode == ERROR_CODE_OK)
		{
			if(Data == 0)
			{
				bAdcOK = true;
				break;
			}
		}
	}

	if( !bAdcOK )
	{
		FTS_TEST_DBG("ADC data NOT ready.  error.\n");
		ReCode = ERROR_CODE_COMM_ERROR;
		goto EndGetAdc;
	}
	SysDelay(300);	//	frank add. 20160517
	pDataSend[0] = 0xF4;

//if( 0 == iCommMode )        ReCode = HY_IIC_IO(hDevice, pDataSend, 1, pDataSend + 1, iReadDataLen);
       ReCode = Comm_Base_IIC_IO(pDataSend, 1, pDataSend + 1, iReadDataLen);
	if(ReCode == ERROR_CODE_OK)
	{
		FTS_PRINT("\n Adc Data:\n");
		for(i = 0; i < iReadDataLen/2; i++)
		{
			pRevBuffer[i] = (pDataSend[1 + 2*i]<<8) + pDataSend[1 + 2*i + 1];
			FTS_PRINT("%d,    ", pRevBuffer[i]);	//	FTS_PRINT("pRevBuffer[%d]:%d,    ", i, pRevBuffer[i]);
		}
		FTS_PRINT("\n");
	}
	else
	{
		FTS_TEST_DBG("Comm_Base_IIC_IO error. error:%d. \n", ReCode);
	}

EndGetAdc:
	if(pDataSend != NULL)
	{
		fts_free(pDataSend);
		pDataSend = NULL;
	}

	FTS_TEST_DBG(" END.\n");

	return ReCode;
#else
	unsigned char ReCode = ERROR_CODE_OK;
	unsigned short usReturnNum = 0;
	unsigned short usTotalReturnNum = 0;
	int i=1;
	 int index = 0;
	unsigned char RegMark = 0;
	int iReadNum = AllAdcDataLen / BYTES_PER_TIME;
	unsigned char wBuffer[2];	
	//unsigned char pReadBuffer[80 * 80 * 2];
	int iReadDataLen = AllAdcDataLen;//Offset*2 + (ClbData + TxNum + RxNum)*2*2
	unsigned char *pReadBuffer = NULL;

	pReadBuffer = fts_malloc(iReadDataLen + 1);	
	if(pReadBuffer == NULL)
	{
		FTS_TEST_DBG(" pReadBuffer malloc failed .  error.\n");
		return ERROR_CODE_ALLOCATE_BUFFER_ERROR;
	}
	memset( pReadBuffer, 0, iReadDataLen + 1);

	memset( wBuffer, 0, sizeof(wBuffer) );

	if((AllAdcDataLen % BYTES_PER_TIME) > 0) ++iReadNum;

	wBuffer[0] = 0x89;
	ReCode = WriteReg( 0x0F, 1 );

	for ( index = 0; index < 250; ++index )
	{
		//SysDelay( 50 );
		SysDelay( 10 );
		ReCode = ReadReg( 0x10, &RegMark );
		if( ERROR_CODE_OK == ReCode && 0 == RegMark )
			break;
	}

	if( 250 == index )
	{
		FTS_TEST_DBG("ADC data NOT ready.  error.\n");
		ReCode = ERROR_CODE_COMM_ERROR;
		goto EndGetAdc;
	}
	
	if(1)
	{
		usTotalReturnNum = 0;

		usReturnNum = BYTES_PER_TIME;
		if(ReCode == ERROR_CODE_OK)
		{
			ReCode = Comm_Base_IIC_IO(wBuffer, 1, pReadBuffer, usReturnNum);
		}

		for(i=1; i<iReadNum; i++)
		{
			if(ReCode != ERROR_CODE_OK) break;

			if(i==iReadNum-1)//last packet
			{
				usReturnNum = AllAdcDataLen-BYTES_PER_TIME*i;
				ReCode = Comm_Base_IIC_IO(NULL, 0, pReadBuffer+BYTES_PER_TIME*i, usReturnNum);
			}
			else
			{
				usReturnNum = BYTES_PER_TIME;
				ReCode = Comm_Base_IIC_IO( NULL, 0, pReadBuffer+BYTES_PER_TIME*i, usReturnNum);
			}
		}
	}


	for (index = 0; index < AllAdcDataLen/2; ++index )
	{
		pRevBuffer[index] = (pReadBuffer[index * 2] << 8) + pReadBuffer[index * 2 + 1];
	}
	
EndGetAdc:
	if(pReadBuffer != NULL)
	{
		fts_free(pReadBuffer);
		pReadBuffer = NULL;
	}

	return ReCode;
#endif
}


/************************************************************************
* Name: FT8716_TestItem_ShortCircuitTest
* Brief:  
* Input: none
* Output: bTestResult, PASS or FAIL
* Return: Comm Code. Code = 0x00 is OK, else fail.
***********************************************************************/
unsigned char FT8716_TestItem_ShortCircuitTest(bool* bTestResult)
{
	bool btmpresult = true;
	unsigned char ReCode = ERROR_CODE_OK;
	int ResMin = 0;
	int tmpAdc = 0;
	int iRow = 0;
	int iCol = 0;
	int i = 0;
	int iAllAdcDataNum = 0;
	unsigned char iTxNum = 0, iRxNum = 0, iChannelNum = 0;	

	FTS_TEST_DBG("\n\n==============================Test Item: --------  Short Circuit Test\n");

	/*
	ReCode = EnterFactory();
	if(ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Enter factory mode...");
		goto TEST_ERR;
	}
	*/
	ReCode = ReadReg(0x02, &iTxNum);
	ReCode = ReadReg(0x03, &iRxNum);
	if (ERROR_CODE_OK != ReCode)
	{
		FTS_TEST_DBG("Failed to Read Reg...");
		goto TEST_ERR;
	}
	FTS_TEST_DBG(" iTxNum:%d, iRxNum:%d.  ", iTxNum, iRxNum);

	iChannelNum = iTxNum + iRxNum;
	iAllAdcDataNum = iTxNum * iRxNum + g_stSCapConfEx.KeyNumTotal;

	memset(m_iAdcData, 0, sizeof(m_iAdcData));

	for (i=0; i<1; i++)
	{
		ReCode =  WeakShort_GetAdcData(iAllAdcDataNum*2,m_iAdcData);
		//SysDelay(50);
		if (ERROR_CODE_OK != ReCode)
		{
			FTS_TEST_DBG("Failed to get AdcData...");
			goto TEST_ERR;
		}
	}
	
	ResMin = g_stCfg_FT8716_BasicThreshold.ShortCircuit_ResMin;
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum + 1; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			if( (0 == g_stCfg_MCap_DetailThreshold.InvalidNode[iRow][iCol]) )  
			{
				continue;
			}
			if( iRow >= g_stSCapConfEx.ChannelXNum && iCol >= g_stSCapConfEx.KeyNum ) 
			{
				continue;
			}
			
			tmpAdc = m_iAdcData[iRow *iRxNum + iCol];
			if (2047 == tmpAdc)	tmpAdc = 2046;
			m_iTempData[iRow][iCol] = (tmpAdc * 100) / (2047 - tmpAdc);

			FTS_PRINT("%6d", m_iTempData[iRow][iCol]);

			if(ResMin > m_iTempData[iRow][iCol])
			{
				btmpresult = false;
			}
		}

		FTS_PRINT("\n");
	}

	//print ADC and Resistance Data
	FTS_PRINT("ADC Data: \n");
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum + 1; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			FTS_PRINT("%d  ",m_iAdcData[iRow *iRxNum + iCol]);
		}
		FTS_PRINT("\n");
	}	
	FTS_PRINT("Resistance Data: \n");
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum + 1; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			FTS_PRINT("%d  ",m_iTempData[iRow][iCol]);
		}
		FTS_PRINT("\n");
	}

	if(btmpresult)
	{
		* bTestResult = true;
		FTS_TEST_DBG("\n\n//Short Circuit Test is OK!");
	}
	else
	{
		* bTestResult = false;
		FTS_TEST_DBG("\n\n//Short Circuit Test is NG!");
	}

	return ReCode;

TEST_ERR:

	* bTestResult = false;
	FTS_TEST_DBG("\n\n//Short Circuit Test is NG!");
	return ReCode;	
}

/*static int fts_get_average(int data[TX_NUM_MAX][RX_NUM_MAX], int itx_num, int irx_num)
{
	int average = 0;
	int sum = 0;
	int i = 0;
	int j = 0;

	for(i=0; i<itx_num; i++)
	{
		for(j=0; j<irx_num; j++)
		{
			sum+=data[i][j];
		}
	}

	if(itx_num*irx_num > 0) average = sum/(itx_num*irx_num);
	else average = -1;

	return average;
}*/

#if 0
static int fts_get_min(int data[TX_NUM_MAX][RX_NUM_MAX], int itx_num, int irx_num)
{
	int min = 0;
	int i = 0;
	int j = 0;

	min = data[0][0];
	for(i=0; i<itx_num; i++)
	{
		for(j=0; j<irx_num; j++)
		{
			if(min>data[i][j]) min = data[i][j];
		}
	}

	return min;
}
#endif


void fts_get_test_result(char *result_buf, char *rawdata_buf, int buf_size)
{
	unsigned char ucDevice = 0;
	int iItemCount=0;
	int iRow = 0;
	int iCol = 0;
//	int min = 0;
	char buf[128];
	int cur_len = 0;
	bool result_test = true;

	
	for(iItemCount = 0; iItemCount < g_TestItemNum; iItemCount++)
	{
		m_ucTestItemCode = g_stTestItem[ucDevice][iItemCount].ItemCode;

		///////////////////////////////////////////////////////FT8716_CHANNEL_NUM_TEST
		if(Code_FT8716_CHANNEL_NUM_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "0P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "0F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		///////////////////////////////////////////////////////FT8716_RAWDATA_TEST

		if(Code_FT8716_RAWDATA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "1P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "1F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		///////////////////////////////////////////////////////FT8716_CB_TEST

		if(Code_FT8716_CB_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "2P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "2F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		///////////////////////////////////////////////////////FT8716_CB_UNIFORMITY_TEST

		if(Code_FT8716_CB_UNIFORMITY_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "3P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "3F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		///////////////////////////////////////////////////////FT8716_Noise_TEST
		if(Code_FT8716_NOISE_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "4P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "4F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		///////////////////////////////////////////////////////FT8716_OPEN_TEST
		if(Code_FT8716_OPEN_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "5P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "5F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		///////////////////////////////////////////////////////FT8716_SHORT_TEST
		if(Code_FT8716_SHORT_CIRCUIT_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "6P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "6F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		if(Code_FT8716_TX2TX_DELTA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "7P-", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "7F-", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

		if(Code_FT8716_RX2RX_DELTA_TEST == g_stTestItem[ucDevice][iItemCount].ItemCode)
		{
			if(RESULT_PASS == g_stTestItem[ucDevice][iItemCount].TestResult)
			{
				strncat(result_buf, "8P", FTS_LEN_MAX_STR-cur_len);
			}
			else
			{
				strncat(result_buf, "8F", FTS_LEN_MAX_STR-cur_len);
				result_test = false;
			}
			cur_len=strlen(result_buf);
		}

	}
	if (g_stEnableTestItems_DTS.Enable_CBTest == false ||
		g_stEnableTestItems_DTS.Enable_NOISETest == false ||
		g_stEnableTestItems_DTS.Enable_RAWDATATest == false ||
		g_stEnableTestItems_DTS.Enable_RX2RXTest == false ||
		g_stEnableTestItems_DTS.Enable_TX2TXTest == false)
	{
		strncat(result_buf, "-tp_initial_failed;", FTS_LEN_MAX_STR-cur_len);
	}
	if (result_test == false &&
		g_stEnableTestItems_DTS.Enable_CBTest == true &&
		g_stEnableTestItems_DTS.Enable_NOISETest == true &&
		g_stEnableTestItems_DTS.Enable_RAWDATATest == true &&
		g_stEnableTestItems_DTS.Enable_RX2RXTest == true &&
		g_stEnableTestItems_DTS.Enable_TX2TXTest == true)
	{
		strncat(result_buf, "-panel_reason;", FTS_LEN_MAX_STR-cur_len);
	}
	FTS_COMMON_DBG("Result:%s", result_buf);

	snprintf(buf, sizeof(buf),"\ntx num:%d, rx num:%d \n", g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum);
	strncat(result_buf, buf, FTS_LEN_MAX_STR-cur_len);
	/*RAWDATA */
	cur_len = 0;
	//min = fts_get_min(m_RawData, g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum);
	snprintf(buf, sizeof(buf),"RawData:\n");
	strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			//snprintf(buf, sizeof(buf), "%d,", m_RawData[iRow][iCol] - min);
			snprintf(buf, sizeof(buf), "%d,", m_RawData[iRow][iCol]);
			cur_len=strlen(rawdata_buf);
			strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
		}
		snprintf(buf, sizeof(buf), "\n");
		cur_len=strlen(rawdata_buf);
		strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	}
	/*CBDATA */
	//min = fts_get_min(m_CBData_Ori, g_stSCapConfEx.ChannelXNum, g_stSCapConfEx.ChannelYNum);
	snprintf(buf, sizeof(buf),"CBData:\n");
	cur_len=strlen(rawdata_buf);
	strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			//snprintf(buf, sizeof(buf), "%d,", m_CBData_Ori[iRow][iCol] - min);
			snprintf(buf, sizeof(buf), "%d,", m_CBData_Ori[iRow][iCol]);
			cur_len=strlen(rawdata_buf);
			strncat(rawdata_buf, buf, (PAGE_SIZE*4)-cur_len);
		}
		snprintf(buf, sizeof(buf), "\n");
		cur_len=strlen(rawdata_buf);
		strncat(rawdata_buf, buf, (PAGE_SIZE*4)-cur_len);
	}

	/* Noise Data */
	snprintf(buf, sizeof(buf),"NoiseData:\n");
	cur_len=strlen(rawdata_buf);
	strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			snprintf(buf, sizeof(buf), "%d,", m_Lcd_Noise_RawData[iRow * g_stSCapConfEx.ChannelYNum + iCol]);
			cur_len=strlen(rawdata_buf);
			strncat(rawdata_buf, buf, (PAGE_SIZE*4)-cur_len);
		}
		snprintf(buf, sizeof(buf), "\n");
		cur_len=strlen(rawdata_buf);
		strncat(rawdata_buf, buf, (PAGE_SIZE*4)-cur_len);
	}

	/* tx2tx delta data */
	snprintf(buf, sizeof(buf),"Tx2TxData:\n");
	cur_len=strlen(rawdata_buf);
	strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum-1; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum; ++iCol )
		{
			snprintf(buf, sizeof(buf), "%d,", Tx2tx_delta_buffer[iRow][iCol]);
			cur_len=strlen(rawdata_buf);
			strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
		}
		snprintf(buf, sizeof(buf), "\n");
		cur_len=strlen(rawdata_buf);
		strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	}
	/* Rx2rx delta data */
	snprintf(buf, sizeof(buf),"Rx2RxData:\n");
	cur_len=strlen(rawdata_buf);
	strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	for ( iRow = 0; iRow < g_stSCapConfEx.ChannelXNum; ++iRow )
	{
		for (iCol = 0; iCol < g_stSCapConfEx.ChannelYNum-1; ++iCol )
		{
			snprintf(buf, sizeof(buf), "%d,", Rx2rx_delta_buffer[iRow][iCol]);
			cur_len=strlen(rawdata_buf);
			strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
		}
		snprintf(buf, sizeof(buf), "\n");
		cur_len=strlen(rawdata_buf);
		strncat(rawdata_buf, buf, 4*PAGE_SIZE-cur_len);
	}

}
