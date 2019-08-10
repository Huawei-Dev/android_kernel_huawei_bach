#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/stat.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/kprobes.h>
#include <linux/reboot.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>
#include <asm/barrier.h>
#include <linux/platform_device.h>
#include <linux/of_fdt.h>
#include <linux/list.h>
#include <linux/miscdevice.h>
#include <linux/huawei_reset_detect.h>
#include <linux/of_address.h>

#define TRUE 1
#define FALSE 0

#define  TZ_MAGIC_NUM(X) (0x545A4500+X)    /* 'T' 'Z' 'E' 'NUL'+ X */
#define  RESET_MAGIC_TZ_OTHERS    0x545A4F54    /* 'T' 'Z' 'O' 'T' */

#define TZBSP_ERR_FATAL_NONE                         0
#define TZBSP_ERR_FATAL_NON_SECURE_WDT               1
#define TZBSP_ERR_FATAL_SECURE_WDT                   2
#define TZBSP_ERR_FATAL_AHB_TIMEOUT                  3
#define TZBSP_ERR_FATAL_RPM_WDOG                     4
#define TZBSP_ERR_FATAL_RPM_ERR                      5
#define TZBSP_ERR_FATAL_NOC_ERROR                    6
#define TZBSP_ERR_FATAL_BIMC_ERROR                   7
#define TZBSP_ERR_FATAL_SMEM                         8
#define TZBSP_ERR_FATAL_XPU_VIOLATION                9
#define TZBSP_ERR_FATAL_SMMU_FAULT                   10
#define TZBSP_ERR_FATAL_QSEE_ERR                     11

#define TZBSP_ERR_FATAL_EL3_SP_EL0_SYNCH             12
#define TZBSP_ERR_FATAL_EL3_SP_EL0_IRQ               13
#define TZBSP_ERR_FATAL_EL3_SP_EL0_FIQ               14
#define TZBSP_ERR_FATAL_EL3_SP_EL0_ERR               15
#define TZBSP_ERR_FATAL_EL3_SP_EL3_SYNCH             16
#define TZBSP_ERR_FATAL_EL3_SP_EL3_IRQ               17
#define TZBSP_ERR_FATAL_EL3_SP_EL3_FIQ               18
#define TZBSP_ERR_FATAL_EL3_SP_EL3_ERR               19
#define TZBSP_ERR_FATAL_EL3_LEL64_SYNCH              20
#define TZBSP_ERR_FATAL_EL3_LEL64_IRQ                21
#define TZBSP_ERR_FATAL_EL3_LEL64_FIQ                22
#define TZBSP_ERR_FATAL_EL3_LEL64_ERR                23
#define TZBSP_ERR_FATAL_EL3_LEL32_SYNCH              24
#define TZBSP_ERR_FATAL_EL3_LEL32_IRQ                25
#define TZBSP_ERR_FATAL_EL3_LEL32_FIQ                26
#define TZBSP_ERR_FATAL_EL3_LEL32_ERR                27

#define TZBSP_ERR_FATAL_EL1_SP_EL0_SYNCH             28
#define TZBSP_ERR_FATAL_EL1_SP_EL0_IRQ               29
#define TZBSP_ERR_FATAL_EL1_SP_EL0_FIQ               30
#define TZBSP_ERR_FATAL_EL1_SP_EL0_ERR               31
#define TZBSP_ERR_FATAL_EL1_SP_EL1_SYNCH             32
#define TZBSP_ERR_FATAL_EL1_SP_EL1_IRQ               33
#define TZBSP_ERR_FATAL_EL1_SP_EL1_FIQ               34
#define TZBSP_ERR_FATAL_EL1_SP_EL1_ERR               35
#define TZBSP_ERR_FATAL_EL1_LEL64_SYNCH              36
#define TZBSP_ERR_FATAL_EL1_LEL64_IRQ                37
#define TZBSP_ERR_FATAL_EL1_LEL64_FIQ                38
#define TZBSP_ERR_FATAL_EL1_LEL64_ERR                39
#define TZBSP_ERR_FATAL_EL1_LEL32_SYNCH              40
#define TZBSP_ERR_FATAL_EL1_LEL32_IRQ                41
#define TZBSP_ERR_FATAL_EL1_LEL32_FIQ                42
#define TZBSP_ERR_FATAL_EL1_LEL32_ERR                43

#define TZBSP_ERR_FATAL_RPM_DRIVER_ERR               44
#define TZBSP_ERR_FATAL_RESET_TIMER_EXP              45
#define TZBSP_ERR_FATAL_ICE_ERR                      46
#define TZBSP_ERR_FATAL_LMH_DRIVER_ERR               47
#define TZBSP_ERR_FATAL_ACCESS_CONTROL               48
#define TZBSP_ERR_FATAL_CLOCK                        49
#define TZBSP_ERR_FATAL_GIC_CPU_MAP_INVALID          50
#define TZBSP_ERR_FATAL_SEC_WDT_TIMER_TRIGGER        51
#define TZBSP_ERR_FATAL_FAULT_DETECTED               52

