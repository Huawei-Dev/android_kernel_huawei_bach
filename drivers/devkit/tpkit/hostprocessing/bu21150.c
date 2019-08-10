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
//lint -e528
#include "bu21150.h"
#include <linux/delay.h>

#if defined(CONFIG_HUAWEI_DSM)
static struct dsm_dev dsm_tphostprocessing =
{
    .name = "dsm_tphostprocessing",
    .device_name = "TPHOSTPROCESSING",
    .ic_name = "syn",
    .module_name = "NNN",
    .fops = NULL,
    .buff_size = 1024,
};

struct dsm_client* tphostprocessing_dclient;

bool host_detfail_dsm;

#endif
unsigned int  g_abs_max_x = 0;
unsigned int  g_abs_max_y = 0;

/* define */
#define DEVICE_NAME	  "jdi_bu21150"
#define SYSFS_PROPERTY_PATH	  "afe_properties"
#define SYSFS_TOUCH_PATH   "touchscreen"
#define SYSFS_PLAT_TOUCH_PATH   "huawei_touch"

#define REG_OTP_ADDR (0x0088)
#define REG_OTP_ADDR_RECALL (0x0098)
#define OTP_OK	 0
#define OTP_SIZE			8

char *g_anti_false_touch_string[]={
	ANTI_FALSE_TOUCH_FEATURE_ALL,
	ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT,
	ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT,
	ANTI_FALSE_TOUCH_FEATURE_REBACK_BT,
	ANTI_FALSE_TOUCH_LCD_WIDTH,
	ANTI_FALSE_TOUCH_LCD_HEIGHT,
	ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT,
	ANTI_FALSE_TOUCH_CLICK_TIME_BT,
	ANTI_FALSE_TOUCH_EDGE_POISION,
	ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE,
	ANTI_FALSE_TOUCH_BT_EDGE_X,
	ANTI_FALSE_TOUCH_BT_EDGE_Y,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_X,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_Y,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT,
	ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T,
	ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T,
	ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT,
	ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT,
	ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T,
	ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM,
	//for driver
	ANTI_FALSE_TOUCH_DRV_STOP_WIDTH,
	ANTI_FALSE_TOUCH_SENSOR_X_WIDTH,
	ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH,

	/* emui5.1 new support */
	ANTI_FALSE_TOUCH_FEATURE_SG,
	ANTI_FALSE_TOUCH_SG_MIN_VALUE,
	ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST,
	ANTI_FALSE_TOUCH_MAX_DISTANCE_DT,
	ANTI_FALSE_TOUCH_FEATURE_BIG_DATA,
	ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION,
	ANTI_FALSE_TOUCH_MIN_CLICK_TIME,
};

/* static function declaration */
static int bu21150_probe(struct spi_device* client);
static int bu21150_remove(struct spi_device* client);
static int bu21150_open(struct inode* inode, struct file* filp);
static int bu21150_release(struct inode* inode, struct file* filp);
static long bu21150_ioctl(struct file* filp, unsigned int cmd,
                          unsigned long arg);
static long bu21150_ioctl_get_frame(unsigned long arg);
static long bu21150_ioctl_reset(unsigned long arg);
static long bu21150_ioctl_spi_read(unsigned long arg);
static long bu21150_ioctl_spi_write(unsigned long arg);
static long bu21150_ioctl_suspend(void);
static long bu21150_ioctl_resume(void);
static long bu21150_ioctl_unblock(void);
static long bu21150_ioctl_unblock_release(void);
static long bu21150_ioctl_set_timeout(unsigned long arg);
static long bu21150_ioctl_set_scan_mode(unsigned long arg);
static long bu21150_ioctl_set_power(unsigned long arg);
static irqreturn_t bu21150_irq_thread(int irq, void* dev_id);
static void swap_2byte(unsigned char* buf, unsigned int size);
static int bu21150_read_register(u32 addr, u16 size, u8* data);
static int bu21150_write_register(u32 addr, u16 size, u8* data);
static void wake_up_frame_waitq(struct bu21150_data* ts);
static long wait_frame_waitq(struct bu21150_data* ts, u8 flag);
static int is_same_bu21150_ioctl_get_frame_data(
    struct bu21150_ioctl_get_frame_data* data1,
    struct bu21150_ioctl_get_frame_data* data2);
static void copy_frame(struct bu21150_data* ts);
#ifdef CHECK_SAME_FRAME
static void check_same_frame(struct bu21150_data* ts);
#endif
static int parse_dtsi(struct device* dev, struct bu21150_data* ts);
static void get_frame_timer_init(void);
static void get_frame_timer_handler(unsigned long data);
static void get_frame_timer_delete(void);
static int bu21150_fb_suspend(struct device* dev);
static int bu21150_fb_early_resume(struct device* dev);
static int bu21150_fb_resume(struct device* dev);
static int fb_notifier_callback(struct notifier_block* self,
                                unsigned long event, void* data);
static void bu21150_spi_cs_set(u32 control);
#if defined(HOST_CHARGER_FB)
static int charger_detect_notifier_callback(struct notifier_block* self,
                                unsigned long event, void* data);
#endif

/* static variables */
static struct spi_device* g_client_bu21150;
static int g_io_opened;
static struct timer_list get_frame_timer;

static ssize_t bu21150_wake_up_enable_store(struct kobject* kobj,
        struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    unsigned long state;
    ssize_t ret;

    ret = kstrtoul(buf, 10, &state);

    if (ret)
    { return ret; }

    if (!!state == ts->wake_up)
    { return count; }

    mutex_lock(&ts->mutex_wake);

    if (state == 0)
    {
        if (!ts->wake_up)
        { goto exit;}



        disable_irq_wake(ts->client->irq);
        device_init_wakeup(&ts->client->dev, false);
        ts->wake_up = false;
    }
    else
    {
        if (ts->wake_up)
        { goto exit;}



        device_init_wakeup(&ts->client->dev, true);
        enable_irq_wake(ts->client->irq);
        ts->wake_up = true;
    }

exit:
    mutex_unlock(&ts->mutex_wake);

    return count;
}

static ssize_t bu21150_wake_up_enable_show(struct kobject* kobj,
        struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    return snprintf(buf, PAGE_SIZE, "%u", ts->wake_up);
}

static ssize_t bu21150_trigger_esd_store(struct kobject* kobj,
        struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    disable_irq(ts->client->irq);
    msleep(ESD_TEST_TIMER_MS);
    enable_irq(ts->client->irq);

    return count;
}

static ssize_t bu21150_hallib_name_show(struct kobject* kobj,
                                        struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    int index = snprintf(buf, PAGE_SIZE, "libafehal");

    if (ts->afe_vendor)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->afe_vendor);

    if (ts->panel_model)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->panel_model);

    if (ts->afe_version)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->afe_version);

    if (ts->pitch_type)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->pitch_type);

    return index;
}

static ssize_t bu21150_cfg_name_show(struct kobject* kobj,
                                     struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    int index = snprintf(buf, PAGE_SIZE, "hbtp");

    if (ts->afe_vendor)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->afe_vendor);

    if (ts->panel_model)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->panel_model);

    if (ts->afe_version)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->afe_version);

    if (ts->pitch_type)
        index += snprintf(buf + index, PAGE_SIZE, "_%s",
                          ts->pitch_type);

    return index;
}

static ssize_t bu21150_chip_info_show(struct kobject* kobj,
                                      struct kobj_attribute* attr, char* buf)
{
    int index = snprintf(buf, PAGE_SIZE, "rohm-VCTO32080-jdi\n");
    return index;
}

static ssize_t host_sensitivity_show(struct kobject* kobj,
                                     struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ssize_t ret;

    HOST_LOG_DEBUG("%s called\n", __func__);

    ret = snprintf(buf, 32, "%d\n", ts->host_enable_flag.holster_enable);

    HOST_LOG_DEBUG("host_sensitivity_show done\n");
    return ret;
}

static ssize_t host_sensitivity_store(struct kobject* kobj,
                                      struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    unsigned int value;
    int error;

    HOST_LOG_INFO("%s called\n", __func__);

    error = sscanf(buf, "%u", &value);

    if (error <= 0)
    {
        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
        error = -EINVAL;
        goto out;
    }

    HOST_LOG_INFO("sscanf value is %u\n", value);

    ts->host_enable_flag.holster_enable = value;

    error = count;

out:
    HOST_LOG_DEBUG("host_sensitivity_store done\n");
    return error;
}

static ssize_t host_touch_window_show(struct kobject* kobj,
                                      struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    int error = 0;
    HOST_LOG_INFO("%s called\n", __func__);

    error =
        snprintf(buf, 128, "%d %d %d %d %d\n", ts->host_info.window_enable,
                 ts->host_info.top_left_x0, ts->host_info.top_left_y0,
                 ts->host_info.bottom_right_x1, ts->host_info.bottom_right_y1);
    HOST_LOG_DEBUG("host_touch_window_show done\n");

    return error;
}

