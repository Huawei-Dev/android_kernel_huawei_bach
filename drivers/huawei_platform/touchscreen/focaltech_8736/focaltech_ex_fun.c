/*
 *
 * FocalTech fts TouchScreen driver.
 * 
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
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

 /*******************************************************************************
*
* File Name: Focaltech_ex_fun.c
*
* Author: Xu YongFeng
*
* Created: 2015-01-29
*   
* Modify by mshl on 2015-10-26
*
* Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/
#include "focaltech_core.h"
#include "test_lib.h"
#include <huawei_platform/touchscreen/hw_tp_common.h>
#include <linux/platform_device.h>
#include "Global.h"
/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
/*create apk debug channel*/
#define PROC_UPGRADE			0
#define PROC_READ_REGISTER		1
#define PROC_WRITE_REGISTER	2
#define PROC_AUTOCLB			4
#define PROC_UPGRADE_INFO		5
#define PROC_WRITE_DATA		6
#define PROC_READ_DATA			7
#define PROC_SET_TEST_FLAG				8
#define PROC_NAME	"ftxxxx-debug"

#define WRITE_BUF_SIZE		512
#define READ_BUF_SIZE		512

#define FTS_RAW_DATA_SIZE (PAGE_SIZE*4)

/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/

/*******************************************************************************
* Static variables
*******************************************************************************/
static unsigned char proc_operate_mode = PROC_UPGRADE;
static struct proc_dir_entry *fts_proc_entry;
/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
unsigned char g_ucGloveStatusBak = 0;

/*Cover*/
unsigned char g_CoverStatusBufBak[10] = {0};


