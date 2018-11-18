#include "ilitek_ts.h"
#include <linux/firmware.h>
#include <linux/vmalloc.h>

#define ILITEK_FW_FILENAME	"ilitek_i2c.bin"

extern struct i2c_data i2c;
static char touch_info[50] = {0};
char firmware_version_for_roi[50] = {0};
static unsigned char * CTPM_FW = NULL;

int inwrite(unsigned int address)
{
	uint8_t outbuff[64];
	int data, ret;
	outbuff[0] = REG_START_DATA;
	outbuff[1] = (char)((address & DATA_SHIFT_0) >> 0);
	outbuff[2] = (char)((address & DATA_SHIFT_8) >> 8);
	outbuff[3] = (char)((address & DATA_SHIFT_16) >> 16);
	ret = ilitek_i2c_write(i2c.client, outbuff, 4);
	udelay(10);
	ret = ilitek_i2c_read(i2c.client, outbuff, 4);
	data = (outbuff[0] + outbuff[1] * 256 + outbuff[2] * 256 * 256 + outbuff[3] * 256 * 256 * 256);
	tp_log_debug("%s, data=0x%x, outbuff[0]=%x, outbuff[1]=%x, outbuff[2]=%x, outbuff[3]=%x\n", __func__, data, outbuff[0], outbuff[1], outbuff[2], outbuff[3]);
	return data;
}

int outwrite(unsigned int address, unsigned int data, int size)
{
	int ret, i;
	char outbuff[64];
	outbuff[0] = REG_START_DATA;
	outbuff[1] = (char)((address & DATA_SHIFT_0) >> 0);
	outbuff[2] = (char)((address & DATA_SHIFT_8) >> 8);
	outbuff[3] = (char)((address & DATA_SHIFT_16) >> 16);
	for(i = 0; i < size; i++)
	{
		outbuff[i + 4] = (char)(data >> (8 * i));
	}
	ret = ilitek_i2c_write(i2c.client, outbuff, size + 4);
	return ret;
}

static int ilitek_set_appinfo(unsigned char firmware_ver[4])
{
	int ret = -1;
	snprintf(firmware_version_for_roi,sizeof(firmware_version_for_roi),"%x.%x.%02x",firmware_ver[1],firmware_ver[2],firmware_ver[3]);
	snprintf(touch_info, sizeof(touch_info),
		"ilitek_2120_%x.%x.%02x",
		firmware_ver[1],
		firmware_ver[2],
		firmware_ver[3]);
	ret = app_info_set("touch_panel", touch_info);
	if (ret < 0) {
		tp_log_err("%s(line %d): error,ret=%d\n",__func__,__LINE__,ret);
	}
	return ret;
}

