/* Copyright (c) 2013-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <linux/io.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/qpnp/power-on.h>
#include <linux/of_address.h>

#include <asm/cacheflush.h>
#include <asm/system_misc.h>

#include <soc/qcom/scm.h>
#include <soc/qcom/restart.h>
#include <soc/qcom/watchdog.h>
#ifdef CONFIG_HUAWEI_RESET_DETECT
#include <linux/huawei_reset_detect.h>
#endif
#include <chipset_common/bfmr/bfm/chipsets/qcom/bfm_qcom.h>

#include <linux/fcntl.h>
#include <linux/syscalls.h>
#define MISC_DEVICE "/dev/block/bootdevice/by-name/misc"
#define USB_UPDATE_POLL_TIME  2000
struct bootloader_message {
    char command[32];
    char status[32];
    char recovery[768];
    char stage[32];
};
#define EMERGENCY_DLOAD_MAGIC1    0x322A4F99
#define EMERGENCY_DLOAD_MAGIC2    0xC67E4350
#define EMERGENCY_DLOAD_MAGIC3    0x77777777
#define EMMC_DLOAD_TYPE		0x2

#define SCM_IO_DISABLE_PMIC_ARBITER	1
#define SCM_IO_DEASSERT_PS_HOLD		2
#define SCM_WDOG_DEBUG_BOOT_PART	0x9
#define SCM_DLOAD_MODE			0X10
#define SCM_EDLOAD_MODE			0X01
#define SCM_DLOAD_CMD			0x10

#ifdef CONFIG_HUAWEI_DEBUG_MODE
extern char *saved_command_line;
#endif

static int restart_mode;
static void *dload_type_addr;
static void *restart_reason;
static bool scm_pmic_arbiter_disable_supported;
static bool scm_deassert_ps_hold_supported;
/* Download mode master kill-switch */
static void __iomem *msm_ps_hold;
static phys_addr_t tcsr_boot_misc_detect;
static void scm_disable_sdi(void);

#ifdef CONFIG_MSM_DLOAD_MODE
/* Runtime could be only changed value once.
* There is no API from TZ to re-enable the registers.
* So the SDI cannot be re-enabled when it already by-passed.
*/
static int download_mode = 1;
#else
static const int download_mode;
#endif

#ifdef CONFIG_MSM_DLOAD_MODE
#define EDL_MODE_PROP "qcom,msm-imem-emergency_download_mode"
#define DL_MODE_PROP "qcom,msm-imem-download_mode"

#define SDUPDATE_FLAG_MAGIC_NUM  0x77665528
#define USBUPDATE_FLAG_MAGIC_NUM  0x77665523
#define SD_UPDATE_RESET_FLAG   "sdupdate"
#define USB_UPDATE_RESET_FLAG   "usbupdate"
static int in_panic;
static void *dload_mode_addr;
static bool dload_mode_enabled;
static void *emergency_dload_mode_addr;
static bool scm_dload_supported;
static struct kobject dload_kobj;
static void *dload_type_addr;

static int dload_set(const char *val, struct kernel_param *kp);
/* interface for exporting attributes */
struct reset_attribute {
	struct attribute        attr;
	ssize_t (*show)(struct kobject *kobj, struct attribute *attr,
			char *buf);
	size_t (*store)(struct kobject *kobj, struct attribute *attr,
			const char *buf, size_t count);
};
#define to_reset_attr(_attr) \
	container_of(_attr, struct reset_attribute, attr)
#define RESET_ATTR(_name, _mode, _show, _store)	\
	static struct reset_attribute reset_attr_##_name = \
			__ATTR(_name, _mode, _show, _store)

module_param_call(download_mode, dload_set, param_get_int,
			&download_mode, 0644);

static int panic_prep_restart(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	in_panic = 1;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_prep_restart,
};

int scm_set_dload_mode(int arg1, int arg2)
{
	struct scm_desc desc = {
		.args[0] = arg1,
		.args[1] = arg2,
		.arginfo = SCM_ARGS(2),
	};

	if (!scm_dload_supported) {
		if (tcsr_boot_misc_detect)
			return scm_io_write(tcsr_boot_misc_detect, arg1);

		return 0;
	}

	if (!is_scm_armv8())
		return scm_call_atomic2(SCM_SVC_BOOT, SCM_DLOAD_CMD, arg1,
					arg2);

	return scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_BOOT, SCM_DLOAD_CMD),
				&desc);
}

