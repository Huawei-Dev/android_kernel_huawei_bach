/*
 * Japan Display Inc. INPUT_MT_WRAPPER Device Driver
 *
 * Copyright (C) 2013-2014 Japan Display Inc.
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include "input_mt_wrapper.h"
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#define DEVICE_NAME	  "input_mt_wrapper"
/*#define TYPE_B_PROTOCOL*/

static int input_mt_wrapper_open(struct inode* inode, struct file* filp);
static int input_mt_wrapper_release(struct inode* inode, struct file* filp);
static long input_mt_wrapper_ioctl(struct file* filp, unsigned int cmd,
                                   unsigned long arg);
static long input_mt_wrapper_ioctl_set_coordinate(unsigned long arg);
static void input_mt_wrapper_release_coordinate(void);

static const struct file_operations g_input_mt_wrapper_fops =
{
    .owner = THIS_MODULE,
    .open = input_mt_wrapper_open,
    .release = input_mt_wrapper_release,
    .unlocked_ioctl = input_mt_wrapper_ioctl,
};

static struct miscdevice g_input_mt_wrapper_misc_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = DEVICE_NAME,
    .fops = &g_input_mt_wrapper_fops,
};

struct input_mt_wrapper_data
{
    struct input_dev* input_dev;
#if defined(CONFIG_FB)
    struct notifier_block fb_notif;
#endif
};

static struct input_mt_wrapper_data* input_mt_wrapper;
u8 host_ts_log_cfg;
extern unsigned int g_abs_max_x;
extern unsigned int g_abs_max_y;
#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block* self,
                                unsigned long event, void* data)
{
    int blank;
    struct fb_event* evdata = data;
    struct input_mt_wrapper_data* input_mt_wrapper_data =
        container_of(self, struct input_mt_wrapper_data, fb_notif);

    if (evdata && evdata->data && event == FB_EVENT_BLANK &&
        input_mt_wrapper_data && input_mt_wrapper_data->input_dev)
    {
        blank = *(int*)(evdata->data);

        if (blank == FB_BLANK_UNBLANK)
        {
            HOST_LOG_INFO("input_mt_wrapper resume\n");
            kobject_uevent(
                &input_mt_wrapper_data->input_dev->dev.kobj,
                KOBJ_ONLINE);
        }
        else if (blank == FB_BLANK_POWERDOWN)
        {
            HOST_LOG_INFO("input_mt_wrapper suspend\n");
            input_mt_wrapper_release_coordinate();
            kobject_uevent(
                &input_mt_wrapper_data->input_dev->dev.kobj,
                KOBJ_OFFLINE);
        }
    }

    return 0;
}
#endif

int input_mt_wrapper_init(void)
{
    struct input_dev* input_dev;
    int error;

    input_mt_wrapper =
        kzalloc(sizeof(struct input_mt_wrapper_data), GFP_KERNEL);

    if (!input_mt_wrapper)
    {
        return -ENOMEM;
    }

    input_dev = input_allocate_device();

    if (!input_dev)
    {
        HOST_LOG_ERR("Unable to allocated input device\n");
        kfree(input_mt_wrapper);
        return	-ENOMEM;
    }

    input_dev->name = "input_mt_wrapper";

    __set_bit(EV_SYN, input_dev->evbit);
    __set_bit(EV_KEY, input_dev->evbit);
    __set_bit(EV_ABS, input_dev->evbit);
    __set_bit(BTN_TOUCH, input_dev->keybit);
    __set_bit(BTN_TOOL_FINGER, input_dev->keybit);
    __set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

    input_set_abs_params(input_dev, ABS_MT_POSITION_X,
                         INPUT_MT_WRAPPER_MIN_X, g_abs_max_x -1, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
                         INPUT_MT_WRAPPER_MIN_Y, g_abs_max_y -1, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_PRESSURE,
                         INPUT_MT_WRAPPER_MIN_Z, INPUT_MT_WRAPPER_MAX_Z, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TRACKING_ID,
                         0, INPUT_MT_WRAPPER_MAX_FINGERS - 1, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
                         INPUT_MT_WRAPPER_MIN_MAJOR, INPUT_MT_WRAPPER_MAX_MAJOR, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOUCH_MINOR,
                         INPUT_MT_WRAPPER_MIN_MINOR, INPUT_MT_WRAPPER_MAX_MINOR, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_ORIENTATION,
                         INPUT_MT_WRAPPER_MIN_ORIENTATION,
                         INPUT_MT_WRAPPER_MAX_ORIENTATION, 0, 0);
    input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE,
                         INPUT_MT_WRAPPER_TOOL_TYPE_MIN,
                         INPUT_MT_WRAPPER_TOOL_TYPE_MAX, 0, 0);

#ifdef TYPE_B_PROTOCOL
    input_mt_init_slots(input_dev, INPUT_MT_WRAPPER_MAX_FINGERS);
#endif

    error = input_register_device(input_dev);

    if (error)
    {
        HOST_LOG_ERR("Failed to register %s input device\n",
                     input_dev->name);
        kfree(input_mt_wrapper);
        return error;
    }

    error = misc_register(&g_input_mt_wrapper_misc_device);

    if (error)
    {
        HOST_LOG_ERR("Failed to register misc device\n");
        input_unregister_device(input_dev);
        kfree(input_mt_wrapper);
        return error;
    }

