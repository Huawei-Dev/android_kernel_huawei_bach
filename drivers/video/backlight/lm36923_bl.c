/*
* Simple driver for Texas Instruments LM3639 Backlight + Flash LED driver chip
* Copyright (C) 2012 Texas Instruments
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/backlight.h>
#include <linux/mfd/lm36923.h>
#include <misc/app_info.h>


#ifdef LM36923_BL_POWER_TEST_ONLY_FOR_KERNEL
#include <linux/gpio.h>
#define LM36923_BL_HWEN_GPIO 93
#endif

//#if defined (CONFIG_HUAWEI_DSM)
#if 0
#include <dsm/dsm_pub.h>
extern struct dsm_client *lcd_dclient;
#endif

struct class *lm36923_class;
struct lm36923_chip_data *g_pchip;

unsigned lm36923_msg_level = 7;
uint32_t last_brightness = -1;
uint32_t lm36923_bl_is_used = 0;

ssize_t Is_lm36923_used(void)
{
	return lm36923_bl_is_used;
}
EXPORT_SYMBOL(Is_lm36923_used);

/* initialize chip */
int lm36923_chip_init(struct lm36923_chip_data *pchip)
{
	int ret = -1;
	struct device_node *np = NULL;
	uint32_t lm36923_ctrl_mode = 0;
	int enable_reg = 0x0F;
	int protect_disable_flag = 0;
	int mapping_mode = 0, brightness_mode = 0, ramp_enable = 0, ramp_rate = 0, adj_default = 0;
	int pwm_rate = 0, pwm_input_polarity = 0, pwm_hysteresis = 0, pwm_pulse_filter = 0;

	LM36923_DEBUG("in!\n");
	if(pchip==NULL){
		LM36923_ERR("g_pchip is NULL\n");
		return ret;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_TI_LM36923);
	if (!np) {
		LM36923_ERR("NOT FOUND device node %s!\n", DTS_COMP_TI_LM36923);
		goto out;
	}

	//lm36923-ctrl-mode
	ret = of_property_read_u32(np, LM36923_CTRL_MODE, &lm36923_ctrl_mode);
	if (ret) {
		LM36923_ERR("get lm36923_ctrl_mode failed!\n");
		lm36923_ctrl_mode = BRT_RAMP_MUL_MODE;
	}

	//mapping-mode
	ret = of_property_read_u32(np, TI_LM36923_MAPPING_MODE, &mapping_mode);
	if (ret < 0) {
		LM36923_ERR("get mapping_mode failed!\n");
		mapping_mode = LM36923_BLED_MODE_LINEAR;
	}
	else{
		mapping_mode = (mapping_mode==1?LM36923_BLED_MODE_EXPONETIAL:LM36923_BLED_MODE_LINEAR);
	}

	//brightness-mode
	ret = of_property_read_u32(np, TI_LM36923_BRIGHTNESS_MODE, &brightness_mode);
	if (ret < 0) {
		LM36923_ERR("get brightness_mode failed!\n");
		brightness_mode = BRT_MUL_RAMP;
	}
	else{
		brightness_mode = (brightness_mode>=LM36923_BRT_MODE_MAX?BRT_REG_ONLY:(brightness_mode<0?BRT_REG_ONLY:(BRT_REG_ONLY+(brightness_mode<<5))));
	}

	//ramp-enable
	ret = of_property_read_u32(np, TI_LM36923_RAMP_ENABLE, &ramp_enable);
	if (ret < 0) {
		LM36923_ERR("get ramp_enable failed!\n");
		ramp_enable = LM36923_RAMP_EN;
	}
	else{
		ramp_enable = (ramp_enable==1?LM36923_RAMP_EN:LM36923_RAMP_DIS);
	}

	//ramp-rate
	ret = of_property_read_u32(np, TI_LM36923_RAMP_RATE, &ramp_rate);
	if (ret < 0) {
		LM36923_ERR("get ramp_rate failed!\n");
		ramp_rate = RAMP_RATE_1;
	}
	else{
		ramp_rate = (ramp_rate>=LM36923_RAMP_RATE_MAX?RAMP_RATE_0125:(ramp_rate<0?RAMP_RATE_0125:(RAMP_RATE_0125+(ramp_rate<<1))));
	}

	//adj-default
	ret = of_property_read_u32(np, TI_LM36923_ADJ_DEFAULT, &adj_default);
	if (ret < 0) {
		LM36923_ERR("get adj_default failed!\n");
		adj_default = BL_ADJ_HIGHT;
	}
	else{
		adj_default = (adj_default==1?BL_ADJ_HIGHT:BL_ADJ_LOW);
	}

	//pwm-rate
	ret = of_property_read_u32(np, TI_LM36923_PWM_RATE, &pwm_rate);
	if (ret < 0) {
		LM36923_ERR("get pwm-rate failed!\n");
		pwm_rate = LM36923_PWM_SAMPLE_RATE_240;
	}
	else{
		pwm_rate = (pwm_rate>=LM36923_PWM_RATE_MAX?LM36923_PWM_SAMPLE_RATE_8:(pwm_rate<0?LM36923_PWM_SAMPLE_RATE_8:(LM36923_PWM_SAMPLE_RATE_8+(pwm_rate<<6))));
	}

	//pwm-input-polarity
	ret = of_property_read_u32(np, TI_LM36923_PWM_INPUT_POLARITY, &pwm_input_polarity);
	if (ret < 0) {
		LM36923_ERR("get pwm_input_polarity failed!\n");
		pwm_input_polarity = LM36923_PWM_IN_POL_H;
	}
	else{
		pwm_input_polarity = (pwm_input_polarity>=LM36923_PWM_INPUT_POLARITY_MAX?LM36923_PWM_IN_POL_L:(pwm_input_polarity<0?LM36923_PWM_IN_POL_L:(LM36923_PWM_IN_POL_L+(pwm_input_polarity<<5))));
	}

	//pwm-hysteresis
	ret = of_property_read_u32(np, TI_LM36923_PWM_HYSTERESIS, &pwm_hysteresis);
	if (ret < 0) {
		LM36923_ERR("get pwm_hysteresis failed!\n");
		pwm_hysteresis = LM36923_PWM_HYS_NONE;
	}
	else{
		pwm_hysteresis = (pwm_hysteresis>=LM36923_PWM_HYSTERESIS?LM36923_PWM_HYS_NONE:(pwm_hysteresis<0?LM36923_PWM_HYS_NONE:(LM36923_PWM_HYS_NONE+(pwm_hysteresis<<2))));
	}

	//pwm-filter
	ret = of_property_read_u32(np, TI_LM36923_PWM_PULSE_FILTER, &pwm_pulse_filter);
	if (ret < 0) {
		LM36923_ERR("get pwm_pulse_filter failed!\n");
		pwm_pulse_filter = LM36923_PWM_FILTER_100;
	}
	else{
		pwm_pulse_filter = (pwm_pulse_filter>=LM36923_PWM_FITLER?LM36923_PWM_FILTER_100:(pwm_pulse_filter<0?LM36923_PWM_FILTER_100:(LM36923_PWM_FILTER_100+pwm_pulse_filter)));
	}
	LM36923_DEBUG("Brightness Control(0x11):0x%x, PWM control(0x12):0x%x\n",mapping_mode|brightness_mode|ramp_enable|ramp_rate|adj_default,
		pwm_rate|pwm_input_polarity|pwm_hysteresis|pwm_pulse_filter);

	ret = regmap_update_bits(pchip->regmap, REG_BRT_CTR, MASK_CHANGE_MODE,
			mapping_mode|brightness_mode|ramp_enable|ramp_rate|adj_default);

	ret = of_property_read_u32(np, TI_LM36923_ENABLE_REG, &enable_reg);
	if (ret >= 0) {
		LM36923_INFO("lm36923_enable_reg = %d\n", enable_reg);
		ret = regmap_write(pchip->regmap, REG_ENABLE, enable_reg);
		if (ret < 0)
			goto out;
	}

	/* set PWM Hysteresis to 0 */
	ret = regmap_update_bits(pchip->regmap, REG_PWM_CTR, MASK_CHANGE_MODE,
		pwm_rate|pwm_input_polarity|pwm_hysteresis|pwm_pulse_filter);
	if (ret < 0)
		goto out;

	ret = of_property_read_u32(np, TI_LM36923_PROTECT_DISABLE, &protect_disable_flag);
	LM36923_DEBUG("lm36923 protect flag = %d\n", protect_disable_flag);
	if (ret < 0) {
		/* Enable OCP/OVP Detect,IC will shutdowm when OCP/OVP occur */
		ret = regmap_write(pchip->regmap, REG_FAULT_CTR, OVP_OCP_SHUTDOWN_ENABLE);
		if (ret < 0)
			goto out;
	}

	LM36923_DEBUG("ok!\n");

	return ret;

out:
	dev_err(pchip->dev, "i2c failed to access register\n");
	return ret;
}

