#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/module.h>
#include <asm/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gfp.h>
#include <asm/io.h>
#include <asm/memory.h>
//#include <arm/outercache.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include "slt.h"

#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#define  SLTTEST_NO_ERROR               0
#define  SLTTEST_SOC_ERROR              1
#define  SLTTEST_OTHER_ERROR            2
#define  SLTTEST_CPURUNNING_ERROR       -1

extern long fp3_memcpyL2_start(long cpu_id);

//the number of correct  loops
static int g_iCPU0_PassFail, g_iCPU1_PassFail,g_iCPU2_PassFail, g_iCPU3_PassFail;
static int g_iCPU4_PassFail, g_iCPU5_PassFail,g_iCPU6_PassFail, g_iCPU7_PassFail;
//save the Register value "CPUMERRSR"
static long g_iCPU0_CpuError, g_iCPU1_CpuError,g_iCPU2_CpuError, g_iCPU3_CpuError;
static long g_iCPU4_CpuError, g_iCPU5_CpuError,g_iCPU6_CpuError, g_iCPU7_CpuError;
//save the Register value "L2MERRSR"
static long g_iCPU0_L2Error, g_iCPU1_L2Error,g_iCPU2_L2Error, g_iCPU3_L2Error;
static long g_iCPU4_L2Error, g_iCPU5_L2Error,g_iCPU6_L2Error, g_iCPU7_L2Error;
// save the number of  "soc error"  loop
static int g_iCPU0_SocError, g_iCPU1_SocError,g_iCPU2_SocError, g_iCPU3_SocError;
static int g_iCPU4_SocError, g_iCPU5_SocError,g_iCPU6_SocError, g_iCPU7_SocError;
// save the number of  "other error"  loop
static int g_iCPU0_OtherError, g_iCPU1_OtherError,g_iCPU2_OtherError, g_iCPU3_OtherError;
static int g_iCPU4_OtherError, g_iCPU5_OtherError,g_iCPU6_OtherError, g_iCPU7_OtherError;
//the value of loops counter (get from "slt_memcpyL2_loop_count" ) 
static int g_iMemcpyL2LoopCount;  
//every core has its own memory space
unsigned long g_iTestMem_CPU0, g_iTestMem_CPU1,g_iTestMem_CPU2, g_iTestMem_CPU3;
unsigned long g_iTestMem_CPU4, g_iTestMem_CPU5,g_iTestMem_CPU6, g_iTestMem_CPU7;

//the program state : 1-running ; 0-stop
static int g_iCPU0_Running = 0;
static int g_iCPU1_Running = 0;
static int g_iCPU2_Running = 0;
static int g_iCPU3_Running = 0;
static int g_iCPU4_Running = 0;
static int g_iCPU5_Running = 0;
static int g_iCPU6_Running = 0;
static int g_iCPU7_Running = 0;

