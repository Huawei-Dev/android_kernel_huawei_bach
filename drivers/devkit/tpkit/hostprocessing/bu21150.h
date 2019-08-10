/*
 * Japan Display Inc. BU21150 touch screen driver.
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
#ifndef _BU21150_H_
#define _BU21150_H_
#include "input_mt_wrapper.h"
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <asm/byteorder.h>
#include <linux/timer.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/amba/pl022.h>
#include <linux/platform_device.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#define HOST_CHARGER_FB	/*define HOST_CHARGER_FB here to enable charger notify callback*/
#if defined(HOST_CHARGER_FB)
#include <linux/hisi/usb/hisi_usb.h>
#endif

/* return value */
#define BU21150_UNBLOCK		(5)
#define BU21150_TIMEOUT		(6)

/* ioctl(IOCTL_CMD_RESET) */
#define BU21150_RESET_LOW	(0)
#define BU21150_RESET_HIGH	(1)

/* ioctl(IOCTL_CMD_SET_TIMEOUT) */
#define BU21150_TIMEOUT_DISABLE (0)
#define BU21150_TIMEOUT_ENABLE	(1)

/* ioctl(IOCTL_CMD_SET_POWER) */
#define BU21150_POWER_OFF	(0)
#define BU21150_POWER_ON	(1)

struct anti_false_touch_param{
	int feature_all;
	int feature_resend_point;
	int feature_orit_support;
	int feature_reback_bt;
	int lcd_width;
	int lcd_height;
	int click_time_limit;
	int click_time_bt;
	int edge_position;
	int edge_postion_secondline;
	int bt_edge_x;
	int bt_edge_y;
	int move_limit_x;
	int move_limit_y;
	int move_limit_x_t;
	int move_limit_y_t;
	int move_limit_x_bt;
	int move_limit_y_bt;
	int edge_y_confirm_t;
	int edge_y_dubious_t;
	int edge_y_avg_bt;
	int edge_xy_down_bt;
	int edge_xy_confirm_t;
	int max_points_bak_num;

	/* emui5.1 new support */
	int feature_sg;
	int sg_min_value;
	int feature_support_list;
	int max_distance_dt;
	int feature_big_data;
	int feature_click_inhibition;
	int min_click_time;

	//for driver
	int drv_stop_width;
	int sensor_x_width;
	int sensor_y_width;

	/* if x > drv_stop_width, and then the same finger x < drv_stop_width, report it */
	int edge_status;
};

//#define ANTI_FALSE_TOUCH_STRING_NUM 27
#define ANTI_FALSE_TOUCH_FEATURE_ALL "feature_all"
#define ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT "feature_resend_point"
#define ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT "feature_orit_support"
#define ANTI_FALSE_TOUCH_FEATURE_REBACK_BT "feature_reback_bt"
#define ANTI_FALSE_TOUCH_LCD_WIDTH "lcd_width"
#define ANTI_FALSE_TOUCH_LCD_HEIGHT "lcd_height"
#define ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT "click_time_limit"
#define ANTI_FALSE_TOUCH_CLICK_TIME_BT "click_time_bt"
#define ANTI_FALSE_TOUCH_EDGE_POISION "edge_position"
#define ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE "edge_postion_secondline"
#define ANTI_FALSE_TOUCH_BT_EDGE_X "bt_edge_x"
#define ANTI_FALSE_TOUCH_BT_EDGE_Y "bt_edge_y"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_X "move_limit_x"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_Y "move_limit_y"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T "move_limit_x_t"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T "move_limit_y_t"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT "move_limit_x_bt"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT "move_limit_y_bt"
#define ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T "edge_y_confirm_t"
#define ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T "edge_y_dubious_t"
#define ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT "edge_y_avg_bt"
#define ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT "edge_xy_down_bt"
#define ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T "edge_xy_confirm_t"
#define ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM "max_points_bak_num"
//for driver
#define ANTI_FALSE_TOUCH_DRV_STOP_WIDTH "drv_stop_width"
#define ANTI_FALSE_TOUCH_SENSOR_X_WIDTH "sensor_x_width"
#define ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH "sensor_y_width"
#define ANTI_FALSE_TOUCH_FW_FUN_SWITCH "fw_anti_false_touch_switch"

