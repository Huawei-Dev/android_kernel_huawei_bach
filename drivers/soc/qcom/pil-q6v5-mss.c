/*
 * Copyright (c) 2012-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/of_gpio.h>
#include <linux/clk/msm-clk.h>
#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
#include <linux/sched.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/store_exception.h>

#endif

#include <soc/qcom/subsystem_restart.h>
#include <soc/qcom/ramdump.h>
#include <soc/qcom/smem.h>
#include <soc/qcom/smsm.h>

#include "peripheral-loader.h"
#include "pil-q6v5.h"
#include "pil-msa.h"

#define MAX_VDD_MSS_UV		1150000
#define PROXY_TIMEOUT_MS	10000
#define MAX_SSR_REASON_LEN	81U
#define STOP_ACK_TIMEOUT_MS	1000

#define subsys_to_drv(d) container_of(d, struct modem_data, subsys_desc)

#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
#define SSR_REASON_LEN  MAX_SSR_REASON_LEN
#define DATA_LOG_DIR    "/data/log/modem_crash/"
#define MODEM_EXCEPTION "modem_exception"
#define MODEM_CRASH_LOG "modem_crash_log"
#define MODEM_F3_TRACE      "modem_f3_trace"

#define ERR_DATA_MAX_SIZE 0x4000U //16K
#define DIAG_F3_TRACE_BUFFER_SIZE 0x4000U  //16K

/* define work data sturct*/
struct work_data{
    struct work_struct log_modem_work; //WORK
    struct workqueue_struct *log_modem_work_queue; //WORK QUEUE
    char reset_reason[SSR_REASON_LEN];
    char crash_log_valid;
    char crash_log[ERR_DATA_MAX_SIZE];
    char f3_trace_log_valid;
    char f3_trace_log[DIAG_F3_TRACE_BUFFER_SIZE + 4];
};

//struct work_struct log_modem_work; //used to fill the temp work
static struct work_data *g_work_data = NULL;

/*the queue work handle, store the modem reason to the exception file*/
static void log_modem_work_func(struct work_struct *work)
{
    struct work_data *work_data_self = container_of(work, struct work_data, log_modem_work);
    if(NULL == work_data_self)
    {
        pr_err("[log_modem_reset]work_data_self is NULL!\n");
        return;
    }

    store_exception(DATA_LOG_DIR, MODEM_EXCEPTION, work_data_self->reset_reason, strlen(work_data_self->reset_reason));
    if(g_work_data->crash_log_valid){
        store_exception(DATA_LOG_DIR, MODEM_CRASH_LOG, work_data_self->crash_log, sizeof(work_data_self->crash_log));
        g_work_data->crash_log_valid = false;
    }

    if(g_work_data->f3_trace_log_valid){
        store_exception(DATA_LOG_DIR, MODEM_F3_TRACE, work_data_self->f3_trace_log, sizeof(work_data_self->f3_trace_log));
        g_work_data->f3_trace_log_valid = false;
    }

    pr_info("[log_modem_reset]log_modem_reset_work after write exception inode work_data_self->reset_reason=%s \n",work_data_self->reset_reason);
    g_work_data->reset_reason[0] = '\0';
}

/*creat the queue that log the modem reset reason*/
int log_modem_queue_create(void)
{
    int error = 0;

    g_work_data = kzalloc(sizeof(struct work_data),GFP_KERNEL);
    if (NULL == g_work_data) {
        error = -ENOMEM;
        pr_err("[log_modem_reset]work_data_temp is NULL, Don't log this!\n");
        return error;
    }

    INIT_WORK(&(g_work_data->log_modem_work), log_modem_work_func);

    g_work_data->log_modem_work_queue = create_singlethread_workqueue("log_modem_reset");
    if (NULL == g_work_data->log_modem_work_queue ) {
        error = -ENOMEM;
        pr_err("[log_modem_reset]log modem reset queue created failed!\n");
        return error;
    }

    pr_info("[log_modem_reset]log modem reset queue created success \n");
    return error;
}