#define TZBSP_ERR_FATAL_UNKNOWN                      0xFFFFFFFF

static void *reset_magic_addr = NULL;
static void *tz_reset_reason_addr = NULL;

void raw_writel(unsigned long value,void *p_addr)
{
	pr_info(RESET_DETECT_TAG "raw_writel p_addr=%p value=%08lx\n", p_addr, value);
	if(NULL == p_addr)
	{
		return;
	}
	__raw_writel(value,p_addr);
	return;
}

unsigned long raw_readl(void *p_addr)
{
	unsigned long return_value = 0;
	pr_info(RESET_DETECT_TAG "raw_readl p_addr=%p \n",p_addr);
	if(p_addr == NULL)
		return return_value;

	return_value = __raw_readl(p_addr);
	pr_info(RESET_DETECT_TAG "raw_readl value=%08lx \n",return_value);
	return return_value;
}

void set_reset_magic(int magic_number)
{
	/* no need to check reset_magic_addr here, raw_writel will check */
	raw_writel(magic_number, reset_magic_addr);
	mb();
}

void clear_reset_magic()
{
	/* no need to check reset_magic_addr here, raw_writel will check */
	raw_writel(0, reset_magic_addr);
	mb();
}

static int huawei_apanic_handler(struct notifier_block *this,
				  unsigned long event, void *ptr)
{
	int magic_number_test=0;
	pr_info(RESET_DETECT_TAG "huawei_apanic_handler enters \n");


#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	preempt_count_add(PREEMPT_ACTIVE);
#endif
	magic_number_test= raw_readl(reset_magic_addr);
	if(magic_number_test!= COMBINED_KEY_RESET_REASON_MAGIC_NUM)
	{
		set_reset_magic(RESET_MAGIC_APANIC);
	}
#ifdef CONFIG_PREEMPT
	preempt_count_sub(PREEMPT_ACTIVE);
#endif

	return NOTIFY_DONE;
}

static struct notifier_block huawei_apanic_event_nb = {
	.notifier_call  = huawei_apanic_handler,
	.priority = INT_MAX,
};

static void register_huawei_apanic_notifier(void)
{
	/* regitster the panic & reboot notifier */
	atomic_notifier_chain_register(&panic_notifier_list, &huawei_apanic_event_nb);
}


static int reset_magic_open(struct inode *inode, struct file *file)
{
	pr_debug(RESET_DETECT_TAG "%s enter\n", __func__);
	return 0;
}

static int reset_magic_release(struct inode *inode, struct file *file)
{
	pr_debug(RESET_DETECT_TAG "%s enter\n", __func__);
	return 0;
}


static ssize_t reset_magic_read(struct file *file, char __user *buf, size_t count,
			     loff_t *pos)
{
	unsigned magic_number = 0;

	if(count != RESET_MAGIC_NUM_LEN ){
		printk(RESET_DETECT_TAG "ERROR: %s magic number len must be 4\n", __func__);
		return -EINVAL;
	}

	magic_number = raw_readl(reset_magic_addr);

	pr_info(RESET_DETECT_TAG "%s hardware_reset_magic_number is %04x\n", __func__, magic_number);

	count = sizeof(magic_number);

	if(copy_to_user(buf, &magic_number, count))
	{
		printk(RESET_DETECT_TAG "ERROR: %s copy fail\n", __func__);
		return -EFAULT;
	}

	return count;
}


static ssize_t reset_magic_write(struct file *fp, const char __user *buf,
	      size_t count, loff_t *pos)
{
	unsigned long magic_number = RESET_MAGIC_HW_RESET;

	if(count != RESET_MAGIC_NUM_LEN )
	{
		pr_err(RESET_DETECT_TAG "ERROR: %s magic number len must be 4\n", __func__);
		return -EINVAL;
	}

	if(copy_from_user(&magic_number, buf, RESET_MAGIC_NUM_LEN))
	{
		pr_err(RESET_DETECT_TAG "ERROR: %s copy fail\n", __func__);
		return -EINVAL;
	}

	raw_writel(magic_number, reset_magic_addr);
	pr_info(RESET_DETECT_TAG "%s written hardware reset magic number[%lx] to imem[%lx]\n", __func__, magic_number, (unsigned long)reset_magic_addr);

	return count;
}