ssize_t lm36923_chip_initial(void)
{
	ssize_t ret = -1;

	ret = (ssize_t)lm36923_chip_init(g_pchip);

	return ret;
}
EXPORT_SYMBOL(lm36923_chip_initial);

/**
 * lm36923_set_backlight_mode(): Set Backlight working mode
 *
 * @bl_mode: BRT_PWM_ONLY
 *		BRT_REG_ONLY
 *		BRT_MUL_RAMP
 * 		BRT_RAMP_MUL
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36923_set_backlight_mode(uint32_t bl_mode)
{
	uint32_t mode = 0;
	ssize_t ret = -1;
	unsigned int ctrl_mode[4] = {0x00,0x20,0x40,0x60};

	if(g_pchip==NULL){
		LM36923_ERR("g_pchip is NULL\n");
		return ret;
	}

	mode = bl_mode;

	if (mode > BRT_RAMP_MUL_MODE) {
		dev_err(g_pchip->dev, "%s:input unvalid\n", __func__);
		return -EINVAL;
	}

	/* Update IC control mode */
	ret = regmap_update_bits(g_pchip->regmap, REG_BRT_CTR, MASK_BRT_MODE,ctrl_mode[mode]);
	if (ret < 0) {
		dev_err(g_pchip->dev, "%s:regmap update BRT_REG_ONLY_MODE failure\n", __func__);
		goto i2c_error;
	}

	return ret;