#if defined(CONFIG_FB)
    input_mt_wrapper->fb_notif.notifier_call = fb_notifier_callback;
    error = fb_register_client(&input_mt_wrapper->fb_notif);

    if (error)
    {
        HOST_LOG_ERR("%s: Unable to register fb_notifier: %d\n",
                     DEVICE_NAME, error);
        goto err_fb_reg;
    }

#endif

    input_mt_wrapper->input_dev = input_dev;

    return 0;

#if defined(CONFIG_FB)
err_fb_reg:
    fb_unregister_client(&input_mt_wrapper->fb_notif);
#endif
    misc_deregister(&g_input_mt_wrapper_misc_device);
    kfree(input_mt_wrapper);

    return error;
}

void input_mt_wrapper_exit(void)
{
    input_unregister_device(input_mt_wrapper->input_dev);
    misc_deregister(&g_input_mt_wrapper_misc_device);
}

static int input_mt_wrapper_open(struct inode* inode, struct file* filp)
{
    return 0;
}

static int input_mt_wrapper_release(struct inode* inode, struct file* filp)
{
    return 0;
}

static long input_mt_wrapper_ioctl(struct file* filp, unsigned int cmd,
                                   unsigned long arg)
{
    long ret;

    switch (cmd)
    {
        case INPUT_MT_WRAPPER_IOCTL_CMD_SET_COORDINATES:
            ret = input_mt_wrapper_ioctl_set_coordinate(arg);
            break;

        default:
            HOST_LOG_ERR("cmd unkown.\n");
            ret = -EINVAL;
    }

    return ret;
}

static long input_mt_wrapper_ioctl_set_coordinate(unsigned long arg)
{
    long ret = 0;
    void __user* argp = (void __user*)arg;
    struct input_dev* input_dev = input_mt_wrapper->input_dev;
    struct input_mt_wrapper_ioctl_touch_data data;
    u8 i;

    if (arg == 0)
    {
        HOST_LOG_ERR("arg == 0.\n");
        return -EINVAL;
    }

    if (copy_from_user(&data, argp,
                       sizeof(struct input_mt_wrapper_ioctl_touch_data)))
    {
        HOST_LOG_ERR("Failed to copy_from_user().\n");
        return -EFAULT;
    }

    for (i = 0; i < INPUT_MT_WRAPPER_MAX_FINGERS; i++)
    {
#ifdef TYPE_B_PROTOCOL
        input_mt_slot(input_dev, i);
        input_mt_report_slot_state(input_dev,
                                   data.touch[i].tool_type, data.touch[i].valid != 0);
#endif

        if (data.touch[i].valid != 0)
        {
            input_report_abs(input_dev, ABS_MT_POSITION_X,
                             data.touch[i].x);
            input_report_abs(input_dev, ABS_MT_POSITION_Y,
                             data.touch[i].y);
            input_report_abs(input_dev, ABS_MT_PRESSURE,
                             data.touch[i].pressure);
            input_report_abs(input_dev, ABS_MT_TRACKING_ID,
                             data.touch[i].tracking_id);
            input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
                             data.touch[i].major);
            input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
                             data.touch[i].minor);
            input_report_abs(input_dev, ABS_MT_ORIENTATION,
                             data.touch[i].orientation);
            input_report_abs(input_dev, ABS_MT_TOOL_TYPE,
                             data.touch[i].tool_type);
#ifndef TYPE_B_PROTOCOL
            input_mt_sync(input_dev);
#endif
        }
    }

    /* BTN_TOUCH DOWN */
    if (data.state == INPUT_MT_WRAPPER_STATE_FIRST_TOUCH)
    { input_report_key(input_dev, BTN_TOUCH, 1); }

    /* BTN_TOUCH UP */
    if (data.state == INPUT_MT_WRAPPER_STATE_LAST_TOUCH)
    {
#ifndef TYPE_B_PROTOCOL
        input_mt_sync(input_dev);
#endif
        input_report_key(input_dev, BTN_TOUCH, 0);
    }

    /* SYN_REPORT */
    input_sync(input_dev);

    return ret;
}

static void input_mt_wrapper_release_coordinate(void)
{

    struct input_dev* input_dev = input_mt_wrapper->input_dev;
    struct input_mt_wrapper_ioctl_touch_data data;
    u8 i;

    memset(&data, 0, sizeof(data));

    for (i = 0; i < INPUT_MT_WRAPPER_MAX_FINGERS; i++)
    {
        input_report_abs(input_dev, ABS_MT_POSITION_X,
                         data.touch[i].x);
        input_report_abs(input_dev, ABS_MT_POSITION_Y,
                         data.touch[i].y);
        input_report_abs(input_dev, ABS_MT_PRESSURE,
                         data.touch[i].pressure);
        input_report_abs(input_dev, ABS_MT_TRACKING_ID,
                         data.touch[i].tracking_id);
        input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR,
                         data.touch[i].major);
        input_report_abs(input_dev, ABS_MT_TOUCH_MINOR,
                         data.touch[i].minor);
        input_report_abs(input_dev, ABS_MT_ORIENTATION,
                         data.touch[i].orientation);
        input_report_abs(input_dev, ABS_MT_TOOL_TYPE,
                         data.touch[i].tool_type);
        input_mt_sync(input_dev);
    }

    /* SYN_REPORT */
    input_report_key(input_dev, BTN_TOUCH, 0);
    input_sync(input_dev);
}