static void set_dload_mode(int on)
{
	int ret;

	if (dload_mode_addr) {
		__raw_writel(on ? 0xE47B337D : 0, dload_mode_addr);
		__raw_writel(on ? 0xCE14091A : 0,
		       dload_mode_addr + sizeof(unsigned int));
		mb();
	}

	ret = scm_set_dload_mode(on ? SCM_DLOAD_MODE : 0, 0);
	if (ret)
		pr_err("Failed to set secure DLOAD mode: %d\n", ret);

	dload_mode_enabled = on;
}

void clear_dload_mode(void)
{
	set_dload_mode(0);
}

static bool get_dload_mode(void)
{
	return dload_mode_enabled;
}

/*This function write usb_update sign to the misc partion,
   if write successfull, then hard restart the device*/
void huawei_restart(void)
{
    struct bootloader_message boot;
    int fd  = 0;

    memset((void*)&boot, 0x0, sizeof(struct bootloader_message));
    strlcpy(boot.command, "boot-recovery", sizeof(boot.command));
    strcpy(boot.recovery, "recovery\n");
    strcat(boot.recovery, "--");
    strcat(boot.recovery, "usb_update");

    fd = sys_open(MISC_DEVICE,O_RDWR,0);
    if( fd < 0 )
    {
        pr_err("open the devices %s fail",MISC_DEVICE);
        return ;
    }
    if(sys_write((unsigned int )fd, (char*)&boot, sizeof(boot)) < 0)
    {
        pr_err("write to the devices %s fail",MISC_DEVICE);
        return;
    }
    sys_sync();
    kernel_restart(NULL);
}
/*This function poll the address restart_reason,if
   there is the magic SDUPDATE_FLAG_MAGIC_NUM, restart
   the devices.,the magic is written by modem when the
   user click the usb update tool to update*/
int usb_update_thread(void *__unused)
{
    unsigned int  dload_magic = 0;
    for(;;)
    {
        if(NULL != restart_reason)
        {
             dload_magic = __raw_readl(restart_reason);
        }
        else
        {
             pr_info("restart_reason is null,wait for ready\n");
        }
        if(SDUPDATE_FLAG_MAGIC_NUM == dload_magic)
        {
            pr_info("update mode, restart to usb update\n");
            huawei_restart();
        }
        msleep(USB_UPDATE_POLL_TIME);
    }
    return 0;
}
#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
static void enable_emergency_dload_mode(void)
{
	int ret;

	if (emergency_dload_mode_addr) {
		__raw_writel(EMERGENCY_DLOAD_MAGIC1,
				emergency_dload_mode_addr);
		__raw_writel(EMERGENCY_DLOAD_MAGIC2,
				emergency_dload_mode_addr +
				sizeof(unsigned int));
		__raw_writel(EMERGENCY_DLOAD_MAGIC3,
				emergency_dload_mode_addr +
				(2 * sizeof(unsigned int)));

		/* Need disable the pmic wdt, then the emergency dload mode
		 * will not auto reset. */
		qpnp_pon_wd_config(0);
		mb();
	}

	ret = scm_set_dload_mode(SCM_EDLOAD_MODE, 0);
	if (ret)
		pr_err("Failed to set secure EDLOAD mode: %d\n", ret);
}
#endif
static int dload_set(const char *val, struct kernel_param *kp)
{
	int ret;
	int old_val = download_mode;

	ret = param_set_int(val, kp);

	if (ret)
		return ret;

	/* If download_mode is not zero or one, ignore. */
	if (download_mode >> 1) {
		download_mode = old_val;
		return -EINVAL;
	}

	set_dload_mode(download_mode);

	return 0;
}
#else
static void set_dload_mode(int on)
{
	return;
}

static void enable_emergency_dload_mode(void)
{
	pr_err("dload mode is not enabled on target\n");
}

static bool get_dload_mode(void)
{
	return false;
}
#endif

static void scm_disable_sdi(void)
{
	int ret;
	struct scm_desc desc = {
		.args[0] = 1,
		.args[1] = 0,
		.arginfo = SCM_ARGS(2),
	};

	/* Needed to bypass debug image on some chips */
	if (!is_scm_armv8())
		ret = scm_call_atomic2(SCM_SVC_BOOT,
			       SCM_WDOG_DEBUG_BOOT_PART, 1, 0);
	else
		ret = scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_BOOT,
			  SCM_WDOG_DEBUG_BOOT_PART), &desc);
	if (ret)
		pr_err("Failed to disable secure wdog debug: %d\n", ret);
}