#if GTP_ESD_PROTECT
int apk_debug_flag = 0;
#endif
/*******************************************************************************
* Static function prototypes
*******************************************************************************/

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
/*interface of write proc*/
/************************************************************************
*   Name: fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static ssize_t fts_debug_write(struct file *filp, const char __user *buff, size_t count, loff_t *ppos)
{
	unsigned char writebuf[WRITE_BUF_SIZE] = {0};
	int buflen = count;
	int writelen = 0;
	int ret = 0;
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&fts_i2c_client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];

	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			snprintf(upgrade_file_path, sizeof(upgrade_file_path), "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			FTS_DBG("%s. file path:%s\n", __func__, upgrade_file_path);
			disable_irq(fts_i2c_client->irq);
			#if GTP_ESD_PROTECT
			apk_debug_flag = 1;
			#endif
			
			ret = fts_ctpm_fw_upgrade_with_app_file(fts_i2c_client, upgrade_file_path);
			#if GTP_ESD_PROTECT
			apk_debug_flag = 0;
			#endif
			enable_irq(fts_i2c_client->irq);
			if (ret < 0) {
				dev_err(&fts_i2c_client->dev, "[FTS] %s:upgrade failed.\n", __func__);
				return ret;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = fts_i2c_write(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = fts_i2c_write(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		FTS_DBG("%s: autoclb\n", __func__);
		fts_ctpm_auto_clb(fts_i2c_client);
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = count - 1;
		ret = fts_i2c_write(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}
	

	return count;
}

/*interface of read proc*/
/************************************************************************
*   Name: fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use 
* Output: page point to data
* Return: read char number
***********************************************************************/
static ssize_t fts_debug_read(struct file *filp, char __user *buff, size_t count, loff_t *ppos)
{
	int ret = 0;
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	unsigned char buf[READ_BUF_SIZE];
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		//after calling fts_debug_write to upgrade
		regaddr = 0xA6;
		ret = fts_read_reg(fts_i2c_client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = snprintf(buf, READ_BUF_SIZE, "%s", "get fw version failed.\n");
		else
			num_read_chars = snprintf(buf, READ_BUF_SIZE, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = fts_i2c_read(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			ERROR_COMMON_FTS("%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = count;
		ret = fts_i2c_read(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			ERROR_COMMON_FTS("%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}
	
	if (copy_to_user(buff, buf, num_read_chars)) {
		ERROR_COMMON_FTS("%s:copy to user error\n", __func__);
		return -EFAULT;
	}

	return num_read_chars;
}
static const struct file_operations fts_proc_fops = {
		.owner = THIS_MODULE,
		.read = fts_debug_read,
		.write = fts_debug_write,
		
};
#else
/*interface of write proc*/
/************************************************************************
*   Name: fts_debug_write
*  Brief:interface of write proc
* Input: file point, data buf, data len, no use
* Output: no
* Return: data len
***********************************************************************/
static int fts_debug_write(struct file *filp, 
	const char __user *buff, unsigned long len, void *data)
{
	unsigned char writebuf[WRITE_BUF_SIZE] = {0};
	int buflen = len;
	int writelen = 0;
	int ret = 0;
	
	
	if (copy_from_user(&writebuf, buff, buflen)) {
		dev_err(&fts_i2c_client->dev, "%s:copy from user error\n", __func__);
		return -EFAULT;
	}
	proc_operate_mode = writebuf[0];

	switch (proc_operate_mode) {
	
	case PROC_UPGRADE:
		{
			char upgrade_file_path[128];
			memset(upgrade_file_path, 0, sizeof(upgrade_file_path));
			snprintf(upgrade_file_path, sizeof(upgrade_file_path), "%s", writebuf + 1);
			upgrade_file_path[buflen-1] = '\0';
			FTS_DBG("%s file path:%s.\n", __func__, upgrade_file_path);
			disable_irq(fts_i2c_client->irq);
			#if GTP_ESD_PROTECT
				apk_debug_flag = 1;
			#endif
			ret = fts_ctpm_fw_upgrade_with_app_file(fts_i2c_client, upgrade_file_path);
			#if GTP_ESD_PROTECT
				apk_debug_flag = 0;
			#endif
			enable_irq(fts_i2c_client->irq);
			if (ret < 0) {
				dev_err(&fts_i2c_client->dev, "[FTS]%s:upgrade failed.\n", __func__);
				return ret;
			}
		}
		break;
	case PROC_READ_REGISTER:
		writelen = 1;
		ret = fts_i2c_write(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_WRITE_REGISTER:
		writelen = 2;
		ret = fts_i2c_write(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	case PROC_AUTOCLB:
		FTS_DBG("%s: autoclb\n", __func__);
		fts_ctpm_auto_clb(fts_i2c_client);
		break;
	case PROC_READ_DATA:
	case PROC_WRITE_DATA:
		writelen = len - 1;
		ret = fts_i2c_write(fts_i2c_client, writebuf + 1, writelen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:write iic error\n", __func__);
			return ret;
		}
		break;
	default:
		break;
	}
	

	return len;
}

/*interface of read proc*/
/************************************************************************
*   Name: fts_debug_read
*  Brief:interface of read proc
* Input: point to the data, no use, no use, read len, no use, no use 
* Output: page point to data
* Return: read char number
***********************************************************************/
static int fts_debug_read( char *page, char **start,
	off_t off, int count, int *eof, void *data )
{
	int ret = 0;
	unsigned char buf[READ_BUF_SIZE];
	int num_read_chars = 0;
	int readlen = 0;
	u8 regvalue = 0x00, regaddr = 0x00;
	
	switch (proc_operate_mode) {
	case PROC_UPGRADE:
		//after calling fts_debug_write to upgrade
		regaddr = 0xA6;
		ret = fts_read_reg(fts_i2c_client, regaddr, &regvalue);
		if (ret < 0)
			num_read_chars = snprintf(buf, READ_BUF_SIZE, "%s", "get fw version failed.\n");
		else
			num_read_chars = snprintf(buf, READ_BUF_SIZE, "current fw version:0x%02x\n", regvalue);
		break;
	case PROC_READ_REGISTER:
		readlen = 1;
		ret = fts_i2c_read(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		} 
		num_read_chars = 1;
		break;
	case PROC_READ_DATA:
		readlen = count;
		ret = fts_i2c_read(fts_i2c_client, NULL, 0, buf, readlen);
		if (ret < 0) {
			dev_err(&fts_i2c_client->dev, "%s:read iic error\n", __func__);
			return ret;
		}
		
		num_read_chars = readlen;
		break;
	case PROC_WRITE_DATA:
		break;
	default:
		break;
	}
	
	memcpy(page, buf, num_read_chars);
	return num_read_chars;
}
#endif
/************************************************************************
* Name: fts_create_apk_debug_channel
* Brief:  create apk debug channel
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/
int fts_create_apk_debug_channel(struct i2c_client * client)
{	
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
		fts_proc_entry = proc_create(PROC_NAME, 0777, NULL, &fts_proc_fops);		
	#else
		fts_proc_entry = create_proc_entry(PROC_NAME, 0777, NULL);
	#endif
	if (NULL == fts_proc_entry) 
	{
		dev_err(&client->dev, "Couldn't create proc entry!\n");
		
		return -ENOMEM;
	} 
	else 
	{
		dev_info(&client->dev, "Create proc entry success!\n");
		
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0))
			fts_proc_entry->write_proc = fts_debug_write;
			fts_proc_entry->read_proc = fts_debug_read;
		#endif
	}
	return 0;
}
/************************************************************************
* Name: fts_release_apk_debug_channel
* Brief:  release apk debug channel
* Input: no
* Output: no
* Return: no
***********************************************************************/
void fts_release_apk_debug_channel(void)
{
	
	if (fts_proc_entry)
		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0))
			proc_remove(fts_proc_entry);
		#else
			remove_proc_entry(NULL, fts_proc_entry);
		#endif
}

/************************************************************************
* Name: fts_tpfwver_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_tpfwver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	u8 fwver = 0;
	mutex_lock(&fts_input_dev->mutex);
	if (fts_read_reg(fts_i2c_client, FTS_REG_FW_VER, &fwver) < 0)
	{
		mutex_unlock(&fts_input_dev->mutex);
		return -1;
	}
	
	if (fwver == 255)
		num_read_chars = snprintf(buf, 128,"get tp fw version fail!\n");
	else
	{
		num_read_chars = snprintf(buf, 128, "%02X\n", fwver);
	}
	
	mutex_unlock(&fts_input_dev->mutex);
	
	return num_read_chars;
}
/************************************************************************
* Name: fts_tpfwver_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tpfwver_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: fts_tpdriver_version_show
* Brief:  show tp fw vwersion
* Input: device, device attribute, char buf
* Output: no
* Return: char number
***********************************************************************/
static ssize_t fts_tpdriver_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	
	mutex_lock(&fts_input_dev->mutex);
	
	num_read_chars = snprintf(buf, 128,"%s \n", FTS_DRIVER_INFO);
	
	mutex_unlock(&fts_input_dev->mutex);
	
	return num_read_chars;
}
/************************************************************************
* Name: fts_tpdriver_version_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tpdriver_version_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: fts_tprwreg_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_tprwreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}
/************************************************************************
* Name: fts_tprwreg_store
* Brief:  read/write register
* Input: device, device attribute, char buf, char count
* Output: print register value
* Return: char count
***********************************************************************/
static ssize_t fts_tprwreg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval = 0;
	long unsigned int wmreg=0;
	u8 regaddr=0xff,regvalue=0xff;
	u8 valbuf[5]={0};

	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&fts_input_dev->mutex);	
	num_read_chars = count - 1;
	if (num_read_chars != 2) 
	{
		if (num_read_chars != 4) 
		{
			dev_err(dev, "please input 2 or 4 character\n");
			goto error_return;
		}
	}
	memcpy(valbuf, buf, num_read_chars);
//	retval = strict_strtoul(valbuf, 16, &wmreg);	//	frank comment.
//	strToByte(char * result, char * str)
//	strToBytes((char*)buf, num_read_chars, valbuf, &retval);
	if (0 != retval) 
	{
		dev_err(dev, "%s() - ERROR: Could not convert the given input to a number. The given input was: \"%s\"\n", __FUNCTION__, buf);
		goto error_return;
	}
	if (2 == num_read_chars) 
	{
		/*read register*/
		regaddr = wmreg;
		printk("[focal][test](0x%02x)\n", regaddr);
		if (fts_read_reg(client, regaddr, &regvalue) < 0)
			printk("[Focal] %s : Could not read the register(0x%02x)\n", __func__, regaddr);
		else
			printk("[Focal] %s : the register(0x%02x) is 0x%02x\n", __func__, regaddr, regvalue);
	} 
	else 
	{
		regaddr = wmreg>>8;
		regvalue = wmreg;
		if (fts_write_reg(client, regaddr, regvalue)<0)
			dev_err(dev, "[Focal] %s : Could not write the register(0x%02x)\n", __func__, regaddr);
		else
			dev_dbg(dev, "[Focal] %s : Write 0x%02x into register(0x%02x) successful\n", __func__, regvalue, regaddr);
	}
	error_return:
	mutex_unlock(&fts_input_dev->mutex);
	
	return count;
}
/************************************************************************
* Name: fts_fwupdate_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_fwupdate_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/************************************************************************
* Name: fts_fwupdate_store
* Brief:  upgrade from *.i
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t fts_fwupdate_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 uc_host_fm_ver;
	int i_ret;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	mutex_lock(&fts_input_dev->mutex);
	
	disable_irq(client->irq);
	#if GTP_ESD_PROTECT
		apk_debug_flag = 1;
	#endif
	
	i_ret = fts_ctpm_fw_upgrade_with_i_file(client);
	if (i_ret == 0)
	{
		msleep(300);
		uc_host_fm_ver = fts_ctpm_get_i_file_ver();
		dev_dbg(dev, "%s [FTS] upgrade to new version 0x%x\n", __func__, uc_host_fm_ver);
	}
	else
	{
		dev_err(dev, "%s ERROR:[FTS] upgrade failed ret=%d.\n", __func__, i_ret);
	}
	
	#if GTP_ESD_PROTECT
		apk_debug_flag = 0;
	#endif
	enable_irq(client->irq);
	mutex_unlock(&fts_input_dev->mutex);
	
	return count;
}
/************************************************************************
* Name: fts_fwupgradeapp_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_fwupgradeapp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
	return -EPERM;
}

/************************************************************************
* Name: fts_fwupgradeapp_store
* Brief:  upgrade from app.bin
* Input: device, device attribute, char buf, char count
* Output: no
* Return: char count
***********************************************************************/
static ssize_t fts_fwupgradeapp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	char fwname[128] = {0};
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	memset(fwname, 0, sizeof(fwname));
	snprintf(fwname, 128, "%s", buf);
	fwname[count-1] = '\0';

	FTS_DBG(" %s. \n", __func__);
	FTS_DBG(" %s. fwname:%s \n", __func__, fwname);
	mutex_lock(&fts_input_dev->mutex);
	
	disable_irq(client->irq);
	#if GTP_ESD_PROTECT
				apk_debug_flag = 1;
			#endif
	fts_ctpm_fw_upgrade_with_app_file(client, fwname);
	#if GTP_ESD_PROTECT
				apk_debug_flag = 0;
			#endif
	enable_irq(client->irq);
	
	mutex_unlock(&fts_input_dev->mutex);
	return count;
}
/************************************************************************
* Name: fts_ftsgetprojectcode_show
* Brief:  no
* Input: device, device attribute, char buf
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_getprojectcode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	
	return -EPERM;
}
/************************************************************************
* Name: fts_ftsgetprojectcode_store
* Brief:  no
* Input: device, device attribute, char buf, char count
* Output: no
* Return: EPERM
***********************************************************************/
static ssize_t fts_getprojectcode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */
	return -EPERM;
}

extern ssize_t fts_fts8716_getcapacitancedata_show(struct kobject *dev,struct kobj_attribute *attr, char *buf);


static ssize_t fts_getcapacitancedata_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	printk("[FTS] %s \n", __func__);

	
	mutex_lock(&fts_input_dev->mutex);
	
	num_read_chars = snprintf(buf, 128,"%s \n", FTS_DRIVER_INFO);
	
	mutex_unlock(&fts_input_dev->mutex);
	
	return num_read_chars;

}


