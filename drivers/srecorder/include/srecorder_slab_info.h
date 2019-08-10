

#ifndef SRECORDER_SLAB_INFO_H
#define SRECORDER_SLAB_INFO_H

#include "srecorder_misc.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
    @function: void srecorder_enable_slab_info(void)
    @brief: 
    @return: 
    @note: 
**/
void srecorder_enable_slab_info(void);

/**
    @function: void srecorder_disable_slab_info(void)
    @brief: 
    @return: 
    @note: 
**/
void srecorder_disable_slab_info(void);

/**
    @function: int srecorder_dump_slab_info()
    @brief: 
    @return: 
    @note: 
**/
int srecorder_dump_slab_info(void);

#ifdef __cplusplus
}
#endif
#endif /* SRECORDER_SLAB_INFO_H */
