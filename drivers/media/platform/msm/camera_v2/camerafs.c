/*
 * camera utile class driver 
 *
 *  Author: 	Zhoujie (zhou.jie1981@163.com)
 *  Date:  	2013/01/16
 *  Version:	1.0
 *  History:	2013/01/16      Frist add driver for dual temperature Led,this is virtual device to manage dual temperature Led 
 *  
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/sched.h>

extern int ctrl_camera_power_status(int pt_action_value);
extern bool huawei_cam_is_factory_mode(void);

typedef  struct  _camerafs_class {
	struct class *classptr;
	struct device *pDevice;
}camerafs_class;

typedef  struct  _camerafs_ois_class {
    struct class *classptr;
    struct device *pDevice;
}camerafs_ois_class;

static camerafs_ois_class camerafs_ois;

static camerafs_class camerafs;

//static int brightness_level = 0;
static dev_t devnum;
static dev_t osi_devnum;

#define CAMERAFS_NODE    "node"
#define CAMERAFS_OIS_NODE "ois"

//#define MAX_BRIGHTNESS_FORMMI   (9)

wait_queue_head_t ois_que;
static int ois_done = 0;
static int ois_check = 0;
#define OIS_TEST_TIMEOUT        (HZ * 8)
static int cross_width = -1;
static int cross_height = -1;
static int ic_num = -1;
//spinlock_t pix_lock = SPIN_LOCK_UNLOCKED;
static DEFINE_SPINLOCK(pix_lock);

int register_camerafs_ois_attr(struct device_attribute *attr);

static ssize_t hw_ois_aging_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int done_flag;
    if(sscanf(buf, "%d", &done_flag) <=0)
    {
        pr_err("write data done_flag error");
        return 0;
    }
    pr_err("%s: done_flag = %d", __func__, done_flag);
    ois_done = done_flag;
    wake_up_interruptible(&ois_que);
    return count;
}

static ssize_t hw_ois_aging_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;
    char *offset = buf;

    pr_err("Enter: %s", __func__);
    ois_done = 0;
    //wait for start command
    msleep(50);
    ret = wait_event_interruptible_timeout(ois_que, ois_done != 0, OIS_TEST_TIMEOUT);
    if(ret <= 0) {
        pr_err("%s: wait ois signal timeout", __func__);
    }
    ret = snprintf(offset, PAGE_SIZE, "%d", ois_done);
    offset += ret;

    return (offset - buf);
}

static struct device_attribute hw_ois_aging =
__ATTR(ois_aging, 0664, hw_ois_aging_show, hw_ois_aging_store);

static ssize_t hw_ois_check_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int done_flag;
    if(sscanf(buf, "%d", &done_flag) <= 0)
    {
        pr_err("%s: write data done_flag error", __func__);
        return 0;
    }
    pr_err("%s: done_flag = %d", __func__, done_flag);
    ois_check = done_flag;
    wake_up_interruptible(&ois_que);
    return count;
}

static ssize_t hw_ois_check_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;
    char *offset = buf;

    pr_err("Enter: %s", __func__);
    ois_check = 0;
    //wait for start command
    msleep(50);
    ret = wait_event_interruptible_timeout(ois_que, ois_check != 0, OIS_TEST_TIMEOUT);
    if(ret <= 0) {
        pr_err("%s: wait ois signal timeout", __func__);
    }
    ret = snprintf(offset, PAGE_SIZE, "%d", ois_check);
    offset += ret;

    return (offset - buf);
}

static struct device_attribute hw_ois_check =
__ATTR(ois_check, 0664, hw_ois_check_show, hw_ois_check_store);

// add for ois mmi test

static ssize_t hw_ois_test_mmi_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;
    char *offset = buf;

    pr_err("Enter: %s", __func__);
    spin_lock(&pix_lock);
    ret = snprintf(offset, PAGE_SIZE, "%d,%d\n",
            cross_width, cross_height);
    cross_width = -1;
    cross_height = -1;
    spin_unlock(&pix_lock);
    offset += ret;

    return (offset - buf);
}

static ssize_t hw_ois_test_mmi_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int width, height;

    spin_lock(&pix_lock);

    if(sscanf(buf, "%d,%d", &width, &height) <= 0)
    {
        spin_unlock(&pix_lock);
        pr_err("%s: write data width height error", __func__);
        return 0;
    }

    cross_width = width;
    cross_height = height;
    spin_unlock(&pix_lock);
    pr_err("Enter: %s (%d, %d).", __func__, cross_width, cross_height);

    return count;
}

static struct device_attribute hw_ois_pixel =
__ATTR(ois_pixel, 0664, hw_ois_test_mmi_show, hw_ois_test_mmi_store);
//---

static ssize_t hw_ois_ic_num_show(struct device *dev,
struct device_attribute *attr,char *buf)
{
    int ret = -1;
    char *offset = buf;

    pr_err("Enter: %s", __func__);
    spin_lock(&pix_lock);
    ret = snprintf(offset, PAGE_SIZE, "%d\n", ic_num);
    spin_unlock(&pix_lock);
    offset += ret;

    return (offset - buf);
}

static ssize_t hw_ois_ic_num_store(struct device *dev,
struct device_attribute *attr, const char *buf, size_t count)
{
    int icnum;

    spin_lock(&pix_lock);

    if(sscanf(buf, "%d", &icnum) <= 0)
    {
        spin_unlock(&pix_lock);
        pr_err("%s: write data icnum error", __func__);
        return 0;
    }

    ic_num = icnum;
    spin_unlock(&pix_lock);
    pr_err("Enter: %s (%d).", __func__, ic_num);

    return count;
}

static struct device_attribute hw_ois_icnum =
__ATTR(ois_icnum, 0664, hw_ois_ic_num_show, hw_ois_ic_num_store);

int register_camerafs_attr(struct device_attribute *attr);

static int PT_camera_power_action_value = 0;

static ssize_t store_camerafs_power_status(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t len)
{
    int ret = 0;
    int val = 0;

    if(!dev || !buf)
    {
        pr_err("%s: invalid parameter: null ptr\n", __func__);
        return -1;
    }

    if(!huawei_cam_is_factory_mode())
    {
        pr_err("%s: no factory mode\n",__func__);
        return len;
    }

    /*bit0: 1->power up, 0->power off*/

    /*main camera power*/
    /*bit1: DVDD£ºL2*/
    /*bit2: AF_AVDD£ºL17*/
    /*bit3: AVDD£ºL22*/
    /*bit4: VREG_DOVDD_1P8£ºGPIO134*/

    /*slave camera power*/
    /*bit5: DVDD£ºL23*/
    /*bit6: AVDD£ºGPIO128*/
    /*bit7: VREG_DOVDD_1P8£ºGPIO134*/
    if(sscanf(buf, "%4d", &val) <= 0)
    {
        pr_err("%s: convert power_status error", __func__);
        return 0;
    }
    if ( 0 > val )
    {
        pr_err("%s: get power_status error, val %d", __func__, val);
        return 0;
    }
    PT_camera_power_action_value = val;

    pr_info("%s: action_value = %d\n",__func__,PT_camera_power_action_value);

    ret = ctrl_camera_power_status(PT_camera_power_action_value);
    if(ret < 0)
    {
        pr_err("%s: ctrl_camera_power_status fail ret = %d\n",__func__,ret);
    }

    return len;
}

