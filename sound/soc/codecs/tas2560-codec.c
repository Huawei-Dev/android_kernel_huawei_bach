/*
** =============================================================================
** Copyright (c) 2016  Texas Instruments Inc.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; version 2.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
** Street, Fifth Floor, Boston, MA 02110-1301, USA.
**
** File:
**     tas2560-codec.c
**
** Description:
**     ALSA SoC driver for Texas Instruments TAS2560 High Performance 4W Smart Amplifier
**
** =============================================================================
*/
#define DEBUG
#define pr_fmt(fmt) "%s: " fmt, __func__
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/firmware.h>
#include <linux/regmap.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>

#include "tas2560.h"
#include "tas2560-core.h"

static unsigned int tas2560_read(struct snd_soc_codec *codec,  unsigned int reg)
{
	struct tas2560_priv *pTAS2560 = snd_soc_codec_get_drvdata(codec);
	unsigned int value = 0;
	int ret;

	ret = pTAS2560->read(pTAS2560, reg, &value);
	if(ret >=0 )
		return value;
	else
		return ret;
}

static int tas2560_write(struct snd_soc_codec *codec,
		unsigned int reg, unsigned int value)
{
    struct tas2560_priv *pTAS2560 = snd_soc_codec_get_drvdata(codec);

	return pTAS2560->write(pTAS2560, reg, value);
}

static int tas2560_AIF_post_event(struct snd_soc_dapm_widget *w,
		struct snd_kcontrol *kcontrol, int event)
{
	struct tas2560_priv *pTAS2560 = NULL;
	int be_id = TAS2560_PRI_BE;

	if (w->name && !strncmp(w->name, "ASI2", strlen("ASI2")))
		be_id = TAS2560_SEC_BE;

	mutex_lock(&tas2560_codecs_lock);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
			if (pTAS2560->beid != be_id || pTAS2560->enabled == 0)
				continue;
			pr_info("tas %u power up\n", pTAS2560->type);
			tas2560_enable(pTAS2560, true);
		}
		break;
	case SND_SOC_DAPM_POST_PMD:
		list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
			if (pTAS2560->beid != be_id || pTAS2560->enabled == 0)
				continue;
			pr_info("tas %u power down\n", pTAS2560->type);
			tas2560_enable(pTAS2560, false);
		}
		break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	return 0;
}

static const struct snd_soc_dapm_widget tas2560_dapm_widgets[] =
{
	SND_SOC_DAPM_AIF_IN_E("ASI1", "ASI1 Playback", 0, SND_SOC_NOPM, 0, 0,
			      tas2560_AIF_post_event, SND_SOC_DAPM_POST_PMU |
			      SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_AIF_IN_E("ASI2", "ASI2 Playback", 1, SND_SOC_NOPM, 0, 0,
			      tas2560_AIF_post_event, SND_SOC_DAPM_POST_PMU |
			      SND_SOC_DAPM_POST_PMD),
	SND_SOC_DAPM_DAC("DAC", NULL, SND_SOC_NOPM, 0, 0),

	SND_SOC_DAPM_OUT_DRV("ClassD", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_SUPPLY("PLL", SND_SOC_NOPM, 0, 0, NULL, 0),

	SND_SOC_DAPM_OUTPUT("OUT")
};

static const struct snd_soc_dapm_route tas2560_audio_map[] = {
	{"DAC", NULL, "ASI1"},
	{"DAC", NULL, "ASI2"},
	{"ClassD", NULL, "DAC"},
	{"OUT", NULL, "ClassD"},
	{"DAC", NULL, "PLL"},
};

static int tas2560_startup(struct snd_pcm_substream *substream,
	struct snd_soc_dai *dai)
{
	struct tas2560_priv *pTAS2560 = NULL;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;

	if (!rtd)
		return 0;
	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (rtd->dai_link->be_id != pTAS2560->beid ||
			pTAS2560->enabled == 0)
			continue;
		pTAS2560->mnClkid = -1;
		pTAS2560->mnClkin = -1;
	}
	mutex_unlock(&tas2560_codecs_lock);

	return 0;
}

static int tas2560_mute(struct snd_soc_dai *dai, int mute)
{
	struct tas2560_priv *pTAS2560 = NULL;
	int be_id = TAS2560_PRI_BE;

	if (dai && dai->id == TAS2560_SEC_DAI)
		be_id = TAS2560_SEC_BE;

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->beid != be_id || pTAS2560->enabled == 0)
			continue;
		pr_debug("tas %u %s\n", pTAS2560->type, mute ? "mute" : "unmute");
		tas2560_set_mute(pTAS2560, mute);
	}
	mutex_unlock(&tas2560_codecs_lock);

	return 0;
}