i2c_error:
	dev_err(g_pchip->dev, "i2c failed to access register\n");
	return ret;
}
EXPORT_SYMBOL(lm36923_set_backlight_mode);

/**
 * lm36923_set_backlight_reg(): Set Backlight working mode
 *
 * @bl_level: value for backlight ,range from 0 to 2047
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36923_set_backlight_reg(uint32_t bl_level)
{
	ssize_t ret = -1;
	uint32_t level = 0;
	int bl_msb = 0;
	int bl_lsb = 0;
	static int last_level = -1;

	LM36923_DEBUG("bl_level:%d\n",bl_level);

	if(g_pchip==NULL){
		LM36923_ERR("g_pchip is NULL\n");
		return ret;
	}

	level = bl_level;

	if (level > BL_MAX) {
		level = BL_MAX;
	}

	/* 11-bit brightness code */
	bl_msb = level >> 3;
	bl_lsb = level & 0x07;

	if ((BL_MIN == last_level && LOG_LEVEL_INFO == lm36923_msg_level)
		|| (BL_MIN == level && LOG_LEVEL_INFO == lm36923_msg_level)
		|| (-1 == last_level)) {
		LM36923_DEBUG("level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);
	}

	LM36923_DEBUG("level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);

	ret = regmap_update_bits(g_pchip->regmap, REG_BRT_VAL_L, MASK_BL_LSB,bl_lsb);
	if (ret < 0) {
		goto i2c_error;
	}

	ret = regmap_write(g_pchip->regmap, REG_BRT_VAL_M, bl_msb);
	if (ret < 0) {
		goto i2c_error;
	}

	last_level = level;

	return ret;

i2c_error:
	dev_err(g_pchip->dev, "%s:i2c access fail to register\n", __func__);
	return ret;
}
EXPORT_SYMBOL(lm36923_set_backlight_reg);

/**
 * lm36923_set_reg(): Set lm36923 reg
 *
 * @bl_reg: which reg want to write
 * @bl_mask: which bits of reg want to change
 * @bl_val: what value want to write to the reg
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36923_set_reg(u8 bl_reg,u8 bl_mask,u8 bl_val)
{
	ssize_t ret = -1;
	u8 reg = bl_reg;
	u8 mask = bl_mask;
	u8 val = bl_val;

	if(g_pchip==NULL){
		LM36923_ERR("g_pchip is NULL\n");
		return ret;
	}

	if (REG_MAX < reg) {
		LM36923_ERR("Invalid argument!!!\n");
		return ret;
	}

	LM36923_DEBUG("%s:reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(g_pchip->regmap, reg, mask, val);
	if (ret < 0) {
		LM36923_ERR("i2c access fail to register\n");
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(lm36923_set_reg);

/**
 * lm36923_ramp_brightness(): Use RampRate func
 *
 * @cur_brightness: current brightness
 *
 */
void lm36923_ramp_brightness(uint32_t cur_brightness)
{
    int i = 0;
    uint32_t amount_brightness = 0;
    uint32_t ramp_mode[MAX_RATE_NUM] = {1,2,4,8,16,32,64,128,299};
    u8 ramp_rate[MAX_RATE_NUM] = {RAMP_RATE_16,RAMP_RATE_8,
		RAMP_RATE_4,RAMP_RATE_2,RAMP_RATE_1,
		RAMP_RATE_05,RAMP_RATE_025,RAMP_RATE_0125,RAMP_RATE_0125};
    if (last_brightness != -1){
        amount_brightness = abs(cur_brightness - last_brightness);
        for(i=0;i<MAX_RATE_NUM;i++){
            if (ramp_mode[i] >= amount_brightness ){
                lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_EN,LM36923_RAMP_EN);
                lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_RATE,ramp_rate[i]);
                last_brightness = cur_brightness;
                LM36923_DEBUG("amount_brightness=%d,ramp_rate=0x%x\n",
					amount_brightness,ramp_rate[i]);
                break;
            } else {
                last_brightness = cur_brightness;
                lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_EN,LM36923_RAMP_DIS);
            }
        }
    }else{
        last_brightness = cur_brightness;
        lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_EN,LM36923_RAMP_DIS);
    }

    return;
}
EXPORT_SYMBOL(lm36923_ramp_brightness);

