#define pr_fmt(fmt) "%s: " fmt, __func__
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <sound/smart_amp.h>
#include <sound/hw_audio_info.h>


/* Holds the Packet data required for processing */
struct tas_dsp_pkt {
	u8 slave_id;
	u8 book;
	u8 page;
	u8 offset;
	u8 data[TAS_PAYLOAD_SIZE * 4];	/* each package has 4 data */
};

static int smartamp_params_ctrl(struct tas_dsp_pkt *packet, u8 dir, u8 count)
{
	u32 length = count / sizeof(u32);
	u32 paramid = 0;
	u32 instance = 1;
	u32 index;
	int ret = 0;

	if (!packet) {
		pr_err("packet is null!\n");
		return -EFAULT;
	}

	index = ALGO_PARAM_INDEX_CALIC(packet->page, packet->offset);
	pr_debug("index = %d\n", index);
	if (index < 0 || index > MAX_DSP_PARAM_INDEX) {
		audio_dsm_report_info(DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO,
				"%s: index %u invalid\n", __func__, index);
		pr_err("invalid index !\n");
		return -EINVAL;
	}

	/* 4-speakers are differentiated by slave ids */
	if (packet->slave_id == SLAVE1 || packet->slave_id == SLAVE2)
		instance = SMARTAMP_INSTANCE_ONE;
	else if (packet->slave_id == SLAVE3 || packet->slave_id == SLAVE4)
		instance = SMARTAMP_INSTANCE_TWO;

	paramid = (paramid | (index << ALGO_PARAM_INDEX_OFFSET) |
		(length << ALGO_PARAM_LENGTH_OFFSET) |
		(instance << ALGO_PARAM_INSTANCE_OFFSET));

	/*
	 * Note: In any case calculated paramid should not match with
	 * AFE_PARAM_ID_ENABLE and AFE_PARAM_ID_SMARTAMP_DEFAULT
	 */
	if (paramid == AFE_PARAM_ID_ENABLE ||
		paramid == AFE_PARAM_ID_SMARTAMP_DEFAULT) {
		pr_err("%s Slave 0x%x params failed, paramid mismatch\n",
			dir == TAS_GET_PARAM ? "get" : "set", packet->slave_id);
		return -EIO;
	}
	ret = afe_smartamp_algo_ctrl(packet->data, paramid,
			dir, count, packet->slave_id);
	if (ret)
		pr_err("%s Slave 0x%x params failed from afe\n",
			dir == TAS_GET_PARAM ? "get" : "set", packet->slave_id);

	return ret;
}

static int tas_calib_open(struct inode *inode, struct file *fd)
{
	return 0;
}

static ssize_t tas_calib_write(struct file *file,
		const char __user *buffer, size_t count, loff_t *offp)
{
	struct tas_dsp_pkt packet;
	int rc = 0;

	if (!buffer) {
		pr_err("buffer is null!\n");
		return -EINVAL;
	}

	if (count > sizeof(struct tas_dsp_pkt) || count == 0) {
		pr_err("buffer size %d invalid !\n", count);
		return -ENOMEM;
	}

	if (copy_from_user(&packet, buffer, sizeof(struct tas_dsp_pkt)))
		return -EFAULT;
	rc = smartamp_params_ctrl(&packet, TAS_SET_PARAM, count);
	return rc;
}

static ssize_t tas_calib_read(struct file *file, char __user *buffer,
		size_t count, loff_t *ptr)
{
	struct tas_dsp_pkt packet;
	int rc;

	if (!buffer) {
		pr_err("buffer is null!\n");
		return -EINVAL;
	}

	if (copy_from_user(&packet, buffer, sizeof(struct tas_dsp_pkt)))
		return -EFAULT;
	rc = smartamp_params_ctrl(&packet, TAS_GET_PARAM, count);
	if (rc < 0)
		count = rc;
	if (copy_to_user(buffer, &packet, sizeof(struct tas_dsp_pkt)))
		return -EFAULT;
	return count;
}

static long tas_calib_ioctl(struct file *filp, uint cmd, ulong arg)
{
	return 0;
}

static int tas_calib_release(struct inode *inode, struct file *fd)
{
	return 0;
}

const struct file_operations tas_calib_fops = {
	.owner			= THIS_MODULE,
	.open			= tas_calib_open,
	.write			= tas_calib_write,
	.read			= tas_calib_read,
	.release		= tas_calib_release,
	.unlocked_ioctl	= tas_calib_ioctl,
};

static struct miscdevice tas_calib_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "tas_calib",
	.fops = &tas_calib_fops,
};

int tas_calib_register(void)
{
	int rc;
	rc = misc_register(&tas_calib_misc);
	if (rc)
		pr_err("register calib misc failed\n");
	return rc;
}

int tas_calib_deregister(void)
{
	return misc_deregister(&tas_calib_misc);
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 I2C Smart Amplifier driver");
MODULE_LICENSE("GPLv2");
