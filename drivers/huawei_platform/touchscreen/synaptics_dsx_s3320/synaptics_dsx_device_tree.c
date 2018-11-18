
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <huawei_platform/touchscreen/hw_tp_common.h>
#include "synaptics_dsx.h"
#include "synaptics_dsx_i2c.h"
#include "synaptics_dsx_device_tree.h"

static const char * const module_dts_config_node_name_list[] = {
	"huawei,ofilm",    // module id:0
	"huawei,eely",     // module id:1
	"huawei,truly",    // module id:2
	"huawei,mutto",    // module id:3
	"huawei,gis",      // module id:4
	"huawei,junda",    // module id:5
	"huawei,lensone",  // module id:6
	"huawei,yassy",    // module id:7
	"huawei,jdi",      // module id:8
	"huawei,samsung",  // module id:9
	"huawei,lg",       // module id:10
	"huawei,tianma",   // module id:11
	"huawei,cmi",      // module id:12
	"huawei,boe",      // module id:13
	"huawei,ctc",      // module id:14
	"huawei,edo",      // module id:15
	"huawei,sharp",    // module id:16
	"huawei,boe2",     // module id:17
};

const char * get_module_dts_config_node_name(unsigned long module_id)
{
	unsigned int node_list_length = ARRAY_SIZE(module_dts_config_node_name_list);
	if (module_id >= node_list_length) {
		//tp_log_err("%s %d:Index out of bounds, index=%l\n",
		//	__func__, __LINE__, module_id);
		tp_log_err("%s %d:Index out of bounds, index=%lu\n", 
			__func__, __LINE__, module_id);
		return NULL;
	}

	return module_dts_config_node_name_list[module_id];
}
/**
 * get_of_u32_val() - get the u32 type value fro the DTSI.
 * @np: device node
 * @name: The DTSI key name
 * @default_value: If the name is not matched, return a default value.
 */
u32 get_of_u32_val(struct device_node *np, const char *name,u32 default_val)
{
	u32 tmp = 0;
	int err = 0;
	err = of_property_read_u32(np, name, &tmp);
	if (!err)
		return tmp;
	else {
		tp_log_info("%s:get configuration fail from dts, name=%s, default_val=%d!\n",
			__func__, name, default_val);
		return default_val;
	}
}
/*To create the array according to the node size and get the property from the dtsi*/
u16 *create_and_get_u16_array(struct device_node *dev_node, const char *name, int *size)
{
	const __be32 *values;
	u16 *val_array;
	int len;
	int sz;
	int rc;
	int i;

	/*To get the property by the property name in the node*/
	values = of_get_property(dev_node, name, &len);
	if (values == NULL)
		return NULL;

	sz = len / sizeof(u32);
	tp_log_debug("%s: %s size:%d\n", __func__, name, sz);

	val_array = kzalloc(sz * sizeof(u16), GFP_KERNEL);
	if (val_array == NULL) {
		rc = -ENOMEM;
		goto fail;
	}

	for (i = 0; i < sz; i++)
		val_array[i] = (u16)be32_to_cpup(values++);

	*size = sz;

	return val_array;

fail:
	return ERR_PTR(rc);
}

#ifdef CONFIG_OF
static void synaptics_print_dtsi_info(struct synaptics_dsx_platform_data *pdata)
{
	tp_log_debug("synaptics,product_name=%s\n", pdata->product_name);
	tp_log_debug("synaptics,ic_type=%d\n", pdata->ic_type);
	tp_log_debug("synaptics,reset-gpio=%d\n", pdata->reset_gpio);
	tp_log_debug("synaptics,lcd-x=%d; lcd-y=%d; lcd-all=%d\n", pdata->lcd_x,pdata->lcd_y,pdata->lcd_all);
	tp_log_debug("synaptics,reset-on-status=%d power-delay-ms=%d reset-delay-ms=%d reset-active-ms=%d\n",
					pdata->reset_on_state,pdata->power_delay_ms,pdata->reset_delay_ms,pdata->reset_active_ms);
	tp_log_debug("synaptics,gesture_enabled=%d; easy_wakeup_supported_gestures=0x%x\n",
					pdata->gesture_enabled,pdata->easy_wakeup_supported_gestures);
	tp_log_debug("synaptics,double tap zone=(%d,%d)(%d,%d)\n",
				pdata->dtz_x0,pdata->dtz_y0,pdata->dtz_x1,pdata->dtz_y1);
	tp_log_debug("synaptics,glove_enabled=%d\n", pdata->glove_enabled);
	tp_log_debug("synaptics,fast_relax_gesture=%d\n", pdata->fast_relax_gesture);
	tp_log_debug("synaptics,pdata->holster_supported=%d\n", pdata->holster_supported);
	tp_log_debug("synaptics,pdata->roi_supported=%d\n", pdata->roi_supported);
	tp_log_debug("synaptics,pdata->tx_rx_supported=%d\n", pdata->tx_rx_support);
	tp_log_debug("synaptics,glove_edge_switch_supported=%d\n", pdata->glove_edge_switch_supported);
	tp_log_debug("synaptics,grip_algorithm_supported=%d\n", pdata->grip_algorithm_supported);
	tp_log_debug("synaptics,grip_algorithm_enabled=%d\n", pdata->grip_algorithm_enabled);
	tp_log_info("synaptics,i2c_addr=%x\n", pdata->i2c_addr);
	tp_log_info("synaptics,ub_i2c_addr=%x\n", pdata->ub_i2c_addr);
}

