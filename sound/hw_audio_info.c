/* Copyright (c) 2014-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <sound/soc.h>
#include <linux/regulator/consumer.h>
#include <sound/hw_audio_info.h>
#ifdef CONFIG_HUAWEI_DSM
static struct dsm_dev audio_dsm_info = {
	.name = DSM_AUDIO_MOD_NAME,
	.fops = NULL,
	.buff_size = DSM_AUDIO_BUF_SIZE,
};
static struct dsm_client *audio_dclient = NULL;
#endif

/* Audio property is an unsigned 32-bit integer stored in the variable of
audio_property.The meaning of audio_property is defined as following MACROs
 in groups of 4 bits */

/* Bit4 ~ bit7:
   Actually existing mics on the phone, it's NOT relevant to fluence. Using
   one bit to denote the existence of one kind of mic, possible mics are:
     master mic: the basic mic used for call and record, if it doesn't exist
                 means the config or software is wrong.
     secondary mic: auxiliary mic, usually used for fluence or paired with
                    speaker in handsfree mode, it's possible that this mic
                    exist but fluence isn't enabled.
     error mic: used in handset ANC. */
#define AUDIO_PROP_MASTER_MIC_EXIST_NODE    "builtin-master-mic-exist"
#define AUDIO_PROP_SECONDARY_MIC_EXIST_NODE "builtin-second-mic-exist"
#define AUDIO_PROP_ERROR_MIC_EXIST_NODE     "builtin-error-mic-exist"
#define AUDIO_PROP_MASTER_MIC_EXIST_MASK    0x00000010
#define AUDIO_PROP_SECONDARY_MIC_EXIST_MASK 0x00000020
#define AUDIO_PROP_ERROR_MIC_EXIST_MASK     0x00000040

/* Bit12 ~ bit15:
   Denote which mic would be used in hand held mode, please add as needed */
#define AUDIO_PROP_HANDHELD_MASTER_MIC_NODE "hand_held_master_mic_strategy"
#define AUDIO_PROP_HANDHELD_DUAL_MIC_NODE   "hand_held_dual_mic_strategy"
#define AUDIO_PROP_HANDHELD_AANC_MIC_NODE   "hand_held_aanc_mic_strategy"
#define AUDIO_PROP_HANDHELD_MASTER_MIC      0x00001000
#define AUDIO_PROP_HANDHELD_DUAL_MIC        0x00002000
#define AUDIO_PROP_HANDHELD_AANC_MIC        0x00004000

#define PRODUCT_IDENTIFIER_NODE             "product-identifier"
#define PRODUCT_IDENTIFIER_BUFF_SIZE        64
#define AUD_PARAM_VER_NODE                  "aud_param_ver"

#define SPEAKER_PA_NODE                     "speaker-pa"
#define SMARTPA_NUM                         "smartpa-num"
#define SMARTPA_ALGO_PARAMSIZE              "smartpa-algo-paramsize"
#define VOICE_LVM_MODE                      "voice-lvm-mode"

#define SPEAKER_BOX_NODE                    "speaker-box"
#define SPEAKER_BOX_GPIO                    "spk-box-id"
#define READ_SPK_ID_TYPE                    "spk-box-read-direct"
#define PINCTRL_DEFAULT_STATE               "box_default"
#define PINCTRL_SLEEP_STATE                 "box_sleep"
#define AUDIO_DTS_BUFF_SIZE                 32
#define AUDIO_BOX_NAME_SIZE                 128

#define BOXID_DEV_ENABLE                    "boxID-dev-enabled"
#define AUDIO_CE_PARAM_INSIDE               "audio-ce-param-inside"
#define SPEAKER_TEST_CONTENT                "speaker_test_content"
#define MIC1_DIFF_MODE_ENABLE              "mic1-differential-mode-enabled"