static ssize_t host_touch_window_store(struct kobject* kobj,
                                       struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    int window_enable;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    int error;

    HOST_LOG_INFO("%s called\n", __func__);

    error = sscanf(buf, "%d %d %d %d %d",
                   &window_enable, &x0, &y0, &x1, &y1);

    if (error <= 0)
    {
        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
        error = -EINVAL;
        goto out;
    }

    HOST_LOG_INFO("sscanf value is %d (%d,%d), (%d,%d)\n",
                  window_enable, x0, y0, x1, y1);

    if (window_enable &&
        ((x0 < 0) || (y0 < 0) || (x1 <= x0) || (y1 <= y0)))
    {
        HOST_LOG_ERR("value is %d (%d,%d), (%d,%d)\n",
                     window_enable, x0, y0, x1, y1);
        error = -EINVAL;
        goto out;
    }

    ts->host_info.top_left_x0 = x0;
    ts->host_info.top_left_y0 = y0;
    ts->host_info.bottom_right_x1 = x1;
    ts->host_info.bottom_right_y1 = y1;
    ts->host_info.window_enable = window_enable;

    error = count;

out:
    HOST_LOG_DEBUG("ts_touch_window_store done\n");
    return error;
}

static ssize_t host_glove_mode_show(struct kobject* kobj,
                                    struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ssize_t ret;

    HOST_LOG_DEBUG("%s called\n", __func__);

    ret = snprintf(buf, 32, "%d\n", ts->host_enable_flag.glove_enable);

    HOST_LOG_DEBUG("host_glove_mode_show done\n");
    return ret;
}

static ssize_t host_glove_mode_store(struct kobject* kobj,
                                     struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    unsigned int value;
    int error;

    HOST_LOG_INFO("%s called\n", __func__);


    error = sscanf(buf, "%u", &value);

    if (error <= 0)
    {
        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
        error = -EINVAL;
        goto out;
    }

    HOST_LOG_INFO("sscanf value is %u\n", value);

    ts->host_enable_flag.glove_enable = value;

    error = count;

out:
    HOST_LOG_DEBUG("host_glove_mode_store done\n");
    return error;
}


#define ROI
#ifdef ROI
#define ROI_DATA_LENGTH		49
#define ROI_DATA_STR_LENGTH    ROI_DATA_LENGTH * 10
static char host_roi_data_str[ROI_DATA_STR_LENGTH] = { 0 };
static short host_roi_data_report[ROI_DATA_LENGTH] = { 0 };

#endif

static ssize_t host_roi_enable_show(struct kobject* kobj,
                                    struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ssize_t ret;

    HOST_LOG_INFO("%s called\n", __func__);

    ret = snprintf(buf, 32, "%d\n", ts->host_enable_flag.roi_enable);

    HOST_LOG_DEBUG("host_roi_enable_show done\n");
    return ret;

}

static ssize_t host_roi_enable_store(struct kobject* kobj,
                                     struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    unsigned int value;
    int error;

    HOST_LOG_INFO("%s called\n", __func__);

    error = sscanf(buf, "%u", &value);

    if (error <= 0)
    {
        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
        error = -EINVAL;
        goto out;
    }

    HOST_LOG_INFO("sscanf value is %u\n", value);

    ts->host_enable_flag.roi_enable = value;

    error = count;

out:
    HOST_LOG_INFO("host_glove_mode_store done\n");
    return error;

}

static ssize_t host_roi_data_show(struct kobject* kobj,
                                  struct kobj_attribute* attr, char* buf)
{
    /*the ROI data you want to send to Qeexo*/
    memcpy(buf, host_roi_data_report, ROI_DATA_LENGTH * sizeof(short));
    return ROI_DATA_LENGTH * sizeof(short);
}

static const char* skip_space(const char* str)
{
    char c;
    bool space = true;

    while (space)
    {
        c = *str;
        space = (c == ' ') || (c == '\t');
        space = space || (c == '\n') || (c == '\r');

        if (!space)
        {
            break;
        }

        ++str;
    }

    return str;
}

static const char* move_to_next_number(const char* str)
{
    str = skip_space(str);
    if (*str && *str == '-') {
        str++;
    }
    while (*str)
    {
        if (*str > '9' || *str < '0') {
            break;
        }
        str++;
    }
    str = skip_space(str);
    return str;
}

static ssize_t host_roi_data_store(struct kobject* kobj,
                                   struct kobj_attribute* attr, const char* buf, size_t count)
{
    int i = 0;
    int num;
    const char* str = host_roi_data_str;

    strncpy(host_roi_data_str, buf, ROI_DATA_STR_LENGTH - 1);
    count = ROI_DATA_STR_LENGTH;

    i = 0;

    while (i < ROI_DATA_LENGTH && *str)
    {
        if (sscanf(str, "%d", &num) < 0) {
            break;
        };
        str = move_to_next_number(str);
        host_roi_data_report[i] = num;
        ++i;
    }

    return count;
}

static ssize_t host_roi_data_debug_show(struct kobject* kobj,
                                        struct kobj_attribute* attr, char* buf)
{
    int count = 0;
    int i = 0;

    for (i = 0; i < ROI_DATA_LENGTH; ++i)
    {
        count += snprintf(buf + count, PAGE_SIZE - count, "%4d\t", host_roi_data_report[i]);

        if ((i + 1) % 7 == 0)
        {
            count += snprintf(buf + count, PAGE_SIZE - count, "\n");
        }
    }

    return count;
}

static ssize_t host_anti_false_touch_param_show(struct kobject* kobj,
                                        struct kobj_attribute* attr, char* buf)
{
    struct anti_false_touch_param* local_param = NULL;
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    HOST_LOG_INFO("%s +\n", __func__);

    if (!ts)
    {
        HOST_LOG_ERR("%s ts_data is null\n", __func__);
        return snprintf(buf, PAGE_SIZE, "error1\n");
    }

    local_param = &(ts->anti_false_touch_param_data);
    return snprintf(buf, PAGE_SIZE, "feature_all=%d,"
                    "feature_resend_point=%d,"
                    "feature_orit_support=%d,"
                    "feature_reback_bt=%d,"
                    "lcd_width=%d,"
                    "lcd_height=%d,"
                    "click_time_limit=%d,"
                    "click_time_bt=%d,"
                    "edge_position=%d,"
                    "edge_postion_secondline=%d,"
                    "bt_edge_x=%d,"
                    "bt_edge_y=%d,"
                    "move_limit_x=%d,"
                    "move_limit_y=%d,"
                    "move_limit_x_t=%d,"
                    "move_limit_y_t=%d,"
                    "move_limit_x_bt=%d,"
                    "move_limit_y_bt=%d,"
                    "edge_y_confirm_t=%d,"
                    "edge_y_dubious_t=%d,"
                    "edge_y_avg_bt=%d,"
                    "edge_xy_down_bt=%d,"
                    "edge_xy_confirm_t=%d,"
                    "max_points_bak_num=%d,"
                    "drv_stop_width=%d,"
                    "sensor_x_width=%d,"
                    "sensor_y_width=%d,"

                    /* emui5.1 new support */
                    "feature_sg=%d,"
                    "sg_min_value=%d,"
                    "feature_support_list=%d,"
                    "max_distance_dt=%d,"
                    "feature_big_data=%d,"
                    "feature_click_inhibition=%d,"
                    "min_click_time=%d,\n",
                    local_param->feature_all,
                    local_param->feature_resend_point,
                    local_param->feature_orit_support,
                    local_param->feature_reback_bt,
                    local_param->lcd_width,
                    local_param->lcd_height,
                    local_param->click_time_limit,
                    local_param->click_time_bt,
                    local_param->edge_position,
                    local_param->edge_postion_secondline,
                    local_param->bt_edge_x,
                    local_param->bt_edge_y,
                    local_param->move_limit_x,
                    local_param->move_limit_y,
                    local_param->move_limit_x_t,
                    local_param->move_limit_y_t,
                    local_param->move_limit_x_bt,
                    local_param->move_limit_y_bt,
                    local_param->edge_y_confirm_t,
                    local_param->edge_y_dubious_t,
                    local_param->edge_y_avg_bt,
                    local_param->edge_xy_down_bt,
                    local_param->edge_xy_confirm_t,
                    local_param->max_points_bak_num,
                    local_param->drv_stop_width,
                    local_param->sensor_x_width,
                    local_param->sensor_y_width,

                    /* emui5.1 new support */
                    local_param->feature_sg,
                    local_param->sg_min_value,
                    local_param->feature_support_list,
                    local_param->max_distance_dt,
                    local_param->feature_big_data,
                    local_param->feature_click_inhibition,
                    local_param->min_click_time);
}