// frank add
static ssize_t fts_getcapacitancedata_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	/* place holder for future use */

	printk("[FTS] %s \n", __func__);
	return -EPERM;
}

/****************************************/
/* sysfs */
/*get the fw version
*example:cat ftstpfwver
*/
static DEVICE_ATTR(ftstpfwver, S_IRUGO|S_IWUSR, fts_tpfwver_show, fts_tpfwver_store);

static DEVICE_ATTR(ftstpdriverver, S_IRUGO|S_IWUSR, fts_tpdriver_version_show, fts_tpdriver_version_store);
/*upgrade from *.i
*example: echo 1 > ftsfwupdate
*/
static DEVICE_ATTR(ftsfwupdate, S_IRUGO|S_IWUSR, fts_fwupdate_show, fts_fwupdate_store);
/*read and write register
*read example: echo 88 > ftstprwreg ---read register 0x88
*write example:echo 8807 > ftstprwreg ---write 0x07 into register 0x88
*
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(ftstprwreg, S_IRUGO|S_IWUSR, fts_tprwreg_show, fts_tprwreg_store);
/*upgrade from app.bin
*example:echo "*_app.bin" > ftsfwupgradeapp
*/
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO|S_IWUSR, fts_fwupgradeapp_show, fts_fwupgradeapp_store);
static DEVICE_ATTR(ftsgetprojectcode, S_IRUGO|S_IWUSR, fts_getprojectcode_show, fts_getprojectcode_store);