#define MAX_SPK_BOX_COUNT                   6
#define PIN_VOLTAGE_NONE                    (-1)
#define PIN_VOLTAGE_LOW                     0
#define PIN_VOLTAGE_HIGH                    1
#define PIN_VOLTAGE_FLOAT                   2
/*add nrec function source */
#define AUDIO_PROP_BTSCO_NREC_ADAPT_MASK    0xf0000000
#define AUDIO_PROP_BTSCO_NREC_ADAPT_OFF     0x10000000
#define AUDIO_PROP_BTSCO_NREC_ADAPT_ON      0x20000000

#define PRODUCT_NERC_ADAPT_CONFIG           "product-btsco-nrec-adapt"
#define RPODUCT_STERO_SMARTPA_CONFIG        "dual_smartpa_support"
/* Bit16 ~ bit19:
   Denote which mic would be used in loud speaker mode, please add as needed */
#define AUDIO_LOUDSPEAKER_MASTER_MIC_NODE  "loud_speaker_master_mic_strategy"
#define AUDIO_LOUDSPEAKER_SECOND_MIC_NODE  "loud_speaker_second_mic_strategy"
#define AUDIO_LOUDSPEAKER_ERROR_MIC_NODE   "loud_speaker_error_mic_strategy"
#define AUDIO_LOUDSPEAKER_DUAL_MIC_NODE    "loud_speaker_dual_mic_strategy"
#define AUDIO_LOUDSPEAKER_MASTER_MIC       0x00010000
#define AUDIO_LOUDSPEAKER_SECOND_MIC       0x00020000
#define AUDIO_LOUDSPEAKER_ERROR_MIC        0x00040000
#define AUDIO_LOUDSPEAKER_DUAL_MIC         0x00080000
#define SMARTPA_ALGO_PARAMSIZE_DEFAULT     4096

#ifdef CONFIG_SND_SOC_VRM
#define VRM_ARRAY_LEN       116
#define PAYLOAD_START       100
#define VRM_DIR_WRITE       1
#define VRM_DIR_READ         0
#define VRM_SIZE_RW          (16 * sizeof(int))

extern int afe_vrm_param_ctrl(uint32_t dir, uint32_t size, uint8_t *payload);

static int param_array[VRM_ARRAY_LEN];
static uint8_t *payload = (uint8_t *)&param_array[PAYLOAD_START];
#endif // CONFIG_SND_SOC_VRM

/* speaker box name, when use different just modif here */
static char speaker_box_name[MAX_SPK_BOX_COUNT][AUDIO_DTS_BUFF_SIZE];
/**
* audio_property        Product specified audio properties
* product_identifier    means to use which acdb files
* speaker_pa            means which smartpa used
* aud_param_ver         means acdb files version, this maybe not used later
* speaker_box_id        means speaker box id name
* audio_stero_smartpa   means use one or two smartpa
* boxID_dev_enabled     means use dev/deviceboxID
* speaker_test_content  means speaker box connected
* smartpa_num           means speaker smartpa number
* smartpa_algo_paramsize means smartpa algorithm parameters size
*/
static unsigned int audio_property;
static char product_identifier[PRODUCT_IDENTIFIER_BUFF_SIZE] = "default";
static char speaker_pa[AUDIO_DTS_BUFF_SIZE] = "none";
static char voice_lvm_mode[AUDIO_DTS_BUFF_SIZE] = "0";
static char aud_param_ver[AUDIO_DTS_BUFF_SIZE] = "default";
static char speaker_box_id[AUDIO_DTS_BUFF_SIZE] = "none";
static unsigned int audio_stero_smartpa;
static unsigned int boxID_dev_enabled = 0;
static unsigned int audio_ce_param_inside = 0;
static unsigned int smartpa_num;
static unsigned int smartpa_algo_paramsize;
static char speaker_test_content[AUDIO_BOX_NAME_SIZE] = "speaker";
static bool mic1_differential_mode_enabled = false;

static ssize_t audio_property_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%08X\n", audio_property);
}
DRIVER_ATTR(audio_property, 0444, audio_property_show, NULL);

static ssize_t product_identifier_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%s", product_identifier);
}
DRIVER_ATTR(product_identifier, 0444, product_identifier_show, NULL);