static int tas2560_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

static struct snd_soc_dai_ops tas2560_dai_ops = {
	.startup = tas2560_startup,
	.digital_mute = tas2560_mute,
	.set_fmt = tas2560_set_dai_fmt,
};

static struct snd_soc_dai_driver tas2560_dai_driver[] = {
	{
		.name = "tas2560-asi-pri",
		.id = TAS2560_PRI_DAI,
		.playback = {
			.stream_name    = "ASI1 Playback",
			.channels_min   = 2, /* min channels */
			.channels_max   = 2, /* max channels */
			.rates      = SNDRV_PCM_RATE_8000_192000,
			.formats    = TAS2560_SUPPORTED_BITFMT,
		},
		.ops = &tas2560_dai_ops,
		.symmetric_rates = 1,
	},
	{
		.name = "tas2560-asi-sec",
		.id = TAS2560_SEC_DAI,
		.playback = {
			.stream_name    = "ASI2 Playback",
			.channels_min   = 2, /* min channels */
			.channels_max   = 2, /* max channels */
			.rates      = SNDRV_PCM_RATE_8000_192000,
			.formats    = TAS2560_SUPPORTED_BITFMT,
		},
		.ops = &tas2560_dai_ops,
		.symmetric_rates = 1,
	},
};

static int tas2560_codec_probe(struct snd_soc_codec *codec)
{
	snd_soc_dapm_ignore_suspend(&codec->dapm, "OUT");
	return 0;
}

static int tas2560_codec_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static int tas2560_boostload_get(struct snd_kcontrol *pKcontrol,
			struct snd_ctl_elem_value *pUcontrol)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	pUcontrol->value.integer.value[0] = pTAS2560->mnLoad;
	return 0;
}

static int tas2560_boostload_set(struct snd_kcontrol *pKcontrol,
			struct snd_ctl_elem_value *pUcontrol)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Boostload", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	pTAS2560->mnLoad = pUcontrol->value.integer.value[0];
	tas2560_setLoad(pTAS2560, pTAS2560->mnLoad);
	return 0;
}

static int tas2560_powerctl_get(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	pValue->value.integer.value[0] = pTAS2560->mbPowerUp;
	return 0;
}

static int tas2560_powerctl_put(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;
	int bPowerUp = pValue->value.integer.value[0];

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Powerctl", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	tas2560_enable(pTAS2560, bPowerUp);
	return 0;
}

static int tas2560_channel_get(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	pValue->value.integer.value[0] = pTAS2560->channel;
	return 0;
}

static int tas2560_channel_put(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type = 0;
	unsigned int channel = pValue->value.integer.value[0];

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Channel", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	if (channel == 0)
		tas2560_set_offset(pTAS2560, TAS2560_DSP_CHAN_OFFSET_L);
	else
		tas2560_set_offset(pTAS2560, TAS2560_DSP_CHAN_OFFSET_R);
	pTAS2560->channel = channel;
	return 0;
}

static int tas2560_mute_get(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	pValue->value.integer.value[0] = pTAS2560->mute;
	return 0;
}

static int tas2560_mute_put(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;
	unsigned int mute = pValue->value.integer.value[0];

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Mute", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	tas2560_set_mute(pTAS2560, mute);
	pTAS2560->mute = mute;
	return 0;
}

