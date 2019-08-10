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
//#include <asm/outercache.h>
#include <linux/spinlock.h>

#include <linux/sched.h>
#include <linux/vmalloc.h>
#include "slt.h"
#include <linux/io.h>
#include <asm/pgtable.h>

#define  SLTTEST_NO_ERROR               0
#define  SLTTEST_SOC_ERROR              1
#define  SLTTEST_OTHER_ERROR            2
#define  SLTTEST_CPURUNNING_ERROR       -1

extern long fp0_maxpower_start(unsigned long cpu_id);//

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

static int g_iMaxPowerLoopCount;

//running flag of each task.1-running;0-not run
static int g_iCPU0_Running = 0;
static int g_iCPU1_Running = 0;
static int g_iCPU2_Running = 0;
static int g_iCPU3_Running = 0;

static int g_iCPU4_Running = 0;
static int g_iCPU5_Running = 0;
static int g_iCPU6_Running = 0;
static int g_iCPU7_Running = 0;

static struct device_driver slt_cpu0_maxpower_drv =
{
    .name = "slt_cpu0_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu1_maxpower_drv =
{
    .name = "slt_cpu1_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu2_maxpower_drv =
{
    .name = "slt_cpu2_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu3_maxpower_drv =
{
    .name = "slt_cpu3_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};
static struct device_driver slt_cpu4_maxpower_drv =
{
    .name = "slt_cpu4_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu5_maxpower_drv =
{
    .name = "slt_cpu5_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu6_maxpower_drv =
{
    .name = "slt_cpu6_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_cpu7_maxpower_drv =
{
    .name = "slt_cpu7_maxpower",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

static struct device_driver slt_maxpower_loop_count_drv =
{
    .name = "slt_maxpower_loop_count",
    .bus = &platform_bus_type,
    .owner = THIS_MODULE,
};

#define DEFINE_SLT_CPU_MAXPOWER_SHOW(_N)    \
static ssize_t slt_cpu##_N##_maxpower_show(struct device_driver *driver, char *buf) \
{   \
    if(g_iCPU##_N##_PassFail == -1)                                                       \
    {                                                                                     \
        return snprintf(buf, PAGE_SIZE, "CPU%d maxpower - PowerOff %d %ld %ld %d %d\n",   \
            _N,g_iCPU##_N##_PassFail,g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error,          \
            g_iCPU##_N##_SocError,g_iCPU##_N##_OtherError);                               \
    }                                                                                     \
    else if (g_iCPU##_N##_Running)                                                        \
    {                                                                                     \
        return snprintf(buf, PAGE_SIZE, "CPU%d maxpower - Running %d %ld %ld %d %d\n", _N,\
            g_iCPU##_N##_PassFail,g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error,             \
		    g_iCPU##_N##_SocError,g_iCPU##_N##_OtherError);                               \
    }                                                                                     \
    else                                                                                  \
    {                                                                                     \
        return snprintf(buf, PAGE_SIZE, "CPU%d maxpower - %s %d %ld %ld %d %d\n", _N,     \
		    g_iCPU##_N##_PassFail != g_iMaxPowerLoopCount ? "FAIL" : "PASS",              \
		    g_iCPU##_N##_PassFail,g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error,             \
		    g_iCPU##_N##_SocError,g_iCPU##_N##_OtherError);                               \
    }                                                                                     \
}

DEFINE_SLT_CPU_MAXPOWER_SHOW(0)
DEFINE_SLT_CPU_MAXPOWER_SHOW(1)
DEFINE_SLT_CPU_MAXPOWER_SHOW(2)
DEFINE_SLT_CPU_MAXPOWER_SHOW(3)

DEFINE_SLT_CPU_MAXPOWER_SHOW(4)
DEFINE_SLT_CPU_MAXPOWER_SHOW(5)
DEFINE_SLT_CPU_MAXPOWER_SHOW(6)
DEFINE_SLT_CPU_MAXPOWER_SHOW(7)

static ssize_t slt_maxpower_loop_count_show(struct device_driver *driver, char *buf)
{
    return snprintf(buf, PAGE_SIZE, "MaxPower Loop Count = %d\n", g_iMaxPowerLoopCount);
}

#define DEFINE_SLT_CPU_MAXPOWER_STORE(_N)                                                                   \
static ssize_t slt_cpu##_N##_maxpower_store(struct device_driver *driver, const char *buf, size_t count)    \
{                                                                                       \
    unsigned int i;                                                                     \
	long ret;                                                                           \
    unsigned long mask;                                                                 \
    int retry=0;                                                                        \
    DEFINE_SPINLOCK(cpu##_N##_lock);                                                    \
    unsigned long cpu##_N##_flags;                                                      \
    long val_hex,val_hex1;                                                          \
                                                                                        \
    g_iCPU##_N##_PassFail = 0;                                                          \
	g_iCPU##_N##_Running = 0;                                                           \
                                                                                        \
    mask = (1 << _N); /* processor _N */                                                \
    while(sched_setaffinity(0, (struct cpumask*) &mask) < 0)                           \
    {                                                                                   \
        printk(KERN_ERR "Could not set cpu%d affinity for current process(%d)!\n", _N, retry);   \
        g_iCPU##_N##_PassFail = -1;                                                     \
        retry++;                                                                        \
        if(retry > 100)                                                                 \
        {                                                                               \
            return count;                                                               \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    printk("\n>> CPU%d maxpower test start (cpu id = %d) <<\n\n", _N,                   \
        raw_smp_processor_id());                                                        \
    g_iCPU##_N##_Running = 1;                                                           \
    g_iCPU##_N##_CpuError = 0;                                                          \
	g_iCPU##_N##_L2Error = 0;                                                           \
    g_iCPU##_N##_OtherError = 0;                                                        \
    g_iCPU##_N##_SocError = 0;                                         	                \
    /*disable irq before write eccsr register */                                        \
	spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                                \
	                                                                                    \
	__asm__ __volatile__(                                                               \
        "MOV x11, #0\n"                                                                 \
        "MSR S3_1_c15_c2_2, x11\n"                                                      \
        "MSR S3_1_c15_c2_3, x11\n"                                                      \
        "MRS %[val_hex], S3_1_c15_c2_2\n"                                               \
        "MRS %[val_hex1], S3_1_c15_c2_3\n"                                              \
        :[val_hex]"=&r"(val_hex),[val_hex1]"=&r"(val_hex1)                              \
        ::                                                                              \
    );                                                                                  \
    printk(KERN_ERR "CPU%dL2 Init:cpu==0x%lx,l2==0x%lx\n",_N, val_hex,val_hex1);        \
	spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                           \
                                                                                        \
    for (i = 0, g_iCPU##_N##_PassFail = 0; i < g_iMaxPowerLoopCount; i++)               \
    {                                                                                   \
        spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                            \
        ret = fp0_maxpower_start(_N);    /* 1: PASS, 0:Fail, -1: target CPU power off */\
        /* printk("\n>> CPU%d ret=%ld <<\n\n", _N, ret); */                             \
        spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                       \
        if(SLTTEST_NO_ERROR == ret)                                                     \
        {                                                                               \
            g_iCPU##_N##_PassFail++;                                                    \
        }                                                                               \
        else if (SLTTEST_SOC_ERROR == ret)                                              \
	    {                                                                               \
	        g_iCPU##_N##_SocError ++;                                                   \
	    }                                                                               \
        else if (SLTTEST_OTHER_ERROR == ret)                                            \
	    {                                                                               \
	        g_iCPU##_N##_OtherError ++;                                                 \
	    }                                                                               \
        else 		                                                                    \
        {                                                                               \
            g_iCPU##_N##_PassFail = -1;                                                 \
            break;                                                                      \
        }                                                                               \
    }                                                                                   \
                                                                                        \
    spin_lock_irqsave(&cpu##_N##_lock, cpu##_N##_flags);                                \
    /*get ecc error counter */                                                          \
    __asm__ __volatile__(                                                               \
		"MRS %[val_hex], S3_1_c15_c2_2\n"                                               \
		"MRS %[val_hex1], S3_1_c15_c2_3\n"                                              \
        :[val_hex]"=&r"(val_hex),[val_hex1]"=&r"(val_hex1)                              \
        ::                                                                              \
    );                                                                                  \
    g_iCPU##_N##_CpuError = val_hex;                                                    \
    g_iCPU##_N##_L2Error = val_hex1;                                                    \
	printk(KERN_ERR "CPU%dL2 counter:l1==0x%lx,l2==0x%lx\n", _N,                        \
	    g_iCPU##_N##_CpuError,g_iCPU##_N##_L2Error);                                    \
                                                                                        \
    spin_unlock_irqrestore(&cpu##_N##_lock, cpu##_N##_flags);                           \
	g_iCPU##_N##_Running = 0;                                                           \
    if (g_iCPU##_N##_PassFail == g_iMaxPowerLoopCount)                                  \
    {                                                                                   \
        printk("\n>> CPU%d maxpower test pass (loop count = %d / %d )<<\n\n", _N,       \
            g_iCPU##_N##_PassFail,g_iMaxPowerLoopCount);                                \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        printk("\n>> CPU%d maxpower test fail (loop count = %d / %d )<<\n\n", _N,       \
            g_iCPU##_N##_PassFail,g_iMaxPowerLoopCount);                                \
    }                                                                                   \
                                                                                        \
    return count;                                                                       \
}

DEFINE_SLT_CPU_MAXPOWER_STORE(0)
DEFINE_SLT_CPU_MAXPOWER_STORE(1)
DEFINE_SLT_CPU_MAXPOWER_STORE(2)
DEFINE_SLT_CPU_MAXPOWER_STORE(3)

DEFINE_SLT_CPU_MAXPOWER_STORE(4)
DEFINE_SLT_CPU_MAXPOWER_STORE(5)
DEFINE_SLT_CPU_MAXPOWER_STORE(6)
DEFINE_SLT_CPU_MAXPOWER_STORE(7)

static ssize_t slt_maxpower_loop_count_store(struct device_driver *driver, const char *buf, size_t count)
{
    int result;

    if ((result = sscanf(buf, "%d", &g_iMaxPowerLoopCount)) == 1)
    {
        printk("set SLT MaxPower test loop count = %d successfully\n", g_iMaxPowerLoopCount);
    }
    else
    {
        printk("bad argument!!\n");
        return -EINVAL;
    }

    return count;
}

DRIVER_ATTR(slt_cpu0_maxpower, 0644, slt_cpu0_maxpower_show, slt_cpu0_maxpower_store);
DRIVER_ATTR(slt_cpu1_maxpower, 0644, slt_cpu1_maxpower_show, slt_cpu1_maxpower_store);
DRIVER_ATTR(slt_cpu2_maxpower, 0644, slt_cpu2_maxpower_show, slt_cpu2_maxpower_store);
DRIVER_ATTR(slt_cpu3_maxpower, 0644, slt_cpu3_maxpower_show, slt_cpu3_maxpower_store);

DRIVER_ATTR(slt_cpu4_maxpower, 0644, slt_cpu4_maxpower_show, slt_cpu4_maxpower_store);
DRIVER_ATTR(slt_cpu5_maxpower, 0644, slt_cpu5_maxpower_show, slt_cpu5_maxpower_store);
DRIVER_ATTR(slt_cpu6_maxpower, 0644, slt_cpu6_maxpower_show, slt_cpu6_maxpower_store);
DRIVER_ATTR(slt_cpu7_maxpower, 0644, slt_cpu7_maxpower_show, slt_cpu7_maxpower_store);

DRIVER_ATTR(slt_maxpower_loop_count, 0644, slt_maxpower_loop_count_show, slt_maxpower_loop_count_store);

#define DEFINE_SLT_CPU_MAXPOWER_INIT(_N)    \
int __init slt_cpu##_N##_maxpower_init(void) \
{   \
    int ret;    \
    \
    ret = driver_register(&slt_cpu##_N##_maxpower_drv);  \
    if (ret) {  \
        printk("fail to create SLT CPU%d MaxPower driver\n",_N);    \
    }   \
    else    \
    {   \
        printk("success to create SLT CPU%d MaxPower driver\n",_N); \
    }   \
    \
    ret = driver_create_file(&slt_cpu##_N##_maxpower_drv, &driver_attr_slt_cpu##_N##_maxpower);   \
    if (ret) {  \
        printk("fail to create SLT CPU%d MaxPower sysfs files\n",_N);   \
    }   \
    else    \
    {   \
        printk("success to create SLT CPU%d MaxPower sysfs files\n",_N);    \
    }   \
    \
    return 0;   \
}

DEFINE_SLT_CPU_MAXPOWER_INIT(0)
DEFINE_SLT_CPU_MAXPOWER_INIT(1)
DEFINE_SLT_CPU_MAXPOWER_INIT(2)
DEFINE_SLT_CPU_MAXPOWER_INIT(3)

DEFINE_SLT_CPU_MAXPOWER_INIT(4)
DEFINE_SLT_CPU_MAXPOWER_INIT(5)
DEFINE_SLT_CPU_MAXPOWER_INIT(6)
DEFINE_SLT_CPU_MAXPOWER_INIT(7)

unsigned long pMaxPowerTestMem;
int __init slt_maxpower_loop_count_init(void)
{
    int ret;

    pMaxPowerTestMem = (unsigned long)vmalloc(8*1024);
    if((void*)pMaxPowerTestMem == NULL)
    {
        printk("allocate memory for cpu maxpower test fail\n");
        return 0;
    }
    else
    {
        printk("maxpower test memory = 0x%lx\n",pMaxPowerTestMem);
    }

    ret = driver_register(&slt_maxpower_loop_count_drv);
    if (ret) {
        printk("fail to create MaxPower loop count driver\n");
    }
    else
    {
        printk("success to create MaxPower loop count driver\n");
    }


    ret = driver_create_file(&slt_maxpower_loop_count_drv, &driver_attr_slt_maxpower_loop_count);
	
    if (ret) 
	{
        printk("fail to create MaxPower loop count sysfs files\n");
    }
    else
    {
        printk("success to create MaxPower loop count sysfs files\n");
    }

    g_iMaxPowerLoopCount = SLT_LOOP_CNT;

    return 0;
}
arch_initcall(slt_cpu0_maxpower_init);
arch_initcall(slt_cpu1_maxpower_init);
arch_initcall(slt_cpu2_maxpower_init);
arch_initcall(slt_cpu3_maxpower_init);

arch_initcall(slt_cpu4_maxpower_init);
arch_initcall(slt_cpu5_maxpower_init);
arch_initcall(slt_cpu6_maxpower_init);
arch_initcall(slt_cpu7_maxpower_init);

arch_initcall(slt_maxpower_loop_count_init);