int log_modem_queue(void)
{
    //insert the work to the queue
    if(NULL == g_work_data->log_modem_work_queue){
        pr_err("[log_modem_reset]log_modem_reset_work_queue is NULL, return !!!!!!!\n");
        return -2;
    }

    pr_info("[log_modem_reset]modem reset reason inserted the log_modem_reset_queue \n");
    queue_work(g_work_data->log_modem_work_queue, &(g_work_data->log_modem_work));
    return 0;
}

static void log_modem_crash_log(int record_enabled)
{
    u32 size;
    char *crash_log;

    crash_log = smem_get_entry_no_rlock(SMEM_ERR_CRASH_LOG, &size, 0,
                    SMEM_ANY_HOST_FLAG);
    if (!crash_log || !size) {
        pr_err("log_modem_crash_log failure reason: (unknown, smem_get_entry_no_rlock failed).\n");
        return;
    }

    if (!crash_log[0]) {
        pr_err("log_modem_crash_log failure reason: (unknown, empty string found).\n");
        return;
    }

    if(record_enabled && !subsystem_restart_requested){
        memcpy(g_work_data->crash_log, crash_log, ERR_DATA_MAX_SIZE - 1);
        g_work_data->crash_log_valid = true;
    }

    crash_log[0] = '\0';
    wmb();
}

static void log_modem_f3_track_log(int record_enabled)
{
    u32 size;
    char *smem_f3_track;

    smem_f3_track = smem_get_entry_no_rlock(SMEM_ERR_F3_TRACE_LOG, &size, 0,
                    SMEM_ANY_HOST_FLAG);
    if (!smem_f3_track || !size) {
        pr_err("log_modem_f3_track_log failure reason: (unknown, smem_get_entry_no_rlock failed).\n");
        return;
    }

    if (!smem_f3_track[0]) {
        pr_err("log_modem_f3_track_log failure reason: (unknown, empty string found).\n");
        return;
    }

    if(record_enabled && !subsystem_restart_requested){
        memcpy(g_work_data->f3_trace_log, smem_f3_track, DIAG_F3_TRACE_BUFFER_SIZE - 1);
        g_work_data->f3_trace_log_valid = true;
    }

    smem_f3_track[0] = '\0';
    wmb();

}

#endif

static void log_modem_sfr(int record_enabled)
{
    u32 size;
    char *smem_reason, reason[MAX_SSR_REASON_LEN];

    smem_reason = smem_get_entry_no_rlock(SMEM_SSR_REASON_MSS0, &size, 0,
                            SMEM_ANY_HOST_FLAG);
    if (!smem_reason || !size) {
        pr_err("modem subsystem failure reason: (unknown, smem_get_entry_no_rlock failed).\n");
        return;
    }
    if (!smem_reason[0]) {
        pr_err("modem subsystem failure reason: (unknown, empty string found).\n");
        return;
    }

    strlcpy(reason, smem_reason, min(size, MAX_SSR_REASON_LEN));
    pr_err("modem subsystem failure reason: %s.\n", reason);

#ifdef CONFIG_HUAWEI_KERNEL
    if(strstr(reason,"huawei") || strstr(reason,"cm_hw_request_modem_reset")){
        pr_err("reset modem subsystem by huawei\n");
        subsystem_restart_requested = 1;
    }else if(record_enabled){
        //fill the reason
        strncpy(g_work_data->reset_reason,reason,SSR_REASON_LEN - 1);
    }
#endif

    smem_reason[0] = '\0';
    wmb();
}

static void restart_modem(struct modem_data *drv)
{
    int is_pending = work_pending(&g_work_data->log_modem_work);
    log_modem_sfr(!is_pending);
    log_modem_crash_log(!is_pending);
    log_modem_f3_track_log(!is_pending);

    if(is_pending){
        pr_err("modem reset work is pending\n");
    }else{
        log_modem_queue();
        pr_info("[log_modem_reset]put done \n");
    }

    subsystem_restart_dev(drv->subsys);
}