int synaptics_rmi4_parse_dt(struct device *dev, struct synaptics_dsx_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	struct property *prop;
	u32 tmp = 0;
	int err = 0;

	pdata->x_flip = of_property_read_bool(np, "synaptics,x-flip");
	pdata->y_flip = of_property_read_bool(np, "synaptics,y-flip");
	pdata->swap_axes = of_property_read_bool(np, "synaptics,swap-axes");

	/*
	*   @About regulator
	*   "synaptics,vdd","synaptics,vbus","synaptics,reg-en" are moved to synaptics_rmi4_power_on()
	*   "synaptics,power-gpio" and "synaptics,power-on-status" are deleted.
	*/

	/* reset, irq gpio info */
	pdata->irq_gpio = of_get_named_gpio_flags(np, "synaptics,irq-gpio", 0, NULL);
	/* In case that system has not enough time to process irq */
	pdata->irq_flags = IRQF_TRIGGER_LOW | IRQF_ONESHOT;

	if (of_find_property(np, "synaptics,reset-gpio", NULL)) {
		pdata->reset_gpio = of_get_named_gpio_flags(np, "synaptics,reset-gpio", 0, NULL);
	} else {
		/*invalid reset GPIO*/
		pdata->reset_gpio = -1;
	}

	/* Modify JDI tp/lcd power on/off to reduce power consumption */
	if (of_find_property(np, "synaptics,vci-gpio", NULL)) {
		pdata->vci_gpio = of_get_named_gpio_flags(np, "synaptics,vci-gpio", 0, NULL);
	} else {
		/*invalid vci enable GPIO*/
		pdata->vci_gpio = -1;
	}
	/* delete get ic type form dts */

	/*LCD resolution. Default is WVGA*/
	pdata->lcd_x = get_of_u32_val(np, "synaptics,lcd-x", LCD_X_DEFAULT);
	pdata->lcd_y = get_of_u32_val(np, "synaptics,lcd-y", LCD_Y_DEFAULT);
	pdata->lcd_all = get_of_u32_val(np, "synaptics,lcd-all", LCD_ALL_DEFAULT);

#ifdef ENABLE_VIRTUAL_KEYS
	err = setup_virtual_keys(np, DRIVER_NAME, &pdata->vkeys);
	if (err) {
		/*If the virtual keys are not supported the TP should work fine;*/
		tp_log_err("%s: Cannot setup virtual keys, only TP will work now!err = %d\n",__func__,err);
	}
#endif
	pdata->shutdown_flag = get_of_u32_val(np, "synaptics,shutdown_flag",0);
	pdata->appinfo_display_flag = get_of_u32_val(np,"synaptics,appinfo_display_flag",0);
	err = of_property_read_string(np, "synaptics,product_name",&pdata->product_name);
	if (err){
		tp_log_err("Unable to read firmware_name\n");			
		pdata->product_name = "Unknow";
	}

	pdata->reset_on_state = get_of_u32_val(np, "synaptics,reset-on-status", 0);
	pdata->power_delay_ms = get_of_u32_val(np, "synaptics,power-delay-ms", 160);
	pdata->reset_delay_ms = get_of_u32_val(np, "synaptics,reset-delay-ms", 10);
	pdata->reset_active_ms = get_of_u32_val(np, "synaptics,reset-active-ms", 100);

	pdata->sleep_strategy 
		= get_of_u32_val(np, "synaptics,sleep-strategy", SLEEP_STRATEGY_DEEP_SLEEP);
	tp_log_info("%s %d:sleep-strategy:%d\n", __func__, __LINE__, pdata->sleep_strategy);

	prop = of_find_property(np, "synaptics,button-map", NULL);
	if (prop) {
		pdata->cap_button_map->nbuttons = prop->length / sizeof(tmp);
		err = of_property_read_u32_array(np,
			"synaptics,button-map", (unsigned int *)pdata->cap_button_map->map,
			pdata->cap_button_map->nbuttons);
		if (err) {
			tp_log_err("Unable to read key codes\n");
			pdata->cap_button_map->map = NULL;
		}
	}

	pdata->gesture_enabled = get_of_u32_val(np, "synaptics,gesture_enabled", 0);
	pdata->easy_wakeup_supported_gestures = get_of_u32_val(np,
			"synaptics,easy_wakeup_supported_gestures", 0);

	pdata->wakeup_keys = create_and_get_wakeup_keys(np);
	if (IS_ERR_OR_NULL(pdata->wakeup_keys)) {
		tp_log_err("%s: Wakeup gesture is not configured!\n", __func__);
		pdata->wakeup_keys = NULL;
	} else {
		/*change to debug*/
		tp_log_warning("%s: Wakeup gesture is configured for %d guestures\n", 
			__func__,pdata->wakeup_keys->size);
	}

	/* Double tap zone defined in dtsi  0 means invalid*/
	pdata->dtz_x0 = get_of_u32_val(np, "huawei,dtz_x0", 0);

	pdata->dtz_y0 = get_of_u32_val(np, "huawei,dtz_y0", 0);

	pdata->dtz_x1 = get_of_u32_val(np, "huawei,dtz_x1", 0);
	
	pdata->dtz_y1 = get_of_u32_val(np, "huawei,dtz_y1", 0);
	
	s3320_cap_test_fake_status = get_of_u32_val(np, "synaptics,TRT_cap_test_fake_status",0);

	err = of_property_read_u32(np, "synaptics,glove_enabled", &tmp);
	if (!err) {
		pdata->glove_enabled = tmp;
		pdata->glove_supported = true;
	} else {
		pdata->glove_supported = false;
	}
	pdata->fast_relax_gesture = get_of_u32_val(np, "synaptics,fast_relax_gesture", 0);

	pdata->holster_supported = (bool)get_of_u32_val(np, "synaptics,holster_supported", 0);
	pdata->roi_supported = (bool)get_of_u32_val(np, "synaptics,roi_supported", 0);
	pdata->esd_support = (bool)get_of_u32_val(np, "synaptics,esd_support", 0);
	pdata->tx_rx_support = (bool)get_of_u32_val(np, "synaptics,tx_rx_supported", 0);
	pdata->tddi_esd_flag = (bool)get_of_u32_val(np, "huawei,tddi_esd_flag", 0);

	pdata->glove_edge_switch_supported= (bool)get_of_u32_val(np, "synaptics,glove_edge_switch_supported", 0);
	pdata->grip_algorithm_supported= (bool)get_of_u32_val(np, "synaptics,grip_algorithm_supported", 0);
	pdata->grip_algorithm_enabled= (bool)get_of_u32_val(np, "synaptics,grip_algorithm_enabled", 0);
	pdata->grip_left_lsb= get_of_u32_val(np, "huawei,grip_left_lsb", 0);
	pdata->grip_left_msb= get_of_u32_val(np, "huawei,grip_left_msb", 0);
	pdata->grip_right_lsb= get_of_u32_val(np, "huawei,grip_right_lsb", 0x4c);
	pdata->grip_right_msb= get_of_u32_val(np, "huawei,grip_right_msb", 0x04);
	pdata->ub_i2c_addr= get_of_u32_val(np, "synaptics,ub-i2c-addr", 0x2c);
	pdata->i2c_addr= get_of_u32_val(np, "synaptics,i2c-addr", 0x70);
	/*DEBUG: print tp dtsi info*/
	synaptics_print_dtsi_info(pdata);
	
	return 0;
}
#else
int synaptics_rmi4_parse_dt(struct device *dev, struct synaptics_dsx_platform_data *pdata)
{
	return -ENODEV;
}
#endif

