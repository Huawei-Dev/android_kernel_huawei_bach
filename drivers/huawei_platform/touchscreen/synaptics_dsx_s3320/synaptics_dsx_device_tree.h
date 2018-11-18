
#ifndef _SYNAPTICS_DSX_DEVICES_TREE_
#define _SYNAPTICS_DSX_DEVICES_TREE_

#include <linux/types.h>
#include "synaptics_dsx_i2c.h"

#define LCD_X_DEFAULT 480
#define LCD_Y_DEFAULT 800
#define LCD_ALL_DEFAULT 854

const char * get_module_dts_config_node_name(unsigned long module_id);
int synaptics_rmi4_parse_dt(struct device *dev, struct synaptics_dsx_platform_data *pdata);
u32 get_of_u32_val(struct device_node *np, const char *name,u32 default_val);
u16 *create_and_get_u16_array(struct device_node *dev_node, const char *name, int *size);
struct syanptics_wakeup_keys *create_and_get_wakeup_keys(struct device_node *dev_node);
int synaptics_parse_dwt(struct device_node *dev_node);

#endif