static ssize_t show_camerafs_power_status(struct device *dev,
		struct device_attribute *attr, char *buf)
{
    int ret = 0;
    char *offset = NULL;
    int action_value = 0;

    if(huawei_cam_is_factory_mode())
    {
        action_value = PT_camera_power_action_value;
    }

    offset = buf;

    ret=snprintf(buf, PAGE_SIZE, "%d\n", action_value);
	pr_info("%s: buf=%s, ret=%d\n", __func__, buf, ret);
    offset += ret;

    return (offset-buf);
}

static DEVICE_ATTR(pt_camera_test, 0664, show_camerafs_power_status, store_camerafs_power_status);

static int __init camerafs_module_init(void)
{
	int ret;
	camerafs.classptr = NULL;
	camerafs.pDevice = NULL;
	spin_lock_init(&pix_lock);

	ret = alloc_chrdev_region(&devnum, 0, 1, CAMERAFS_NODE);
	if(ret)
	{
		pr_err("error %s fail to alloc a dev_t!!!\n",__func__);
		return -1;
	}

	ret = alloc_chrdev_region(&osi_devnum, 0, 1, CAMERAFS_OIS_NODE);
	if(ret)
	{
		pr_err("error %s fail to alloc a dev_t!!!\n",__func__);
		return -1;
	}

	camerafs.classptr= class_create(THIS_MODULE, "camerafs");
	camerafs_ois.classptr = camerafs.classptr;
	if (IS_ERR(camerafs.classptr)) {
		pr_err("class_create failed %d\n", ret);
		ret = PTR_ERR(camerafs.classptr);
		return -1;
	}

	camerafs.pDevice  = device_create(camerafs.classptr, NULL, devnum,NULL,"%s",CAMERAFS_NODE);
	if (IS_ERR(camerafs.pDevice)) {
		pr_err("class_device_create failed %s \n", CAMERAFS_NODE);
		ret = PTR_ERR(camerafs.pDevice);
		return -1;
	}
	camerafs_ois.pDevice = device_create(camerafs_ois.classptr, NULL, osi_devnum, NULL, "%s", CAMERAFS_OIS_NODE);
	if (IS_ERR(camerafs_ois.pDevice)) {
		pr_err("class_device_create failed %s \n", CAMERAFS_OIS_NODE);
		ret = PTR_ERR(camerafs_ois.pDevice);
		return -1;
	}

	register_camerafs_ois_attr(&hw_ois_aging);
	register_camerafs_ois_attr(&hw_ois_pixel);
	register_camerafs_ois_attr(&hw_ois_check);
	register_camerafs_ois_attr(&hw_ois_icnum);
	init_waitqueue_head(&ois_que);

	if(huawei_cam_is_factory_mode()){
		//create camera pt test node
		ret = device_create_file(camerafs.pDevice, &dev_attr_pt_camera_test);
		if (ret<0){
			pr_err("camera fs creat dev attr[%s] fail", dev_attr_pt_camera_test.attr.name);
			return -1;
		}
	}
	pr_err("%s end",__func__);
	return 0;
}