static DEVICE_ATTR(ftsgetcapacitancedata, S_IRUGO|S_IWUSR, fts_getcapacitancedata_show, fts_getcapacitancedata_store);

/*add your attr in here*/
static struct attribute *fts_attributes[] = {
	&dev_attr_ftstpfwver.attr,
	&dev_attr_ftstpdriverver.attr,
	&dev_attr_ftsfwupdate.attr,
	&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	&dev_attr_ftsgetprojectcode.attr,
	&dev_attr_ftsgetcapacitancedata.attr,
	NULL
};

static struct attribute_group fts_attribute_group = {
	.attrs = fts_attributes
};

static ssize_t hw_touch_test_type_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{

	return snprintf(buf, PAGE_SIZE - 1,
		"%s:%s\n",
		NORMALIZE_TP_CAPACITANCE_TEST,
		TP_JUDGE_LAST_RESULT);
}

static ssize_t touch_chip_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char reg_addr = 0xa6;
	char reg_value = 0;
	int err = 0;
	struct i2c_client *client = fts_i2c_client;

	err = fts_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0)
		FTS_COMMON_DBG("%s,focal_fw_version read failed\n",__func__);

	 return snprintf(buf, PAGE_SIZE - 1, "Focal8716-MLA-tianma-fw_version_0%x\n", reg_value);
}

static ssize_t  hw_touch_chip_info_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	char reg_addr = 0xa6;
	char reg_value = 0;
	int err = 0;
	struct i2c_client *client = fts_i2c_client;

	err = fts_i2c_read(client, &reg_addr, 1, &reg_value, 1);
	if (err < 0)
		FTS_COMMON_DBG("%s,focal_fw_version read failed\n",__func__);

	 return snprintf(buf, PAGE_SIZE - 1, "Focal8716-MLA-tianma-fw_version_0%x\n", reg_value);
}

extern struct i2c_client *fts_i2c_client;
static ssize_t fts_holster_func_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	int enable = 0;
	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;
	u8 i2c_write_buf[10] = {0};
	struct i2c_client *client = fts_i2c_client;
	u8 regaddr=0x8C;
	int i = 0;
	FTS_DBG("");
	mutex_lock(&fts_input_dev->mutex);	
	ret = sscanf(buf, "%d %d %d %d %d",&enable, &x0, &y0, &x1, &y1);

	if (!ret) {
		ERROR_COMMON_FTS("sscanf return invaild :%d\n", ret);
		ret = -EINVAL;
		goto out;
	}
	FTS_DBG(" sscanf value is enable=%d, top:(x:%d, y:%d), bottom(x:%d, y:%d)\n", enable, x0, y0, x1, y1);
	if (enable && ((x0 < 0) || (y0 < 0) || (x1 <= x0) || (y1 <= y0))) {
		ERROR_COMMON_FTS("%s:invalid value is %d (%d,%d), (%d,%d)\n",__func__, enable, x0, y0, x1, y1);
		ret = -EINVAL;
		goto out;
	}

	i2c_write_buf[0] = regaddr;
	i2c_write_buf[1] = (u8)((x0&0xFF00)>>8);
	i2c_write_buf[2] = (u8)(x0&0x00FF);
	i2c_write_buf[3] = (u8)((y0&0xFF00)>>8);
	i2c_write_buf[4] = (u8)(y0&0x00FF);
	i2c_write_buf[5] = (u8)((x1&0xFF00)>>8);
	i2c_write_buf[6] = (u8)(x1&0x00FF);
	i2c_write_buf[7] = (u8)((y1&0xFF00)>>8);
	i2c_write_buf[8] = (u8)(y1&0x00FF);
	i2c_write_buf[9] = (u8)enable;

	memcpy(&g_CoverStatusBufBak[0],&i2c_write_buf[0],10);

	if (g_suspend_state) {
		FTS_COMMON_DBG("panel is suspend,do not store holster\n");
		goto out;
	}

	for(i = 1; i < sizeof(i2c_write_buf); i++)
	{
		ret = fts_write_reg(client, regaddr++, i2c_write_buf[i]);
		if (ret < 0)
		{
			ERROR_COMMON_FTS("[Focal] %s : Could not write the register(0x%02x).fts_write_reg return:%d.\n", __func__, regaddr, ret);
			goto out;
		}
	}