static const struct file_operations reset_magic_fops = {
	.owner = THIS_MODULE,
	.open = reset_magic_open,
	.release = reset_magic_release,
	.read = reset_magic_read,
	.write = reset_magic_write,
};

static struct miscdevice reset_magic_miscdev = {
	.minor = RESET_DETECT_MINOR,
	.name = "reset_magic",
	.fops = &reset_magic_fops
};

static int __init reset_magic_address_get(void)
{
	struct device_node *np;
	unsigned long magic_number = RESET_MAGIC_HW_RESET;

	np = of_find_compatible_node(NULL, NULL, "qcom,msm-imem-reset_magic_addr");
	if (!np)
	{
		pr_err(RESET_DETECT_TAG "unable to find DT imem reset_magic_addr node\n");
		return -1;
	}

	reset_magic_addr = of_iomap(np, 0);
	if (!reset_magic_addr)
	{
		pr_err(RESET_DETECT_TAG "unable to map imem reset_magic_addr offset\n");
		return -1;
	}

	pr_info(RESET_DETECT_TAG "reset_magic_addr=%p \n", reset_magic_addr);

	magic_number = raw_readl(reset_magic_addr);
	pr_info("init reset MAGIC NUM 0x%lx\n", magic_number);

	return 0;
}

static int is_tz_reset(unsigned int magic_number) {
	switch (magic_number) {
		case TZBSP_ERR_FATAL_NON_SECURE_WDT:
		case TZBSP_ERR_FATAL_SECURE_WDT:
		case TZBSP_ERR_FATAL_AHB_TIMEOUT:
		case TZBSP_ERR_FATAL_RPM_WDOG:
		case TZBSP_ERR_FATAL_RPM_ERR:
		case TZBSP_ERR_FATAL_NOC_ERROR:
		case TZBSP_ERR_FATAL_BIMC_ERROR:
		case TZBSP_ERR_FATAL_SMEM:
		case TZBSP_ERR_FATAL_XPU_VIOLATION:
		case TZBSP_ERR_FATAL_SMMU_FAULT:
		case TZBSP_ERR_FATAL_QSEE_ERR:
		case TZBSP_ERR_FATAL_EL3_SP_EL0_SYNCH:
		case TZBSP_ERR_FATAL_EL3_SP_EL0_IRQ:
		case TZBSP_ERR_FATAL_EL3_SP_EL0_FIQ:
		case TZBSP_ERR_FATAL_EL3_SP_EL0_ERR:
		case TZBSP_ERR_FATAL_EL3_SP_EL3_SYNCH:
		case TZBSP_ERR_FATAL_EL3_SP_EL3_IRQ:
		case TZBSP_ERR_FATAL_EL3_SP_EL3_FIQ:
		case TZBSP_ERR_FATAL_EL3_SP_EL3_ERR:
		case TZBSP_ERR_FATAL_EL3_LEL64_SYNCH:
		case TZBSP_ERR_FATAL_EL3_LEL64_IRQ:
		case TZBSP_ERR_FATAL_EL3_LEL64_FIQ:
		case TZBSP_ERR_FATAL_EL3_LEL64_ERR:
		case TZBSP_ERR_FATAL_EL3_LEL32_SYNCH:
		case TZBSP_ERR_FATAL_EL3_LEL32_IRQ:
		case TZBSP_ERR_FATAL_EL3_LEL32_FIQ:
		case TZBSP_ERR_FATAL_EL3_LEL32_ERR:
		case TZBSP_ERR_FATAL_EL1_SP_EL0_SYNCH:
		case TZBSP_ERR_FATAL_EL1_SP_EL0_IRQ:
		case TZBSP_ERR_FATAL_EL1_SP_EL0_FIQ:
		case TZBSP_ERR_FATAL_EL1_SP_EL0_ERR:
		case TZBSP_ERR_FATAL_EL1_SP_EL1_SYNCH:
		case TZBSP_ERR_FATAL_EL1_SP_EL1_IRQ:
		case TZBSP_ERR_FATAL_EL1_SP_EL1_FIQ:
		case TZBSP_ERR_FATAL_EL1_SP_EL1_ERR:
		case TZBSP_ERR_FATAL_EL1_LEL64_SYNCH:
		case TZBSP_ERR_FATAL_EL1_LEL64_IRQ:
		case TZBSP_ERR_FATAL_EL1_LEL64_FIQ:
		case TZBSP_ERR_FATAL_EL1_LEL64_ERR:
		case TZBSP_ERR_FATAL_EL1_LEL32_SYNCH:
		case TZBSP_ERR_FATAL_EL1_LEL32_IRQ:
		case TZBSP_ERR_FATAL_EL1_LEL32_FIQ:
		case TZBSP_ERR_FATAL_EL1_LEL32_ERR:
		case TZBSP_ERR_FATAL_RPM_DRIVER_ERR:
		case TZBSP_ERR_FATAL_RESET_TIMER_EXP:
		case TZBSP_ERR_FATAL_ICE_ERR:
		case TZBSP_ERR_FATAL_LMH_DRIVER_ERR:
		case TZBSP_ERR_FATAL_ACCESS_CONTROL:
		case TZBSP_ERR_FATAL_CLOCK:
		case TZBSP_ERR_FATAL_GIC_CPU_MAP_INVALID:
		case TZBSP_ERR_FATAL_SEC_WDT_TIMER_TRIGGER:
		case TZBSP_ERR_FATAL_FAULT_DETECTED:
		case TZBSP_ERR_FATAL_UNKNOWN:
			return TRUE;

		default:
			return FALSE;
	}
}