ssize_t get_product_identifier(char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, sizeof(product_identifier), "%s", product_identifier);
}
EXPORT_SYMBOL(get_product_identifier);

static ssize_t speaker_pa_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%s", speaker_pa);
}
DRIVER_ATTR(speaker_pa, 0444, speaker_pa_show, NULL);

static ssize_t voice_lvm_mode_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%s", voice_lvm_mode);
}
DRIVER_ATTR(voice_lvm_mode, 0444, voice_lvm_mode_show, NULL);

static ssize_t smartpa_num_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%u", smartpa_num);
}
DRIVER_ATTR(smartpa_num, 0444, smartpa_num_show, NULL);

static ssize_t box_id_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%s", speaker_box_id);
}
DRIVER_ATTR(speaker_box_id, 0444, box_id_show, NULL);

ssize_t get_speaker_box_id(char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, sizeof(speaker_box_id), "%s", speaker_box_id);
}
EXPORT_SYMBOL(get_speaker_box_id);

static ssize_t audiopara_version_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%s\n", aud_param_ver);
}
DRIVER_ATTR(aud_param_ver, 0444, audiopara_version_show, NULL);

static ssize_t audio_stero_smartpa_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%d", audio_stero_smartpa);
}
DRIVER_ATTR(stero_smartpa, 0444, audio_stero_smartpa_show, NULL);

#ifdef CONFIG_SND_SOC_VRM
#define VRM_APRv2_RSV_CODE 	200

static ssize_t vrm_data_show(struct device_driver *driver, char *buf)
{
	uint32_t *result_payload = NULL;

	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	(void)afe_vrm_param_ctrl(VRM_DIR_READ, VRM_SIZE_RW, payload);

	result_payload = (uint32_t*) payload;

	return sprintf(buf, "MIC0:%d, MIC1:%d, VPTX:%d, VENC:%d, VDEC:%d, VPRX:%d, RCV:%d, ERR:0x%x", 
				result_payload[0], result_payload[1], result_payload[2], result_payload[3], 
				result_payload[4], result_payload[5], result_payload[6], result_payload[7]);
}

static ssize_t vrm_data_store(struct device_driver *drv, const char *buf, size_t count)
{
	unsigned long nstate = 0;
	int *param = NULL;

	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}

	if (count > 0)
	{
		while (' ' == *buf)
		buf++;

		if (0 == strncmp(buf, "on", 2)) {
			nstate = 1;
		}
		else if (0 == strncmp(buf, "off", 3)) {
			nstate = 0;
		}
		else {
			pr_err("%s, invalid param\n", __func__);
			return count;
		}

		param = (int *) payload;
		param[0] = nstate;
		param[1] = VRM_APRv2_RSV_CODE;

		(void)afe_vrm_param_ctrl(VRM_DIR_WRITE, 16, (uint8_t *)param);
	}
	return count;
}
DRIVER_ATTR(vrm_data, 0664, vrm_data_show, vrm_data_store);
#endif //CONFIG_SND_SOC_VRM

static ssize_t audio_boxID_enabled_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%u", boxID_dev_enabled);
}
DRIVER_ATTR(boxID_dev_enabled, 0444, audio_boxID_enabled_show, NULL);

static ssize_t audio_ce_param_inside_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%u", audio_ce_param_inside);
}
DRIVER_ATTR(audio_ce_param_inside, 0444, audio_ce_param_inside_show, NULL);

static ssize_t audio_speaker_test_content_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%s", speaker_test_content);
}
DRIVER_ATTR(speaker_test_content, 0444, audio_speaker_test_content_show, NULL);

static ssize_t audio_smartpa_algo_paramsize_show(struct device_driver *driver, char *buf)
{
	if (NULL == buf) {
		pr_err("%s: buf is null", __func__);
		return 0;
	}
	return scnprintf(buf, PAGE_SIZE, "%u", smartpa_algo_paramsize);
}
DRIVER_ATTR(smartpa_algo_paramsize, 0444, audio_smartpa_algo_paramsize_show, NULL);

static struct snd_soc_codec *registered_codec;