out:
	mutex_unlock(&fts_input_dev->mutex);
	if (ret < 0)
		return ret;
	return size;
}
static ssize_t fts_holster_func_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	struct i2c_client *client = fts_i2c_client;
	ssize_t num_read_chars = 0;
	u8 regaddr=0x8C;
	u8 readbuf[9] = {0};
	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;
	int nenable = 0;

	FTS_DBG("");
	if (g_suspend_state) {
		FTS_COMMON_DBG("panel is suspend,do not show holster\n");
		return num_read_chars;
	}
	mutex_lock(&fts_input_dev->mutex);
	FTS_DBG("[focal] regaddr:(0x%02x)\n", regaddr);
	ret = fts_i2c_read(client, (char*)&regaddr, 1, readbuf, 9);
	if (ret < 0)
	{
		ERROR_COMMON_FTS("[Focal] %s : Could not read the register(0x%02x). error:%d. \n", __func__, regaddr, ret);
		num_read_chars = snprintf(buf, PAGE_SIZE-1, "fts_i2c_read error return:%d\n", ret);
	}
	else
	{
		x0 = (readbuf[0]<<8)|readbuf[1];
		y0 = (readbuf[2]<<8)|readbuf[3];
		x1 = (readbuf[4]<<8)|readbuf[5];
		y1 = (readbuf[6]<<8)|readbuf[7];
		nenable = (int)readbuf[8];
		num_read_chars = snprintf(buf,PAGE_SIZE,"%d %d %d %d %d\n", nenable, x0,
				y0, x1, y1);
	}
	FTS_DBG(" buf:%s \n", buf);
	mutex_unlock(&fts_input_dev->mutex);
	return num_read_chars;
}
static ssize_t hw_holster_func_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	FTS_DBG("");
	return fts_holster_func_store(dev, attr, buf, size);
}
static ssize_t hw_holster_func_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	FTS_DBG("");
	return fts_holster_func_show(dev, attr, buf);
}
static ssize_t fts_glove_func_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	int ret = 0;
	struct i2c_client *client = fts_i2c_client;
	ssize_t num_read_chars = 0;
	u8 regaddr=0xC0;
	u8 regvalue=0x00;

	FTS_DBG("");
	if (g_suspend_state) {
		FTS_COMMON_DBG("panel is suspend,do not show glove\n");
		return num_read_chars;
	}
	mutex_lock(&fts_input_dev->mutex);	
	FTS_DBG("[focal] regaddr:(0x%02x)\n", regaddr);
	ret = fts_read_reg(client, regaddr, &regvalue);
	if (ret < 0)
	{
		ERROR_COMMON_FTS("[Focal] %s : Could not read the register(0x%02x). error:%d. \n", __func__, regaddr, ret);
		num_read_chars = snprintf(buf, PAGE_SIZE, "fts_read_reg error return:%d\n", ret);
	}
	else
	{
		FTS_DBG("[Focal] %s : the register(0x%02x) is 0x%02x\n", __func__, regaddr, regvalue);
		FTS_DBG("%s: glove_enabled=%d\n", __func__, regvalue);
		num_read_chars = snprintf(buf, PAGE_SIZE, "%d\n", regvalue);
	}
	FTS_DBG(" buf:%s \n", buf);
	mutex_unlock(&fts_input_dev->mutex);
	return num_read_chars;
}
static ssize_t fts_glove_func_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	unsigned long value = 0;
	int ret = 0;
	struct i2c_client *client = fts_i2c_client;
	u8 regaddr=0xC0;
	u8 regvalue=0x00;

	FTS_DBG("Func:%s\n", __func__);
	mutex_lock(&fts_input_dev->mutex);
	ret = kstrtoul(buf, 10, &value);
	FTS_DBG("%s: value=%d\n", __func__, (unsigned int)value);
	if (ret < 0){
		ERROR_COMMON_FTS("%s: kstrtoul error,ret=%d\n", __func__, ret);
		goto out;
	}
	regvalue = (u8)(!!value);

	if (0 == regvalue)
	{
		g_ucGloveStatusBak = 0;
	}
	else
	{
		g_ucGloveStatusBak = 1;
	}

	if (g_suspend_state) {
		FTS_COMMON_DBG("panel is suspend,do not store glove_enable\n");
                ret = size;
		goto out;
	}

	ret = fts_write_reg(client, regaddr, regvalue);
	if (ret < 0)
		ERROR_COMMON_FTS("[Focal] %s : Could not write the register(0x%02x).fts_write_reg return:%d.\n", __func__, regaddr, ret);
	else {
		FTS_COMMON_DBG("[Focal] %s : Write 0x%02x into register(0x%02x) successful.\n", __func__, regvalue, regaddr);
                ret = size;
        }
out:
	mutex_unlock(&fts_input_dev->mutex);
	return ret;
}
static ssize_t hw_glove_func_show(struct kobject *dev,
		struct kobj_attribute *attr, char *buf)
{
	FTS_DBG("");
	return fts_glove_func_show(dev, attr, buf);
}
static ssize_t hw_glove_func_store(struct kobject *dev,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	FTS_DBG("");
	FTS_DBG("Func:%s\n", __func__);
	return fts_glove_func_store(dev, attr, buf, size);
}