static void host_set_value_to_param(char* tag, int value)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    struct anti_false_touch_param* local_param = NULL;

    if (!tag || !ts)
    {
        HOST_LOG_ERR("%s aft get tag/ts null\n", __func__);
        return ;
    }

    local_param = &(ts->anti_false_touch_param_data);

    if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_ALL))
    {
        local_param->feature_all = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT))
    {
        local_param->feature_resend_point = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT))
    {
        local_param->feature_orit_support = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_REBACK_BT))
    {
        local_param->feature_reback_bt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_LCD_WIDTH))
    {
        local_param->lcd_width = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_LCD_HEIGHT))
    {
        local_param->lcd_height = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT))
    {
        local_param->click_time_limit = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_CLICK_TIME_BT))
    {
        local_param->click_time_bt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_POISION))
    {
        local_param->edge_position = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE))
    {
        local_param->edge_postion_secondline = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_BT_EDGE_X))
    {
        local_param->bt_edge_x = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_BT_EDGE_Y))
    {
        local_param->bt_edge_y = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_X))
    {
        local_param->move_limit_x = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y))
    {
        local_param->move_limit_y = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T))
    {
        local_param->move_limit_x_t = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T))
    {
        local_param->move_limit_y_t = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT))
    {
        local_param->move_limit_x_bt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT))
    {
        local_param->move_limit_y_bt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T))
    {
        local_param->edge_y_confirm_t = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T))
    {
        local_param->edge_y_dubious_t = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT))
    {
        local_param->edge_y_avg_bt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT))
    {
        local_param->edge_xy_down_bt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T))
    {
        local_param->edge_xy_confirm_t = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM))
    {
        local_param->max_points_bak_num = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_DRV_STOP_WIDTH))
    {
        local_param->drv_stop_width = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_SENSOR_X_WIDTH))
    {
        local_param->sensor_x_width = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH))
    {
        local_param->sensor_y_width = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_SG))
    {
        local_param->feature_sg = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_SG_MIN_VALUE))
    {
        local_param->sg_min_value = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST))
    {
        local_param->feature_support_list = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MAX_DISTANCE_DT))
    {
        local_param->max_distance_dt = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_BIG_DATA))
    {
        local_param->feature_big_data = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION))
    {
        local_param->feature_click_inhibition = value;
    }
    else if (!strcmp(tag, ANTI_FALSE_TOUCH_MIN_CLICK_TIME))
    {
        local_param->min_click_time = value;
    }

    HOST_LOG_INFO("%s: set %s to %d\n", __func__, tag, value);
}

/*lint -save -e158 */
static int host_anti_false_touch_get_param(const char* buf, char* tag)
{
    char* ptr_begin = NULL, *ptr_end = NULL;
    char tmp_str[128] = {0};
    char value_str[32] = {0};
    int len;
    int value = 0;
    int error;

    if (!buf || !tag)
    {
        HOST_LOG_ERR("misoper get error string\n");
        return -EINVAL;
    }

    HOST_LOG_DEBUG("%s:%s\n", buf, tag);
    snprintf(tmp_str, sizeof(tmp_str), "%s=", tag);

    if (NULL != (ptr_begin = strstr(buf, tmp_str)))
    {
        ptr_begin += strlen(tmp_str);

        if (ptr_begin)
        {
            ptr_end = strstr(ptr_begin, ",");

            if (ptr_end)
            {
                len = ptr_end - ptr_begin;

                if (len > 0 && len < (int)sizeof(value_str))
                {
                    HOST_LOG_DEBUG("%s: get %s string %s\n", __func__, tag, value_str);
                    strncpy(value_str, ptr_begin, len);
                    error = sscanf(value_str, "%u", &value);

                    if (error <= 0)
                    {
                        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
                        return -EINVAL;
                    }

                    host_set_value_to_param(tag, value);
                }
            }
        }
    }

    return 0;
}
/*lint -restore*/

static ssize_t host_anti_false_touch_param_store(struct kobject* kobj,
                                   struct kobj_attribute* attr, const char* buf, size_t count)
{
    int i;
    int str_num = 0;

    HOST_LOG_INFO("%s +\n", __func__);

    str_num = sizeof(g_anti_false_touch_string) / sizeof(char*);
    HOST_LOG_DEBUG("str_num:%d, input buf is [%s]\n", str_num, buf);

    for (i = 0; i < str_num; i++)
    {
        host_anti_false_touch_get_param(buf, g_anti_false_touch_string[i]);
    }

    HOST_LOG_INFO("%s -\n", __func__);
    return count;
}

#if defined(HOST_CHARGER_FB)
static ssize_t host_charger_state_show(struct kobject* kobj,
                                    struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ssize_t ret;
    HOST_LOG_DEBUG("%s called\n", __func__);
    ret = snprintf(buf, 32, "%d\n", ts->host_enable_flag.charger_state);
    HOST_LOG_DEBUG("host_charger_state_show done, ret=%ld \n", ret);
    return ret;
}
static ssize_t host_charger_state_store(struct kobject* kobj,
                                     struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    unsigned int value;
    int error;

    HOST_LOG_INFO("%s called\n", __func__);


    error = sscanf(buf, "%u", &value);

    if (error <= 0)
    {
        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
        error = -EINVAL;
        goto out;
    }

    HOST_LOG_INFO("sscanf value is %u\n", value);

    ts->host_enable_flag.charger_state = value;

    error = count;

out:
    HOST_LOG_DEBUG("host_charger_state_store done\n");
    return error;
}
#endif
static ssize_t host_touch_switch_show(struct kobject* kobj,
                                    struct kobj_attribute* attr, char* buf)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ssize_t ret;
    HOST_LOG_DEBUG("%s called\n", __func__);
    ret = snprintf(buf, 32, "%d\n", ts->host_enable_flag.touch_switch);
    HOST_LOG_DEBUG("%s done\n", __func__);
    return ret;
}

static ssize_t host_touch_switch_store(struct kobject* kobj,
                                     struct kobj_attribute* attr, const char* buf, size_t count)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    unsigned int value;
    int error;

    HOST_LOG_INFO("%s called\n", __func__);
    error = sscanf(buf, "%u", &value);
    if (error <= 0)
    {
        HOST_LOG_ERR("sscanf return invaild :%d\n", error);
        error = -EINVAL;
        goto out;
    }

    HOST_LOG_INFO("sscanf value is %u\n", value);
    ts->host_enable_flag.touch_switch = value;
    error = count;

out:
    HOST_LOG_DEBUG("%s done\n", __func__);
    return error;
}


DEVICE_ATTR(roi_data, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_roi_data_show, host_roi_data_store);
DEVICE_ATTR(roi_data_debug, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_roi_data_debug_show, host_roi_data_store);
DEVICE_ATTR(touch_chip_info, S_IRUGO,
    bu21150_chip_info_show, NULL);
DEVICE_ATTR(roi_enable, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_roi_enable_show, host_roi_enable_store);
DEVICE_ATTR(touch_glove, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_glove_mode_show, host_glove_mode_store);
DEVICE_ATTR(touch_sensitivity, (S_IRUSR | S_IWUSR),
    host_sensitivity_show, host_sensitivity_store);
DEVICE_ATTR(touch_window, (S_IRUSR | S_IWUSR),
    host_touch_window_show, host_touch_window_store);
DEVICE_ATTR(anti_false_touch_param, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_anti_false_touch_param_show, host_anti_false_touch_param_store);
#if defined(HOST_CHARGER_FB)
DEVICE_ATTR(charger_state, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_charger_state_show, host_charger_state_store);
#endif
DEVICE_ATTR(touch_switch, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
    host_touch_switch_show, host_touch_switch_store);

static struct attribute *bu21150_ts_attributes[] = {
	&dev_attr_roi_data.attr,
	&dev_attr_roi_data_debug.attr,
	&dev_attr_touch_chip_info.attr,
	&dev_attr_roi_enable.attr,
	&dev_attr_touch_glove.attr,
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_touch_window.attr,
	&dev_attr_anti_false_touch_param.attr,
#if defined(HOST_CHARGER_FB)
	&dev_attr_charger_state.attr,
#endif
	&dev_attr_touch_switch.attr,
	NULL
};

static const struct attribute_group bu21150_ts_attr_group = {
	.attrs = bu21150_ts_attributes,
 };

DEVICE_ATTR(hallib_name, S_IRUGO, bu21150_hallib_name_show, NULL);
DEVICE_ATTR(cfg_name, S_IRUGO, bu21150_cfg_name_show, NULL);
DEVICE_ATTR(wake_up_enable, (S_IRUGO | S_IWUSR | S_IWGRP),
    bu21150_wake_up_enable_show, bu21150_wake_up_enable_store);
DEVICE_ATTR(trigger_esd, (S_IRUGO | S_IWUSR | S_IWGRP),
    NULL, bu21150_trigger_esd_store);

static struct attribute *bu21150_prop_attrs[] =
{
    &dev_attr_hallib_name.attr,
    &dev_attr_cfg_name.attr,
    &dev_attr_wake_up_enable.attr,
    &dev_attr_trigger_esd.attr,
    NULL
};

static const struct attribute_group bu21150_ts_prop_attr_group = {
	.attrs = bu21150_prop_attrs,
};

static const struct of_device_id g_bu21150_psoc_match_table[] =
{
    {.compatible = "jdi,bu21150",},
    { },
};

static const struct file_operations g_bu21150_fops =
{
    .owner = THIS_MODULE,
    .open = bu21150_open,
    .release = bu21150_release,
    .unlocked_ioctl = bu21150_ioctl,
};

static struct miscdevice g_bu21150_misc_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &g_bu21150_fops,
};

static const struct spi_device_id g_bu21150_device_id[] =
{
    { DEVICE_NAME, 0 },
    { }
};
MODULE_DEVICE_TABLE(spi, g_bu21150_device_id);

static struct spi_driver g_bu21150_spi_driver =
{
    .probe = bu21150_probe,
    .remove = bu21150_remove,
    .id_table = g_bu21150_device_id,
    .driver = {
        .name = DEVICE_NAME,
        .owner = THIS_MODULE,
        .bus = &spi_bus_type,
        .of_match_table = g_bu21150_psoc_match_table,
    },
};