/* emui5.1 new support */
#define ANTI_FALSE_TOUCH_FEATURE_SG "feature_sg"
#define ANTI_FALSE_TOUCH_SG_MIN_VALUE "sg_min_value"
#define ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST "feature_support_list"
#define ANTI_FALSE_TOUCH_MAX_DISTANCE_DT "max_distance_dt"
#define ANTI_FALSE_TOUCH_FEATURE_BIG_DATA "feature_big_data"
#define ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION "feature_click_inhibition"
#define ANTI_FALSE_TOUCH_MIN_CLICK_TIME "min_click_time"

/* struct */
struct bu21150_ioctl_get_frame_data
{
    char __user* buf;
    unsigned int size;
    char __user* tv; /* struct timeval* */
    unsigned int keep_block_flag; /* for suspend */
};

struct bu21150_ioctl_spi_data
{
    unsigned long addr;
    char __user* buf;
    unsigned int count;
    unsigned int next_mode;
};

struct bu21150_ioctl_timeout_data
{
    unsigned int timeout_enb;
    unsigned int report_interval_us;
};

#define HOST_NO_SYNC_TIMEOUT		0
#define HOST_SHORT_SYNC_TIMEOUT		5

enum host_ts_pm_type
{
    HOST_TS_BEFORE_SUSPEND = 0,
    HOST_TS_SUSPEND_DEVICE,
    HOST_TS_RESUME_DEVICE,
    HOST_TS_AFTER_RESUME,
    HOST_TS_IC_SHUT_DOWN,
};

#define HOST_TS_CHIP_TYPE_RESERVED 6
#define HOST_TS_CHIP_TYPE_LEN_RESERVED 5

#define HOST_TS_PRODUCT_ID_TYPE 0x00
#define HOST_TS_2D_BARCODE_TYPE 0x01
#define HOST_TS_BRIGHTNESS_TYPE 0x02
#define HOST_TS_COLOROFWHITE_TYPE 0x03
#define HOST_TS_BRIGHTNESSANDCOLOR_TYPE 0x04
#define HOST_TS_REWORK_TYPE 0x05

/* commands */
#define BU21150_IO_TYPE	 (0xB8)
#define BU21150_IOCTL_CMD_GET_FRAME		  _IOWR(BU21150_IO_TYPE, 0x01, \
        struct bu21150_ioctl_get_frame_data)
#define BU21150_IOCTL_CMD_RESET			  _IO(BU21150_IO_TYPE, 0x02)
#define BU21150_IOCTL_CMD_SPI_READ		  _IOW(BU21150_IO_TYPE, 0x03, \
        struct bu21150_ioctl_spi_data)
#define BU21150_IOCTL_CMD_SPI_WRITE		  _IOR(BU21150_IO_TYPE, 0x04, \
        struct bu21150_ioctl_spi_data)
#define BU21150_IOCTL_CMD_UNBLOCK		  _IO(BU21150_IO_TYPE, 0x05)
#define BU21150_IOCTL_CMD_SUSPEND		  _IO(BU21150_IO_TYPE, 0x06)
#define BU21150_IOCTL_CMD_RESUME		  _IO(BU21150_IO_TYPE, 0x07)
#define BU21150_IOCTL_CMD_UNBLOCK_RELEASE _IO(BU21150_IO_TYPE, 0x08)
#define BU21150_IOCTL_CMD_SET_TIMEOUT	  _IO(BU21150_IO_TYPE, 0x09)
#define BU21150_IOCTL_CMD_SET_SCAN_MODE	  _IO(BU21150_IO_TYPE, 0x0A)
#define BU21150_IOCTL_CMD_SET_POWER		  _IO(BU21150_IO_TYPE, 0x0B)

#define STR_COMMUNICATE_TEST ("HOST_SPI")
#define LENGTH_COMMUNICATE_TEST (8)
#define REG_SIZE             (2)
#define REG_COMMUNICATE_TEST (0x0010)
#define REG_IRQ_SETTINGS (0x007E)
#define REG_INT_RUN_ENB (0x00CE)
#define REG_SENS_START (0x0086)
#define REG_READ_DATA (0x0400)
#define MAX_FRAME_SIZE (8*1024+16)	/* byte */
#define SPI_HEADER_SIZE (3)
#define SPI_BITS_PER_WORD_READ (8)
#define SPI_BITS_PER_WORD_WRITE (8)
#define FRAME_HEADER_SIZE (16)	/* byte */
#define GPIO_LOW  (0)
#define GPIO_HIGH (1)
#define DUR_RESET_HIGH	(1)
#define DUR_RESET_LOW	(0)
#define WAITQ_WAIT	 (0)
#define WAITQ_WAKEUP (1)
/* #define CHECK_SAME_FRAME */
#define APQ8074_DRAGONBOARD (0x01)
#define MSM8974_FLUID		(0x02)
#define TIMEOUT_SCALE		(50)
#define ESD_TEST_TIMER_MS	(10000)
#define BU21150_VTG_MIN_UV (2700000)
#define BU21150_VTG_MAX_UV (3300000)
#define BU21150_VTG_DIG_MIN_UV (1800000)
#define BU21150_VTG_DIG_MAX_UV (1800000)
#define BU21150_SPI_SPEED_DEFAULT (20 * 1000 * 1000)