static struct kobj_attribute hw_touch_test_capacitance_data = {
	.attr = {.name = "tp_legacy_capacitance_data", .mode = 0444},
	.show = NULL,
	.store = NULL,
};

static struct kobj_attribute hw_touch_test_type = {
	.attr = {.name = "tp_capacitance_test_type", .mode = 0444},
	.show = hw_touch_test_type_show,
	.store = NULL,
};

static struct kobj_attribute hw_touch_chip_info = {
	.attr = {.name = "touch_chip_info", .mode = 0444},
	.show = hw_touch_chip_info_show,
	.store = NULL,
};

static struct kobj_attribute hw_touch_test_capacitance_test = {
	.attr = {.name = "tp_legacy_capacitance_test", .mode = 0444},
	.show = NULL,
	.store = NULL,
};

static struct kobj_attribute hw_touch_glove_func = {
	.attr = {.name = "signal_disparity", .mode = (S_IRUGO | S_IWUSR | S_IWGRP)},
	//.attr = {.name = "signal_disparity", .mode = 0664},
	.show = hw_glove_func_show,
	.store = hw_glove_func_store,
};
static struct kobj_attribute hw_touch_holster_func = {
	.attr = {.name = "holster_touch_window", .mode = 0664},
	.show = hw_holster_func_show,
	.store = hw_holster_func_store,
};
static int fts_rawdata_proc_show(struct seq_file *m, void *v)
{
	int len = 0;
	char *proc_buf = NULL;
	if (m->size <= FTS_RAW_DATA_SIZE) {
		m->count = m->size;
		return 0;
	}
	if(ft8607_cap_test_fake_status)
	{	
		ERROR_COMMON_FTS("%s TRT_cap_test_fake_status: %d\n", __func__, ft8607_cap_test_fake_status);
		seq_printf(m, "%s\n", "0P-1P-2P-3P-ft8607-cmi");
		return 0;
	}
	proc_buf = (char *)kzalloc(FTS_RAW_DATA_SIZE, GFP_KERNEL);
	if (!proc_buf) {
		ERROR_COMMON_FTS(" Faild to kzalloc proc_buf\n");
		return -ENOMEM;
	}	
	len = fts_fts8716_getcapacitancedata_show(NULL, NULL, proc_buf);
	if (len <= 0) {
		ERROR_COMMON_FTS(" mmi test result show error!\n");
		kfree(proc_buf);
		return len;
	}
	seq_printf(m, "%s\n", proc_buf);
//	FTS_COMMON_DEBUG(" done\n");
	kfree(proc_buf);
	return 0;
}
static int fts_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, fts_rawdata_proc_show, NULL);
}
static const struct file_operations fts_proc_test_fops = {
	.open = fts_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static int fts_read_roi_switch(int *fts_switch)
{
	int retval = 0;
	unsigned short roi_ctrl_addr = 0;
	unsigned char roi_switch = 0;
	u8 write_buf[2]={0};

	roi_ctrl_addr = 0x9B;//0x9C 0x7E
	write_buf[0] = (u8)roi_ctrl_addr;

	retval = fts_i2c_read(fts_i2c_client, write_buf, 1, &roi_switch, sizeof(roi_switch));
	if (retval < 0) {
		FTS_COMMON_DBG("%s failed: %d\n", __func__, retval);
		return retval;
	}
	*fts_switch = roi_switch;
	FTS_COMMON_DBG("roi_ctrl_read_addr=0x%04x, roi_switch=%d\n", roi_ctrl_addr, *fts_switch);

	return retval;
}

static int fts_set_roi_switch(u8 roi_switch)
{
	int retval = 0;
	int i = 0;
	unsigned short roi_ctrl_addr = 0;
	u8 temp_roi_switch = 0;
	u8 write_buf[2]={0};

	roi_ctrl_addr = 0x9B;

	FTS_COMMON_DBG("roi_ctrl_write_addr=0x%04x, roi_switch=%d\n",
					roi_ctrl_addr, roi_switch);

	if (roi_switch)
	{
		temp_roi_switch = 1;
		g_Roi_Enable_9B_RegValue = 1;
	}
	else
	{
		temp_roi_switch = 0;
		g_Roi_Enable_9B_RegValue = 0;
	}
	FTS_COMMON_DBG("%s,roi_enable = %d\n", __func__, g_Roi_Enable_9B_RegValue);

	if (g_suspend_state) {
		gu_roi= (u8)temp_roi_switch;
		FTS_COMMON_DBG("panel is suspend,do not set roi_enable_store\n");
		return -1;
	}

	write_buf[0] = (u8)roi_ctrl_addr;
	write_buf[1] = (u8)temp_roi_switch;
	retval = fts_i2c_write(fts_i2c_client, write_buf, 2);
	if (retval < 0) {
		gu_roi= (u8)temp_roi_switch;
		FTS_COMMON_DBG("set %s failed: %d, roi_ctrl_write_addr=0x%04x, roi_enable: %d\n",
			__func__, retval, roi_ctrl_addr, gu_roi);
		return retval;
	}
	else
	{
		gbset_roi= true;
		gu_roi= (u8)temp_roi_switch;
		FTS_COMMON_DBG("[Focal] %s : Write 0x%02x into register(0x%02x) successful.\n", __func__, gu_roi, roi_ctrl_addr);
	}

	//f51_roi_switch = temp_roi_switch;
	if (!roi_switch) {
		for (i = 0; i < ROI_DATA_READ_LENGTH; i++) {
			global_roi_data[i] = 0;
		}
	}
	FTS_COMMON_DBG("set %s success, roi_ctrl_write_addr=0x%04x, value is %d\n", __func__, roi_ctrl_addr, temp_roi_switch);
	return retval;
}

static unsigned char *fts_roi_rawdata(void)
{
	return (unsigned char *)global_roi_data;
}

static ssize_t ts_roi_enable_store(struct device *dev,
									struct device_attribute *attr,
									const char *buf, size_t count)
{
	unsigned int value = 0;
	int error = 0;

	FTS_COMMON_DBG("%s:", __func__);

	error = sscanf(buf, "%u", &value);
	if (value < 0) {
		FTS_COMMON_DBG("%s: sscanf value %d is invalid\n", __func__, value);
		return -EINVAL;
	}
	if (error <= 0) {
		FTS_COMMON_DBG("sscanf return invalid :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	FTS_COMMON_DBG("sscanf value is %u\n", value);

	error = fts_set_roi_switch((u8)value);
	if (error < 0) {
		FTS_COMMON_DBG("%s failed\n", __func__);
		error = -ENOMEM;
		goto out;
	}

out:
	FTS_COMMON_DBG("roi_enable_store done\n");
	return count;
}

static ssize_t ts_roi_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned int value = 0;
	int error = 0;

	if (g_suspend_state) {
		FTS_COMMON_DBG("panel is suspend,do not read roi_enable\n");
		goto out;
	}
	error = fts_read_roi_switch(&value);
	if (error < 0) {
		FTS_COMMON_DBG("%s failed, error code is %d\n", __func__, error);
		error = -ENOMEM;
		goto out;
	}

	error = snprintf(buf, PAGE_SIZE, "%d\n", value);
out:
	FTS_COMMON_DBG("%s done\n", __func__);
	return error;
}


static ssize_t ts_roi_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	unsigned char *roi_data_p = NULL;
	roi_data_p = fts_roi_rawdata();	
	if (NULL == roi_data_p) {
		FTS_COMMON_DBG("not define ROI for roi_data_show \n");
		return -ENOMEM;
	}
	/*roi_data_temp <-- This is the buffer that has the ROI data you want to send to Qeexo*/
	memcpy(buf, roi_data_p + ROI_HEAD_DATA_LENGTH, ROI_DATA_SEND_LENGTH);
	return ROI_DATA_SEND_LENGTH;
}

static ssize_t ts_roi_data_debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int cnt = 0;
	int count = 0;
	int i, j = 0;
	short roi_data_16[ROI_DATA_SEND_LENGTH / 2] = { 0 };
	unsigned char *roi_data_p = NULL;

	int tx_num = 0;
	int rx_num = 0;
	int roi_ID = 0;

	roi_data_p = fts_roi_rawdata();

	if (NULL == roi_data_p) {
		FTS_COMMON_DBG("not define ROI for roi_data_show \n");
		return -ENOMEM;
	}

	cnt = snprintf(buf, PAGE_SIZE - count, "9C status: %3d\n", (char)global_roi_data[ROI_DATA_READ_LENGTH]);
	buf += cnt;
	count += cnt;
	
	tx_num = (((char)roi_data_p[0]&0xC0) >> 6) + (((char)roi_data_p[1]&0x0F) <<2);
	rx_num = ((char)roi_data_p[0]&0x3F) >> 0;
	roi_ID = ((char)roi_data_p[1]&0xF0) >> 4;

	cnt = snprintf(buf, PAGE_SIZE - count, " TX: %3d\n", (char)tx_num);
	buf += cnt;
	count += cnt;

	cnt = snprintf(buf, PAGE_SIZE - count, " RX: %3d\n", (char)rx_num);
	buf += cnt;
	count += cnt;

	cnt = snprintf(buf, PAGE_SIZE - count, " ID: %3d\n", (char)roi_ID);
	buf += cnt;
	count += cnt;

	cnt = snprintf(buf, PAGE_SIZE - count, " Differ:\n");
	buf += cnt;
	count += cnt;

	for (i = ROI_HEAD_DATA_LENGTH; i < ROI_DATA_READ_LENGTH; i += 2, j++) {
		roi_data_16[j] = roi_data_p[i] | (roi_data_p[i + 1] << 8);
		cnt = snprintf(buf, PAGE_SIZE - count, "%5d", roi_data_16[j]);
		buf += cnt;
		count += cnt;

		if ((j + 1) % 7 == 0) {
			cnt = snprintf(buf, PAGE_SIZE - count, "\n");
			buf += cnt;
			count += cnt;
		}
	}
	snprintf(buf, PAGE_SIZE - count, "\n");
	count++;

	memset(global_roi_data,0,sizeof(global_roi_data));
	return count;
}

