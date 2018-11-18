

#ifndef SRECORDER_DEV_H
#define SRECORDER_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "srecorder_log.h"

typedef struct
{
    log_category_e  category;
    unsigned        len;
} log_info_t;

/* srecorder ioctl commands */
#define SRECORDER_IOC_MAGIC 'S'
#define SRECORDER_IOC_ENABLE_CATEGORY_FLAG      _IOR(SRECORDER_IOC_MAGIC, 0, unsigned)
#define SRECORDER_IOC_DISABLE_CATEGORY_FLAG     _IOR(SRECORDER_IOC_MAGIC, 1, unsigned)
#define SRECORDER_IOC_ENABLE_TYPE_FLAG          _IOR(SRECORDER_IOC_MAGIC, 2, unsigned)
#define SRECORDER_IOC_DISABLE_TYPE_FLAG         _IOR(SRECORDER_IOC_MAGIC, 3, unsigned)
#define SRECORDER_IOC_GET_LOG_INFO              _IOWR(SRECORDER_IOC_MAGIC, 4, log_info_t)

/**
    @function: int srecorder_init_dev()
    @brief: 
    @param: pinit_params 
    @return: 
    @note: 
*/
int srecorder_init_dev(void);

/**
    @function: void srecorder_exit_dev(void)
    @brief: 
    @param: none
    @return: none
    @note: 
*/
void srecorder_exit_dev(void);

#ifdef __cplusplus
}
#endif
#endif /* SRECORDER_DEV_H */