static ssize_t lm36923_bled_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	unsigned int mode = 0;
	ssize_t ret = -1;

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_read(pchip->regmap, REG_BRT_CTR, &mode);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36923 I2C read error\n");

	return snprintf(buf, PAGE_SIZE, "LM36923 bl_mode:0x%x\n",mode);
}

static ssize_t lm36923_bled_mode_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36923_chip_data *pchip = dev_get_drvdata(dev);
	uint32_t mode = 0;

	ret = kstrtouint(buf, 16, &mode);
	if (ret)
		goto out_input;

	if(g_pchip==NULL){
		LM36923_ERR("g_pchip is NULL\n");
		goto i2c_error;
	}

	ret = regmap_update_bits(g_pchip->regmap, REG_BRT_CTR, MASK_CHANGE_MODE, mode);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}

static DEVICE_ATTR(bled_mode, S_IRUGO|S_IWUSR, lm36923_bled_mode_show, lm36923_bled_mode_store);

static ssize_t lm36923_reg_bl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;
	unsigned int bl_lsb = 0;
	unsigned int bl_msb = 0;
	int bl_level = 0;

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_read(pchip->regmap, REG_BRT_VAL_M, &bl_msb);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36923 I2C read error\n");

	ret = regmap_read(pchip->regmap, REG_BRT_VAL_L, &bl_lsb);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36923 I2C read error\n");

	bl_level = (bl_msb << 3) | bl_lsb;

	return snprintf(buf, PAGE_SIZE, "LM36923 bl_level:%d\n", bl_level);
}

static ssize_t lm36923_reg_bl_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36923_chip_data *pchip = dev_get_drvdata(dev);
	unsigned int bl_level = 0;
	unsigned int bl_msb = 0;
	unsigned int bl_lsb = 0;

	ret = kstrtouint(buf, 10, &bl_level);
	if (ret)
		goto out_input;

	LM36923_DEBUG("%s:buf=%s,state=%d\n", __func__, buf, bl_level);

	if (bl_level < BL_MIN)
		bl_level = BL_MIN;

	if (bl_level > BL_MAX)
		bl_level = BL_MAX;

	/* 11-bit brightness code */
	bl_msb = bl_level >> 3;
	bl_lsb = bl_level & 0x07;

	LM36923_DEBUG("bl_level = %d, bl_msb = %d, bl_lsb = %d\n", bl_level, bl_msb, bl_lsb);

	ret = regmap_update_bits(pchip->regmap, REG_BRT_VAL_L, MASK_BL_LSB, bl_lsb);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_write(pchip->regmap, REG_BRT_VAL_M, bl_msb);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}

