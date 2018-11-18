/* < DTS2014XXXXXXXXX chengfeifei 20141127 begin */
/*
 *
 * Copyright (C) 2013 HUAWEI, Inc.
 *File Name: kernel/include/linux/hw_power_monitor.h
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __HW_POWER_MONITOR_H__
#define __HW_POWER_MONITOR_H__

/* define id of radar scene */
typedef enum {
	AP_SLEEP = 0,
	MODEM_SLEEP,
	SUSPEND_FAILED,
	FREEZING_FAILED,
	WAKEUP_ABNORMAL,
	DRIVER_ABNORMAL,
	WAKEUP_IRQ,
	WAKEUP_GPIO,
	POWER_MONITOR_MAX,
} POWER_MONITOR_ID;

/* Maximum length of power monitor data */
#define MONITOR_BUFF_SIZE	( 128 )
#define BUFF_SIZE			( 64 )

/*Maximum length of wake up name*/
#define WAKE_UP_NAME_SIZE   (45)
#define WAKE_UP_RECORD_MAX_NUM   (15)

/*Maximum length of wake gpio name*/
#define WAKE_UP_GPIO_SIZE   (5)
#define GPIO_RECORD_MAX_NUM   (10)

/* define struct of power_monitor info */
struct power_monitor_info {
	u32 idx;			/* number of power monitor scene */
	u32 count;			/* count of error in total */
	pid_t pgid;			/* pid value of process */
	size_t len;			/* length of string buffer */
	char buffer[MONITOR_BUFF_SIZE];	/* buffer of power monitor info */
};

extern bool freezing_wakeup_check;

struct wake_up_info{
    char wake_up_name[WAKE_UP_NAME_SIZE]; /*buffer of wake up name*/
    u32 count;                                                       /*times of wake up*/
};


struct wake_gpio_info{
    char wake_gpio_name[WAKE_UP_GPIO_SIZE]; /*buffer of wake up gpio name*/
    u32 count;                                                         /*times of wake up*/
};

int power_monitor_report(unsigned int id, const char *fmt, ...);

#endif
/* DTS2014XXXXXXXXX chengfeifei 20141127 end > */
