#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/highmem.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/of.h>
#include <asm/uaccess.h>
#include <linux/of_address.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include "srecorder_log.h"
#include "srecorder_sahara.h"

#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
struct workqueue_sbl1_lk_log workqueue_get_log;
#endif

#ifdef CONFIG_MINIDUMP_TRACE_INFO
unsigned int minidump_trace_fidle_len[TR_MAX] = {MINIDUMP_IRQ_INFO_SIZE, MINIDUMP_TASK_INFO_SIZE};
minidump_percpu_buffer_info minidump_trace_buffer_addr[TR_MAX];
minidump_trace_dts_mem_info minidump_trace_mem_info[TR_MAX];
static atomic_t g_qcom_ap_hook_on[TR_MAX] = { ATOMIC_INIT(0) };
static int minidump_ioremap_dts_memory(const char *name,minidump_trace_type mode);
#endif

extern void get_random_bytes(void *buf, int nbytes);

void srecorder_sahara_clear(struct sahara_boot_log *head)
{
	printk(KERN_ERR "srecorderclear:\n");
	printk(KERN_ERR "head->sbl_log_addr:%x\n",head->sbl_log_addr);
	printk(KERN_ERR "head->lk_log_addr:%x\n",head->lk_log_addr);
	printk(KERN_ERR "head->sbl_log_size:%x\n",head->sbl_log_size);
	printk(KERN_ERR "head->lk_log_size:%x\n",head->lk_log_size);
//	head->sbl_log_addr=0;
//	head->lk_log_addr=0;
//	head->sbl_log_size=0;
//	head->lk_log_size=0;
}

#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
void lk_log_write_to_kernel(struct work_struct *work)
{
    struct workqueue_sbl1_lk_log *sbl1_lk_head=NULL;
    char *buf = NULL;
    unsigned int len=0;
    unsigned int i=0;
    char *p=NULL;
    if(work==NULL)
    {
        return;
    }
    sbl1_lk_head=container_of(work,struct workqueue_sbl1_lk_log,sbl1_lk_log_handle);
    buf = sbl1_lk_head->start_addr;
    len=sbl1_lk_head->size;
    p=buf;
    if(p==NULL)
    {
        return;
    }
    if(len>=SAHARA_BOOT_LOG_SIZE_MAX)
    {
        printk(KERN_ERR "Srecorder sahara bBootloader log size error \n");
        return;
    }
    printk(KERN_ERR "Bootloader log start : %lx\n",(long unsigned int)buf);
    for(i=0;i<len;i++)
    {
        if(buf[i]=='\0')
            buf[i]=' ';
        if(buf[i]=='\n')
        {
            buf[i]='\0';
            printk(KERN_ERR "Bootloader log: %s\n",p);
            buf[i]='\n';
            p=&buf[i+1];
        }
    }

    printk(KERN_ERR "srecorder:i:%d   len:%d\n",i,len);
    printk(KERN_ERR "Bootloader log end\n");

}
#endif


/*===========================================================================
**  Function :  minidump_get_kramdom
* @brief
*   This function get real random number in kernel.
* 
* @param[in] 
*   None
*
* @par Dependencies
*   None
* 
* @retval
*   random number
* 
* @par Side Effects
*   None
** ==========================================================================*/
uint32_t minidump_get_kramdom(void)
{
    uint32_t dumpid_ramdom=0;
    printk(KERN_ERR "Minidump get real random number.\n");
    get_random_bytes(&dumpid_ramdom, sizeof(uint32_t));
    printk(KERN_ERR "Minidump get random number: %ld\n", dumpid_ramdom);
    return dumpid_ramdom;
}

