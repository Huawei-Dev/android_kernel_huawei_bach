#ifndef __HW_FAC_INFO_H__
#define __HW_FAC_INFO_H__

#include <linux/printk.h>

#define FAC_ERR  1
#define FAC_INFO 2
#define FAC_DBG  3

#define HW_FAC_TAG  "[HW_FAC_INFO] "

extern int KERNEL_HWFLOW;
extern int hw_fac_info_debug_mask;

#ifndef fac_log_err
#define fac_log_err(x...)                \
do{                                     \
    if( hw_fac_info_debug_mask >= FAC_ERR )   \
    {                                   \
        pr_err(HW_FAC_TAG x); \
    }                                   \
                                        \
}while(0)
#endif

#ifndef fac_log_info
#define fac_log_info(x...)               \
do{                                     \
    if( (KERNEL_HWFLOW) && (hw_fac_info_debug_mask >= FAC_INFO))  \
    {                                   \
        pr_err(HW_FAC_TAG x); \
    }                                   \
                                        \
}while(0)
#endif

#ifndef fac_log_debug
#define fac_log_debug(x...)              \
do{                                     \
    if( hw_fac_info_debug_mask >= FAC_DBG )   \
    {                                   \
        pr_err(HW_FAC_TAG x); \
    }                                   \
                                        \
}while(0)
#endif

#endif
