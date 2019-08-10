#ifndef SRECORDER_SAHARA_H
#define SRECORDER_SAHARA_H
#ifdef CONFIG_ARM
#include <linux/sched.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
#define SAHARA_BOOT_LOG_ADDR  0x92000000
#define SAHARA_BOOT_LOG_SIZE  0x00050000
#define SAHARA_BOOT_LOG_SIZE_MAX  0x00050000

#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
#define SAHARA_BOOT_SBL_LK_DELAY 20000
#endif

#ifdef CONFIG_MINIDUMP_TRACE_INFO
#define MINIDUMP_IRQ_INFO_SIZE 16
#define MINIDUMP_TASK_INFO_SIZE 40
#endif

struct minidump_dumpid
{
    uint32_t cpuid;
    uint32_t random;
};

struct sahara_boot_log
{
    uint32_t sbl_log_addr;
    uint32_t lk_log_addr;
    uint32_t kernel_log_addr;
    uint32_t sbl_log_size;
    uint32_t lk_log_size;
    uint32_t kernel_log_size;
	struct minidump_dumpid dumpid;
};

#ifdef CONFIG_HUAWEI_KERNEL_DEBUG
struct workqueue_sbl1_lk_log
{
    char *start_addr;
    unsigned int size;
    struct delayed_work  sbl1_lk_log_handle;
};
#endif
void srecorder_save_kernel_log_addr(void);
void minidump_set_dumpid(void);

#ifdef CONFIG_MINIDUMP_TRACE_INFO
/*irq*/
typedef struct
{
    u64 clock;
/*  u64 jiff;*/
    u32 irq;
    u8 dir;
}
minidump_irq_info;

typedef struct
{
    u64 clock;
    u64 stack;
    u32 pid;
    char comm[TASK_COMM_LEN];
}
minidump_task_info;

typedef struct
{
    unsigned char * buffer_addr;
    unsigned char * percpu_addr[NR_CPUS];
    unsigned int    percpu_length;
    unsigned int    buffer_size;
}
minidump_percpu_buffer_info;


typedef struct
{
	u32 max_num;
	u32 field_len;
	u32 rear;
	u32 is_full;
	u8 data[1];
}
minidump_trace_buffer_s;

typedef struct
{
    unsigned long  paddr;
    unsigned long  vaddr;
    unsigned int    size;
}
minidump_trace_dts_mem_info;


typedef enum{
    TR_IRQ = 0,
    TR_TASK,
    TR_MAX
}
minidump_trace_type;
#endif
#ifdef __cplusplus
}
#endif

#endif /* SRECORDER_SAHARA_H*/