static irqreturn_t modem_err_fatal_intr_handler(int irq, void *dev_id)
{
	struct modem_data *drv = subsys_to_drv(dev_id);

	/* Ignore if we're the one that set the force stop GPIO */
	if (drv->crash_shutdown)
		return IRQ_HANDLED;

	pr_err("Fatal error on the modem.\n");
	subsys_set_crash_status(drv->subsys, true);
	restart_modem(drv);
	return IRQ_HANDLED;
}

static irqreturn_t modem_stop_ack_intr_handler(int irq, void *dev_id)
{
	struct modem_data *drv = subsys_to_drv(dev_id);
	pr_info("Received stop ack interrupt from modem\n");
	complete(&drv->stop_ack);
	return IRQ_HANDLED;
}

static int modem_shutdown(const struct subsys_desc *subsys, bool force_stop)
{
	struct modem_data *drv = subsys_to_drv(subsys);
	unsigned long ret;

	if (subsys->is_not_loadable)
		return 0;

	if (!subsys_get_crash_status(drv->subsys) && force_stop &&
	    subsys->force_stop_gpio) {
		gpio_set_value(subsys->force_stop_gpio, 1);
		ret = wait_for_completion_timeout(&drv->stop_ack,
				msecs_to_jiffies(STOP_ACK_TIMEOUT_MS));
		if (!ret)
			pr_warn("Timed out on stop ack from modem.\n");
		gpio_set_value(subsys->force_stop_gpio, 0);
	}

	if (drv->subsys_desc.ramdump_disable_gpio) {
		drv->subsys_desc.ramdump_disable = gpio_get_value(
					drv->subsys_desc.ramdump_disable_gpio);
		 pr_warn("Ramdump disable gpio value is %d\n",
			drv->subsys_desc.ramdump_disable);
	}

	pil_shutdown(&drv->q6->desc);

	return 0;
}

#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
#define OEM_QMI "libqmi_oem_main"
static void restart_oem_qmi(void)
{
    struct task_struct *tsk = NULL;

    for_each_process(tsk)
    {
        if (tsk->comm && !strcmp(tsk->comm, OEM_QMI))
        {
            send_sig(SIGKILL, tsk, 0);
            return;
        }
    }
}
#endif

static int modem_powerup(const struct subsys_desc *subsys)
{
    struct modem_data *drv = subsys_to_drv(subsys);
#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
    int ret = 0;
#endif
    if (subsys->is_not_loadable)
        return 0;
    /*
     * At this time, the modem is shutdown. Therefore this function cannot
     * run concurrently with the watchdog bite error handler, making it safe
     * to unset the flag below.
     */
    reinit_completion(&drv->stop_ack);
    drv->subsys_desc.ramdump_disable = 0;
    drv->ignore_errors = false;
    drv->q6->desc.fw_name = subsys->fw_name;
#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
    ret = pil_boot(&drv->q6->desc);
    /* after modem restart, restart oem qmi */
    restart_oem_qmi();
    return ret;
#else
    return pil_boot(&drv->q6->desc);
#endif
}

static void modem_crash_shutdown(const struct subsys_desc *subsys)
{
	struct modem_data *drv = subsys_to_drv(subsys);
	drv->crash_shutdown = true;
	if (!subsys_get_crash_status(drv->subsys) &&
		subsys->force_stop_gpio) {
		gpio_set_value(subsys->force_stop_gpio, 1);
		mdelay(STOP_ACK_TIMEOUT_MS);
	}
}