struct syanptics_wakeup_keys *create_and_get_wakeup_keys(
		struct device_node *dev_node)
{
	struct syanptics_wakeup_keys *wakeup_keys;
	u16 *keys;
	int size;
	int rc;

	keys = create_and_get_u16_array(dev_node, "synaptics,easy_wakeup_gesture_keys", &size);
	if (IS_ERR_OR_NULL(keys))
		return (void *)keys;

	/* Check for valid abs size */
	if (size > GESTURE_MAX) {
		rc = -EINVAL;
		tp_log_err("%s:fail line=%d\n", __func__,__LINE__);
		goto fail_free_keys;
	}

	wakeup_keys = kzalloc(sizeof(*wakeup_keys), GFP_KERNEL);
	if (wakeup_keys == NULL) {
		rc = -ENOMEM;
		tp_log_err("%s:fail line=%d\n", __func__,__LINE__);
		goto fail_free_keys;
	}

	wakeup_keys->keys = keys;
	wakeup_keys->size = size;

	return wakeup_keys;

fail_free_keys:
	kfree(keys);

	return ERR_PTR(rc);
}

/* dwt represent doze wakeup threshold in gesture mode */
int synaptics_parse_dwt(struct device_node *dev_node) 
{
	return get_of_u32_val(dev_node, "huawei,dwt", 0);
}