void msm_set_restart_mode(int mode)
{
	restart_mode = mode;
}
EXPORT_SYMBOL(msm_set_restart_mode);

/*
 * Force the SPMI PMIC arbiter to shutdown so that no more SPMI transactions
 * are sent from the MSM to the PMIC.  This is required in order to avoid an
 * SPMI lockup on certain PMIC chips if PS_HOLD is lowered in the middle of
 * an SPMI transaction.
 */
static void halt_spmi_pmic_arbiter(void)
{
	struct scm_desc desc = {
		.args[0] = 0,
		.arginfo = SCM_ARGS(1),
	};

	if (scm_pmic_arbiter_disable_supported) {
		pr_crit("Calling SCM to disable SPMI PMIC arbiter\n");
		if (!is_scm_armv8())
			scm_call_atomic1(SCM_SVC_PWR,
					 SCM_IO_DISABLE_PMIC_ARBITER, 0);
		else
			scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_PWR,
				  SCM_IO_DISABLE_PMIC_ARBITER), &desc);
	}
}

static void msm_restart_prepare(const char *cmd)
{
	bool need_warm_reset = false;

#ifdef CONFIG_MSM_DLOAD_MODE

	/* Write download mode flags if we're panic'ing
	 * Write download mode flags if restart_mode says so
	 * Kill download mode if master-kill switch is set
	 */

	set_dload_mode(download_mode &&
			(in_panic || restart_mode == RESTART_DLOAD));
#endif
#ifdef CONFIG_HUAWEI_RESET_DETECT
	/* if the restart is triggered by panic, keep the magic number
	* if the restart is a nomal reboot, clear the reset magic number*/
	if(!in_panic)
	{
		clear_reset_magic();
	}
#endif

    /* only panic & watchdog, we record as a bootfail */
	if(!in_panic) {
		hwboot_clear_magic();
	}

	if (qpnp_pon_check_hard_reset_stored()) {
		/* Set warm reset as true when device is in dload mode */
		if (get_dload_mode() ||
			((cmd != NULL && cmd[0] != '\0') &&
			!strcmp(cmd, "edl")))
			need_warm_reset = true;
	} else {
		need_warm_reset = (in_panic || get_dload_mode() ||
				(cmd != NULL && cmd[0] != '\0'));
	}

	/* Hard reset the PMIC unless memory contents must be maintained. */
	if (need_warm_reset) {
		qpnp_pon_system_pwr_off(PON_POWER_OFF_WARM_RESET);
	} else {
		qpnp_pon_system_pwr_off(PON_POWER_OFF_HARD_RESET);
	}

	if (cmd != NULL) {
		if (!strncmp(cmd, "bootloader", 10)) {
			qpnp_pon_set_restart_reason(
				PON_RESTART_REASON_BOOTLOADER);
			__raw_writel(0x77665500, restart_reason);
		} else if (!strncmp(cmd, "recovery", 8)) {
			qpnp_pon_set_restart_reason(
				PON_RESTART_REASON_RECOVERY);
			__raw_writel(0x77665502, restart_reason);
		} else if (!strcmp(cmd, "rtc")) {
			qpnp_pon_set_restart_reason(
				PON_RESTART_REASON_RTC);
			__raw_writel(0x77665503, restart_reason);
		} else if (!strcmp(cmd, "dm-verity device corrupted")) {
			qpnp_pon_set_restart_reason(
				PON_RESTART_REASON_DMVERITY_CORRUPTED);
			__raw_writel(0x77665508, restart_reason);
		} else if (!strcmp(cmd, "dm-verity enforcing")) {
			qpnp_pon_set_restart_reason(
				PON_RESTART_REASON_DMVERITY_ENFORCE);
			__raw_writel(0x77665509, restart_reason);
		} else if (!strcmp(cmd, "keys clear")) {
			qpnp_pon_set_restart_reason(
				PON_RESTART_REASON_KEYS_CLEAR);
			__raw_writel(0x7766550a, restart_reason);
		} else if (!strncmp(cmd, "oem-", 4)) {
			unsigned long code;
			int ret;
			ret = kstrtoul(cmd + 4, 16, &code);
			if (!ret)
				__raw_writel(0x6f656d00 | (code & 0xff),
					     restart_reason);
#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
		} else if (!strncmp(cmd, "edl", 3)) {
			enable_emergency_dload_mode();
#endif
#ifdef CONFIG_HUAWEI_RESET_DETECT
		}else if (!strncmp(cmd, "emergency_restart", 17)) {
		pr_info("do nothing\n");
#endif
		} else {
			__raw_writel(0x77665501, restart_reason);
		}
	}

	flush_cache_all();

	/*outer_flush_all is not supported by 64bit kernel*/
#ifndef CONFIG_ARM64
	outer_flush_all();
#endif

}