/*lint -save -e528 */
module_spi_driver(g_bu21150_spi_driver);
/*lint -restore*/
MODULE_AUTHOR("Japan Display Inc");
MODULE_DESCRIPTION("JDI BU21150 Device Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("spi:bu21150");

static void bu21150_read_str(u32 addr, u16 size, u8*data)
{
    u16 i;
    for (i = 0; i < size; i += REG_SIZE) {
        bu21150_read_register(addr + i, REG_SIZE, data + i);
        HOST_LOG_INFO("read: %c, %c\n", (char)data[i], (char)data[i + 1]);
    }
}

int bu21150_otp_read(char *otp_data)
{
	int ret;
	uint16_t i;
	u8 otp_recall[2] = {0x01, 0x00};
	u8 otp_read[2] = {0x00, 0x00};
	ret = bu21150_write_register(REG_OTP_ADDR_RECALL, (u16)sizeof(otp_recall), otp_recall);
	if(ret != OTP_OK){
		HOST_LOG_ERR("bu21150 OTP write failed!");
		return ret;
	}

	msleep(1);

	for(i = 0; i < OTP_SIZE; i++){
		bu21150_read_register((REG_OTP_ADDR+i * 2), (u16)sizeof(otp_read), otp_read);
		sprintf(&otp_data[i*2], "%c", otp_read[0]);
		sprintf(&otp_data[i*2+1], "%c", otp_read[1]);
	}
	HOST_LOG_INFO("bu21150 OTP read data: %s\n", otp_data);
	return ret;
}


static int bu21150_communicate_check(void)
{
    int i;
    u8 buf_zero[LENGTH_COMMUNICATE_TEST] = {0};
    u8 buf_read[LENGTH_COMMUNICATE_TEST + 1] = {0};

    for (i = 0; i < BU21150_RW_TRIES; i++) {
        bu21150_write_register(REG_COMMUNICATE_TEST, LENGTH_COMMUNICATE_TEST, (u8 *)STR_COMMUNICATE_TEST);
        bu21150_read_str(REG_COMMUNICATE_TEST, LENGTH_COMMUNICATE_TEST, buf_read);
        bu21150_write_register(REG_COMMUNICATE_TEST, LENGTH_COMMUNICATE_TEST, buf_zero);

        HOST_LOG_INFO("spi communicate, get str: %s\n", buf_read);
        if (!strncmp((char *)buf_read, STR_COMMUNICATE_TEST, (unsigned long)LENGTH_COMMUNICATE_TEST)) {
            HOST_LOG_INFO("spi communicate check success\n");
            return 0;
        } else {
            HOST_LOG_INFO("spi communicate failed %d\n", i);
            msleep(50);
        }
    }
    return -1;
}

static int bu21150_spi_setup(struct spi_device* client, struct bu21150_data* ts)
{
    int rc;
    g_client_bu21150 = client;
    ts->client = client;

    client->mode = SPI_MODE_0;
    client->max_speed_hz = ts->max_speed_hz;
    client->bits_per_word = 8;
    client->controller_data = &ts->spidev0_chip_info;
    rc = spi_setup(client);

    if (rc)
    {
        HOST_LOG_ERR("probe - setup spi error");
        return rc;
    }

    /* Initialize IRQ */
    ts->client->irq = gpio_to_irq(ts->irq_gpio);

    if (ts->client->irq < 0)
    {
        HOST_LOG_ERR("error irq: %d\n", ts->client->irq);
        rc = -EINVAL;
        return rc;
    }

    return 0;
}

static int bu21150_gpio_init(struct bu21150_data* ts)
{
    int rc;

    rc = gpio_request(ts->irq_gpio, "bu21150_ts_int");
    if (rc)
    {
        HOST_LOG_ERR("IRQ gpio_request failed\n");
        return rc;
    }
    gpio_direction_input(ts->irq_gpio);

    rc = gpio_request(ts->cs_gpio, "bu21150_ts_cs");
    if (rc)
    {
        HOST_LOG_ERR("gpio_request(%d) failed\n", ts->cs_gpio);
        gpio_free(ts->irq_gpio);
        return rc;
    }

    /* set reset */
    rc = gpio_request(ts->rst_gpio, "bu21150_ts_reset");
    if (rc)
    {
        HOST_LOG_ERR("gpio_request(%d) failed\n", ts->rst_gpio);
        gpio_free(ts->irq_gpio);
        gpio_free(ts->cs_gpio);
        return rc;
    }

    gpio_direction_output(ts->rst_gpio, GPIO_LOW);
    gpio_set_value(ts->rst_gpio, BU21150_RESET_LOW);

    msleep(DUR_RESET_LOW);
    gpio_set_value(ts->rst_gpio, BU21150_RESET_HIGH);
    msleep(DUR_RESET_HIGH);

    return 0;
}

static void bu21150_free_gpio(struct bu21150_data* ts)
{
    gpio_free(ts->irq_gpio);
    gpio_free(ts->cs_gpio);
    gpio_free(ts->rst_gpio);
}

static int bu21150_init_data(struct spi_device* client, struct bu21150_data* ts)
{
    int rc;

    /*step 1 : init mutex */
    mutex_init(&ts->mutex_frame);
    mutex_init(&ts->mutex_wake);
    mutex_init(&ts->spi_mutex_lock);

    /*step 2 : parse dtsi */
    if (parse_dtsi(&client->dev, ts))
    {
        HOST_LOG_ERR("jdi Invalid dtsi\n");
        rc = -EINVAL;
        return rc;
    }

    /*step 3 : config gpio&irq */
    rc = bu21150_gpio_init(ts);
    if (rc) {
        HOST_LOG_ERR("bu21150_gpio_init fail\n");
        return rc;
    }

    /*step 4 : setup spi */
    rc = bu21150_spi_setup(client, ts);
    if (rc) {
        bu21150_free_gpio(ts);
        return rc;
    }

    dev_set_drvdata(&client->dev, ts);

#if defined(CONFIG_HUAWEI_DSM)
    if (!tphostprocessing_dclient)
    {
        tphostprocessing_dclient =
            dsm_register_client(&dsm_tphostprocessing);
    }
#endif

    rc = bu21150_communicate_check();
    if (rc < 0)
    {
        HOST_LOG_INFO("not find bu21150 device\n");
        bu21150_free_gpio(ts);
        return rc;
    }
    return 0;
}

static int bu21150_init_sysfs(struct bu21150_data* ts)
{
    int rc;

    ts->ts_dev = platform_device_alloc(SYSFS_PLAT_TOUCH_PATH, -1);
    if (!ts->ts_dev) {
        HOST_LOG_ERR("platform device malloc failed\n");
        rc = -ENOMEM;
        goto err_platform_device_alloc;
    }

    rc = platform_device_add(ts->ts_dev);
    if (rc) {
        HOST_LOG_ERR("platform device add failed :%d\n", rc);
        goto err_platform_device_add;
    }

    rc = sysfs_create_group(&ts->ts_dev->dev.kobj, &bu21150_ts_attr_group);
    if (rc) {
        HOST_LOG_ERR("can't create ts's sysfs\n");
        goto err_del_platform_dev;
    }

    rc = sysfs_create_link(NULL, &ts->ts_dev->dev.kobj, SYSFS_TOUCH_PATH);
    if (rc) {
        HOST_LOG_ERR("%s: Fail create link error = %d\n", __func__, rc);
        goto err_create_ts_sysfs;
    }

    /* add afe_properties/ sysfs */
    ts->bu21150_obj = kobject_create_and_add(SYSFS_PROPERTY_PATH, NULL);
    if (!ts->bu21150_obj)
    {
        HOST_LOG_ERR("unable to create kobject\n");
        rc = -EINVAL;
        goto err_create_ts_sysfs;
    }
    rc = sysfs_create_group(ts->bu21150_obj, &bu21150_ts_prop_attr_group);
    if (rc) {
        HOST_LOG_ERR("failed to create attributes\n");
        goto err_create_ts_sysfs;
    }
    /* add afe_properties/ sysfs end*/

    return 0;

err_create_ts_sysfs:
    sysfs_remove_group(&ts->ts_dev->dev.kobj, &bu21150_ts_attr_group);
err_del_platform_dev:
    platform_device_del(ts->ts_dev);
    misc_deregister(&g_bu21150_misc_device);
err_platform_device_add:
    platform_device_put(ts->ts_dev);
err_platform_device_alloc:
    return rc;
}

static int bu21150_probe(struct spi_device* client)
{
    struct bu21150_data* ts;
    int rc;

    HOST_LOG_INFO("bu21150_probe in\n");
    ts = kzalloc(sizeof(struct bu21150_data), GFP_KERNEL);
    if (!ts)
    {
        HOST_LOG_ERR("Out of memory\n");
        return -ENOMEM;
    }

    rc = bu21150_init_data(client, ts);
    if (rc) {
        HOST_LOG_ERR("bu21150_init_data fail: %d\n", rc);
        goto err_init;
    }

    ts->lcd_on = LCD_DEFAULT;
    INIT_LIST_HEAD(&ts->frame_list.list);
    init_waitqueue_head(&(ts->frame_waitq));

    ts->fb_notif.notifier_call = fb_notifier_callback;
    rc = fb_register_client(&ts->fb_notif);

    if (rc)
    {
        HOST_LOG_ERR("Unable to register fb_notifier: %d\n", rc);
        goto err_register_fb_notif;
    }

    rc = misc_register(&g_bu21150_misc_device);
    if (rc)
    {
        HOST_LOG_ERR("Failed to register misc device\n");
        goto err_register_misc;
    }

    rc = input_mt_wrapper_init();
    if (rc)
    {
        HOST_LOG_ERR("Failed to init input_mt_wrapper\n");
        goto err_register_misc;
    }

    rc = bu21150_init_sysfs(ts);
    if (rc) {
        HOST_LOG_ERR("Failed to create sysfs\n");
        goto err_register_misc;
    }

#if defined(CONFIG_HUAWEI_DSM)
    host_detfail_dsm = true;
#endif
    g_host_tp_choose = true;

#if defined(HOST_CHARGER_FB)
    ts->charger_detect_notif.notifier_call = charger_detect_notifier_callback;
    rc = hisi_charger_type_notifier_register(&ts->charger_detect_notif);
    if (rc < 0) {
        HOST_LOG_ERR("%s, hisi_charger_type_notifier_register failed\n", __func__);
        ts->charger_detect_notif.notifier_call = NULL;
    } else {
        HOST_LOG_INFO("%s, already registe charger_detect_notifier_callback\n", __func__);
        charger_detect_notifier_callback(NULL, hisi_get_charger_type(), NULL);
    }
#endif

    if (ts->wake_up)
    { device_init_wakeup(&client->dev, ts->wake_up); }

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
    set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif

    return 0;

err_register_misc:
    fb_unregister_client(&ts->fb_notif);
err_register_fb_notif:
    bu21150_free_gpio(ts);
err_init:
    mutex_destroy(&ts->mutex_frame);
    mutex_destroy(&ts->mutex_wake);
    mutex_destroy(&ts->spi_mutex_lock);
    kfree(ts);
    return rc;
}

static int bu21150_fb_suspend(struct device* dev)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    struct spi_device* client = ts->client;
    int rc;
    u8 buf1[2] = {0x00, 0x00};
    u8 buf2[2] = {0x04, 0x00};
    struct bu21150_frame* temp;
    struct list_head* pos, *n;
    struct mutex* spi_mutex_lock = &ts->spi_mutex_lock;

    if (ts->suspended)
    { return 0; }

    rc = bu21150_write_register(REG_SENS_START, (u16)sizeof(buf1), buf1);

    if (rc)
    { HOST_LOG_ERR("failed to disable sensing (%d)\n", rc); }

    ts->timeout_enb = 0;
    get_frame_timer_delete();

    ts->unblock_flag = 1;
    /* wake up */
    wake_up_frame_waitq(ts);

    /* empty list */
    mutex_lock(&ts->mutex_frame);
    list_for_each_safe(pos, n, &ts->frame_list.list)
    {
        temp = list_entry(pos, struct bu21150_frame, list);
        list_del(pos);
        kfree(temp);
    }
    ts->frame_count = 0;
    mutex_unlock(&ts->mutex_frame);

    rc = bu21150_write_register(REG_INT_RUN_ENB, (u16)sizeof(buf2), buf2);

    if (rc)
    { HOST_LOG_ERR("failed to write to REG_INT_RUN_ENB (%d)\n", rc); }

    if (!ts->wake_up)
    { disable_irq(client->irq); }

    ts->suspended = true;

    // pull down reset
    mutex_lock(spi_mutex_lock);
    gpio_set_value(ts->rst_gpio, BU21150_RESET_LOW);
    gpio_set_value(ts->cs_gpio, GPIO_LOW);
    HOST_LOG_INFO("bu21150_fb_suspend, pull cs_gpio and rst_gpio down. cs_gpio: %d\n", gpio_get_value(ts->cs_gpio));
    mutex_unlock(spi_mutex_lock);

    return 0;
}