int ilitek_upgrade_firmware(void)
{
	int ret = 0, upgrade_status = 0, i = 0, j = 0, k = 0;
	unsigned char buf[300] = {0};
	unsigned long ap_startaddr = 0, ap_endaddr = 0, temp = 0, ap_len = 0;
	unsigned char firmware_ver[4];
	int retry = 0;
	const struct firmware *fw;
	int set_appinfo_ret;
	CTPM_FW = vmalloc(FW_SIZE + 1);

	if (NULL == CTPM_FW) {
		tp_log_info("ilitek CTPM_FW alloctation memory failed\n");
		return NOT_NEED_UPGRADE;
	}
	ap_startaddr = AP_STARTADDR;
	ap_endaddr = AP_ENDADDR;
	ap_len = ap_endaddr - ap_startaddr;
	tp_log_info("ap_startaddr=0x%lX, ap_endaddr=0x%lX,\n", ap_startaddr, ap_endaddr);
	ret = request_firmware(&fw, ILITEK_FW_FILENAME, &i2c.client->dev);
	if (ret) {
		dev_err(&i2c.client->dev, "[ILITEK] failed to request firmware %s: %d\n",
			ILITEK_FW_FILENAME, ret);
		return ret;
	}
	tp_log_info("ilitek fw->size = %d\n", (int)fw->size);
	if ((fw->size > FW_SIZE) || (fw->size == 0)) {
		tp_log_err("get fw size error\n");
		release_firmware(fw);
		return NOT_NEED_UPGRADE;
	}
	for (ret = 0; ret < fw->size; ret++) {
		CTPM_FW[ret] = fw->data[ret];
	}
	firmware_ver[0] = 0x0;
	firmware_ver[1] = CTPM_FW[FW_VERSION1];
	firmware_ver[2] = CTPM_FW[FW_VERSION2];
	firmware_ver[3] = CTPM_FW[FW_VERSION3];
	release_firmware(fw);

	tp_log_info("firmware_ver[0] = %d, firmware_ver[1] = %d firmware_ver[2]=%d firmware_ver[3]=%d\n",firmware_ver[0], firmware_ver[1], firmware_ver[2], firmware_ver[3]);
	if (!(i2c.force_upgrade)) {
		for(i = 0; i < 4; i++)
		{
			tp_log_info("i2c.firmware_ver[%d] = %d, firmware_ver[%d] = %d\n", i, i2c.firmware_ver[i], i, firmware_ver[i ]);
			if((i2c.firmware_ver[i] > firmware_ver[i ]) || ((i == 3) && (i2c.firmware_ver[3] == firmware_ver[3])))
			{
				set_appinfo_ret = ilitek_set_appinfo(i2c.firmware_ver);
				if (set_appinfo_ret) {
					tp_log_info("%s:%d set app info err\n",__FUNCTION__,__LINE__);
				}
				tp_log_info("ilitek_upgrade_firmware Do not need update\n");
				return NOT_NEED_UPGRADE;
			}
			else if(i2c.firmware_ver[i] < firmware_ver[i])
			{
				break;
			}
		}
	}
	if (ic2120) {
Retry:
		ret = outwrite(ENTER_ICE_MODE, ENTER_ICE_MODE_NO_DATA, 0);
		msleep(1);
		ret = outwrite(WDTRLDT, WDTRLDT_CLOSE, 2);
		msleep(1);
		ret = outwrite(WDTCNT1, WDTCNT1_OPEN, 1);
		msleep(1);
		ret = outwrite(WDTCNT1, WDTCNT1_CLOSE, 1);
		msleep(1);
		ret = outwrite(CLOSE_10K_WDT1, CLOSE_10K_WDT1_VALUE, 4);
		msleep(1);
		ret = outwrite(CLOSE_10K_WDT2, CLOSE_10K_WDT2_VALUE, 1);
		msleep(1);
		ret = outwrite(CLOSE_10K_WDT3, CLOSE_10K_WDT3_VALUE, 4);
		msleep(1);
		tp_log_info("%s, release Power Down Release mode\n", __func__);
		ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_RELEASE_FROM_POWER_DOWN, 1);
		msleep(1);
		ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
		msleep(1);
		ret = outwrite(REG_TIMING_SET, REG_TIMING_SET_10MS, 1);
		msleep(5);
		for (i = 0; i <= SECTOR_ENDADDR; i += SECTOR_SIZE) {
			tp_log_debug("%s, i = %X\n", __func__, i);
			ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_WRITE_ENABLE, 1);
			msleep(1);
			ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
			msleep(1);
			temp = (i << 8) + REG_FLASH_CMD_DATA_ERASE;
			ret = outwrite(REG_FLASH_CMD, temp, 4);
			msleep(1);
			ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
			msleep(15);
			for (j = 0; j < 50; j++) {
				ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_READ_FLASH_STATUS, 1);
				ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
				msleep(1);
				buf[0] = inwrite(FLASH_STATUS);
				tp_log_debug("%s, buf[0] = %X\n", __func__, buf[0]);
				if (buf[0] == 0) {
					break;
				}
				else {
					msleep(2);
				};
			}
			if (j >= 50) {
				tp_log_info("FLASH_STATUS ERROR j = %d, buf[0] = 0x%X\n", j, buf[0]);
			}
		}
		msleep(100);
		for(i = ap_startaddr; i < ap_endaddr; i += UPGRADE_TRANSMIT_LEN) {
			tp_log_debug("%s, i = %X\n", __func__, i);
			ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_WRITE_ENABLE, 1);
			ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
			temp = (i << 8) + REG_FLASH_CMD_DATA_PROGRAMME;
			ret = outwrite(REG_FLASH_CMD, temp, 4);
			ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_256, 4);
			buf[0] = REG_START_DATA;
			buf[3] = (char)((REG_PGM_DATA  & DATA_SHIFT_16) >> 16);
			buf[2] = (char)((REG_PGM_DATA  & DATA_SHIFT_8) >> 8);
			buf[1] = (char)((REG_PGM_DATA  & DATA_SHIFT_0));
			for(k = 0; k < UPGRADE_TRANSMIT_LEN; k++)
			{
				buf[4 + k] = CTPM_FW[i  + k];
			}

			if(ilitek_i2c_write(i2c.client, buf, UPGRADE_TRANSMIT_LEN + REG_LEN) < 0) {
				tp_log_err("%s, write data error, address = 0x%X, start_addr = 0x%X, end_addr = 0x%X\n", __func__, (int)i, (int)ap_startaddr, (int)ap_endaddr);
				return TRANSMIT_ERROR;
			}
			upgrade_status = (i * 100) / ap_len;
			mdelay(3);
		}
		buf[0] = (unsigned char)(EXIT_ICE_MODE & DATA_SHIFT_0);
		buf[1] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_8) >> 8);
		buf[2] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_16) >> 16);
		buf[3] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_24) >> 24);
		ilitek_i2c_write(i2c.client, buf, 4);
		ilitek_reset(i2c.reset_gpio);
		for (ret = 0; ret < 30; ret++ ) {
			j = ilitek_poll_int();
			tp_log_info("ilitek int status = %d\n", j);
			if (j == 0) {
				break;
			}
			else {
				mdelay(5);
			}
		}
		if (ret >= 30) {
			tp_log_err("ilitek reset but int not pull low so retry\n");
			if (retry < 2) {
				retry++;
				goto Retry;
			}
		}
		else {
			tp_log_info("ilitek reset  int  pull low  write 0x10 cmd\n");
		}
		buf[0] = ILITEK_TP_CMD_READ_DATA;
		ilitek_i2c_write(i2c.client, buf, 1);
		ret = ilitek_i2c_read(i2c.client, buf, 3);
		tp_log_info("%s, buf = %X, %X, %X\n", __func__, buf[0], buf[1], buf[2]);
		if (buf[1] >= FW_OK) {
			tp_log_info("upgrade ok ok \n");
			set_appinfo_ret = ilitek_set_appinfo(firmware_ver);
			if (set_appinfo_ret) {
				tp_log_info("%s:%d set app info err\n",__FUNCTION__,__LINE__);
			}
		}else {
			if (retry < 2) {
				retry++;
				goto Retry;
			}
		}
	}
	msleep(10);
	if (CTPM_FW) {
		vfree(CTPM_FW);
		CTPM_FW = NULL;
	}
	return UPGRADE_OK;
}