void hw_get_registered_codec(struct snd_soc_codec *codec)
{
	registered_codec = codec;
}
EXPORT_SYMBOL_GPL(hw_get_registered_codec);

/* the string length of reg dump line
	 4 for codec register name
	 2 for ': '
	 2 for codec register value
	 1 for '\n'
*/
#define WCD_CDC_DUMP_LENGTH 9
/* the size of reg dump line */
#define WCD_CDC_DUMP_BUF_SIZE (WCD_CDC_DUMP_LENGTH + 1)
/* word size of codec reg name length */
#define WCD_CDC_DUMP_WORD_SIZE 4
/* register size of codec */
#define WCD_CDC_DUMP_REG_SIZE 2

static ssize_t audio_codec_dump_show(__attribute__((unused))struct device_driver *driver, char *buf)
{
	int i = 0;
	int len = 0;
	int ret = 0;
	size_t total = 0;
	/* the max size of the codec dump */
	const size_t count = PAGE_SIZE -1;
	/* the size of the codec buffer */
	char tmpbuf[WCD_CDC_DUMP_BUF_SIZE];
	/* the string size of regsiter */
	char regbuf[WCD_CDC_DUMP_REG_SIZE + 1];

	struct snd_soc_codec *codec = registered_codec;

	len = WCD_CDC_DUMP_LENGTH;

	if(NULL == buf) {
		pr_err("%s: buf is NULL.\n", __func__);
		return 0;
	}

	if(NULL == codec) {
		pr_err("%s: codec is NULL.\n", __func__);
		return 0;
	}

	for (i = 0; i < codec->driver->reg_cache_size; i ++) {
		if (!codec->readable_register(codec, i))
			continue;

		if (total + len >= count)
			break;

		ret = codec->driver->read(codec, i);
		if (ret < 0) {
			memset(regbuf, 'X', WCD_CDC_DUMP_REG_SIZE);
			regbuf[WCD_CDC_DUMP_REG_SIZE] = '\0';
		} else {
			snprintf(regbuf, WCD_CDC_DUMP_REG_SIZE + 1, "%.*x", WCD_CDC_DUMP_REG_SIZE, ret);
		}

		/* prepare the buffer */
		snprintf(tmpbuf, len + 1, "%.*x: %s\n", WCD_CDC_DUMP_WORD_SIZE, i, regbuf);
		/* copy it back to the caller without the '\0' */
		memcpy(buf + total, tmpbuf, len);

		total += len;
	}

	total = min(total, count);

	return total;
}
DRIVER_ATTR(codec_dump, 0444, audio_codec_dump_show, NULL);

static struct attribute *audio_attrs[] = {
	&driver_attr_audio_property.attr,
	&driver_attr_product_identifier.attr,
	&driver_attr_aud_param_ver.attr,
	&driver_attr_speaker_pa.attr,
	&driver_attr_smartpa_num.attr,
	&driver_attr_stero_smartpa.attr,
	&driver_attr_speaker_box_id.attr,
	&driver_attr_codec_dump.attr,
	&driver_attr_boxID_dev_enabled.attr,
	&driver_attr_audio_ce_param_inside.attr,
	&driver_attr_speaker_test_content.attr,
#ifdef CONFIG_SND_SOC_VRM
	&driver_attr_vrm_data.attr,
#endif // CONFIG_SND_SOC_VRM
	&driver_attr_smartpa_algo_paramsize.attr,
	&driver_attr_voice_lvm_mode.attr,
	NULL,
};

static struct attribute_group audio_group = {
	.name = "hw_audio_info",
	.attrs = audio_attrs,
};

static const struct attribute_group *groups[] = {
	&audio_group,
	NULL,
};