static int bu21150_fb_early_resume(struct device* dev)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    int rc;
    u8 buf[2] = {0x01, 0x00};
    struct bu21150_frame* temp;
    struct list_head* pos, *n;
    struct mutex* spi_mutex_lock = &ts->spi_mutex_lock;

    if (!ts->suspended)
    { return 0; }

    mutex_lock(spi_mutex_lock);
    gpio_set_value(ts->rst_gpio, BU21150_RESET_HIGH);
    mutex_unlock(spi_mutex_lock);
    HOST_LOG_INFO("bu21150_fb_early_resume, pull rst_gpio high\n");

    ts->timeout_enb = 0;
    get_frame_timer_delete();

    rc = bu21150_write_register(REG_INT_RUN_ENB, (u16)sizeof(buf), buf);

    if (rc)
        HOST_LOG_ERR("failed to write %d to REG_INT_RUN_ENB (%d)\n",
                     buf[0], rc);

    msleep(70);

    buf[0] = 0x03;
    rc = bu21150_write_register(REG_INT_RUN_ENB, (u16)sizeof(buf), buf);

    if (rc)
        HOST_LOG_ERR
        ("failed to write %d to REG_INT_RUN_ENB (%d)\n",
         buf[0], rc);

    /* empty list */
    mutex_lock(&ts->mutex_frame);
    list_for_each_safe(pos, n, &ts->frame_list.list)
    {
        temp = list_entry(pos, struct bu21150_frame, list);
        list_del(pos);
        kfree(temp);
    }
    ts->frame_count = 0;
    mutex_unlock(&ts->mutex_frame);

    ts->suspended = false;

    return 0;
}

static int bu21150_fb_resume(struct device* dev)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    int rc;
    u8 buf[2] = {0x81, 0x07};

    if (!ts->wake_up)
    {
        rc = bu21150_write_register(REG_IRQ_SETTINGS,
                                    (u16)sizeof(buf), buf);

        if (rc)
            HOST_LOG_ERR
            ("failed to write %d to REG_IRQ_SETTINGS (%d)\n",
             buf[0], rc);

        enable_irq(ts->client->irq);
    }

    return 0;
}

static int fb_notifier_callback(struct notifier_block* self,
                                unsigned long event, void* data)
{
    struct fb_event* evdata = data;
    int* blank;
    struct bu21150_data* ts =
        container_of(self, struct bu21150_data, fb_notif);
    struct device* dev;

    if (evdata && evdata->data && ts && ts->client)
    {
        HOST_LOG_ERR("g_io_opened not zero.\n");
        dev = &ts->client->dev;
        blank = evdata->data;

        if (event == FB_EARLY_EVENT_BLANK)
        {
            if (*blank == FB_BLANK_UNBLANK)
            {
                ts->lcd_on = LCD_ON;
                bu21150_fb_early_resume(dev);
            }
            else if (*blank == FB_BLANK_POWERDOWN)
            {
                ts->lcd_on = LCD_OFF;
            }
        }
        else if (event == FB_R_EARLY_EVENT_BLANK)
        {
            if (*blank == FB_BLANK_UNBLANK)
            {
                ts->lcd_on = LCD_OFF;
                bu21150_fb_early_resume(dev);
            }
            else if (*blank == FB_BLANK_POWERDOWN)
            {
                ts->lcd_on = LCD_ON;
            }
        }
        else if (event == FB_EVENT_BLANK && *blank ==
                 FB_BLANK_POWERDOWN)
        {
            bu21150_fb_suspend(dev);
        }
        else if (event == FB_EVENT_BLANK && *blank ==
                 FB_BLANK_UNBLANK)
        {
            bu21150_fb_resume(dev);
        }
    }

    return 0;
}

static void get_frame_timer_init(void)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    if (ts->set_timer_flag == 1)
    {
        del_timer_sync(&get_frame_timer);
        ts->set_timer_flag = 0;
    }

    if (ts->timeout > 0)
    {
        ts->set_timer_flag = 1;
        ts->timeout_flag = 0;

        init_timer(&get_frame_timer);

        get_frame_timer.expires	 = jiffies + ts->timeout;
        get_frame_timer.data	 = (unsigned long)jiffies;
        get_frame_timer.function = get_frame_timer_handler;

        add_timer(&get_frame_timer);
    }
}

static void get_frame_timer_handler(unsigned long data)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ts->timeout_flag = 1;
    /* wake up */
    wake_up_frame_waitq(ts);
}

static void get_frame_timer_delete(void)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    if (ts->set_timer_flag == 1)
    {
        ts->set_timer_flag = 0;
        del_timer_sync(&get_frame_timer);
    }
}

static int bu21150_remove(struct spi_device* client)
{
    struct bu21150_data* ts = spi_get_drvdata(client);

    mutex_destroy(&ts->mutex_wake);
    if (ts->wake_up)
    { device_init_wakeup(&client->dev, 0); }

    sysfs_remove_group(ts->bu21150_obj, &bu21150_ts_prop_attr_group);
    sysfs_remove_group(&ts->ts_dev->dev.kobj, &bu21150_ts_attr_group);
    platform_device_unregister(ts->ts_dev);

#if defined(HOST_CHARGER_FB)
    if (NULL != ts->charger_detect_notif.notifier_call) {
        hisi_charger_type_notifier_unregister(&ts->charger_detect_notif);
        HOST_LOG_INFO("hisi_charger_type_notifier_unregister called\n");
    }
#endif

    fb_unregister_client(&ts->fb_notif);
    misc_deregister(&g_bu21150_misc_device);
    mutex_destroy(&ts->mutex_frame);
    kfree(ts);
    input_mt_wrapper_exit();

    return 0;
}