static int is_HLOS_reset(unsigned int magic_number) {
	switch (magic_number) {
		case RESET_MAGIC_APANIC:
		case RESET_MAGIC_WDT_BARK:
		case RESET_MAGIC_THERMAL:
		case RESET_MAGIC_VMREBOOT:
			return TRUE;
		default:
			return FALSE;
	};

	return FALSE;
}

static int set_tz_reset_magic_number(void)
{
	unsigned long magic_number = RESET_MAGIC_HW_RESET;
	unsigned long tz_magic_number = RESET_MAGIC_HW_RESET;
	unsigned long tz_reset_reason = RESET_MAGIC_TZ_OTHERS;

	magic_number = raw_readl(reset_magic_addr);
	tz_reset_reason = raw_readl(tz_reset_reason_addr);

	/*set magic number for tz reset*/
	if (!is_HLOS_reset(magic_number)
			&& (is_tz_reset(tz_reset_reason) ))
	{
		if (tz_reset_reason == TZBSP_ERR_FATAL_UNKNOWN)
		{
			tz_magic_number = RESET_MAGIC_TZ_OTHERS;
		}
		else
		{
			tz_magic_number = TZ_MAGIC_NUM(tz_reset_reason);
		}

		raw_writel(tz_magic_number, reset_magic_addr);

		pr_info(RESET_DETECT_TAG "%s written tz reset"
			" tz_magic number [%lx] to imem[%lx]\n", __func__,
			tz_magic_number, (unsigned long)reset_magic_addr);
	}
	else if (RESET_MAGIC_HW_RESET == magic_number)
	{
		/*If its not an HLOS reboot and TZ reboot set, it is a HARD
		 * reboot*/
		pr_info(RESET_DETECT_TAG "%s: its a HARD RESET",
				__func__);
	}
	else
	{
		pr_info(RESET_DETECT_TAG "%s: its an HLOS triggered reset",
				__func__);
	}

	return 0;
}

static int tz_reset_reason_address_get(void)
{
	struct device_node *np;
	unsigned long tz_reset_reason = TZBSP_ERR_FATAL_UNKNOWN;

	np = of_find_compatible_node(NULL, NULL,
			"qcom,msm-imem-tz-reset-reason-addr");
	if (!np)
	{
		pr_err(RESET_DETECT_TAG "unable to find DT imem"
				" tz_reset_reason_addr node\n");
		return -1;
	}

	tz_reset_reason_addr = of_iomap(np, 0);

	if (!tz_reset_reason_addr)
	{
		pr_err(RESET_DETECT_TAG "unable to map imem"
				" tz_reset_reason_addr offset\n");
		return -1;
	}

	pr_info(RESET_DETECT_TAG "tz_reset_reason_addr=%p \n",
			tz_reset_reason_addr);
	tz_reset_reason = raw_readl(tz_reset_reason_addr);
	pr_info("TZ reset reason stored in IMEM: %lx\n", tz_reset_reason);

	return 0;
}

static int __init huawei_reset_detect_init(void)
{
	int ret = 0;

	/* get reset magic address for dt and iomap it */
	ret = reset_magic_address_get();
	if (ret)
	{
		return -1;
	}

	/* get tz reset reason address from dt and iomap it */
	ret = tz_reset_reason_address_get();
	if (ret)
	{
		return -1;
	}

	/* set the tz reset magic number into IMEM address */
	ret = set_tz_reset_magic_number();
	if (ret)
	{
		return 1;
	}

	/* regitster the panic & reboot notifier */
	register_huawei_apanic_notifier();

	ret = misc_register(&reset_magic_miscdev);
	return ret;
}

static void __exit huawei_reset_detect_exit(void)
{
	misc_deregister(&reset_magic_miscdev);
}

module_init(huawei_reset_detect_init);
module_exit(huawei_reset_detect_exit);
MODULE_LICENSE(GPL);