static DEVICE_ATTR(reg_bl, S_IRUGO|S_IWUSR, lm36923_reg_bl_show, lm36923_reg_bl_store);

static ssize_t lm36923_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;
	unsigned char val[14] = {0};

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_bulk_read(pchip->regmap, REG_REVISION, &val[0], 2);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_bulk_read(pchip->regmap, REG_ENABLE, &val[2], 10);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_bulk_read(pchip->regmap, REG_FAULT_CTR, &val[12], 2);
	if (ret < 0)
		goto i2c_error;

	return snprintf(buf, PAGE_SIZE, "Revision(0x00)= 0x%x\nSoftware Reset(0x01)= 0x%x\n \
			\rEnable(0x10) = 0x%x\nBrightness Control(0x11) = 0x%x\n \
			\rPWM Control(0x12) = 0x%x\nBoost Control 1(0x13) = 0x%x\n \
			\rBoost Control 2(0x14) = 0x%x\nAuto Frequency High Threshold(0x15) = 0x%x\n \
			\rAuto Frequency High Threshold(0x16)  = 0x%x\nBack Light Adjust Threshold(0x17)  = 0x%x\n \
			\rBrightness Register LSB's(0x18) = 0x%x\nBrightness Register MSB's(0x19) = 0x%x\n \
			\rFault Control(0x1E) = 0x%x\nFault Flags(0x1F) = 0x%x\n",
			val[0],val[1],val[2],val[3],val[4],val[5],val[6],val[7],
			val[8],val[9],val[10],val[11],val[12],val[13]);

i2c_error:
	return snprintf(buf, PAGE_SIZE,"%s: i2c access fail to register\n", __func__);
}

static ssize_t lm36923_reg_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36923_chip_data *pchip = dev_get_drvdata(dev);
	unsigned int reg = 0;
	unsigned int mask = 0;
	unsigned int val = 0;

	ret = sscanf(buf, "reg=0x%x, mask=0x%x, val=0x%x",&reg,&mask,&val);
	if (ret < 0) {
		printk("check your input!!!\n");
		goto out_input;
	}

	if (reg > REG_MAX) {
		printk("Invalid argument!!!\n");
		goto out_input;
	}

	LM36923_DEBUG("%s:reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(pchip->regmap, reg, mask, val);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}
static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR, lm36923_reg_show, lm36923_reg_store);

static const struct regmap_config lm36923_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};

/* pointers to created device attributes */
static struct attribute *lm36923_attributes[] = {
	&dev_attr_bled_mode.attr,
	&dev_attr_reg_bl.attr,
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group lm36923_group = {
	.attrs = lm36923_attributes,
};

static int lm36923_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = client->adapter;
	struct lm36923_chip_data *pchip;
	int ret = -1;
	#ifdef LM36923_BL_POWER_TEST_ONLY_FOR_KERNEL
	int bl_en_gpio = LM36923_BL_HWEN_GPIO, rc;
	#endif

//#if defined (CONFIG_HUAWEI_DSM)
#if 0
	unsigned int val = 0;
#endif