static DEVICE_ATTR(roi_data,
					S_IRUGO | S_IWUSR | S_IWGRP,
					ts_roi_data_show,
					NULL);

static DEVICE_ATTR(roi_data_debug,
					S_IRUGO | S_IWUSR | S_IWGRP,
					ts_roi_data_debug_show,
					NULL);

/* ROI switch 0x9B */
static DEVICE_ATTR(roi_enable,
					S_IRUGO | S_IWUSR | S_IWGRP,
					ts_roi_enable_show,
					ts_roi_enable_store);

static DEVICE_ATTR(touch_chip_info,
					S_IRUGO | S_IWUSR | S_IWGRP,
					touch_chip_info_show,
					NULL);

static struct attribute *ts_roi_attributes[] = {
	&dev_attr_roi_enable.attr,
	&dev_attr_roi_data.attr,
	&dev_attr_roi_data_debug.attr,
	&dev_attr_touch_chip_info.attr,
	NULL
};

static const struct attribute_group ts_roi_attr_group = {
	.attrs = ts_roi_attributes,
};
/************************************************************************
* Name: fts_create_sysfs
* Brief:  create sysfs for debug
* Input: i2c info
* Output: no
* Return: success =0
***********************************************************************/

int fts_create_sysfs(struct i2c_client * client)
{
	int err = 0;
	
	struct kobject *properties_kobj;
	struct kobject *glove_kobj;
	struct proc_dir_entry *proc_entry;
	struct platform_device *touch_dev;

	properties_kobj = tp_get_touch_screen_obj();
	if( NULL == properties_kobj ) {
		ERROR_COMMON_FTS("[FTS] %s() - ERROR: tp_get_touch_screen_obj() failed.\n", __func__);
		return -1;
	}

	err = sysfs_create_file(properties_kobj, &hw_touch_test_capacitance_data.attr);
	if (err) {
		kobject_put(properties_kobj);
		ERROR_COMMON_FTS("hw_touch_test_capacitance_data failed.\n");
		return -ENODEV;
	}

	err = sysfs_create_file(properties_kobj, &hw_touch_test_type.attr);
	if (err) {
		kobject_put(properties_kobj);
		ERROR_COMMON_FTS("hw_touch_test_type failed.\n");
		return -ENODEV;
	}

	err = sysfs_create_file(properties_kobj, &hw_touch_chip_info.attr);
	if (err) {
		kobject_put(properties_kobj);
		tp_log_err("hw_touch_test_type failed.\n");
		return -ENODEV;
	}

	err = sysfs_create_file(properties_kobj, &hw_touch_test_capacitance_test.attr);
	if (err) {
		kobject_put(properties_kobj);
		ERROR_COMMON_FTS("hw_touch_test_capacitance_test failed.\n");
		return -ENODEV;
	}
	err = sysfs_create_file(properties_kobj, &hw_touch_holster_func.attr);
	if (err) {
		kobject_put(properties_kobj);
		ERROR_COMMON_FTS("hw_touch_holster_func failed.\n");
		return -ENODEV;
	}
	glove_kobj = tp_get_glove_func_obj();
	if( NULL == glove_kobj )
	{
		ERROR_COMMON_FTS("%s: Error, get kobj failed!\n", __func__);
		return -ENODEV;
	}

	err = sysfs_create_file(glove_kobj, &hw_touch_glove_func.attr);
	if (err)
	{
		kobject_put(glove_kobj);
		err = sysfs_create_file(glove_kobj, &hw_touch_glove_func.attr);
		if (err)
		{
			kobject_put(glove_kobj);
			ERROR_COMMON_FTS("%s: glove_func create file error\n", __func__);
		return -ENODEV;
		}
	}

	err = sysfs_create_group(&client->dev.kobj, &fts_attribute_group);
	if (0 != err)
	{
		dev_err(&client->dev, "[FTS] %s() - ERROR: sysfs_create_group() failed.\n", __func__);
		sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
		return -EIO;
	}
	else
	{
		pr_info("[FTS]:%s() - sysfs_create_group() succeeded.\n",__func__);
	}
	proc_entry = proc_mkdir("touchscreen", NULL);
	if (!proc_entry) {
		ERROR_COMMON_FTS("%s:%d Error, failed to creat procfs.\n",__func__, __LINE__);
		return -EINVAL;
	}
	if (!proc_create("tp_capacitance_data", 0, proc_entry, &fts_proc_test_fops)) {
		ERROR_COMMON_FTS("%s:%d Error, failed to creat procfs.\n",__func__, __LINE__);
		remove_proc_entry("tp_capacitance_data", proc_entry);
		return -EINVAL;
	}

	/*add the node ts_roi_enable for apk to read/write*/
	touch_dev = platform_device_alloc("huawei_touch", -1);
	if (!touch_dev) {
		ERROR_COMMON_FTS("touch_dev platform device malloc failed\n");
		return -ENOMEM;
	}

	err = platform_device_add(touch_dev);
	if (err) {
		ERROR_COMMON_FTS("touch_dev platform device add failed :%d\n", err);
		platform_device_put(touch_dev);
		return err;
	}
	//err = device_create_file(&touch_dev->dev.kobj, &dev_attr_touch_chip_info);
	err = sysfs_create_group(&touch_dev->dev.kobj, &ts_roi_attr_group);
	if (err) {
		ERROR_COMMON_FTS("can't create ts's sysfs\n");
		return err;
	}

	return err;
}
/************************************************************************
* Name: fts_remove_sysfs
* Brief:  remove sys
* Input: i2c info
* Output: no
* Return: no
***********************************************************************/
int fts_remove_sysfs(struct i2c_client * client)
{
	sysfs_remove_group(&client->dev.kobj, &fts_attribute_group);
	return 0;
}