static struct of_device_id audio_info_match_table[] = {
	{ .compatible = "hw,hw_audio_info",},
	{ },
};
/*
*  Function   : read_gpio_according_state
*  Description: according to pinctrl state to set, and then get specific gpio value
*  Input      : pinctrl ------- system pinctrl
*               statename------ pinctrl state name
*               gpio_no-------- specific gpio no
*  return val : gpio value
*/
int read_gpio_according_state(struct pinctrl *pinctrl,
							 const char *statename,
							 int gpio_no)
{
	struct pinctrl_state *pin_state = NULL;

	if (NULL == pinctrl || NULL == statename) {
		pr_err("%s: pinctrl or statename is null\n", __func__);
		return PIN_VOLTAGE_NONE;
	}
	/* use pinctrl function to get pinctrl state related to gpio */
	pin_state = pinctrl_lookup_state(pinctrl, statename);
	if (IS_ERR(pin_state)) {
		pr_err("%s: Unable to get pinctrl %s state handle\n", __func__, statename);
		return PIN_VOLTAGE_NONE;
	}

	pinctrl_select_state(pinctrl, pin_state);
	if (!gpio_is_valid(gpio_no)) {
		return PIN_VOLTAGE_NONE;
	} else {
		udelay(10);
		gpio_direction_input(gpio_no);
		return gpio_get_value(gpio_no);
	}
}

/*
*  Function   : get_pa_box_id
*  Description: get gpio 3 state from one GPIO to identify 3 speaker box
*  Input      : pdev platform pdev parameters ,use to get dtsi property and pinctrl
*  return val : gpio state
*/
int get_pa_box_id(struct platform_device *pdev)
{
	struct pinctrl *pinctrl = NULL;
	int gpio_box_id = 0;
	int ret = 0;
	int box_value_pull_up = -1;
	int box_value_pull_down = -1;

	if (NULL == pdev) {
		pr_err("%s: pdev is null\n", __func__);
		return PIN_VOLTAGE_NONE;
	}
	pinctrl = devm_pinctrl_get(&pdev->dev);
	if (NULL == pinctrl) {
		pr_err("%s: pinctrl get failed\n", __func__);
		return PIN_VOLTAGE_NONE;
	}
	/* get gpio from dtsi and used it to get state*/
	gpio_box_id = of_get_named_gpio(pdev->dev.of_node, SPEAKER_BOX_GPIO, 0);
	if (gpio_box_id < 0) {
		pr_err("%s: failed to get gpio_box_id\n", __func__);
		return PIN_VOLTAGE_NONE;
	}
	ret = gpio_request(gpio_box_id, SPEAKER_BOX_GPIO);
	if (ret) {
		pr_err("%s: unable to request gpio_box_id\n", __func__);
		return PIN_VOLTAGE_NONE;
	}

	/* set pinctrl to different state and get gpio value */
	box_value_pull_up = read_gpio_according_state(pinctrl,
										 PINCTRL_DEFAULT_STATE, gpio_box_id);
	box_value_pull_down = read_gpio_according_state(pinctrl,
										 PINCTRL_SLEEP_STATE, gpio_box_id);
	pr_info("%s: box up = %d, box down = %d\n",
						 __func__, box_value_pull_up, box_value_pull_down);
	gpio_free(gpio_box_id);

	if (of_property_read_bool(pdev->dev.of_node, READ_SPK_ID_TYPE))
		return box_value_pull_up;

	/* compare gpio value to identify 3 state */
	if(box_value_pull_up == box_value_pull_down) {
		if (box_value_pull_up < 0)
			return PIN_VOLTAGE_NONE;

		if(PIN_VOLTAGE_HIGH == box_value_pull_down) {
			/* set pull up when pin is high */
			read_gpio_according_state(pinctrl, PINCTRL_DEFAULT_STATE, -1);
			return PIN_VOLTAGE_HIGH;
		} else {
			/* set pull down when pin is low */
			read_gpio_according_state(pinctrl, PINCTRL_SLEEP_STATE, -1);
			return PIN_VOLTAGE_LOW;
		}
	} else {
		/* pin is float */
		return PIN_VOLTAGE_FLOAT;
	}
}

#ifdef CONFIG_HUAWEI_DSM
int audio_dsm_register(void)
{
	if (NULL != audio_dclient)
		return 0;

	audio_dclient = dsm_register_client(&audio_dsm_info);
	if (NULL == audio_dclient)
		pr_err("audio_dclient register failed!\n");

	return 0;
}

