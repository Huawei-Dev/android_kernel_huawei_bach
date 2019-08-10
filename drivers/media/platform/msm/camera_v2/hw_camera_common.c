
#include <linux/moduleparam.h>
#include <linux/hw_camera_common.h>
#include <linux/stat.h>

unsigned int hw_cmr_log_mask = HW_CMR_LOG_ERR|HW_CMR_LOG_WARN;

module_param_named(hw_camera_log_mask, hw_cmr_log_mask, uint, S_IRUGO|S_IWUSR);

