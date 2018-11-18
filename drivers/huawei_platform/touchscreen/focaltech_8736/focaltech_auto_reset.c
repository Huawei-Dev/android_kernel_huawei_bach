/*
 *
 * FocalTech TouchScreen driver.
 * 
 * Copyright (c) 2010-2016, FocalTech Systems, Ltd., all rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

 /************************************************************************
*
* File Name: focaltech_apk_node.c
*
* Author:	   Xu YF & ZR, Software Department, FocalTech
*
* Created: 2016-03-18
*   
* Modify:
*
* Abstract: 
* 启动ESD保护，在一定的间隔时间检查一下是否需要重启TP，
* 但在有其他情况下使用IIC通信期间，不能启动ESD保护，
* 避免因同步问题造成其他错误发生。
*
************************************************************************/

/*******************************************************************************
* Included header files
*******************************************************************************/
#include <linux/kthread.h>
#include "focaltech_comm.h"
#include "Global.h"
#include "ini.h"

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
#define FOCALTECH_AUTO_RESET_INFO  "File Version of  focaltech_auto_reset.c:  V1.0.0 2016-03-18"

#define FTS_AUTO_RESET_EN			1//0:disable, 1:enable

#define AUTO_RESET_WAIT_TIME 		2000//ms

//#define HAS_ESD_PROTECT_FUNCTION			0//0:not define, 1: defined

/* ESD检查的寄存器 */
#define FTS_ESD_USE_REG_ADDR				0xED // fw中设定为0xED
#define FTS_ESD_USE_REG_VALUE				0x87

#define FTS_ESD_NORMAL_VALUE				0x55
#define FTS_ESD_ERROR_VALUE				0xAA


/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
#if 0
	static void force_reset_guitar(void);
	void gtp_esd_check_func(void);
#endif


/*******************************************************************************
* Static variables
*******************************************************************************/
static struct timeval g_last_comm_time;//the Communication time of i2c RW 	
static struct task_struct *thread_auto_reset = NULL;
static int g_thread_stop_flag = 0;//

static DECLARE_WAIT_QUEUE_HEAD(auto_reset_waiter);

static int g_start_auto_reset = 0;//是否可以开始启动ESD保护，0表示不能，1表示可以
static int g_auto_reset_use_i2c = 0;//
//static int g_auto_reset_checking = 0;//

static unsigned char g_esd_used_reg_addr = FTS_ESD_USE_REG_ADDR;//chip id address
static unsigned char g_esd_used_reg_value = FTS_ESD_USE_REG_VALUE;//chip id value

/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
unsigned char g_Fts_Tp_To_Lcd_Rst_Flag = FTS_ESD_NORMAL_VALUE; //TP通知LCD需要Rst

/*******************************************************************************
* Static function prototypes
*******************************************************************************/
static int fts_auto_reset_thread(void *unused);
static int fts_auto_reset_check(void);

static void fts_esd_check_func(void);

/*******************************************************************************
* functions body
*******************************************************************************/
int fts_auto_reset_init(void)
{
	int err = 0;

	if(0 == FTS_AUTO_RESET_EN)
		return 0;
	
	FTS_COMMON_DBG("[focal] %s", FOCALTECH_AUTO_RESET_INFO);	//show version

	g_thread_stop_flag = 0;
	
	g_start_auto_reset = 1;
	g_auto_reset_use_i2c = 0;
	//g_auto_reset_checking = 0;
	
	do_gettimeofday(&g_last_comm_time);
	
	thread_auto_reset = kthread_run(fts_auto_reset_thread, 0, "focal_auto_reset");
	if (IS_ERR(thread_auto_reset))
	{
		err = PTR_ERR(thread_auto_reset);
		FTS_COMMON_DBG("failed to create kernel thread: %d.", err);
	}

	//init esd used reg
	/*
	if(fts_upgrade_info_curr.chip_id > 0x00 && fts_upgrade_info_curr.chip_id < 0xff)
		g_esd_used_reg_value = fts_upgrade_info_curr.chip_id;
	*/
	FTS_COMMON_DBG("esd used reg, addr = 0x%02x, data = 0x%02x", 
		g_esd_used_reg_addr, g_esd_used_reg_value);
	
	return 0;	
}
int fts_auto_reset_exit(void)
{
	if(0 == FTS_AUTO_RESET_EN)
		return 0;

	g_thread_stop_flag = 1;
	wake_up_interruptible(&auto_reset_waiter);
	msleep(AUTO_RESET_WAIT_TIME);
	//kthread_stop(thread_auto_reset);
	return 0;
}
static int fts_auto_reset_thread(void *unused)
{
	unsigned int iDeltaTime=0;
	unsigned long uljiffies=0;
	struct timeval tv;

	struct sched_param param = { .sched_priority = 5 };
	sched_setscheduler(current, SCHED_RR, &param); 
	uljiffies=msecs_to_jiffies(AUTO_RESET_WAIT_TIME+20);
		
	do
	{
		/*设置等待超时*/
		wait_event_interruptible_timeout(auto_reset_waiter, 1==g_thread_stop_flag, uljiffies);

		if(1==g_thread_stop_flag) break;
		
		/*是否可以开始启动ESD保护*/
		if(0 == g_start_auto_reset)
			continue;
		/*记录当前时间*/
		do_gettimeofday(&tv);
		iDeltaTime = ((tv.tv_sec - g_last_comm_time.tv_sec)*MSEC_PER_SEC + (tv.tv_usec - g_last_comm_time.tv_usec)/1000)/1000;

		/*当前时间与最后一次其他情况下使用IIC通信的时间，相比较，超过一定时间则执行ESD保护检查*/
		if((AUTO_RESET_WAIT_TIME/1000) < iDeltaTime)
		{
			//FTS_DBG("EsdCheck iDeltaTime(ms) %d \n", iDeltaTime);
			
			fts_auto_reset_check();								
		}
	}while(!kthread_should_stop());

	return 0;
}