#define AFE_SCAN_DEFAULT			0x00
#define AFE_SCAN_SELF_CAP			0x01
#define AFE_SCAN_MUTUAL_CAP			0x02
#define AFE_SCAN_GESTURE_SELF_CAP		0x04
#define AFE_SCAN_GESTURE_MUTUAL_CAP		0x08

#define LCD_DEFAULT	(0)
#define LCD_ON		(1)
#define LCD_OFF		(2)
#define BU21150_RW_TRIES				3

#define BU21150_LIST_MAX_FRAMES			20

struct host_enable_info
{
    int glove_enable;
    int roi_enable;
    int holster_enable;
    int touch_switch;
#if defined(HOST_CHARGER_FB)
    int charger_state;
#endif
};

struct host_window_info
{
    int window_enable;
    int top_left_x0;
    int top_left_y0;
    int bottom_right_x1;
    int bottom_right_y1;
    int status;
};

struct bu21150_frame
{
    struct list_head list;
    u8 frame[MAX_FRAME_SIZE];
    struct timeval tv;
};

#if defined(CONFIG_HUAWEI_DSM)
struct host_dsm_info
{
    int constraints_SPI_status;
};
#endif

/* struct */
struct bu21150_data
{
    /* system */
    struct spi_device* client;
    struct notifier_block fb_notif;
    /* frame */
    struct bu21150_ioctl_get_frame_data req_get;
    struct bu21150_frame frame_list;
    u8 frame_count;
    struct bu21150_ioctl_get_frame_data frame_get;
    struct mutex mutex_frame;
    struct mutex mutex_wake;
    struct mutex spi_mutex_lock;
    bool irq_enabled;
    /* frame work */
    u8 frame_work[MAX_FRAME_SIZE];
    struct bu21150_ioctl_get_frame_data frame_work_get;
    struct kobject* bu21150_obj;
    struct kobject* bu21150_ts_obj;
    struct platform_device *ts_dev;
    /* waitq */
    u8 frame_waitq_flag;
    wait_queue_head_t frame_waitq;
    /* reset */
    u8 reset_flag;
    /* timeout */
    u8 timeout_enb;
    u8 set_timer_flag;
    u8 timeout_flag;
    u32 timeout;
    /* spi */
    u8 spi_buf[MAX_FRAME_SIZE];
    u32 max_speed_hz;
    /* power */
    struct regulator* vcc_ana;
    struct regulator* vcc_dig;
    /* dtsi */
    unsigned irq_gpio;
    unsigned rst_gpio;
    unsigned cs_gpio;
    int power_supply;
    const char* panel_model;
    const char* afe_version;
    const char* pitch_type;
    const char* afe_vendor;
    u16 scan_mode;
    bool wake_up;
    bool suspended;
    u8 unblock_flag;
    u8 force_unblock_flag;
    u8 lcd_on;
    struct pl022_config_chip spidev0_chip_info;
#if defined(CONFIG_HUAWEI_DSM)
    struct host_dsm_info dsm_info;
#endif
    struct host_enable_info host_enable_flag;
    struct host_window_info host_info;
    struct anti_false_touch_param anti_false_touch_param_data;
#if defined(HOST_CHARGER_FB)
    struct notifier_block charger_detect_notif;
#endif

};

struct ser_req
{
    struct spi_message	  msg;
    struct spi_transfer	   xfer[2];
    u16 sample ____cacheline_aligned;
};
bool g_host_tp_choose;
int g_host_tp_power_ctrl;
bool g_lcd_control_host_tp_power;

int host_ts_power_control_notify(enum host_ts_pm_type pm_type, int timeout);

#endif /* _BU21150_H_ */