static int modem_ramdump(int enable, const struct subsys_desc *subsys)
{
	struct modem_data *drv = subsys_to_drv(subsys);
	int ret;

	if (!enable)
		return 0;

	ret = pil_mss_make_proxy_votes(&drv->q6->desc);
	if (ret)
		return ret;

	ret = pil_mss_reset_load_mba(&drv->q6->desc);
	if (ret)
		return ret;

	ret = pil_do_ramdump(&drv->q6->desc, drv->ramdump_dev);
	if (ret < 0)
		pr_err("Unable to dump modem fw memory (rc = %d).\n", ret);

	ret = __pil_mss_deinit_image(&drv->q6->desc, false);
	if (ret < 0)
		pr_err("Unable to free up resources (rc = %d).\n", ret);

	pil_mss_remove_proxy_votes(&drv->q6->desc);
	return ret;
}

static irqreturn_t modem_wdog_bite_intr_handler(int irq, void *dev_id)
{
	struct modem_data *drv = subsys_to_drv(dev_id);
	if (drv->ignore_errors)
		return IRQ_HANDLED;

	pr_err("Watchdog bite received from modem software!\n");
	if (drv->subsys_desc.system_debug &&
			!gpio_get_value(drv->subsys_desc.err_fatal_gpio))
		panic("%s: System ramdump requested. Triggering device restart!\n",
							__func__);
	subsys_set_crash_status(drv->subsys, true);
	restart_modem(drv);
	return IRQ_HANDLED;
}

static int pil_subsys_init(struct modem_data *drv,
					struct platform_device *pdev)
{
	int ret;

	drv->subsys_desc.name = "modem";
	drv->subsys_desc.dev = &pdev->dev;
	drv->subsys_desc.owner = THIS_MODULE;
	drv->subsys_desc.shutdown = modem_shutdown;
	drv->subsys_desc.powerup = modem_powerup;
	drv->subsys_desc.ramdump = modem_ramdump;
	drv->subsys_desc.crash_shutdown = modem_crash_shutdown;
	drv->subsys_desc.err_fatal_handler = modem_err_fatal_intr_handler;
	drv->subsys_desc.stop_ack_handler = modem_stop_ack_intr_handler;
	drv->subsys_desc.wdog_bite_handler = modem_wdog_bite_intr_handler;

	drv->q6->desc.modem_ssr = false;
	drv->subsys = subsys_register(&drv->subsys_desc);
	if (IS_ERR(drv->subsys)) {
		ret = PTR_ERR(drv->subsys);
		goto err_subsys;
	}
#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
    log_modem_queue_create();
#endif

	drv->ramdump_dev = create_ramdump_device("modem", &pdev->dev);
	if (!drv->ramdump_dev) {
		pr_err("%s: Unable to create a modem ramdump device.\n",
			__func__);
		ret = -ENOMEM;
		goto err_ramdump;
	}

	return 0;

err_ramdump:
	subsys_unregister(drv->subsys);
err_subsys:
	return ret;
}

static int pil_mss_loadable_init(struct modem_data *drv,
					struct platform_device *pdev)
{
	struct q6v5_data *q6;
	struct pil_desc *q6_desc;
	struct resource *res;
	struct property *prop;
	int ret;

	q6 = pil_q6v5_init(pdev);
	if (IS_ERR_OR_NULL(q6))
		return PTR_ERR(q6);
	drv->q6 = q6;
	drv->xo = q6->xo;

	q6_desc = &q6->desc;
	q6_desc->owner = THIS_MODULE;
	q6_desc->proxy_timeout = PROXY_TIMEOUT_MS;

	q6_desc->ops = &pil_msa_mss_ops;

