

#include <linux/module.h>

#include "srecorder_symbols.h"
#include "srecorder_dev.h"
#include "srecorder_log.h"
#include "srecorder_notifier.h"
#include "srecorder_jprobe.h"

#include "srecorder_sahara.h"
static unsigned long params[3] = {0x0, 0x0, 0x0};
module_param_array(params, ulong, NULL, 0444);
MODULE_PARM_DESC(params, "SRecorder parameters");

/**
    @function: static int __init srecorder_init(void)
    @brief: dump 
    @param: reason
    @return: none
    @note:
**/
static int __init srecorder_init(void)
{
    SRECORDER_PRINTK("Initialization %016lx %016lx.\n", params[0], params[1]);
#ifndef CONFIG_ARM
    srecorder_init_symbols();
#endif
    srecorder_init_dev();

    srecorder_ioremap_dts_memory();

    srecorder_init_log_headers();

    srecorder_resigter_notifiers();

#ifdef CONFIG_KPROBES
//    srecorder_resigter_jprobes();
#endif

    srecorder_enable_log_category_flags();

    srecorder_enable_log_type_flags();

/*
**	get dumpid at srecorder driver
*/
    minidump_set_dumpid();

    srecorder_save_kernel_log_addr();

    SRECORDER_PRINTK("Initialization done.\n");

    return 0;
}

/**
    @function: static void __exit srecorder_exit(void)
    @brief: dump 
    @param: reason
    @return: none
    @note:
**/
static void __exit srecorder_exit(void)
{
    srecorder_iounremap_dts_memory();

    srecorder_unresigter_notifiers();

#ifdef CONFIG_KPROBES
    srecorder_unresigter_jprobes();
#endif
}

module_init(srecorder_init);
module_exit(srecorder_exit);
MODULE_LICENSE("GPL");