/*
 * Deassert PS_HOLD to signal the PMIC that we are ready to power down or reset.
 * Do this by calling into the secure environment, if available, or by directly
 * writing to a hardware register.
 *
 * This function should never return.
 */
static void deassert_ps_hold(void)
{
	struct scm_desc desc = {
		.args[0] = 0,
		.arginfo = SCM_ARGS(1),
	};

	if (scm_deassert_ps_hold_supported) {
		/* This call will be available on ARMv8 only */
		scm_call2_atomic(SCM_SIP_FNID(SCM_SVC_PWR,
				 SCM_IO_DEASSERT_PS_HOLD), &desc);
	}

	/* Fall-through to the direct write in case the scm_call "returns" */
	__raw_writel(0, msm_ps_hold);
}

static void do_msm_restart(enum reboot_mode reboot_mode, const char *cmd)
{
	pr_notice("Going down for restart now\n");

	console_verbose();
	dump_stack();
	msm_restart_prepare(cmd);

#ifdef CONFIG_MSM_DLOAD_MODE
	/*
	 * Trigger a watchdog bite here and if this fails,
	 * device will take the usual restart path.
	 */

	if (WDOG_BITE_ON_PANIC && in_panic)
		msm_trigger_wdog_bite();
#endif

	scm_disable_sdi();
	halt_spmi_pmic_arbiter();
	deassert_ps_hold();

	mdelay(10000);
}

static void do_msm_poweroff(void)
{
	pr_notice("Powering off the SoC\n");

	set_dload_mode(0);
#ifdef CONFIG_HUAWEI_RESET_DETECT
	clear_reset_magic();
#endif
	scm_disable_sdi();
	qpnp_pon_system_pwr_off(PON_POWER_OFF_SHUTDOWN);

	halt_spmi_pmic_arbiter();
	deassert_ps_hold();

	mdelay(10000);
	pr_err("Powering off has failed\n");
	return;
}

#ifdef CONFIG_MSM_DLOAD_MODE
static ssize_t attr_show(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	struct reset_attribute *reset_attr = to_reset_attr(attr);
	ssize_t ret = -EIO;

	if (reset_attr->show)
		ret = reset_attr->show(kobj, attr, buf);

	return ret;
}

static ssize_t attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	struct reset_attribute *reset_attr = to_reset_attr(attr);
	ssize_t ret = -EIO;

	if (reset_attr->store)
		ret = reset_attr->store(kobj, attr, buf, count);

	return ret;
}

static const struct sysfs_ops reset_sysfs_ops = {
	.show	= attr_show,
	.store	= attr_store,
};

static struct kobj_type reset_ktype = {
	.sysfs_ops	= &reset_sysfs_ops,
};

static ssize_t show_emmc_dload(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	uint32_t read_val, show_val;

	read_val = __raw_readl(dload_type_addr);
	if (read_val == EMMC_DLOAD_TYPE)
		show_val = 1;
	else
		show_val = 0;

	return snprintf(buf, sizeof(show_val), "%u\n", show_val);
}

static size_t store_emmc_dload(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	uint32_t enabled;
	int ret;

	ret = kstrtouint(buf, 0, &enabled);
	if (ret < 0)
		return ret;

	if (!((enabled == 0) || (enabled == 1)))
		return -EINVAL;

	if (enabled == 1)
		__raw_writel(EMMC_DLOAD_TYPE, dload_type_addr);
	else
		__raw_writel(0, dload_type_addr);

	return count;
}
RESET_ATTR(emmc_dload, 0644, show_emmc_dload, store_emmc_dload);

static struct attribute *reset_attrs[] = {
	&reset_attr_emmc_dload.attr,
	NULL
};

static struct attribute_group reset_attr_group = {
	.attrs = reset_attrs,
};
#endif

static int msm_restart_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct resource *mem;
	struct device_node *np;
	int ret = 0;