/* 
*   
*记录其他IIC通信开始的时间
*   
*/
int fts_auto_reset_record_time(void)
{
	int i = 0;

	if(0 == FTS_AUTO_RESET_EN)
		return 0;

	/*如果是ESD保护在使用IIC通信，则退出*/
	if(1 == g_auto_reset_use_i2c)
	{	
		/*检查当前是否有ESD保护在使用IIC通信*/
		for(i = 0; i < 10; i++)
		{
			if(0 == g_auto_reset_use_i2c)
				break;
			msleep(2);
			FTS_COMMON_DBG("1 == g_auto_reset_use_i2c");
		}
		if(i == 10)
		{
			FTS_COMMON_DBG("Failed to read/write i2c,  the i2c communication has been accounted for ESD PROTECTION.\n");
			return -1;
		}
	}
	
	/*只要有其他地方使用了IIC通信，ESD保护则可以开始*/
	//if(0 == g_start_auto_reset)
	//	g_start_auto_reset = 1;
	
	/*记录其他IIC通信开始的时间*/
	do_gettimeofday(&g_last_comm_time);
	
		
	return 0;
}

static int fts_auto_reset_check(void)
{
	g_auto_reset_use_i2c = 1;

	//FTS_COMMON_DBG("plese implement ESD PROTECTION function.\n");
	//调用ESD保护检查函数
	fts_esd_check_func();

	g_auto_reset_use_i2c = 0;
	return 0;
}

int  fts_auto_reset_suspend(void)
{
	g_start_auto_reset = 0;
	return 0;
}

int  fts_auto_reset_resume(void)
{
	g_start_auto_reset = 1;
	return 0;
}

unsigned char get_fts_tp_to_lcd_reset_flag(void)
{
	return g_Fts_Tp_To_Lcd_Rst_Flag;
}

void set_fts_tp_esd_flag_normal(void)
{
	g_Fts_Tp_To_Lcd_Rst_Flag = FTS_ESD_NORMAL_VALUE;
}

int is_fts_tp_esd_error(void)
{
	if (g_Fts_Tp_To_Lcd_Rst_Flag == FTS_ESD_NORMAL_VALUE)
		return 0;
	else
		return -1;
}

 /************************************************************************
 * Name: fts_esd_check_func
 * Brief: esd check function
 * Input: struct work_struct
 * Output: no
 * Return: 0
 ***********************************************************************/
static void fts_esd_check_func(void)
{
	int ret = -1;
	u8 uc_data_tmp = 0;

	ret = fts_read_reg_for_esd(g_esd_used_reg_addr,&uc_data_tmp);

	if (ret<0) 
	{
		FTS_COMMON_DBG("read value fail.");	 
	}
	else if (uc_data_tmp == FTS_ESD_ERROR_VALUE)
	{
		/* notify lcd driver rst */
		g_Fts_Tp_To_Lcd_Rst_Flag = FTS_ESD_ERROR_VALUE;
		ERROR_COMMON_FTS("Tp Rawdata Error Tp_To_Lcd_Rst_Flag[%d]", g_Fts_Tp_To_Lcd_Rst_Flag);

		fts_write_reg_for_esd(0xED, FTS_ESD_NORMAL_VALUE);
	}

	return;
}
