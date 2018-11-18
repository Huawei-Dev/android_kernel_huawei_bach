/*
 * Japan Display Inc. INPUT_MT_WRAPPER Device Driver
 *
 * Copyright (C) 2013-2015 Japan Display Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA  02110-1301, USA.
 *
 */
#ifndef _INPUT_MT_WRAPPER_H_
#define _INPUT_MT_WRAPPER_H_

#include <huawei_platform/log/hw_log.h>

#define INPUT_MT_WRAPPER_GAIN (100)
#define INPUT_MT_WRAPPER_MAX_FINGERS (10)
#define INPUT_MT_WRAPPER_MIN_X (0)
#define INPUT_MT_WRAPPER_MAX_X (1079)
#define INPUT_MT_WRAPPER_MIN_Y (0)
#define INPUT_MT_WRAPPER_MAX_Y (1919)
#define INPUT_MT_WRAPPER_MIN_Z (0)
#define INPUT_MT_WRAPPER_MAX_Z (INPUT_MT_WRAPPER_GAIN)
#define INPUT_MT_WRAPPER_MIN_MAJOR (0)
#define INPUT_MT_WRAPPER_MAX_MAJOR (1)
#define INPUT_MT_WRAPPER_MIN_MINOR (0)
#define INPUT_MT_WRAPPER_MAX_MINOR (1)
#define INPUT_MT_WRAPPER_MIN_ORIENTATION (-90*INPUT_MT_WRAPPER_GAIN)
#define INPUT_MT_WRAPPER_MAX_ORIENTATION  (90*INPUT_MT_WRAPPER_GAIN)
#define INPUT_MT_WRAPPER_TOOL_TYPE_MIN (0)
#define INPUT_MT_WRAPPER_TOOL_TYPE_MAX (1)
#define INPUT_MT_WRAPPER_TOOL_TYPE_FINGER (0)
#define INPUT_MT_WRAPPER_TOOL_TYPE_STYLUS (1)

enum input_mt_wrapper_state
{
    INPUT_MT_WRAPPER_STATE_DEFAULT,
    INPUT_MT_WRAPPER_STATE_FIRST_TOUCH = 1,
    INPUT_MT_WRAPPER_STATE_LAST_TOUCH = 2,
    INPUT_MT_WRAPPER_STATE_SAME_REPORT,
    INPUT_MT_WRAPPER_STATE_SAME_ZERO_REPORT,
};

struct input_mt_wrapper_touch_data
{
    unsigned char down;
    unsigned char valid; /* 0:invalid !=0:valid */
    int x;
    int y;
    int pressure;
    int tracking_id;
    int shape;
    int major;
    int minor;
    int orientation;
    unsigned int tool_type;
};

struct input_mt_wrapper_ioctl_touch_data
{
    struct input_mt_wrapper_touch_data touch[INPUT_MT_WRAPPER_MAX_FINGERS];
    enum input_mt_wrapper_state state;
    int t_num;
    int down_num;
};

/* commands */
#define INPUT_MT_WRAPPER_IO_TYPE  (0xB9)
#define INPUT_MT_WRAPPER_IOCTL_CMD_SET_COORDINATES \
    _IOWR(INPUT_MT_WRAPPER_IO_TYPE, 0x01, \
          struct input_mt_wrapper_ioctl_touch_data)

int input_mt_wrapper_init(void);
void input_mt_wrapper_exit(void);

/* external varible*/
extern u8 host_ts_log_cfg;

#define HWLOG_TAG	HOST_TS
HWLOG_REGIST();
#define HOST_LOG_INFO(x...)		_hwlog_info(HWLOG_TAG, ##x)
#define HOST_LOG_ERR(x...)		_hwlog_err(HWLOG_TAG, ##x)
#define HOST_LOG_DEBUG(x...)	\
    do { \
        if (host_ts_log_cfg)	   \
            _hwlog_info(HWLOG_TAG, ##x);	\
    } while (0)

#endif /* _INPUT_MT_WRAPPER_H_ */