#ifdef CONFIG_MSM_DLOAD_MODE
	if (scm_is_call_available(SCM_SVC_BOOT, SCM_DLOAD_CMD) > 0)
		scm_dload_supported = true;

	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	np = of_find_compatible_node(NULL, NULL, DL_MODE_PROP);
	if (!np) {
		pr_err("unable to find DT imem DLOAD mode node\n");
	} else {
		dload_mode_addr = of_iomap(np, 0);
		if (!dload_mode_addr)
			pr_err("unable to map imem DLOAD offset\n");
	}

	np = of_find_compatible_node(NULL, NULL, EDL_MODE_PROP);
	if (!np) {
		pr_err("unable to find DT imem EDLOAD mode node\n");
	} else {
		emergency_dload_mode_addr = of_iomap(np, 0);
		if (!emergency_dload_mode_addr)
			pr_err("unable to map imem EDLOAD mode offset\n");
	}
#ifdef CONFIG_HUAWEI_DEBUG_MODE
	if(strstr(saved_command_line,"huawei_debug_mode=1")!=NULL)
	{
		printk("For debug mode,download_mode is true");
		download_mode = 1;
	}
#endif
	np = of_find_compatible_node(NULL, NULL,
				"qcom,msm-imem-dload-type");
	if (!np) {
		pr_err("unable to find DT imem dload-type node\n");
		goto skip_sysfs_create;
	} else {
		dload_type_addr = of_iomap(np, 0);
		if (!dload_type_addr) {
			pr_err("unable to map imem dload-type offset\n");
			goto skip_sysfs_create;
		}
	}

	ret = kobject_init_and_add(&dload_kobj, &reset_ktype,
			kernel_kobj, "%s", "dload");
	if (ret) {
		pr_err("%s:Error in creation kobject_add\n", __func__);
		kobject_put(&dload_kobj);
		goto skip_sysfs_create;
	}

	ret = sysfs_create_group(&dload_kobj, &reset_attr_group);
	if (ret) {
		pr_err("%s:Error in creation sysfs_create_group\n", __func__);
		kobject_del(&dload_kobj);
	}
skip_sysfs_create:
#endif
	np = of_find_compatible_node(NULL, NULL,
				"qcom,msm-imem-restart_reason");
	if (!np) {
		pr_err("unable to find DT imem restart reason node\n");
	} else {
		restart_reason = of_iomap(np, 0);
		if (!restart_reason) {
			pr_err("unable to map imem restart reason offset\n");
			ret = -ENOMEM;
			goto err_restart_reason;
		}
	}

	mem = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pshold-base");
	msm_ps_hold = devm_ioremap_resource(dev, mem);
	if (IS_ERR(msm_ps_hold))
		return PTR_ERR(msm_ps_hold);

	mem = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "tcsr-boot-misc-detect");
	if (mem)
		tcsr_boot_misc_detect = mem->start;

	pm_power_off = do_msm_poweroff;
	arm_pm_restart = do_msm_restart;

	if (scm_is_call_available(SCM_SVC_PWR, SCM_IO_DISABLE_PMIC_ARBITER) > 0)
		scm_pmic_arbiter_disable_supported = true;

	if (scm_is_call_available(SCM_SVC_PWR, SCM_IO_DEASSERT_PS_HOLD) > 0)
		scm_deassert_ps_hold_supported = true;

	set_dload_mode(download_mode);
	if (!download_mode)
		scm_disable_sdi();

	return 0;

err_restart_reason:
#ifdef CONFIG_MSM_DLOAD_MODE
	iounmap(emergency_dload_mode_addr);
	iounmap(dload_mode_addr);
#endif
	return ret;
}

static const struct of_device_id of_msm_restart_match[] = {
	{ .compatible = "qcom,pshold", },
	{},
};
MODULE_DEVICE_TABLE(of, of_msm_restart_match);

static struct platform_driver msm_restart_driver = {
	.probe = msm_restart_probe,
	.driver = {
		.name = "msm-restart",
		.of_match_table = of_match_ptr(of_msm_restart_match),
	},
};

#ifdef CONFIG_MSM_DLOAD_MODE
static int __init download_mode_setup(char *p)
{
	unsigned int value = 0;

	if (NULL == p) {
		pr_err("%s: input null\n", __func__);
		return -EINVAL;
	}

	/* from string to unsigned int */
	if (kstrtouint(p, 0, &value) < 0) {
		pr_err("%s: Failed to get download mode\n", __func__);
		return -EINVAL;
	}

	download_mode = value;

	return 0;
}

early_param("restart.download_mode", download_mode_setup);
#endif

static int __init msm_restart_init(void)
{
	return platform_driver_register(&msm_restart_driver);
}
device_initcall(msm_restart_init);