static int bu21150_open(struct inode* inode, struct file* filp)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    struct bu21150_frame* temp;
    struct list_head* pos, *n;

    if (g_io_opened)
    {
        HOST_LOG_ERR("g_io_opened not zero.\n");
        return -EBUSY;
    }

    ++g_io_opened;

    get_frame_timer_delete();

    ts->reset_flag = 0;
    ts->set_timer_flag = 0;
    ts->timeout_flag = 0;
    ts->timeout_enb = 0;
    ts->unblock_flag = 0;
    ts->force_unblock_flag = 0;
    ts->scan_mode = AFE_SCAN_MUTUAL_CAP;

    memset(&(ts->req_get), 0, sizeof(struct bu21150_ioctl_get_frame_data));
    /* set default value. */
    ts->req_get.size = FRAME_HEADER_SIZE;
    memset(&(ts->frame_get), 0,
           sizeof(struct bu21150_ioctl_get_frame_data));
    memset(&(ts->frame_work_get), 0,
           sizeof(struct bu21150_ioctl_get_frame_data));

    /* empty list */
    mutex_lock(&ts->mutex_frame);
    list_for_each_safe(pos, n, &ts->frame_list.list)
    {
        temp = list_entry(pos, struct bu21150_frame, list);
        list_del(pos);
        kfree(temp);
    }
    ts->frame_count = 0;
    mutex_unlock(&ts->mutex_frame);

    ts->irq_enabled = false;

    return 0;
}

static int bu21150_release(struct inode* inode, struct file* filp)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    struct spi_device* client = ts->client;

    if (!g_io_opened)
    {
        HOST_LOG_ERR("!g_io_opened\n");
        return -ENOTTY;
    }

    --g_io_opened;

    if (g_io_opened < 0)
    { g_io_opened = 0; }

    wake_up_frame_waitq(ts);
    get_frame_timer_delete();

    if (ts->irq_enabled)
    {
        free_irq(client->irq, ts);
        ts->irq_enabled = false;
    }

    return 0;
}

static long bu21150_ioctl(struct file* filp, unsigned int cmd,
                          unsigned long arg)
{
    long ret;

    switch (cmd)
    {
        case BU21150_IOCTL_CMD_GET_FRAME:
            ret = bu21150_ioctl_get_frame(arg);
            break;

        case BU21150_IOCTL_CMD_RESET:
            ret = bu21150_ioctl_reset(arg);
            break;

        case BU21150_IOCTL_CMD_SPI_READ:
            ret = bu21150_ioctl_spi_read(arg);
            break;

        case BU21150_IOCTL_CMD_SPI_WRITE:
            ret = bu21150_ioctl_spi_write(arg);
            break;

        case BU21150_IOCTL_CMD_UNBLOCK:
            ret = bu21150_ioctl_unblock();
            break;

        case BU21150_IOCTL_CMD_UNBLOCK_RELEASE:
            ret = bu21150_ioctl_unblock_release();
            break;

        case BU21150_IOCTL_CMD_SUSPEND:
            ret = bu21150_ioctl_suspend();
            break;

        case BU21150_IOCTL_CMD_RESUME:
            ret = bu21150_ioctl_resume();
            break;

        case BU21150_IOCTL_CMD_SET_TIMEOUT:
            ret = bu21150_ioctl_set_timeout(arg);
            break;

        case BU21150_IOCTL_CMD_SET_SCAN_MODE:
            ret = bu21150_ioctl_set_scan_mode(arg);
            break;

        case BU21150_IOCTL_CMD_SET_POWER:
            ret = bu21150_ioctl_set_power(arg);
            break;

        default:
            HOST_LOG_ERR("cmd unknown.\n");
            ret = -EINVAL;
    }

    return ret;
}

static long bu21150_ioctl_get_frame(unsigned long arg)
{
    long ret;
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    void __user* argp = (void __user*)arg;
    struct bu21150_ioctl_get_frame_data data;
    u32 frame_size;

    if (arg == 0)
    {
        HOST_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }

    if (copy_from_user(&data, argp,
                       sizeof(struct bu21150_ioctl_get_frame_data)))
    {
        HOST_LOG_ERR("Failed to copy_from_user().\n");
        return -EFAULT;
    }

    if (data.buf == 0 || data.size == 0 ||
        MAX_FRAME_SIZE < data.size || data.tv == 0)
    {
        HOST_LOG_ERR("data.buf == 0 ...\n");
        return -EINVAL;
    }

    if (!ts->irq_enabled)
    {
        if (ts->client->irq < 0)
        { HOST_LOG_ERR("error irq:%d\n", ts->client->irq); }

        ret = request_threaded_irq(ts->client->irq, NULL,
                                   bu21150_irq_thread,
                                   IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                   ts->client->dev.driver->name, ts);

        if (ret)
        { return ret; }

        ts->irq_enabled = true;
    }

    if (ts->timeout_enb == 1)
    { get_frame_timer_init(); }

    do
    {
        ts->req_get = data;
        ret = wait_frame_waitq(ts, data.keep_block_flag);
        ts->unblock_flag = 0;

        if (ret != 0)
        { return ret; }
    }
    while (!is_same_bu21150_ioctl_get_frame_data(&data,
            &(ts->frame_get)));

    if (ts->timeout_enb == 1)
    { get_frame_timer_delete(); }

    /* copy frame */
    mutex_lock(&ts->mutex_frame);
    frame_size = ts->frame_get.size;

    if (!list_empty(&ts->frame_list.list))
    {
        struct bu21150_frame* temp;

        temp = list_first_entry(&ts->frame_list.list,
                                struct bu21150_frame, list);

        if (copy_to_user(data.buf, temp->frame, frame_size))
        {
            mutex_unlock(&ts->mutex_frame);
            HOST_LOG_ERR("Failed to copy_to_user().\n");
            return -EFAULT;
        }

        if (copy_to_user(data.tv, &(temp->tv),
                         sizeof(struct timeval)))
        {
            mutex_unlock(&ts->mutex_frame);
            HOST_LOG_ERR("Failed to copy_to_user().\n");
            return -EFAULT;
        }

        list_del(&temp->list);
        kfree(temp);
        ts->frame_count--;
    }
    else
    {
        HOST_LOG_DEBUG("no frame!!! unblock\n");
        mutex_unlock(&ts->mutex_frame);
        return BU21150_UNBLOCK;
    }

    if (!list_empty(&ts->frame_list.list))
    { wake_up_frame_waitq(ts); }

    mutex_unlock(&ts->mutex_frame);

    return 0;
}

static long bu21150_ioctl_reset(unsigned long reset)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    HOST_LOG_DEBUG("bu21150_ioctl_reset\n");

    if (!(reset == BU21150_RESET_LOW || reset == BU21150_RESET_HIGH))
    {
        HOST_LOG_ERR("arg unknown.\n");
        return -EINVAL;
    }

    gpio_set_value(ts->rst_gpio, reset);

    ts->frame_waitq_flag = WAITQ_WAIT;

    if (reset == BU21150_RESET_LOW)
    { ts->reset_flag = 1; }

    return 0;
}

static long bu21150_ioctl_spi_read(unsigned long arg)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    void __user* argp = (void __user*)arg;
    struct bu21150_ioctl_spi_data data;
    int ret;

    if (arg == 0)
    {
        HOST_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }

    if (copy_from_user(&data, argp,
                       sizeof(struct bu21150_ioctl_spi_data)))
    {
        HOST_LOG_ERR("Failed to copy_from_user().\n");
        return -EFAULT;
    }

    if (data.buf == 0 || data.count == 0 ||
        MAX_FRAME_SIZE < data.count)
    {
        HOST_LOG_ERR("data.buf == 0 ...\n");
        return -EINVAL;
    }

    HOST_LOG_DEBUG("%s called\n", __func__);
    ret = bu21150_read_register(data.addr, data.count, ts->spi_buf);

    if (ret)
    {
        HOST_LOG_ERR("Failed to read register (%d).\n", ret);
        return ret;
    }

    if (copy_to_user(data.buf, ts->spi_buf, data.count))
    {
        HOST_LOG_ERR("Failed to copy_to_user().\n");
        return -EFAULT;
    }

    return 0;
}

static long bu21150_ioctl_spi_write(unsigned long arg)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    void __user* argp = (void __user*)arg;
    struct bu21150_ioctl_spi_data data;
    unsigned int afe_active_mode = AFE_SCAN_SELF_CAP | AFE_SCAN_MUTUAL_CAP;
    unsigned int afe_gesture_mode = AFE_SCAN_GESTURE_SELF_CAP |
                                    AFE_SCAN_GESTURE_MUTUAL_CAP;
    bool valid_op;
    int ret;

    if (arg == 0)
    {
        HOST_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }

    if (copy_from_user(&data, argp,
                       sizeof(struct bu21150_ioctl_spi_data)))
    {
        HOST_LOG_ERR("Failed to copy_from_user().\n");
        return -EFAULT;
    }

    valid_op = (data.next_mode == AFE_SCAN_DEFAULT) ||
               ((data.next_mode & afe_active_mode) && ts->lcd_on == LCD_ON) ||
               ((data.next_mode & afe_gesture_mode) && ts->lcd_on == LCD_OFF);

    if (!valid_op)
    {
        HOST_LOG_INFO("AFE scan mode(%d) and LCD state(%d),\n",
                      data.next_mode, ts->lcd_on);
        return -EINVAL;
    }

    if (data.buf == 0 || data.count == 0 ||
        MAX_FRAME_SIZE < data.count)
    {
        HOST_LOG_ERR("data.buf == 0 ...\n");
        return -EINVAL;
    }

    if (copy_from_user(ts->spi_buf, data.buf, data.count))
    {
        HOST_LOG_ERR("Failed to copy_from_user()..\n");
        return -EFAULT;
    }

    ret = bu21150_write_register(data.addr, data.count, ts->spi_buf);

    if (ret)
    { HOST_LOG_ERR("Failed to write register (%d).\n", ret); }

    return ret;
}

