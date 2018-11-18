#ifdef CONFIG_HUAWEI_EMMC_DSM
#ifndef LINUX_MMC_DSM_EMMC_H
#define LINUX_MMC_DSM_EMMC_H

#include <dsm/dsm_pub.h>

/* define a 4096 size of array as buffer */
#define EMMC_DSM_BUFFER_SIZE  4096
extern unsigned int emmc_dsm_real_upload_size;
#define MSG_MAX_SIZE 200

//eMMC card ext_csd lengh
#define EMMC_EXT_CSD_LENGHT 512

/*
debug version 0x00 , just for test that it could report;
deta version  0x02 ;
*/
#define EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_DEBUG           0x00    /* not defined */
#define EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_BETA            0x02    /* 10% - 20% device life time used */
#define EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_USER            0x05    /* 40% - 50% device life time used */
#define DEVICE_LIFE_TRIGGER_LEVEL  EMMC_LIFE_TIME_TRIGGER_LEVEL_FOR_BETA
/* Error code, decimal[5]: 0 is input, 1 is output, 2 I&O
 * decimal[4:3]: 10 is for eMMC,
 * decimal[2:1]: for different error code.
 */
enum DSM_EMMC_ERR
{
	DSM_EMMC_VDET_ERR          = 928002001,
	DSM_EMMC_PRE_EOL_INFO_ERR  = 928002002,
	DSM_EMMC_LIFE_TIME_EST_ERR = 928002003,
	DSM_EMMC_ERASE_ERR         = 928002004,
	DSM_EMMC_SEND_CXD_ERR      = 928002005,
	DSM_SYSTEM_W_ERR           = 928002006,
	DSM_EMMC_TUNING_ERROR      = 928002007,
	DSM_EMMC_READ_ERR          = 928002008,
	DSM_EMMC_WRITE_ERR         = 928002009,
	DSM_EMMC_RSP_ERR           = 928002011,
	DSM_EMMC_URGENT_BKOPS      = 928002012,
	DSM_EMMC_DYNCAP_NEEDED     = 928002013,
	DSM_EMMC_PACKED_FAILURE    = 928002014,
	DSM_EMMC_SET_BUS_WIDTH_ERR = 928002015,
	DSM_EMMC_HOST_ERR          = 928002016,
	DSM_EMMC_HOST_TIMEOUT_ERR  = 928002017,
	DSM_EMMC_SYSPOOL_EXHAUSTED = 928002018,
	DSM_EMMC_CACHE_TIMEOUT     = 928002019,
	DSM_EMMC_CACHE_ERR         = 928002026, 

};

struct emmc_dsm_log {
	char emmc_dsm_log[EMMC_DSM_BUFFER_SIZE];
	spinlock_t lock;	/* mutex */
};

extern struct dsm_client *emmc_dclient;

/*buffer for transffering to device radar*/
extern struct emmc_dsm_log g_emmc_dsm_log;
extern int dsm_emmc_get_log(void *card, int code, char * err_msg);

/*Transfer the msg to device radar*/
#define DSM_EMMC_LOG(card, no, fmt, a...) \
	do { \
		char msg[MSG_MAX_SIZE]; \
		snprintf(msg, MSG_MAX_SIZE-1, fmt, ## a); \
		spin_lock(&g_emmc_dsm_log.lock); \
		if(dsm_emmc_get_log((card), (no), (msg))){ \
			if(!dsm_client_ocuppy(emmc_dclient)) { \
				dsm_client_copy(emmc_dclient,g_emmc_dsm_log.emmc_dsm_log, emmc_dsm_real_upload_size + 1); \
				dsm_client_notify(emmc_dclient, no); } \
		} \
		spin_unlock(&g_emmc_dsm_log.lock); \
	}while(0)

#endif /* LINUX_MMC_DSM_EMMC_H */
#endif /* CONFIG_HUAWEI_EMMC_DSM */