/*===========================================================================
**  Function :  minidump_get_cpuid
* @brief
*   This function get cpuid or BSN number.
* 
* @param[in] 
*   None
*
* @par Dependencies
*   None
* 
* @retval
*   cpu id number
* 
* @par Side Effects
*   None
** ==========================================================================*/
uint32_t minidump_get_cpuid(void)
{
    return 0;
}
/*===========================================================================
**  Function :  minidump_set_dumpid
* @brief
*   This function set dumpid to sahara_mem reserve ddr region include cpuid and random number.
* 
* @param[in] 
*   None
*
* @par Dependencies
*   None
* 
* @retval
*   None
* 
* @par Side Effects
*   None
** ==========================================================================*/
void minidump_set_dumpid()
{
    struct sahara_boot_log *boot_log = NULL;
    unsigned long sahara_log_addr = 0;
    unsigned long sahara_log_size = 0;
    void *head_addr = NULL;
    struct device_node *sahara_mem_dts_node = NULL;
    const u32 *sahara_mem_dts_basep = NULL;
    sahara_mem_dts_node = of_find_compatible_node(NULL, NULL, "sahara_mem");
    if (sahara_mem_dts_node == 0)
    {
        SRECORDER_PRINTK("sahara_log_addr Finding compatible node failed.\n");
        return;
    }
    sahara_mem_dts_basep = of_get_address(sahara_mem_dts_node, 0, (u64*)&sahara_log_size, NULL);
    sahara_log_addr = (unsigned long)of_translate_address(sahara_mem_dts_node, sahara_mem_dts_basep);
    SRECORDER_PRINTK("sahara_log_addr:0x%lx sahara_log_size:0x%lx\n",sahara_log_addr,sahara_log_size);

    if (sahara_log_addr == 0 || sahara_log_size < SAHARA_BOOT_LOG_SIZE_MAX)
    {
         SRECORDER_PRINTK("srecorder_sahara get dts addr error \n");
         return;
    }
#ifdef CONFIG_ARM
    head_addr = (void *)ioremap_nocache(sahara_log_addr,sahara_log_size);
#else
    head_addr = (void *)ioremap_wc(sahara_log_addr, sahara_log_size);
#endif
    boot_log=(struct sahara_boot_log *)head_addr;
	
	//get cpuid
    boot_log->dumpid.cpuid = minidump_get_cpuid();
	
	//write random number as magic number
    boot_log->dumpid.random = minidump_get_kramdom();
    return;    
}

void srecorder_save_kernel_log_addr()
{
    struct sahara_boot_log *boot_log = NULL;
    unsigned long sahara_log_addr = 0;
    unsigned long sahara_log_size = 0;
    void *head_addr = NULL;
    struct device_node *sahara_mem_dts_node = NULL;
    const u32 *sahara_mem_dts_basep = NULL;
#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
	struct workqueue_struct *wq = NULL;
	unsigned long delay_time = 0;
#endif

    sahara_mem_dts_node = of_find_compatible_node(NULL, NULL, "sahara_mem");
    if (sahara_mem_dts_node == 0)
    {
        SRECORDER_PRINTK("sahara_log_addr Finding compatible node failed.\n");
        return;
    }

    sahara_mem_dts_basep = of_get_address(sahara_mem_dts_node, 0, (u64*)&sahara_log_size, NULL);
    sahara_log_addr = (unsigned long)of_translate_address(sahara_mem_dts_node, sahara_mem_dts_basep);

    SRECORDER_PRINTK("sahara_log_addr:0x%lx sahara_log_size:0x%lx\n",sahara_log_addr,sahara_log_size);

    if (sahara_log_addr == 0 || sahara_log_size < SAHARA_BOOT_LOG_SIZE_MAX)
    {
         SRECORDER_PRINTK("srecorder_sahara get dts addr error \n");
         return;
    }

#ifdef CONFIG_ARM
    head_addr = (void *)ioremap_nocache(sahara_log_addr,sahara_log_size);
#else
    head_addr = (void *)ioremap_wc(sahara_log_addr, sahara_log_size);
#endif

    boot_log=(struct sahara_boot_log *)head_addr;

#ifdef CONFIG_KALLSYMS
    boot_log->kernel_log_addr = virt_to_phys((void *)kallsyms_lookup_name("__log_buf"));
#else
    boot_log->kernel_log_addr = 0;
#endif

#ifdef CONFIG_LOG_BUF_SHIFT
    boot_log->kernel_log_size = (1 << CONFIG_LOG_BUF_SHIFT);
#else
    boot_log->kernel_log_size = 0;
#endif

#ifdef CONFIG_MINIDUMP_TRACE_INFO
	minidump_ioremap_dts_memory("irqtrace_mem",TR_IRQ );
	minidump_ioremap_dts_memory("tasktrace_mem", TR_TASK);
	SRECORDER_PRINTK("srecorder trace function init done\n");
#else
	SRECORDER_PRINTK("srecorder trace function close\n");
#endif

#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
    workqueue_get_log.start_addr=(char *)(head_addr+sizeof(struct sahara_boot_log));
    workqueue_get_log.size=(unsigned int)(boot_log->lk_log_size+boot_log->sbl_log_size);
    wq=create_workqueue("srecorder_sbl1_lk_log");
    delay_time = msecs_to_jiffies(SAHARA_BOOT_SBL_LK_DELAY);
    INIT_DELAYED_WORK(&(workqueue_get_log.sbl1_lk_log_handle),lk_log_write_to_kernel);
    queue_delayed_work(wq,&(workqueue_get_log.sbl1_lk_log_handle), delay_time);
//	lk_log_write_to_kernel2((char *)(boot_log2+sizeof(struct sahara_boot_log)),(unsigned int)(boot_log2->lk_log_size+boot_log2->sbl_log_size));
#endif
    SRECORDER_PRINTK("srecorder_sahara get dts addr %lx\n",(long unsigned int)head_addr);
    SRECORDER_PRINTK("srecorder_sahara sbl1_log_addr:%x sbl1_log_size:%x \n",boot_log->sbl_log_addr,boot_log->sbl_log_size);
    SRECORDER_PRINTK("srecorder_sahara lk_log_addr:%x lk_log_size:%x \n",boot_log->lk_log_addr,boot_log->lk_log_size);
    SRECORDER_PRINTK("srecorder_sahara kernel_log_addr:%x kernel_log_size:%x \n",boot_log->kernel_log_addr,boot_log->kernel_log_size);

    srecorder_sahara_clear(boot_log);
}