static int tas2560_enable_get(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}

	pValue->value.integer.value[0] = pTAS2560->enabled;
	return 0;
}

static int tas2560_enable_put(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	unsigned int type;
	unsigned int enable = pValue->value.integer.value[0];

	if (!strncmp(pKcontrol->id.name,
			"TAS2560 0 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 1 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_PRI_R;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 2 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_L;
	else if (!strncmp(pKcontrol->id.name,
			"TAS2560 3 Enable", strlen(pKcontrol->id.name)))
		type = TAS2560_SEC_R;
	else {
		pr_err("invalid control id !\n");
		return 0;
	}

	pr_debug("tas %u %s\n", type, enable ? "enable" : "disable");
	mutex_lock(&tas2560_codecs_lock);
	list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
		if (pTAS2560->type == type)
			break;
	}
	mutex_unlock(&tas2560_codecs_lock);

	if (!pTAS2560) {
		pr_warn("tas not found !\n");
		return 0;
	}
	pTAS2560->enabled = enable;
	return 0;
}

static int tas2560_regdump_get(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	pValue->value.integer.value[0] = 0;
	return 0;
}

static int tas2560_regdump_put(struct snd_kcontrol *pKcontrol,
				struct snd_ctl_elem_value *pValue)
{
	struct tas2560_priv *pTAS2560 = NULL;
	if (pValue->value.integer.value[0] == 1) {
		mutex_lock(&tas2560_codecs_lock);
		list_for_each_entry(pTAS2560, &tas2560_codecs_list, list) {
			pr_info("dump tas %u registers\n", pTAS2560->type);
			tas2560_dump_regs(pTAS2560);
		}
		mutex_unlock(&tas2560_codecs_lock);
	}
	return 0;
}

static const char *tas2560_boostload_text[] = {"8_Ohm", "6_Ohm", "4_Ohm"};
static const struct soc_enum tas2560_boostload_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tas2560_boostload_text),
			tas2560_boostload_text),
};

static const char *tas2560_powerctl_text[] = {"Off", "On"};
static const struct soc_enum tas2560_powerctl_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tas2560_powerctl_text),
		tas2560_powerctl_text),
};

static const char *tas2560_channel_text[] = {"L", "R"};
static const struct soc_enum tas2560_channel_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tas2560_channel_text),
		tas2560_channel_text),
};

static const char *tas2560_mute_text[] = {"Unmute", "Mute"};
static const struct soc_enum tas2560_mute_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tas2560_mute_text),
		tas2560_mute_text),
};

static const char *tas2560_enable_text[] = {"Disable", "Enable"};
static const struct soc_enum tas2560_enable_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tas2560_enable_text),
		tas2560_enable_text),
};

static const char *tas2560_regdump_text[] = {"No", "Yes"};
static const struct soc_enum tas2560_regdump_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tas2560_regdump_text),
		tas2560_regdump_text),
};

/*
 * DAC digital volumes. From 0 to 15 dB in 1 dB steps
 */
static DECLARE_TLV_DB_SCALE(dac_tlv, 0, 100, 0);