int audio_dsm_report_num(int error_no, unsigned int mesg_no)
{
	int err = 0;

	if (NULL == audio_dclient) {
		pr_err("%s: audio_dclient did not register!\n", __func__);
		return 0;
	}

	err = dsm_client_ocuppy(audio_dclient);
	if (0 != err) {
		pr_err("%s: user buffer is busy!\n", __func__);
		return 0;
	}

	pr_info("%s: after dsm_client_ocuppy, error_no=0x%x, mesg_no=0x%x!\n",
		__func__, error_no, mesg_no);
	dsm_client_record(audio_dclient, "Message code = 0x%x.\n", mesg_no);
	dsm_client_notify(audio_dclient, error_no);

	return 0;
}

int audio_dsm_report_info(int error_no, char *fmt, ...)
{
	int err = 0;
	int ret = 0;
	char dsm_report_buffer[DSM_REPORT_BUF_SIZE] = {0};
	va_list args;

	if (NULL == audio_dclient) {
		pr_err("%s: audio_dclient did not register!\n", __func__);
		return 0;
	}

	if (error_no < DSM_AUDIO_ERROR_NUM) {
		pr_err("%s: input error_no err!\n", __func__);
		return 0;
	}

	va_start(args, fmt);
	ret = vsnprintf(dsm_report_buffer, DSM_REPORT_BUF_SIZE, fmt, args);
	va_end(args);

	err = dsm_client_ocuppy(audio_dclient);
	if (0 != err) {
		pr_err("%s: user buffer is busy!\n", __func__);
		return 0;
	}

	pr_info("%s: after dsm_client_ocuppy, dsm_error_no = %d, %s\n",
			__func__, error_no, dsm_report_buffer);
	dsm_client_record(audio_dclient, "%s\n", dsm_report_buffer);
	dsm_client_notify(audio_dclient, error_no);

	return 0;
}
#else
int audio_dsm_register(void)
{
	return 0;
}
int audio_dsm_report_num(int error_no, unsigned int mesg_no)
{
	return 0;
}

int audio_dsm_report_info(int error_no, char *fmt, ...)
{
	return 0;
}
#endif

bool mic1_differential_mode_enable(void)
{
	return mic1_differential_mode_enabled;
}