#ifdef CONFIG_MINIDUMP_TRACE_INFO
/**********************irq_trace and task_trace ****************************************/
/*===========================================================================
**  Function :  minidump_trace_percup_buffer_init
* @brief
*   This function init trace percup buffer.
*   trace percpu buffer combin to trace buffer.
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
static int minidump_trace_percup_buffer_init(minidump_trace_buffer_s * q,  unsigned int bytes, unsigned int len )
{
	if(q==NULL)
	{
		return -1;
	}
    if (bytes < (sizeof(minidump_trace_buffer_s) + sizeof(u8) * len))
	{
		return -1;
	}

	/* max_num: records count. */
	q->max_num = (bytes - sizeof(minidump_trace_buffer_s)) /(sizeof(u8) * len);
	q->rear = 0;		/* point to the last NULL record. UNIT is record. */
	q->is_full = 0;
	q->field_len = len;	/* How many u8 in ONE record. */
	return 0;

}

/*===========================================================================
**  Function :  minidump_trace_percup_buffer_write
* @brief
*   This function can write trace percup buffer.
*
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
static void minidump_trace_percup_buffer_write(minidump_trace_buffer_s *q, u8 *element)
{
	int rear;
	if(q==NULL || element==NULL)
	{
		return;
	}
	if (q->rear >= q->max_num)
	{
		q->is_full = 1;
		q->rear = 0;
	}

	rear = q->rear++;
	memcpy((void *)&q->data[(long)rear * q->field_len],(void *)element, q->field_len * sizeof(u8));
	return;
}

/*===========================================================================
**  Function :  minidump_smp_processor_id
* @brief
*   This function is same to smp_processor_id function.
*
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
static u8 minidump_smp_processor_id(void)
{
	return (u8) smp_processor_id();
}
/*===========================================================================
**  Function :  minidump_irq_trace_write
* @brief
*   This function record irq trace.
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
void minidump_irq_trace_write(unsigned int dir, unsigned int new_vec)
{
	minidump_irq_info info;
	u8 cpu;
	/*hook is not installed. */
	if (!atomic_read(&g_qcom_ap_hook_on[TR_IRQ]))
	{
		return;
	}
	cpu = (u8) minidump_smp_processor_id();
	info.clock = jiffies_64;
	/*info.jiff=jiffies_64;*/
	info.dir = (u8) dir;
	info.irq = (u32) new_vec;
	minidump_trace_percup_buffer_write((minidump_trace_buffer_s *)minidump_trace_buffer_addr[TR_IRQ].percpu_addr[cpu],(u8 *)&info);
	return;
}
EXPORT_SYMBOL(minidump_irq_trace_write);
/*===========================================================================
**  Function :  minidump_task_trace_write
* @brief
*   This function record task trace.
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
void minidump_task_trace_write(void *next_task)
{
	struct task_struct *task = (struct task_struct *)next_task;
	minidump_task_info info;
	u8 cpu=0;
	if(next_task==NULL)
	{
		return;
	}
	if (!atomic_read(&g_qcom_ap_hook_on[TR_TASK]))
	{
		return;
	}
	cpu = (u8) minidump_smp_processor_id();
	info.clock = jiffies_64;
	info.pid = (u32) task->pid;
	(void)strncpy(info.comm, task->comm, sizeof(task->comm) - 1);
	info.comm[TASK_COMM_LEN - 1] = '\0';
	info.stack = (u64)task->stack;
	minidump_trace_percup_buffer_write((minidump_trace_buffer_s *)minidump_trace_buffer_addr[TR_TASK].percpu_addr[cpu],(u8 *)&info);
	return;
}
EXPORT_SYMBOL(minidump_task_trace_write);

/*===========================================================================
**  Function :  minidump_get_max_cpu_num
* @brief
*   This function same to num_possible_cpus function.
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
static u32 minidump_get_max_cpu_num(void)
{
	return num_possible_cpus();
}

/*===========================================================================
**  Function :  minidump_trace_dts_buffer_init
* @brief
*   This function init trace_dts_buffer.
*   This function will get minidump_trace_percup_buffer_init
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
static int minidump_trace_dts_buffer_init(minidump_percpu_buffer_info *buffer_info, minidump_trace_type mode)
{
	int i, ret;
	u32 cpu_num = minidump_get_max_cpu_num();
	minidump_percpu_buffer_info *buffer = buffer_info;
	if(buffer_info==NULL)
	{
		return -1;
	}
	if(TR_MAX <= mode )
	{
		SRECORDER_PRINTK("Wrong mode minidump_trace_dts_buffer_init.\n");
		return -1;
	}
	buffer->buffer_addr = (unsigned char *)minidump_trace_mem_info[mode].vaddr;
	buffer->percpu_length = minidump_trace_mem_info[mode].size/cpu_num;
	buffer->buffer_size = minidump_trace_mem_info[mode].size;

	for (i = 0; i < TR_MAX; i++)
	{
		if( TR_IRQ == i)
		{
			minidump_trace_fidle_len[i ] = sizeof(minidump_irq_info);
		}
		if( TR_TASK == i)
		{
			minidump_trace_fidle_len[i ] = sizeof(minidump_task_info);
		}
	}
	for (i = 0; i < (int)cpu_num; i++)
	{
		buffer->percpu_addr[i] = (unsigned char *)(minidump_trace_mem_info[mode].vaddr + (unsigned long)(unsigned)(i*(buffer->percpu_length)));
		ret = minidump_trace_percup_buffer_init((minidump_trace_buffer_s *)buffer->percpu_addr[i],buffer->percpu_length, minidump_trace_fidle_len[mode]);
		if(ret )
		{
			printk(KERN_ERR "minidump_trace_percup_buffer_init, cpu [%d] ringbuffer init failed!\n", i);
			return -1;
		}
	}

	atomic_set(&g_qcom_ap_hook_on[mode], 1);
	return 0;
}

/*===========================================================================
**  Function :  minidump_ioremap_dts_memory
* @brief
*   This function init irq and task trace buffer.
* @param[in]
*   None
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   None
** ==========================================================================*/
static int  minidump_ioremap_dts_memory(const char *name,minidump_trace_type mode)
{
	unsigned long sahara_log_addr = 0;
	unsigned long sahara_log_size = 0;
	struct device_node *sahara_mem_dts_node = NULL;
	const u32 *sahara_mem_dts_basep = NULL;
	if(name==NULL)
	{
		return -1;
	}
	if(TR_MAX <= mode )
	{
		SRECORDER_PRINTK("Wrong mode minidump_ioremap_dts_memory.\n");
		return -1;
	}
    sahara_mem_dts_node = of_find_compatible_node(NULL, NULL, name);
    if (sahara_mem_dts_node == 0)
    {
        SRECORDER_PRINTK("minidump_ioremap_dts_memory failed.\n");
        return -1;
    }

    sahara_mem_dts_basep = of_get_address(sahara_mem_dts_node, 0, (u64*)&sahara_log_size, NULL);
    sahara_log_addr = (unsigned long)of_translate_address(sahara_mem_dts_node, sahara_mem_dts_basep);
    /* get the phsical address and size of the dynamically allocated dts buffer */
    minidump_trace_mem_info[mode].paddr = sahara_log_addr;
    minidump_trace_mem_info[mode].size = sahara_log_size;
    if (minidump_trace_mem_info[mode].paddr  == 0 || minidump_trace_mem_info[mode].size  <= 0)
    {
        SRECORDER_PRINTK("minidump_ioremap_dts_memory:Wrong address or size of the dts buffer.\n");
        minidump_trace_mem_info[mode].paddr = 0;
        minidump_trace_mem_info[mode].size = 0;
        return -1;
    }

    /* ioremap the dynamically allocated dts buffer */
#ifdef CONFIG_ARM
    minidump_trace_mem_info[mode].vaddr  = (unsigned long)ioremap_nocache(minidump_trace_mem_info[mode].paddr, minidump_trace_mem_info[mode].size);
#else
    minidump_trace_mem_info[mode].vaddr  = (unsigned long)ioremap_wc(minidump_trace_mem_info[mode].paddr, minidump_trace_mem_info[mode].size);
#endif
    if (minidump_trace_mem_info[mode].vaddr == 0)
    {
        SRECORDER_PRINTK("minidump_ioremap_dts_memory:Remapping the buffer failed.\n");
        minidump_trace_mem_info[mode].paddr = 0;
        minidump_trace_mem_info[mode].size = 0;
        return -1;
    }
    minidump_trace_dts_buffer_init((minidump_percpu_buffer_info *)&minidump_trace_buffer_addr[mode], mode);
    return 0;
}
#endif