static const struct snd_kcontrol_new tas2560_snd_controls[] = {
	SOC_SINGLE_TLV("DAC Playback Volume", TAS2560_SPK_CTRL_REG,
			0, 0x0f, 0, dac_tlv),
	SOC_ENUM_EXT("TAS2560 0 Boostload", tas2560_boostload_enum[0],
			tas2560_boostload_get, tas2560_boostload_set),
	SOC_ENUM_EXT("TAS2560 1 Boostload", tas2560_boostload_enum[0],
			tas2560_boostload_get, tas2560_boostload_set),
	SOC_ENUM_EXT("TAS2560 2 Boostload", tas2560_boostload_enum[0],
			tas2560_boostload_get, tas2560_boostload_set),
	SOC_ENUM_EXT("TAS2560 3 Boostload", tas2560_boostload_enum[0],
			tas2560_boostload_get, tas2560_boostload_set),
	SOC_ENUM_EXT("TAS2560 0 Powerctl", tas2560_powerctl_enum[0],
			tas2560_powerctl_get, tas2560_powerctl_put),
	SOC_ENUM_EXT("TAS2560 1 Powerctl", tas2560_powerctl_enum[0],
			tas2560_powerctl_get, tas2560_powerctl_put),
	SOC_ENUM_EXT("TAS2560 2 Powerctl", tas2560_powerctl_enum[0],
			tas2560_powerctl_get, tas2560_powerctl_put),
	SOC_ENUM_EXT("TAS2560 3 Powerctl", tas2560_powerctl_enum[0],
			tas2560_powerctl_get, tas2560_powerctl_put),
	SOC_ENUM_EXT("TAS2560 0 Channel", tas2560_channel_enum[0],
			tas2560_channel_get, tas2560_channel_put),
	SOC_ENUM_EXT("TAS2560 1 Channel", tas2560_channel_enum[0],
			tas2560_channel_get, tas2560_channel_put),
	SOC_ENUM_EXT("TAS2560 2 Channel", tas2560_channel_enum[0],
			tas2560_channel_get, tas2560_channel_put),
	SOC_ENUM_EXT("TAS2560 3 Channel", tas2560_channel_enum[0],
			tas2560_channel_get, tas2560_channel_get),
	SOC_ENUM_EXT("TAS2560 0 Mute", tas2560_mute_enum[0],
			tas2560_mute_get, tas2560_mute_put),
	SOC_ENUM_EXT("TAS2560 1 Mute", tas2560_mute_enum[0],
			tas2560_mute_get, tas2560_mute_put),
	SOC_ENUM_EXT("TAS2560 2 Mute", tas2560_mute_enum[0],
			tas2560_mute_get, tas2560_mute_put),
	SOC_ENUM_EXT("TAS2560 3 Mute", tas2560_mute_enum[0],
			tas2560_mute_get, tas2560_mute_put),
	SOC_ENUM_EXT("TAS2560 0 Enable", tas2560_enable_enum[0],
			tas2560_enable_get, tas2560_enable_put),
	SOC_ENUM_EXT("TAS2560 1 Enable", tas2560_enable_enum[0],
			tas2560_enable_get, tas2560_enable_put),
	SOC_ENUM_EXT("TAS2560 2 Enable", tas2560_enable_enum[0],
			tas2560_enable_get, tas2560_enable_put),
	SOC_ENUM_EXT("TAS2560 3 Enable", tas2560_enable_enum[0],
			tas2560_enable_get, tas2560_enable_put),
	SOC_ENUM_EXT("TAS2560 Regdump", tas2560_regdump_enum[0],
			tas2560_regdump_get, tas2560_regdump_put)
};


static struct snd_soc_codec_driver tas2560_codec_driver = {
	.probe				= tas2560_codec_probe,
	.remove				= tas2560_codec_remove,
	.read				= tas2560_read,
	.write				= tas2560_write,
	.controls			= tas2560_snd_controls,
	.num_controls		= ARRAY_SIZE(tas2560_snd_controls),
	.dapm_widgets		= tas2560_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(tas2560_dapm_widgets),
	.dapm_routes		= tas2560_audio_map,
	.num_dapm_routes	= ARRAY_SIZE(tas2560_audio_map),
};

int tas2560_register_codec(struct tas2560_priv *pTAS2560)
{
	int nResult = 0;

	nResult = snd_soc_register_codec(pTAS2560->dev,
		&tas2560_codec_driver,
		tas2560_dai_driver, ARRAY_SIZE(tas2560_dai_driver));
	dev_info(pTAS2560->dev,
		"%s: register codec finished\n", __func__);
	return nResult;
}

int tas2560_deregister_codec(struct tas2560_priv *pTAS2560)
{
	snd_soc_unregister_codec(pTAS2560->dev);
	return 0;
}

MODULE_AUTHOR("Texas Instruments Inc.");
MODULE_DESCRIPTION("TAS2560 ALSA SOC Smart Amplifier driver");
MODULE_LICENSE("GPLv2");