static long bu21150_ioctl_unblock(void)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    ts->force_unblock_flag = 1;
    /* wake up */
    wake_up_frame_waitq(ts);

    return 0;
}

static long bu21150_ioctl_unblock_release(void)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    struct bu21150_frame* temp;
    struct list_head* pos, *n;

    ts->force_unblock_flag = 0;

    ts->frame_waitq_flag = WAITQ_WAIT;

    /* empty list */
    mutex_lock(&ts->mutex_frame);
    list_for_each_safe(pos, n, &ts->frame_list.list)
    {
        temp = list_entry(pos, struct bu21150_frame, list);
        list_del(pos);
        kfree(temp);
    }
    ts->frame_count = 0;
    mutex_unlock(&ts->mutex_frame);

    return 0;
}

static long bu21150_ioctl_suspend(void)
{
    bu21150_ioctl_unblock();

    return 0;
}

static long bu21150_ioctl_resume(void)
{
    bu21150_ioctl_unblock_release();

    return 0;
}

static long bu21150_ioctl_set_timeout(unsigned long arg)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    void __user* argp = (void __user*)arg;
    struct bu21150_ioctl_timeout_data data;

    if (copy_from_user(&data, argp,
                       sizeof(struct bu21150_ioctl_timeout_data)))
    {
        HOST_LOG_ERR("Failed to copy_from_user().\n");
        return -EFAULT;
    }

    ts->timeout_enb = data.timeout_enb;

    if (data.timeout_enb == 1)
    {
        ts->timeout = (unsigned int)(data.report_interval_us
                                     * TIMEOUT_SCALE * HZ / 1000000);
    }
    else
    {
        get_frame_timer_delete();
    }

    return 0;
}

static long bu21150_ioctl_set_scan_mode(unsigned long arg)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    void __user* argp = (void __user*)arg;

    if (copy_from_user(&ts->scan_mode, argp,
                       sizeof(u16)))
    {
        HOST_LOG_ERR("Failed to copy_from_user().\n");
        return -EFAULT;
    }

    return 0;
}

static long bu21150_ioctl_set_power(unsigned long power)
{
    if (!(power == BU21150_POWER_OFF || power == BU21150_POWER_ON))
    {
        HOST_LOG_ERR("arg unknown.\n");
        return -EINVAL;
    }

    /* TODO*/
    /* Add power control function using argument "power"*/

    return 0;
}

static irqreturn_t bu21150_irq_thread(int irq, void* dev_id)
{
    struct bu21150_data* ts = dev_id;
    u8* psbuf = (u8*)ts->frame_work;
    int ret;

    /* get frame */
    ts->frame_work_get = ts->req_get;
    ret = bu21150_read_register(REG_READ_DATA,
                                ts->frame_work_get.size, psbuf);

    if (ret)
    {
        HOST_LOG_ERR("failed to read frame (%d)\n", ret);
        goto err_read_reg;
    }

    if (ts->reset_flag == 0)
    {
#ifdef CHECK_SAME_FRAME
        check_same_frame(ts);
#endif
        copy_frame(ts);
        wake_up_frame_waitq(ts);
    }
    else
    {
        ts->reset_flag = 0;
    }

err_read_reg:
    return IRQ_HANDLED;
}

static int bu21150_spi_transfer(struct bu21150_data* ts,
                                struct spi_transfer* xfer, struct spi_message* msg)
{
    int ret;
    struct spi_device* client = ts->client;
    struct mutex* spi_mutex_lock = &ts->spi_mutex_lock;

    HOST_LOG_DEBUG("%s called\n", __func__);
    spi_message_add_tail(xfer, msg);
    mutex_lock(spi_mutex_lock);
    ret = spi_sync(client, msg);
    if (ts->suspended)
    { gpio_set_value(ts->cs_gpio, GPIO_LOW);}
    mutex_unlock(spi_mutex_lock);

    return ret;
}

static int bu21150_read_register(u32 addr, u16 size, u8* data)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    int ret = -ENOMEM;

    u8* input = kzalloc(sizeof(u8) * (size) + SPI_HEADER_SIZE, GFP_KERNEL);
    if (!input) {
        goto failed;
    }
    u8* output = kzalloc(sizeof(u8) * (size) + SPI_HEADER_SIZE, GFP_KERNEL);
    if (!output) {
        goto failed;
    }
    struct ser_req* req = kzalloc(sizeof(struct ser_req), GFP_KERNEL);
    if (!req) {
        goto failed;
    }

    /* set header */
    input[0] = 0x03;				 /* read command */
    input[1] = (addr & 0xFF00) >> 8; /* address hi */
    input[2] = (addr & 0x00FF) >> 0; /* address lo */

    /* read data */
    spi_message_init(&req->msg);
    req->xfer[0].tx_buf = input;
    req->xfer[0].rx_buf = output;
    req->xfer[0].len = size + SPI_HEADER_SIZE;
    req->xfer[0].cs_change = 1;
    req->xfer[0].bits_per_word = SPI_BITS_PER_WORD_READ;
    HOST_LOG_DEBUG("%s called\n", __func__);
    ret = bu21150_spi_transfer(ts, &req->xfer[0], &req->msg);

    if (ret)
    {
        HOST_LOG_ERR("spi_sync read data error:ret=[%d]\n", ret);
#if defined(CONFIG_HUAWEI_DSM)
        ts->dsm_info.constraints_SPI_status = ret;
        if (!host_detfail_dsm)
        {
            HOST_LOG_ERR("chip read init no need dsm.\n");
            goto failed;
        }

        if (!dsm_client_ocuppy(tphostprocessing_dclient))
        {
            dsm_client_record(tphostprocessing_dclient,
                              "irq_gpio:%d\tvalue:%d.\n\
				reset_gpio:%d\t value:%d.\n\
				Host_SPI_status:%d.\n",
                              ts->irq_gpio,
                              gpio_get_value(ts->irq_gpio),
                              ts->rst_gpio,
                              gpio_get_value(ts->rst_gpio),
                              ts->dsm_info.constraints_SPI_status);
            dsm_client_notify(tphostprocessing_dclient,
                              DSM_TPHOSTPROCESSING_SPI_RW_ERROR_NO);
        }

#endif
    }
    else
    {
        memcpy(data, output + SPI_HEADER_SIZE, size);
        swap_2byte(data, size);
    }

failed:
    if (req)
        kfree(req);
    if (input)
        kfree(input);
    if (output)
        kfree(output);
    return ret;
}

static int bu21150_write_register(u32 addr, u16 size, u8* data)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);
    int ret = -ENOMEM;

    u8* input = kzalloc(sizeof(u8) * (size) + SPI_HEADER_SIZE, GFP_KERNEL);
    if (!input) {
        goto failed;
    }
    struct ser_req* req = kzalloc(sizeof(*req), GFP_KERNEL);
    if (!req) {
        goto failed;
    }

    /* set header */
    input[0] = 0x02;		 /* write command */
    input[1] = (addr & 0xFF00) >> 8; /* address hi */
    input[2] = (addr & 0x00FF) >> 0; /* address lo */

    /* set data */
    memcpy(input + SPI_HEADER_SIZE, data, size);
    swap_2byte(input + SPI_HEADER_SIZE, size);

    /* write data */
    spi_message_init(&req->msg);
    req->xfer[0].tx_buf = input;
    req->xfer[0].rx_buf = NULL;
    req->xfer[0].len = size + SPI_HEADER_SIZE;
    req->xfer[0].cs_change = 1;
    req->xfer[0].bits_per_word = SPI_BITS_PER_WORD_WRITE;
    HOST_LOG_DEBUG("%s called\n", __func__);
    ret = bu21150_spi_transfer(ts, &req->xfer[0], &req->msg);

    if (ret)
    {
        HOST_LOG_ERR("spi_sync write data error:ret=[%d]\n", ret);
#if defined(CONFIG_HUAWEI_DSM)
        ts->dsm_info.constraints_SPI_status = ret;
        if (!host_detfail_dsm)
        {
            HOST_LOG_ERR("chip read init no need dsm.\n");
            goto failed;
        }

        if (!dsm_client_ocuppy(tphostprocessing_dclient))
        {
            dsm_client_record(tphostprocessing_dclient,
                              "irq_gpio:%d\tvalue:%d.\n\
				reset_gpio:%d\t value:%d.\n\
				Host_SPI_status:%d.\n",
                              ts->irq_gpio,
                              gpio_get_value(ts->irq_gpio),
                              ts->rst_gpio,
                              gpio_get_value(ts->rst_gpio),
                              ts->dsm_info.constraints_SPI_status);
            dsm_client_notify(tphostprocessing_dclient,
                              DSM_TPHOSTPROCESSING_SPI_RW_ERROR_NO);
        }

#endif
    }

failed:
    if (req)
        kfree(req);
    if (input)
        kfree(input);
    return ret;
}