static int audio_info_probe(struct platform_device *pdev)
{
	int ret = 0;
	const char *string = NULL;
	int strcount = 0;
	int i = 0;
	char *ptr = NULL;

	if (NULL == pdev) {
		pr_err("hw_audio: audio_info_probe failed, pdev is NULL\n");
		return 0;
	}

	if (NULL == pdev->dev.of_node) {
		pr_err("hw_audio: audio_info_probe failed, of_node is NULL\n");
		return 0;
	}

	audio_dsm_register();
	if (of_property_read_bool(pdev->dev.of_node,
								 AUDIO_PROP_MASTER_MIC_EXIST_NODE))
		audio_property |= AUDIO_PROP_MASTER_MIC_EXIST_MASK;
	else {
		pr_err("hw_audio: check mic config, no master mic found\n");
		audio_dsm_report_info(DSM_AUDIO_CARD_LOAD_FAIL_ERROR_NO, "master mic not found!");
	}

	if (of_property_read_bool(pdev->dev.of_node,
						 AUDIO_PROP_SECONDARY_MIC_EXIST_NODE))
		audio_property |= AUDIO_PROP_SECONDARY_MIC_EXIST_MASK;
	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_PROP_ERROR_MIC_EXIST_NODE))
		audio_property |= AUDIO_PROP_ERROR_MIC_EXIST_MASK;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_PROP_HANDHELD_MASTER_MIC_NODE))
		audio_property |= AUDIO_PROP_HANDHELD_MASTER_MIC;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_PROP_HANDHELD_DUAL_MIC_NODE))
		audio_property |= AUDIO_PROP_HANDHELD_DUAL_MIC;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_PROP_HANDHELD_AANC_MIC_NODE))
		audio_property |= AUDIO_PROP_HANDHELD_AANC_MIC;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_LOUDSPEAKER_MASTER_MIC_NODE))
		audio_property |= AUDIO_LOUDSPEAKER_MASTER_MIC;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_LOUDSPEAKER_SECOND_MIC_NODE))
		audio_property |= AUDIO_LOUDSPEAKER_SECOND_MIC;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_LOUDSPEAKER_ERROR_MIC_NODE))
		audio_property |= AUDIO_LOUDSPEAKER_ERROR_MIC;

	if (of_property_read_bool(pdev->dev.of_node,
									 AUDIO_LOUDSPEAKER_DUAL_MIC_NODE))
		audio_property |= AUDIO_LOUDSPEAKER_DUAL_MIC;

	if (of_property_read_bool(pdev->dev.of_node, RPODUCT_STERO_SMARTPA_CONFIG))
		audio_stero_smartpa = 1;

	if (of_property_read_bool(pdev->dev.of_node, BOXID_DEV_ENABLE))
		boxID_dev_enabled = 1;

	if (of_property_read_bool(pdev->dev.of_node, AUDIO_CE_PARAM_INSIDE))
		audio_ce_param_inside = 1;			/* different ce parameter is needed*/

	if (of_property_read_bool(pdev->dev.of_node, MIC1_DIFF_MODE_ENABLE))
		mic1_differential_mode_enabled = true;		/* enable mic1 differential mode*/

	ret = of_property_read_u32(pdev->dev.of_node, SMARTPA_ALGO_PARAMSIZE,
			&smartpa_algo_paramsize);
	if (ret) {
		pr_err("hw_audio: read %s failed %d\n", SMARTPA_ALGO_PARAMSIZE, ret);
		smartpa_algo_paramsize = SMARTPA_ALGO_PARAMSIZE_DEFAULT;
	}

	string = NULL;
	ret = of_property_read_string(pdev->dev.of_node,
										 PRODUCT_IDENTIFIER_NODE, &string);
	if (ret || (NULL == string)) {
		pr_err("hw_audio: read_string product-identifier failed %d\n", ret);
	} else {
		memset(product_identifier, 0, sizeof(product_identifier));
		strlcpy(product_identifier, string, sizeof(product_identifier));
	}

    string = NULL;
	ret = of_property_read_string(pdev->dev.of_node, VOICE_LVM_MODE, &string);
	if (ret || (NULL == string)) {
		pr_err("hw_audio: read_string voice-lvm-mode failed %d\n", ret);
	} else {
		memset(voice_lvm_mode, 0, sizeof(voice_lvm_mode));
		strlcpy(voice_lvm_mode, string, sizeof(voice_lvm_mode));
		pr_info("%s: getted voice_lvm_mode = %s", __func__, voice_lvm_mode);
	}
	
	string = NULL;
	ret = of_property_read_string(pdev->dev.of_node, SPEAKER_PA_NODE, &string);
	if (ret || (NULL == string)) {
		pr_err("hw_audio: read_string speaker-pa failed %d\n", ret);
	} else {
		memset(speaker_pa, 0, sizeof(speaker_pa));
		strlcpy(speaker_pa, string, sizeof(speaker_pa));
		pr_info("%s: getted pa type = %s", __func__, speaker_pa);
	}

	ret = of_property_read_u32(pdev->dev.of_node, SMARTPA_NUM, &smartpa_num);
	if (ret) {
		pr_err("hw_audio: read %s failed %d\n", SMARTPA_NUM, ret);
		smartpa_num = 0;
	}

	string = NULL;
	strcount = of_property_count_strings(pdev->dev.of_node, SPEAKER_BOX_NODE);
	if(strcount <= 0) {
		pr_err("hw_audio: of_property_count_strings speaker-box failed %d\n", strcount);
	} else {
		/*according to count got from of_property_count_strings we get each string from
		* the dtsi node, and store it to array speaker_box_name, thus when speaker box 
		* changed we just modify dtsi not need to change code
		*
		* NOTICE: this just can support max 5 types(now value is 6, one for type none),
		* if more than this, you must modify the array size that means you can modify
		* macro MAX_SPK_BOX_COUNT
		*/
		for (i = 0; i < strcount; i++) {
			ret = of_property_read_string_index(pdev->dev.of_node,
												 SPEAKER_BOX_NODE, i, &string);
			if (ret) {
				pr_err("%s: hw_audio: get speaker box name%d failed\n", __func__, i);
			} else {
				ret = strlen(string) + 1;
				strlcpy(speaker_box_name[i], string,
						ret > AUDIO_DTS_BUFF_SIZE?AUDIO_DTS_BUFF_SIZE:ret);
			}
			string = NULL;
		}
		/* get_pa_box_id now just can support 3 types, if we need support more types,
		*  more gpio needed also, and this function will be changed and return more values
		*/
		ret = get_pa_box_id(pdev);
		/*modify ret value begin from 0 and to use it to get speaker box name*/
		ret++;
		pr_info("hw_audio: get spk box index = %d\n", ret);
		/* if ret value is valid , then copy name to buffer */
		if (ret < strcount && ret >= 0) {
			memset(speaker_box_id, 0, sizeof(speaker_box_id));
			strlcpy(speaker_box_id, speaker_box_name[ret], sizeof(speaker_box_id));
		} else {
			pr_err("hw_audio: get spk box index error, set to none\n");
		}
	}
	if (false == of_property_read_bool(pdev->dev.of_node,
												 PRODUCT_NERC_ADAPT_CONFIG)) {
		pr_err("hw_audio: read_bool NERC_ADAPT_CONFIG failed %d\n", ret);
		audio_property |=
		  (AUDIO_PROP_BTSCO_NREC_ADAPT_OFF & AUDIO_PROP_BTSCO_NREC_ADAPT_MASK);
	} else {
		audio_property |=
		  (AUDIO_PROP_BTSCO_NREC_ADAPT_ON & AUDIO_PROP_BTSCO_NREC_ADAPT_MASK);
	}

	string = NULL;
	ret = of_property_read_string(pdev->dev.of_node, AUD_PARAM_VER_NODE,
									 &string);
	if (ret || (NULL == string)) {
		pr_err("hw_audio: read_string aud_param_ver failed %d\n", ret);
	} else {
		memset(aud_param_ver, 0, sizeof(aud_param_ver));
		strlcpy(aud_param_ver, string, sizeof(aud_param_ver));
	}
	strcount = of_property_count_strings(pdev->dev.of_node, SPEAKER_TEST_CONTENT);
	if (strcount <= 0) {
		pr_err("%s: missing %s in dt node or length is incorrect, set to default \n", __func__, SPEAKER_TEST_CONTENT);
		strlcpy(speaker_test_content, "speaker", strlen("speaker")+1);
	} else {
		string = NULL;
		ptr = speaker_test_content;
		memset(speaker_test_content, 0, AUDIO_BOX_NAME_SIZE);
		for (i = 0; i < strcount; i++) {
			ret = of_property_read_string_index(pdev->dev.of_node,
					SPEAKER_TEST_CONTENT, i, &string);
			if (ret) {
				pr_err("%s:of read string %s i %d error %d\n",
						__func__, SPEAKER_TEST_CONTENT, i, ret);
			} else {
				memcpy(ptr, string, strlen(string));
				ptr += strlen(string);
				*(ptr++) = ',';
			}
			string = NULL;
		}
		if (ptr != speaker_test_content)
			*(--ptr) = '\0';
	}
	return 0;
}

static struct platform_driver audio_info_driver = {
	.driver = {
		.name  = "hw_audio_info",
		.owner  = THIS_MODULE,
		.groups = groups,
		.of_match_table = audio_info_match_table,
	},

	.probe = audio_info_probe,
	.remove = NULL,
};

static int __init audio_info_init(void)
{
	return platform_driver_register(&audio_info_driver);
}

static void __exit audio_info_exit(void)
{
	platform_driver_unregister(&audio_info_driver);
}

module_init(audio_info_init);
module_exit(audio_info_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hw audio info");