	q6->self_auth = of_property_read_bool(pdev->dev.of_node,
							"qcom,pil-self-auth");
	if (q6->self_auth) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
						    "rmb_base");
		q6->rmb_base = devm_ioremap_resource(&pdev->dev, res);
		if (!q6->rmb_base)
			return -ENOMEM;
		drv->rmb_base = q6->rmb_base;
		q6_desc->ops = &pil_msa_mss_ops_selfauth;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "restart_reg");
	if (!res) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
							"restart_reg_sec");
		q6->restart_reg_sec = true;
	}

	q6->restart_reg = devm_ioremap_resource(&pdev->dev, res);
	if (!q6->restart_reg)
		return -ENOMEM;

	q6->vreg = NULL;

	prop = of_find_property(pdev->dev.of_node, "vdd_mss-supply", NULL);
	if (prop) {
		q6->vreg = devm_regulator_get(&pdev->dev, "vdd_mss");
		if (IS_ERR(q6->vreg))
			return PTR_ERR(q6->vreg);

		ret = regulator_set_voltage(q6->vreg, VDD_MSS_UV,
						MAX_VDD_MSS_UV);
		if (ret)
			dev_err(&pdev->dev, "Failed to set vreg voltage.\n");

		ret = regulator_set_optimum_mode(q6->vreg, 100000);
		if (ret < 0) {
			dev_err(&pdev->dev, "Failed to set vreg mode.\n");
			return ret;
		}
	}

	q6->vreg_mx = devm_regulator_get(&pdev->dev, "vdd_mx");
	if (IS_ERR(q6->vreg_mx))
		return PTR_ERR(q6->vreg_mx);
	prop = of_find_property(pdev->dev.of_node, "vdd_mx-uV", NULL);
	if (!prop) {
		dev_err(&pdev->dev, "Missing vdd_mx-uV property\n");
		return -EINVAL;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
		"cxrail_bhs_reg");
	if (res)
		q6->cxrail_bhs = devm_ioremap(&pdev->dev, res->start,
					  resource_size(res));

	q6->ahb_clk = devm_clk_get(&pdev->dev, "iface_clk");
	if (IS_ERR(q6->ahb_clk))
		return PTR_ERR(q6->ahb_clk);

	q6->axi_clk = devm_clk_get(&pdev->dev, "bus_clk");
	if (IS_ERR(q6->axi_clk))
		return PTR_ERR(q6->axi_clk);

	q6->rom_clk = devm_clk_get(&pdev->dev, "mem_clk");
	if (IS_ERR(q6->rom_clk))
		return PTR_ERR(q6->rom_clk);

	ret = of_property_read_u32(pdev->dev.of_node,
					"qcom,pas-id", &drv->pas_id);
	if (ret)
		dev_warn(&pdev->dev, "Failed to find the pas_id.\n");

	drv->subsys_desc.pil_mss_memsetup =
	of_property_read_bool(pdev->dev.of_node, "qcom,pil-mss-memsetup");

	/* Optional. */
	if (of_property_match_string(pdev->dev.of_node,
			"qcom,active-clock-names", "gpll0_mss_clk") >= 0)
		q6->gpll0_mss_clk = devm_clk_get(&pdev->dev, "gpll0_mss_clk");

	if (of_property_match_string(pdev->dev.of_node,
			"qcom,active-clock-names", "snoc_axi_clk") >= 0)
		q6->snoc_axi_clk = devm_clk_get(&pdev->dev, "snoc_axi_clk");

	if (of_property_match_string(pdev->dev.of_node,
			"qcom,active-clock-names", "mnoc_axi_clk") >= 0)
		q6->mnoc_axi_clk = devm_clk_get(&pdev->dev, "mnoc_axi_clk");

	ret = pil_desc_init(q6_desc);

	return ret;
}