/*
static struct wake_lock slt_cpu0_lock;
static struct wake_lock slt_cpu1_lock;
static struct wake_lock slt_cpu2_lock;
static struct wake_lock slt_cpu3_lock;
static struct wake_lock slt_cpu4_lock;
static struct wake_lock slt_cpu5_lock;
static struct wake_lock slt_cpu6_lock;
static struct wake_lock slt_cpu7_lock;
*/
static struct device_driver slt_cpu0_memcpyL2_drv =
{
    .name = "slt_cpu0_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu1_memcpyL2_drv =
{
    .name = "slt_cpu1_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu2_memcpyL2_drv =
{
    .name = "slt_cpu2_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu3_memcpyL2_drv =
{
    .name = "slt_cpu3_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu4_memcpyL2_drv =
{
    .name = "slt_cpu4_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu5_memcpyL2_drv =
{
    .name = "slt_cpu5_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu6_memcpyL2_drv =
{
    .name = "slt_cpu6_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu7_memcpyL2_drv =
{
    .name = "slt_cpu7_memcpyL2",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_memcpyL2_loop_count_drv =
{
    .name = "slt_memcpyL2_loop_count",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};
//  "core working error" ; "running" ; "result info"
#define DEFINE_SLT_CPU_MEMCPYL2_SHOW(_N)                                                  \
static ssize_t slt_cpu##_N##_memcpyL2_show(struct device_driver *driver, char *buf)       \
{                                                                                         \
    if(g_iCPU##_N##_PassFail == -1)                                                       \
    {                                                                                     \
        return snprintf(buf, PAGE_SIZE, "CPU%d MemcpyL2 - PowerOff %d %ld %ld %d %d\n",   \
            _N,g_iCPU##_N##_PassFail,g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error,          \
            g_iCPU##_N##_SocError,g_iCPU##_N##_OtherError);                               \
    }                                                                                     \
    else if (g_iCPU##_N##_Running)                                                        \
    {                                                                                     \
        return snprintf(buf, PAGE_SIZE, "CPU%d MemcpyL2 - Running %d %ld %ld %d %d\n", _N,\
            g_iCPU##_N##_PassFail,g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error,             \
		    g_iCPU##_N##_SocError,g_iCPU##_N##_OtherError);                               \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
        return snprintf(buf, PAGE_SIZE, "CPU%d MemcpyL2 - %s %d %ld %ld %d %d\n", _N,     \
		    g_iCPU##_N##_PassFail != g_iMemcpyL2LoopCount ? "FAIL" : "PASS",              \
		    g_iCPU##_N##_PassFail,g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error,             \
		    g_iCPU##_N##_SocError,g_iCPU##_N##_OtherError);                               \
    }                                                                                     \
}

DEFINE_SLT_CPU_MEMCPYL2_SHOW(0)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(1)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(2)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(3)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(4)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(5)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(6)
DEFINE_SLT_CPU_MEMCPYL2_SHOW(7)

static ssize_t slt_memcpyL2_loop_count_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "MemcpyL2 Loop Count = %d\n", g_iMemcpyL2LoopCount);
}

#define DEFINE_SLT_CPU_MEMCPYL2_STORE(_N)                                                                   \
static ssize_t slt_cpu##_N##_memcpyL2_store(struct device_driver *driver, const char *buf, size_t count)    \
{                                                                                   \
    unsigned int i;                                                                 \
    unsigned int l1_linesize,l1_waynum,l1_setnum,l1cache_size;  				    \
    /*char templockname[50]; */                                                     \
    long ret,val_hex,val_hex1;                                                      \
    unsigned long mask;                                                             \
    int retry=0;                                                                    \
    unsigned long pTestMem;                                                         \
    DEFINE_SPINLOCK(cpu##_N##_lock);                                                \
    unsigned long cpu##_N##_flags;                                                  \
\
    struct timex  txc;                                                              \
    struct rtc_time tm;                                                             \
                                                                                    \
    g_iCPU##_N##_PassFail = 0;                                                      \
    g_iCPU##_N##_Running = 0;                                                       \
                                                                                    \
                                                                                    \
    mask = (1 << _N); /* processor _N */                                            \
    while(sched_setaffinity(0, (struct cpumask*) &mask) < 0)                       \
    {                                                                               \
        printk("Could not set cpu%d affinity for current process(%d).\n", _N, retry);\
        g_iCPU##_N##_PassFail = -1;                                                 \
        retry++;                                                                    \
        if(retry > 100)                                                             \
        {                                                                           \
            vfree((void*)pTestMem);                                                 \
            return count;                                                           \
        }                                                                           \
    }                                                                               \
                                                                                    \
    /*get the L2 cache size */                                                      \
    __asm__ __volatile__(                                                 		    \
       "MOV    x0, #0 \n"                                                     	    \
       "MSR    csselr_el1,x0 \n"                                           		    \
	   "MRS %[val_hex], CCSIDR_EL1\n"                            				    \
       :[val_hex]"=&r"(val_hex)                                         		    \
       ::                                                            				\
     );                                                                  			\
    /*bit[2:0](Log2(Number of bytes in cache line)) - 4.*/ 							\
    l1_linesize = (val_hex & 0x07) + 4;               								\
    l1_waynum = ((val_hex >> 3) & 0x3F) + 1;                            			\
    l1_setnum = ((val_hex >> 13) & 0x7FFF) + 1;                          			\
    l1cache_size = ((l1_waynum * l1_setnum) << l1_linesize);      					\
    printk(KERN_ERR "CPU%dL1 size:%d[set= %d][way = %d][line = %d][R= 0x%lx]\n", 	\
    _N,l1cache_size,l1_setnum,l1_waynum,l1_linesize,val_hex); 						\
                                                                                    \
    pTestMem = (unsigned long )vmalloc(l1cache_size);                               \
    /*pTestMem = (pTestMem & 0xffffff8000000000); */                                \
    if((void*)pTestMem == NULL)                                                     \
    {                                                                               \
        printk("allocate memory for cpu%d speed test fail!\n", _N);                 \
        return 0;                                                                   \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        g_iTestMem_CPU##_N = pTestMem;                                              \
    }                                                                               \
    /*memset(templockname,0,sizeof(templockname)); */                               \
   /* sprintf(templockname,"slt_%d_lock",_N); */                                    \
   /*wake_lock_init(&slt_cpu##_N##_lock, WAKE_LOCK_SUSPEND, templockname); */       \
   /*wake_lock(&slt_cpu##_N##_lock);	 */     	                                \
\
    do_gettimeofday(&(txc.time));                                                   \
    rtc_time_to_tm(txc.time.tv_sec,&tm);                                            \
    printk("[UTC time :%d-%d-%d %d:%d:%d]",tm.tm_year+1900,tm.tm_mon+1,             \
        tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);                                 \
    printk(">> CPU%d MemcpyL2 test start (cpu id = %d) <<\n", _N, raw_smp_processor_id());\
	                                                                                \
    g_iCPU##_N##_Running = 1;                                                       \
    g_iCPU##_N##_CpuError = 0;                                                      \
	g_iCPU##_N##_L2Error = 0;                                                       \
    g_iCPU##_N##_OtherError = 0;                                                    \
    g_iCPU##_N##_SocError = 0;                                         	            \
    /*disable irq before write eccsr register */                                    \
	spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                            \
	                                                                                \
	__asm__ __volatile__(                                                           \
        "MOV x11, #0\n"                                                             \
        "MSR S3_1_c15_c2_2, x11\n"                                                  \
        "MSR S3_1_c15_c2_3, x11\n"                                                  \
        "MRS %[val_hex], S3_1_c15_c2_2\n"                                           \
        "MRS %[val_hex1], S3_1_c15_c2_3\n"                                          \
        :[val_hex]"=&r"(val_hex),[val_hex1]"=&r"(val_hex1)                          \
        ::                                                                          \
    );                                                                              \
    printk(KERN_ERR "CPU%dL2 Init:cpu==0x%lx,l2==0x%lx\n",_N, val_hex,val_hex1);    \
	spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                       \
    /*enable irq after write eccsr register */                                      \
                                                                                    \
    for (i = 0, g_iCPU##_N##_PassFail = 0; i < g_iMemcpyL2LoopCount; i++)           \
	{                                                                               \
        spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                        \
                                                                                    \
        ret = fp3_memcpyL2_start(_N);                                               \
                                                                                    \
        spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                   \
        if(SLTTEST_NO_ERROR == ret)                                                 \
        {                                                                           \
            g_iCPU##_N##_PassFail++;                                                \
        }                                                                           \
        else if (SLTTEST_SOC_ERROR == ret)                                          \
	    {                                                                           \
	        g_iCPU##_N##_SocError ++;                                               \
	    }                                                                           \
        else if (SLTTEST_OTHER_ERROR == ret)                                        \
	    {                                                                           \
	        g_iCPU##_N##_OtherError ++;                                             \
	    }                                                                           \
        else 		                                                                \
        {                                                                           \
            g_iCPU##_N##_PassFail = -1;                                             \
            break;                                                                  \
        }                                                                           \
    }                                                                               \
		                                                                            \
    spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                            \
    /*get ecc error counter */                                                      \
    __asm__ __volatile__(                                                           \
		"MRS %[val_hex], S3_1_c15_c2_2\n"                                           \
		"MRS %[val_hex1], S3_1_c15_c2_3\n"                                          \
        :[val_hex]"=&r"(val_hex),[val_hex1]"=&r"(val_hex1)                          \
        ::                                                                          \
    );                                                                              \
    g_iCPU##_N##_CpuError = val_hex;                                                \
    g_iCPU##_N##_L2Error = val_hex1;                                                \
	printk(KERN_ERR "CPU%dL2 counter:l1==0x%lx,l2==0x%lx\n", _N,                    \
	    g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error);                                \
                                                                                    \
    spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                       \
    /*wake_unlock(&slt_cpu##_N##_lock);*/	                                        \
    /*wake_lock_destroy(&slt_cpu##_N##_lock);	*/                                  \
	g_iCPU##_N##_Running = 0;                                                       \
	                                                                                \
\
	do_gettimeofday(&(txc.time));                                                   \
    rtc_time_to_tm(txc.time.tv_sec,&tm);                                            \
    printk("[UTC time :%d-%d-%d %d:%d:%d]",tm.tm_year+1900,                         \
        tm.tm_mon+1, tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);                    \
	                                                                                \
    if (g_iCPU##_N##_PassFail == g_iMemcpyL2LoopCount)                              \
    {                                                                               \
        printk(">> CPU%d memcpyL2 test pass (loop count = %d/%d)<<\n", _N,          \
            g_iCPU##_N##_PassFail,g_iMemcpyL2LoopCount);                            \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        printk(">> CPU%d memcpyL2 test fail[%ld] (loop count = %d/%d)<<\n", _N,     \
            ret, g_iCPU##_N##_PassFail,g_iMemcpyL2LoopCount);                       \
    }                                                                               \
                                                                                    \
    vfree((void*)pTestMem);                                                         \
    return count;                                                                   \
}

DEFINE_SLT_CPU_MEMCPYL2_STORE(0)
DEFINE_SLT_CPU_MEMCPYL2_STORE(1)
DEFINE_SLT_CPU_MEMCPYL2_STORE(2)
DEFINE_SLT_CPU_MEMCPYL2_STORE(3)
DEFINE_SLT_CPU_MEMCPYL2_STORE(4)
DEFINE_SLT_CPU_MEMCPYL2_STORE(5)
DEFINE_SLT_CPU_MEMCPYL2_STORE(6)
DEFINE_SLT_CPU_MEMCPYL2_STORE(7)
static ssize_t slt_memcpyL2_loop_count_store(struct device_driver *driver, const char *buf, size_t count)
{
    int result;

    if ((result = sscanf(buf, "%d", &g_iMemcpyL2LoopCount)) == 1)
    {
        printk("set SLT MemcpyL2 test loop count = %d successfully\n", g_iMemcpyL2LoopCount);
    }
    else
    {
        printk("bad argument!!\n");
        return -EINVAL;
    }

    return count;
}

DRIVER_ATTR(slt_cpu0_memcpyL2, 0644, slt_cpu0_memcpyL2_show, slt_cpu0_memcpyL2_store);
DRIVER_ATTR(slt_cpu1_memcpyL2, 0644, slt_cpu1_memcpyL2_show, slt_cpu1_memcpyL2_store);
DRIVER_ATTR(slt_cpu2_memcpyL2, 0644, slt_cpu2_memcpyL2_show, slt_cpu2_memcpyL2_store);
DRIVER_ATTR(slt_cpu3_memcpyL2, 0644, slt_cpu3_memcpyL2_show, slt_cpu3_memcpyL2_store);
DRIVER_ATTR(slt_cpu4_memcpyL2, 0644, slt_cpu4_memcpyL2_show, slt_cpu4_memcpyL2_store);
DRIVER_ATTR(slt_cpu5_memcpyL2, 0644, slt_cpu5_memcpyL2_show, slt_cpu5_memcpyL2_store);
DRIVER_ATTR(slt_cpu6_memcpyL2, 0644, slt_cpu6_memcpyL2_show, slt_cpu6_memcpyL2_store);
DRIVER_ATTR(slt_cpu7_memcpyL2, 0644, slt_cpu7_memcpyL2_show, slt_cpu7_memcpyL2_store);
DRIVER_ATTR(slt_memcpyL2_loop_count, 0644, slt_memcpyL2_loop_count_show, slt_memcpyL2_loop_count_store);

#define DEFINE_SLT_CPU_MEMCPYL2_INIT(_N)                                \
int __init slt_cpu##_N##_memcpyL2_init(void)                            \
{                                                                       \
    int ret;                                                            \
                                                                        \
    ret = driver_register(&slt_cpu##_N##_memcpyL2_drv);                 \
    if (ret)                                                            \
    {                                                                   \
        printk("fail to create SLT CPU%d MemcpyL2 driver\n", _N);       \
    }                                                                   \
    else                                                                \
    {                                                                   \
        printk("success to create SLT CPU%d MemcpyL2 driver\n", _N);    \
    }                                                                   \
                                                                        \
    ret = driver_create_file(&slt_cpu##_N##_memcpyL2_drv, &driver_attr_slt_cpu##_N##_memcpyL2);   \
    if (ret)                                                            \
    {                                                                   \
        printk("fail to create SLT CPU%d MemcpyL2 sysfs files\n", _N);  \
    }                                                                   \
    else                                                                \
    {                                                                   \
        printk("success to create SLT CPU%d MemcpyL2 sysfs files\n", _N);\
    }                                                                   \
                                                                        \
    return 0;                                                           \
}

DEFINE_SLT_CPU_MEMCPYL2_INIT(0)
DEFINE_SLT_CPU_MEMCPYL2_INIT(1)
DEFINE_SLT_CPU_MEMCPYL2_INIT(2)
DEFINE_SLT_CPU_MEMCPYL2_INIT(3)
DEFINE_SLT_CPU_MEMCPYL2_INIT(4)
DEFINE_SLT_CPU_MEMCPYL2_INIT(5)
DEFINE_SLT_CPU_MEMCPYL2_INIT(6)
DEFINE_SLT_CPU_MEMCPYL2_INIT(7)
int __init slt_memcpyL2_loop_count_init(void)
{
    int ret;

    ret = driver_register(&slt_memcpyL2_loop_count_drv);
    if (ret) {
        printk("fail to create MemcpyL2 loop count driver\n");
    }
    else
    {
        printk("success to create MemcpyL2 loop count driver\n");
    }


    ret = driver_create_file(&slt_memcpyL2_loop_count_drv, &driver_attr_slt_memcpyL2_loop_count);
    if (ret) {
        printk("fail to create MemcpyL2 loop count sysfs files\n");
    }
    else
    {
        printk("success to create MemcpyL2 loop count sysfs files\n");
    }

    g_iMemcpyL2LoopCount = SLT_LOOP_CNT;

    return 0;
}

arch_initcall(slt_cpu0_memcpyL2_init);
arch_initcall(slt_cpu1_memcpyL2_init);
arch_initcall(slt_cpu2_memcpyL2_init);
arch_initcall(slt_cpu3_memcpyL2_init);
arch_initcall(slt_cpu4_memcpyL2_init);
arch_initcall(slt_cpu5_memcpyL2_init);
arch_initcall(slt_cpu6_memcpyL2_init);
arch_initcall(slt_cpu7_memcpyL2_init);
arch_initcall(slt_memcpyL2_loop_count_init);