	LM36923_INFO("in! \n");

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c functionality check fail.\n");
		return -EOPNOTSUPP;
	}

	pchip = devm_kzalloc(&client->dev,
				sizeof(struct lm36923_chip_data), GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

	#ifdef LM36923_BL_POWER_TEST_ONLY_FOR_KERNEL
	LM36923_INFO("probe pull up the HWEN gpio%d for kernel test\n",bl_en_gpio);
	rc = gpio_request(bl_en_gpio, "bl_en_gpio");
	if (rc < 0) {
		LM36923_ERR("%s %d:gpio_request fail, gpio = %d,rc = %d\n", __func__, __LINE__, bl_en_gpio, rc);
		gpio_free(bl_en_gpio);
	} else {
		rc = gpio_direction_output(bl_en_gpio, 1);
		if (rc < 0) {
			LM36923_ERR("%s: Fail set output gpio=%d\n",	__func__, bl_en_gpio);
		}
		msleep(60);
		gpio_free(bl_en_gpio);
	}
	#endif

#ifdef CONFIG_REGMAP_I2C
	pchip->regmap = devm_regmap_init_i2c(client, &lm36923_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		dev_err(&client->dev, "fail : allocate register map: %d\n", ret);
		goto err_out;
	}
#endif
	pchip->client = client;
	i2c_set_clientdata(client, pchip);

	/* chip initialize */
	ret = lm36923_chip_init(pchip);
	if (ret < 0) {
		dev_err(&client->dev, "fail : chip init\n");
		goto err_out;
	}

//#if defined (CONFIG_HUAWEI_DSM)
#if 0
	ret = regmap_read(pchip->regmap, REG_FAULT_FLAG, &val);
	if (ret < 0) {
		dev_err(&client->dev, "fail : read chip reg REG_FAULT_FLAG error!\n");
		goto err_out;
	}

	if (0 != val) {
		ret = dsm_client_ocuppy(lcd_dclient);
		if (!ret) {
			dev_err(&client->dev, "fail : REG_FAULT_FLAG statues error 0X1F=%d!\n", val);
			dsm_client_record(lcd_dclient, "REG_FAULT_FLAG statues error 0X1F=%d!\n", val);
			dsm_client_notify(lcd_dclient, DSM_LCD_OVP_ERROR_NO);
        } else {
			dev_err(&client->dev, "dsm_client_ocuppy fail:  ret=%d!\n", ret);
		}
	}
#endif

	pchip->dev = device_create(lm36923_class, NULL, 0, "%s", client->name);
	if (IS_ERR(pchip->dev)) {
		/* Not fatal */
		LM36923_ERR("Unable to create device; errno = %ld\n", PTR_ERR(pchip->dev));
		pchip->dev = NULL;
	} else {
		dev_set_drvdata(pchip->dev, pchip);
		ret = sysfs_create_group(&pchip->dev->kobj, &lm36923_group);
		if (ret)
			goto err_sysfs;
	}

	g_pchip = pchip;

	lm36923_bl_is_used = 1;
	app_info_set("backlight", "lm36923");

	LM36923_INFO("name: %s, address: (0x%x) ok!\n", client->name, client->addr);

	return ret;

err_sysfs:
	device_destroy(lm36923_class, 0);
err_out:
	devm_kfree(&client->dev, pchip);
	return ret;
}

static int lm36923_remove(struct i2c_client *client)
{
	struct lm36923_chip_data *pchip = i2c_get_clientdata(client);

	regmap_write(pchip->regmap, REG_ENABLE, 0x00);

	sysfs_remove_group(&client->dev.kobj, &lm36923_group);

	return 0;
}

static const struct i2c_device_id lm36923_id[] = {
	{LM36923_NAME, 0},
	{}
};
static const struct of_device_id lm36923_of_id_table[] = {
	{.compatible = "ti,lm36923"},
	{},
};

MODULE_DEVICE_TABLE(i2c, lm36923_id);
static struct i2c_driver lm36923_i2c_driver = {
		.driver = {
			.name = "lm36923",
			.owner = THIS_MODULE,
			.of_match_table = lm36923_of_id_table,
		},
		.probe = lm36923_probe,
		.remove = lm36923_remove,
		.id_table = lm36923_id,
};

static int __init lm36923_module_init(void)
{
	int ret = -1;

	LM36923_INFO("in!\n");
	lm36923_bl_is_used = 0;

	lm36923_class = class_create(THIS_MODULE, "lm36923");
	if (IS_ERR(lm36923_class)) {
		LM36923_ERR("Unable to create lm36923 class; errno = %ld\n", PTR_ERR(lm36923_class));
		lm36923_class = NULL;
	}

	ret = i2c_add_driver(&lm36923_i2c_driver);
	if (ret)
		LM36923_ERR("Unable to register lm36923 driver\n");

	LM36923_INFO("ok!\n ");

	return ret;
}
module_init(lm36923_module_init);

MODULE_DESCRIPTION("Texas Instruments Backlight driver for LM36923");
MODULE_AUTHOR("Daniel Jeong <daniel.jeong@ti.com>");
MODULE_AUTHOR("G.Shark Jeong <gshark.jeong@gmail.com>");
MODULE_LICENSE("GPL v2");