static int pil_mss_driver_probe(struct platform_device *pdev)
{
	struct modem_data *drv;
	int ret, is_not_loadable;

	drv = devm_kzalloc(&pdev->dev, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;
	platform_set_drvdata(pdev, drv);

	is_not_loadable = of_property_read_bool(pdev->dev.of_node,
							"qcom,is-not-loadable");
	if (is_not_loadable) {
		drv->subsys_desc.is_not_loadable = 1;
	} else {
		ret = pil_mss_loadable_init(drv, pdev);
		if (ret)
			return ret;
	}
	init_completion(&drv->stop_ack);

	/* Probe the MBA mem device if present */
	ret = of_platform_populate(pdev->dev.of_node, NULL, NULL, &pdev->dev);
	if (ret)
		return ret;

	return pil_subsys_init(drv, pdev);
}

static int pil_mss_driver_exit(struct platform_device *pdev)
{
	struct modem_data *drv = platform_get_drvdata(pdev);

#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
    if(g_work_data)
    {
         kfree(g_work_data);
    }
#endif
	subsys_unregister(drv->subsys);
	destroy_ramdump_device(drv->ramdump_dev);
	pil_desc_release(&drv->q6->desc);
	return 0;
}

static int pil_mba_mem_driver_probe(struct platform_device *pdev)
{
	struct modem_data *drv;

	if (!pdev->dev.parent) {
		pr_err("No parent found.\n");
		return -EINVAL;
	}
	drv = dev_get_drvdata(pdev->dev.parent);
	drv->mba_mem_dev_fixed = &pdev->dev;
	return 0;
}

static struct of_device_id mba_mem_match_table[] = {
	{ .compatible = "qcom,pil-mba-mem" },
	{}
};

static struct platform_driver pil_mba_mem_driver = {
	.probe = pil_mba_mem_driver_probe,
	.driver = {
		.name = "pil-mba-mem",
		.of_match_table = mba_mem_match_table,
		.owner = THIS_MODULE,
	},
};

static struct of_device_id mss_match_table[] = {
	{ .compatible = "qcom,pil-q6v5-mss" },
	{ .compatible = "qcom,pil-q6v55-mss" },
	{ .compatible = "qcom,pil-q6v56-mss" },
	{}
};

static struct platform_driver pil_mss_driver = {
	.probe = pil_mss_driver_probe,
	.remove = pil_mss_driver_exit,
	.driver = {
		.name = "pil-q6v5-mss",
		.of_match_table = mss_match_table,
		.owner = THIS_MODULE,
	},
};

static int __init pil_mss_init(void)
{
	int ret;

	ret = platform_driver_register(&pil_mba_mem_driver);
	if (!ret)
		ret = platform_driver_register(&pil_mss_driver);
	return ret;
}
module_init(pil_mss_init);

static void __exit pil_mss_exit(void)
{
	platform_driver_unregister(&pil_mss_driver);
}
module_exit(pil_mss_exit);

#ifdef CONFIG_HUAWEI_MODEM_CRASH_LOG
static ssize_t pil_mss_ctl_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	return 0;
}

static ssize_t pil_mss_ctl_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	unsigned char cmd[64];
	int len = -1;

	if (count < 1)
		return 0;

	len = count > 63 ? 63 : count;

	if (copy_from_user(cmd, buf, len))
		return -EFAULT;

	cmd[len] = 0;

	/* lazy */
	if (cmd[len-1] == '\n') {
		cmd[len-1] = 0;
		len--;
	}

	if (!strncmp(cmd, "reset", 5)) {
		pr_err("reset modem subsystem requested by huawei\n");
		subsystem_restart_requested = 1;
		subsystem_restart("modem");
	}

	return count;
}

static int pil_mss_ctl_open(struct inode *ip, struct file *fp)
{
	return 0;
}

static int pil_mss_ctl_release(struct inode *ip, struct file *fp)
{
	return 0;
}

static const struct file_operations pil_mss_ctl_fops = {
	.owner = THIS_MODULE,
	.read = pil_mss_ctl_read,
	.write = pil_mss_ctl_write,
	.open = pil_mss_ctl_open,
	.release = pil_mss_ctl_release,
};

static struct miscdevice pil_mss_ctl_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "pil_mss_ctl",
	.fops = &pil_mss_ctl_fops,
};

static int __init pil_mss_ctl_init(void)
{
	return misc_register(&pil_mss_ctl_dev);
}
module_init(pil_mss_ctl_init);

static void __exit pil_mss_ctl_exit(void)
{
	misc_deregister(&pil_mss_ctl_dev);
}
module_exit(pil_mss_ctl_exit);
#endif

MODULE_DESCRIPTION("Support for booting modem subsystems with QDSP6v5 Hexagon processors");
MODULE_LICENSE("GPL v2");