static void wake_up_frame_waitq(struct bu21150_data* ts)
{
    ts->frame_waitq_flag = WAITQ_WAKEUP;
    wake_up_interruptible(&(ts->frame_waitq));
}

/*lint -save -e40 -e10 -e522 -e578 */
static long wait_frame_waitq(struct bu21150_data* ts, u8 flag)
{
    if (ts->force_unblock_flag == 1)
    { return BU21150_UNBLOCK; }

    if (ts->unblock_flag == 1 && flag == 0)
    { return BU21150_UNBLOCK; }

    /* wait event */
    if (wait_event_interruptible(ts->frame_waitq, (ts->frame_waitq_flag == WAITQ_WAKEUP)))
    {
        HOST_LOG_ERR("-ERESTARTSYS\n");
        return -ERESTARTSYS;
    }

    ts->frame_waitq_flag = WAITQ_WAIT;

    if (ts->timeout_enb == 1)
    {
        if (ts->timeout_flag == 1)
        {
            ts->set_timer_flag = 0;
            ts->timeout_flag = 0;
            HOST_LOG_ERR("return BU21150_TIMEOUT\n");
#if defined(CONFIG_HUAWEI_DSM)

            if (!dsm_client_ocuppy(tphostprocessing_dclient))
            {
                dsm_client_record(tphostprocessing_dclient,
                                  "irq_gpio:%d\tvalue:%d.\n\
					reset_gpio:%d\tvalue:%d.\n\
					BU21150_status:%d.\n",
                                  ts->irq_gpio,
                                  gpio_get_value(ts->irq_gpio),
                                  ts->rst_gpio,
                                  gpio_get_value(ts->rst_gpio),
                                  BU21150_TIMEOUT);
                dsm_client_notify(tphostprocessing_dclient,
                                  DSM_TPHOSTPROCESSING_DEV_STATUS_ERROR_NO);
            }

#endif
            return BU21150_TIMEOUT;
        }
    }

    return 0;
}
/*lint -restore*/

static int is_same_bu21150_ioctl_get_frame_data(
    struct bu21150_ioctl_get_frame_data* data1,
    struct bu21150_ioctl_get_frame_data* data2)
{
    size_t i;
    u8* p1 = (u8*)data1;
    u8* p2 = (u8*)data2;

    for (i = 0; i < sizeof(struct bu21150_ioctl_get_frame_data); i++)
    {
        if (p1[i] != p2[i])
        { return 0; }
    }

    return 1;
}

static void copy_frame(struct bu21150_data* ts)
{
    struct bu21150_frame* temp;

    mutex_lock(&(ts->mutex_frame));

    /* check for max limit */
    if (ts->frame_count >= BU21150_LIST_MAX_FRAMES)
    {
        struct bu21150_frame* tmp;

        HOST_LOG_ERR("max limit!!! frame_count=%d\n", ts->frame_count);
        tmp = list_first_entry(&ts->frame_list.list,
                               struct bu21150_frame, list);
        list_del(&tmp->list);
        kfree(tmp);
        ts->frame_count--;
    }

    temp = kzalloc(sizeof(struct bu21150_frame), GFP_KERNEL);
    ts->frame_get = ts->frame_work_get;
    memcpy(temp->frame, ts->frame_work, MAX_FRAME_SIZE);
    do_gettimeofday(&(temp->tv));
    list_add_tail(&(temp->list), &(ts->frame_list.list));
    ts->frame_count++;
    mutex_unlock(&(ts->mutex_frame));
}

static void swap_2byte(unsigned char* buf, unsigned int size)
{
    unsigned int i;

    if (size % REG_SIZE == 1)
    {
        HOST_LOG_ERR("error size is odd. size=[%u]\n", size);
        return;
    }

    for (i = 0; i < size; i += REG_SIZE)
    { be16_to_cpus(buf + i); }
}

#ifdef CHECK_SAME_FRAME
static void check_same_frame(struct bu21150_data* ts)
{
    static int frame_no = -1;
    u16* ps;
    struct bu21150_frame* temp;

    mutex_lock(&ts->mutex_frame);

    if (!list_empty(&ts->frame_list.list))
    {
        /* get the last node */
        temp = list_entry(&ts->frame_list.list->prev,
                          struct bu21150_frame, list);

        ps = (u16*)temp->frame;

        if (ps[2] == frame_no)
        { HOST_LOG_ERR("same_frame_no=[%d]\n", frame_no); }

        frame_no = ps[2];
    }

    mutex_unlock(&ts->mutex_frame);
}
#endif

/*lint -save -e522 -e64 */
static void bu21150_spi_cs_set(u32 control)
{
    int ret = 0;
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    if (!ts)
    {
        HOST_LOG_ERR("no driver data");
        return;
    }

    if (SSP_CHIP_SELECT == control)
    {
        ret = gpio_direction_output(ts->cs_gpio, control);
        /* cs steup time at least 10ns */
        ndelay(10);
    }
    else
    {
        /* cs hold time at least 4*40ns(@25MHz) */
        ret = gpio_direction_output(ts->cs_gpio, control);
        ndelay(200);
    }

    if (ret < 0)
    { HOST_LOG_ERR("fail to set gpio cs, result = %d.", ret); }
}

static int parse_dtsi(struct device* dev, struct bu21150_data* ts)
{
    struct device_node* np = dev->of_node;
    int retval = of_property_read_u32(np, "spi-max-frequency", &ts->max_speed_hz);
    if (retval) {
        ts->max_speed_hz = BU21150_SPI_SPEED_DEFAULT;
    }
    HOST_LOG_INFO("spi-max-frequency: %d", ts->max_speed_hz);
    retval = of_property_read_u32(np, "jdi_bu21150,irq_gpio", &ts->irq_gpio);
    if (retval) {
		HOST_LOG_ERR("get irq_gpio failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "jdi_bu21150,rst_gpio", &ts->rst_gpio);
    if (retval) {
		HOST_LOG_ERR("get rst_gpio failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "jdi_bu21150,cs_gpio", &ts->cs_gpio);
    if (retval) {
		HOST_LOG_ERR("get cs_gpio failed\n");
        return retval;
	}
    ts->spidev0_chip_info.hierarchy = SSP_MASTER;
    retval = of_property_read_u32(np, "pl022,interface",
                         &ts->spidev0_chip_info.iface);
    if (retval) {
		HOST_LOG_ERR("get iface failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "pl022,com-mode",
                         &ts->spidev0_chip_info.com_mode);
    if (retval) {
		HOST_LOG_ERR("get com_mode failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "pl022,rx-level-trig",
                         &ts->spidev0_chip_info.rx_lev_trig);
    if (retval) {
		HOST_LOG_ERR("get rx_lev_trig failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "pl022,tx-level-trig",
                         &ts->spidev0_chip_info.tx_lev_trig);
    if (retval) {
		HOST_LOG_ERR("get tx_lev_trig failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "pl022,ctrl-len",
                         &ts->spidev0_chip_info.ctrl_len);
    if (retval) {
		HOST_LOG_ERR("get ctrl_len failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "pl022,wait-state",
                         &ts->spidev0_chip_info.wait_state);
    if (retval) {
		HOST_LOG_ERR("get wait_state failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "pl022,duplex",
                         &ts->spidev0_chip_info.duplex);
    if (retval) {
		HOST_LOG_ERR("get duplex failed\n");
        return retval;
	}
    retval = of_property_read_u32(np, "jdi_bu21150,abs_max_x",
                         &g_abs_max_x);
    if (retval) {
		HOST_LOG_ERR("get abs_max_x failed\n");
        g_abs_max_x = 1080;
	}
    retval = of_property_read_u32(np, "jdi_bu21150,abs_max_y",
                         &g_abs_max_y);
    if (retval) {
		HOST_LOG_ERR("get abs_max_y failed\n");
        g_abs_max_y = 1920;
	}
    ts->spidev0_chip_info.cs_control = bu21150_spi_cs_set;
    HOST_LOG_INFO("abs_max_x: %d,abs_max_y:%d", g_abs_max_x,g_abs_max_y);
    return 0;
}
/*lint -restore*/

int host_ts_power_control_notify(enum host_ts_pm_type pm_type,	int timeout)
{
    int error = 0;

    switch (pm_type)
    {
        case HOST_TS_BEFORE_SUSPEND:	/*do something before suspend */
            break;
        case HOST_TS_SUSPEND_DEVICE:	/*device power off or sleep */
            break;
        case HOST_TS_RESUME_DEVICE:	/*device power on or wakeup */
            break;
        case HOST_TS_AFTER_RESUME:	/*do something after resume */
            break;
        default:
            HOST_LOG_ERR("pm_type = %d\n", pm_type);
            error = -EINVAL;
            break;
    }

    return error;
}

#if defined(HOST_CHARGER_FB)
static int charger_detect_notifier_callback(struct notifier_block *self,
                                unsigned long event, void *data)
{
    struct bu21150_data* ts = spi_get_drvdata(g_client_bu21150);

    HOST_LOG_INFO("%s called, charger type: %d, [0 in, other out].\n", __func__, event);
    if (CHARGER_TYPE_NONE == event) {
        ts->host_enable_flag.charger_state = 0;
    } else {
        ts->host_enable_flag.charger_state = 1;
    }
    return 0;
}
#endif