int register_camerafs_attr(struct device_attribute *attr)
{
	int ret = 0;

	ret = device_create_file(camerafs.pDevice,attr);
	if (ret<0)
	{
              pr_err("camera fs creat dev attr[%s] fail", attr->attr.name);
		return -1;
	}
       pr_err("camera fs creat dev attr[%s] OK", attr->attr.name);
	return 0;
}

int register_camerafs_ois_attr(struct device_attribute *attr)
{
    int ret = 0;

    ret = device_create_file(camerafs_ois.pDevice,attr);
    if (ret<0)
    {
        pr_err("camera oiscreat dev attr[%s] fail", attr->attr.name);
        return -1;
	}
    pr_err("camera ois creat dev attr[%s] OK", attr->attr.name);
    return 0;
}

EXPORT_SYMBOL(register_camerafs_attr);

static void __exit camerafs_module_deinit(void)
{
	device_destroy(camerafs.classptr, devnum);
    device_destroy(camerafs_ois.classptr, osi_devnum);
	class_destroy(camerafs.classptr);
	unregister_chrdev_region(devnum, 1);
    unregister_chrdev_region(osi_devnum, 1);
}

module_init(camerafs_module_init);
module_exit(camerafs_module_deinit);
MODULE_AUTHOR("Jiezhou");
MODULE_DESCRIPTION("Camera fs virtul device");
MODULE_LICENSE("GPL");
