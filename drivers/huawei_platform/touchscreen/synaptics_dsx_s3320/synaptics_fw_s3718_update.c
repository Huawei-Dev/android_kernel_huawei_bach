/*
 * Synaptics DSX touchscreen driver
 *
 * Copyright (C) 2012 Synaptics Incorporated
 *
 * Copyright (C) 2012 Alexandra Chin <alexandra.chin@tw.synaptics.com>
 * Copyright (C) 2012 Scott Lin <scott.lin@tw.synaptics.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/firmware.h>
#include <linux/pm_runtime.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include "synaptics_dsx.h"
#include "synaptics_dsx_i2c.h"
#ifdef CONFIG_APP_INFO
#include <misc/app_info.h>
#endif /*CONFIG_APP_INFO*/

#include "synaptics_dsx_esd.h"

#define FORCE_UPDATE false
#define DO_LOCKDOWN false

#define MAX_IMAGE_NAME_LEN 256
#define MAX_FIRMWARE_ID_LEN 10
#define RMI_PRODUCT_ID_LENGTH    10

#define IMAGE_HEADER_VERSION_05 0x05
#define IMAGE_HEADER_VERSION_06 0x06
#define IMAGE_HEADER_VERSION_10 0x10
#define PRODUCT_INFO_SIZE 2
#define PRODUCT_ID_SIZE 10
#define BUILD_ID_SIZE 3

#define IMAGE_AREA_OFFSET 0x100
#define LOCKDOWN_SIZE 0x50

#define V5V6_BOOTLOADER_ID_OFFSET 0

#define V5_PROPERTIES_OFFSET 2
#define V5_BLOCK_SIZE_OFFSET 3
#define V5_BLOCK_COUNT_OFFSET 5
#define V5_BLOCK_NUMBER_OFFSET 0
#define V5_BLOCK_DATA_OFFSET 2

#define V6_PROPERTIES_OFFSET 1
#define V6_BLOCK_SIZE_OFFSET 2
#define V6_BLOCK_COUNT_OFFSET 3
#define V6_PROPERTIES_2_OFFSET 4
#define V6_GUEST_CODE_BLOCK_COUNT_OFFSET 5
#define V6_BLOCK_NUMBER_OFFSET 0
#define V6_BLOCK_DATA_OFFSET 1
#define V6_FLASH_COMMAND_OFFSET 2
#define V6_FLASH_STATUS_OFFSET 3
#define V7_FLASH_STATUS_OFFSET 0
#define V7_PARTITION_ID_OFFSET 1
#define V7_BLOCK_NUMBER_OFFSET 2
#define V7_TRANSFER_LENGTH_OFFSET 3
#define V7_COMMAND_OFFSET 4
#define STARTUP_FW_UPDATE_DELAY_MS 1000 /* ms */
#define V7_PAYLOAD_OFFSET 5

#define V7_PARTITION_SUPPORT_BYTES 4
#define FIRMWARE_LEN 300*1024

#define F35_ERROR_CODE_OFFSET 0
#define F35_FLASH_STATUS_OFFSET 5
#define F35_CHUNK_NUM_LSB_OFFSET 0
#define F35_CHUNK_NUM_MSB_OFFSET 1
#define F35_CHUNK_DATA_OFFSET 2
#define F35_CHUNK_COMMAND_OFFSET 18

#define F35_CHUNK_SIZE 16
#define F35_ERASE_ALL_WAIT_MS 5000
#define F35_RESET_WAIT_MS 250

#define SLEEP_MODE_NORMAL (0x00)
#define SLEEP_MODE_SENSOR_SLEEP (0x01)
#define SLEEP_MODE_RESERVED0 (0x02)
#define SLEEP_MODE_RESERVED1 (0x03)

#define ENABLE_WAIT_MS (1 * 1000)
#define WRITE_WAIT_MS (3 * 1000)
#define ERASE_WAIT_MS (5 * 1000)

#define MIN_SLEEP_TIME_US 50
#define MAX_SLEEP_TIME_US 100

#define INT_DISABLE_WAIT_MS 20
#define ENTER_FLASH_PROG_WAIT_MS 20
#define NO_ERR  0
static char touch_s3718_info[50] = {0};

static int fwu_do_reflash(void);

static int fwu_recovery_check_status(void);
static int fwu_start_reflash(void);
bool synaptics_check_fw_s3718_version(void);
int synaptics_fw_s3718_update(void);
static ssize_t fwu_sysfs_do_recovery_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count);

enum f34_version {
	F34_V0 = 0,
	F34_V1,
	F34_V2,
};

enum bl_version {
	BL_V5 = 5,
	BL_V6 = 6,
	BL_V7 = 7,
};

enum flash_area {
	NONE = 0,
	UI_FIRMWARE,
	UI_CONFIG,
};

enum update_mode {
	NORMAL = 1,
	FORCE = 2,
	LOCKDOWN = 8,
};

enum config_area {
	UI_CONFIG_AREA = 0,
	PM_CONFIG_AREA,
	BL_CONFIG_AREA,
	DP_CONFIG_AREA,
	FLASH_CONFIG_AREA,
};

enum v7_partition_id {
	BOOTLOADER_PARTITION = 0x01,
	DEVICE_CONFIG_PARTITION,
	FLASH_CONFIG_PARTITION,
	MANUFACTURING_BLOCK_PARTITION,
	GUEST_SERIALIZATION_PARTITION,
	GLOBAL_PARAMETERS_PARTITION,
	CORE_CODE_PARTITION,
	CORE_CONFIG_PARTITION,
	GUEST_CODE_PARTITION,
	DISPLAY_CONFIG_PARTITION,
};

enum v7_flash_command {
	CMD_V7_IDLE = 0x00,
	CMD_V7_ENTER_BL,
	CMD_V7_READ,
	CMD_V7_WRITE,
	CMD_V7_ERASE,
	CMD_V7_ERASE_AP,
	CMD_V7_SENSOR_ID,
};

enum v5v6_flash_command {
	CMD_V5V6_IDLE = 0x0,
	CMD_V5V6_WRITE_FW = 0x2,
	CMD_V5V6_ERASE_ALL = 0x3,
	CMD_V5V6_WRITE_LOCKDOWN = 0x4,
	CMD_V5V6_READ_CONFIG = 0x5,
	CMD_V5V6_WRITE_CONFIG = 0x6,
	CMD_V5V6_ERASE_UI_CONFIG = 0x7,
	CMD_V5V6_ERASE_BL_CONFIG = 0x9,
	CMD_V5V6_ERASE_DISP_CONFIG = 0xa,
	CMD_V5V6_ERASE_GUEST_CODE = 0xb,
	CMD_V5V6_WRITE_GUEST_CODE = 0xc,
	CMD_V5V6_ENABLE_FLASH_PROG = 0xf,
};

enum flash_command {
	CMD_IDLE = 0,
	CMD_WRITE_FW,
	CMD_WRITE_CONFIG,
	CMD_WRITE_LOCKDOWN,
	CMD_WRITE_GUEST_CODE,
	CMD_READ_CONFIG,
	CMD_ERASE_ALL,
	CMD_ERASE_UI_FIRMWARE,
	CMD_ERASE_UI_CONFIG,
	CMD_ERASE_BL_CONFIG,
	CMD_ERASE_DISP_CONFIG,
	CMD_ERASE_FLASH_CONFIG,
	CMD_ERASE_GUEST_CODE,
	CMD_ENABLE_FLASH_PROG,
};

enum f35_flash_command {
	CMD_F35_IDLE = 0x0,
	CMD_F35_RESERVED = 0x1,
	CMD_F35_WRITE_CHUNK = 0x2,
	CMD_F35_ERASE_ALL = 0x3,
	CMD_F35_RESET = 0x10,
};

enum container_id {
	TOP_LEVEL_CONTAINER = 0,
	UI_CONTAINER,
	UI_CONFIG_CONTAINER,
	BL_CONTAINER,
	BL_IMAGE_CONTAINER,
	BL_CONFIG_CONTAINER,
	BL_LOCKDOWN_INFO_CONTAINER,
	PERMANENT_CONFIG_CONTAINER,
	GUEST_CODE_CONTAINER,
	BL_PROTOCOL_DESCRIPTOR_CONTAINER,
	UI_PROTOCOL_DESCRIPTOR_CONTAINER,
	RMI_SELF_DISCOVERY_CONTAINER,
	RMI_PAGE_CONTENT_CONTAINER,
	GENERAL_INFORMATION_CONTAINER,
	DEVICE_CONFIG_CONTAINER,
	FLASH_CONFIG_CONTAINER,
	GUEST_SERIALIZATION_CONTAINER,
	GLOBAL_PARAMETERS_CONTAINER,
	CORE_CODE_CONTAINER,
	CORE_CONFIG_CONTAINER,
	DISPLAY_CONFIG_CONTAINER,
};

struct pdt_properties {
	union {
		struct {
			unsigned char reserved_1:6;
			unsigned char has_bsr:1;
			unsigned char reserved_2:1;
		} __packed;
		unsigned char data[1];
	};
};

struct partition_table {
	unsigned char partition_id:5;
	unsigned char byte_0_reserved:3;
	unsigned char byte_1_reserved;
	unsigned char partition_length_7_0;
	unsigned char partition_length_15_8;
	unsigned char start_physical_address_7_0;
	unsigned char start_physical_address_15_8;
	unsigned char partition_properties_7_0;
	unsigned char partition_properties_15_8;
} __packed;

struct f01_device_control {
	union {
		struct {
			unsigned char sleep_mode:2;
			unsigned char nosleep:1;
			unsigned char reserved:2;
			unsigned char charger_connected:1;
			unsigned char report_rate:1;
			unsigned char configured:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f34_v7_query_0 {
	union {
		struct {
			unsigned char subpacket_1_size:3;
			unsigned char has_config_id:1;
			unsigned char f34_query0_b4:1;
			unsigned char has_thqa:1;
			unsigned char f34_query0_b6__7:2;
		} __packed;
		unsigned char data[1];
	};
};

struct f34_v7_query_1_7 {
	union {
		struct {
			/* query 1 */
			unsigned char bl_minor_revision;
			unsigned char bl_major_revision;

			/* query 2 */
			unsigned char bl_fw_id_7_0;
			unsigned char bl_fw_id_15_8;
			unsigned char bl_fw_id_23_16;
			unsigned char bl_fw_id_31_24;

			/* query 3 */
			unsigned char minimum_write_size;
			unsigned char block_size_7_0;
			unsigned char block_size_15_8;
			unsigned char flash_page_size_7_0;
			unsigned char flash_page_size_15_8;

			/* query 4 */
			unsigned char adjustable_partition_area_size_7_0;
			unsigned char adjustable_partition_area_size_15_8;

			/* query 5 */
			unsigned char flash_config_length_7_0;
			unsigned char flash_config_length_15_8;

			/* query 6 */
			unsigned char payload_length_7_0;
			unsigned char payload_length_15_8;

			/* query 7 */
			unsigned char f34_query7_b0:1;
			unsigned char has_bootloader:1;
			unsigned char has_device_config:1;
			unsigned char has_flash_config:1;
			unsigned char has_manufacturing_block:1;
			unsigned char has_guest_serialization:1;
			unsigned char has_global_parameters:1;
			unsigned char has_core_code:1;
			unsigned char has_core_config:1;
			unsigned char has_guest_code:1;
			unsigned char has_display_config:1;
			unsigned char f34_query7_b11__15:5;
			unsigned char f34_query7_b16__23;
			unsigned char f34_query7_b24__31;
		} __packed;
		unsigned char data[21];
	};
};

struct f34_v7_data_1_5 {
	union {
		struct {
			unsigned char partition_id:5;
			unsigned char f34_data1_b5__7:3;
			unsigned char block_offset_7_0;
			unsigned char block_offset_15_8;
			unsigned char transfer_length_7_0;
			unsigned char transfer_length_15_8;
			unsigned char command;
			unsigned char payload_0;
			unsigned char payload_1;
		} __packed;
		unsigned char data[8];
	};
};

struct f34_v5v6_flash_properties {
	union {
		struct {
			unsigned char reg_map:1;
			unsigned char unlocked:1;
			unsigned char has_config_id:1;
			unsigned char has_pm_config:1;
			unsigned char has_bl_config:1;
			unsigned char has_disp_config:1;
			unsigned char has_ctrl1:1;
			unsigned char has_query4:1;
		} __packed;
		unsigned char data[1];
	};
};

struct f34_v5v6_flash_properties_2 {
	union {
		struct {
			unsigned char has_guest_code:1;
			unsigned char reserved:7;
		} __packed;
		unsigned char data[1];
	};
};

struct register_offset {
	unsigned char properties;
	unsigned char properties_2;
	unsigned char block_size;
	unsigned char block_count;
	unsigned char gc_block_count;
	unsigned char flash_status;
	unsigned char partition_id;
	unsigned char block_number;
	unsigned char transfer_length;
	unsigned char flash_cmd;
	unsigned char payload;
};

struct block_count {
	unsigned short ui_firmware;
	unsigned short ui_config;
	unsigned short dp_config;
	unsigned short fl_config;
	unsigned short pm_config;
	unsigned short bl_config;
	unsigned short lockdown;
	unsigned short guest_code;
};

struct physical_address {
	unsigned short ui_firmware;
	unsigned short ui_config;
	unsigned short dp_config;
	unsigned short guest_code;
};

struct container_descriptor {
	unsigned char content_checksum[4];
	unsigned char container_id[2];
	unsigned char minor_version;
	unsigned char major_version;
	unsigned char reserved_08;
	unsigned char reserved_09;
	unsigned char reserved_0a;
	unsigned char reserved_0b;
	unsigned char container_option_flags[4];
	unsigned char content_options_length[4];
	unsigned char content_options_address[4];
	unsigned char content_length[4];
	unsigned char content_address[4];
};

struct image_header_10 {
	unsigned char checksum[4];
	unsigned char reserved_04;
	unsigned char reserved_05;
	unsigned char minor_header_version;
	unsigned char major_header_version;
	unsigned char reserved_08;
	unsigned char reserved_09;
	unsigned char reserved_0a;
	unsigned char reserved_0b;
	unsigned char top_level_container_start_addr[4];
};

struct image_header_05_06 {
	/* 0x00 - 0x0f */
	unsigned char checksum[4];
	unsigned char reserved_04;
	unsigned char reserved_05;
	unsigned char options_firmware_id:1;
	unsigned char options_bootloader:1;
	unsigned char options_guest_code:1;
	unsigned char options_tddi:1;
	unsigned char options_reserved:4;
	unsigned char header_version;
	unsigned char firmware_size[4];
	unsigned char config_size[4];
	/* 0x10 - 0x1f */
	unsigned char product_id[PRODUCT_ID_SIZE];
	unsigned char package_id[2];
	unsigned char package_id_revision[2];
	unsigned char product_info[PRODUCT_INFO_SIZE];
	/* 0x20 - 0x2f */
	unsigned char bootloader_addr[4];
	unsigned char bootloader_size[4];
	unsigned char ui_addr[4];
	unsigned char ui_size[4];
	/* 0x30 - 0x3f */
	unsigned char ds_id[16];
	/* 0x40 - 0x4f */
	union {
		struct {
			unsigned char cstmr_product_id[PRODUCT_ID_SIZE];
			unsigned char reserved_4a_4f[6];
		};
		struct {
			unsigned char dsp_cfg_addr[4];
			unsigned char dsp_cfg_size[4];
			unsigned char reserved_48_4f[8];
		};
	};
	/* 0x50 - 0x53 */
	unsigned char firmware_id[4];
};

struct block_data {
	unsigned int size;
	const unsigned char *data;
};

struct image_metadata {
	bool contains_firmware_id;
	bool contains_bootloader;
	bool contains_disp_config;
	bool contains_guest_code;
	bool contains_flash_config;
	unsigned int firmware_id;
	unsigned int checksum;
	unsigned int bootloader_size;
	unsigned int disp_config_offset;
	unsigned char bl_version;
	unsigned char product_id[PRODUCT_ID_SIZE + 1];
	unsigned char cstmr_product_id[PRODUCT_ID_SIZE + 1];
	struct block_data bootloader;
	struct block_data ui_firmware;
	struct block_data ui_config;
	struct block_data dp_config;
	struct block_data fl_config;
	struct block_data bl_config;
	struct block_data guest_code;
	struct block_data lockdown;
	struct block_count blkcount;
	struct physical_address phyaddr;
};

struct synaptics_rmi4_fwu_handle {
	enum bl_version bl_version;
	bool initialized;
	bool in_bl_mode;
	bool in_ub_mode;
	bool force_update;
	bool do_lockdown;
	bool has_guest_code;
	bool new_partition_table;
	unsigned int data_pos;
	unsigned char *fw_entry_sd;
	const struct firmware *fw_entry_boot;
	unsigned char *ext_data_source;
	unsigned char *read_config_buf;
	unsigned char intr_mask;
	unsigned char command;
	unsigned char bootloader_id[2];
	unsigned char flash_status;
	unsigned char partitions;
	unsigned short block_size;
	unsigned short config_size;
	unsigned short config_area;
	unsigned short config_block_count;
	unsigned short flash_config_length;
	unsigned short payload_length;
	unsigned short partition_table_bytes;
	unsigned short read_config_buf_size;
	const unsigned char *config_data;
	const unsigned char *image;
	unsigned char *image_name;
	unsigned char *firmware_name;
	unsigned int image_size;
	struct image_metadata img;
	struct register_offset off;
	struct block_count blkcount;
	struct physical_address phyaddr;
	struct f34_v5v6_flash_properties flash_properties;
	struct synaptics_rmi4_fn_desc f01_fd;
	struct synaptics_rmi4_fn_desc f34_fd;
	struct synaptics_rmi4_fn_desc f35_fd;
	struct synaptics_rmi4_data *rmi4_data;
	struct workqueue_struct *fwu_workqueue;
	struct delayed_work fwu_work;
	struct synaptics_rmi4_access_ptr *fn_ptr;
};

static struct synaptics_rmi4_fwu_handle *fwu;

DECLARE_COMPLETION(fwu_remove_complete);

static inline int secure_memcpy(unsigned char *dest, unsigned int dest_size,
		const unsigned char *src, unsigned int src_size,
		unsigned int count)
{
	if (dest == NULL || src == NULL)
		return -EINVAL;

	if (count > dest_size || count > src_size)
		return -EINVAL;

	memcpy((void *)dest, (const void *)src, count);

	return 0;
}

static unsigned int le_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] +
			(unsigned int)ptr[1] * 0x100 +
			(unsigned int)ptr[2] * 0x10000 +
			(unsigned int)ptr[3] * 0x1000000;
}

static unsigned int be_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[3] +
			(unsigned int)ptr[2] * 0x100 +
			(unsigned int)ptr[1] * 0x10000 +
			(unsigned int)ptr[0] * 0x1000000;
}

static void fwu_compare_partition_tables(void)
{
	if (fwu->phyaddr.ui_firmware != fwu->img.phyaddr.ui_firmware) {
		fwu->new_partition_table = true;
		return;
	}

	if (fwu->phyaddr.ui_config != fwu->img.phyaddr.ui_config) {
		fwu->new_partition_table = true;
		return;
	}

	if (fwu->flash_properties.has_disp_config) {
		if (fwu->phyaddr.dp_config != fwu->img.phyaddr.dp_config) {
			fwu->new_partition_table = true;
			return;
		}
	}

	if (fwu->flash_properties.has_disp_config) {
		if (fwu->phyaddr.dp_config != fwu->img.phyaddr.dp_config) {
			fwu->new_partition_table = true;
			return;
		}
	}

	if (fwu->has_guest_code) {
		if (fwu->phyaddr.guest_code != fwu->img.phyaddr.guest_code) {
			fwu->new_partition_table = true;
			return;
		}
	}

	fwu->new_partition_table = false;

	return;
}

/*
static char * get_cof_module_name_s3718(void)
{
	int len = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;
	struct synaptics_rmi4_device_info *rmi = &(rmi4_data->rmi4_mod_info);
	char *product_id = rmi->product_id_string;
	memcpy(fwu->rmi4_data->product_id,rmi->product_id_string,SYNAPTICS_RMI4_PRODUCT_ID_SIZE);
	tp_log_info("%s: LINE = %d  product_id = %s ,\n", __func__,__LINE__,product_id);	
	if(NULL == product_id)
	{
		tp_log_err("%s: product_id = NULL ,LINE = %d\n", __func__,__LINE__);		
		return "unknow";
	}
	else if (0 == strncasecmp(product_id, PID_JDI_S3320, PID_JDI_LEN))
	{
		return "jdi";
	}
	else if(0 == strncasecmp(product_id, PID_BOE_PLK11130, PID_BOE_LEN))
	{
		return "boe";
	}
	else if(0 == strncasecmp(product_id, PID_LG_S332U, PID_LG_LEN))
	{
		return "lg";
	}
	len = strlen(product_id);

	if (len > MODULE_STR_LEN) {
		product_id += (len - MODULE_STR_LEN);
		tp_log_info("%s: the last three characters of product_id = %s,LINE = %d\n", __func__,product_id,__LINE__);
	} else {
		tp_log_err("%s: failed to get the module name,LINE = %d\n", __func__,__LINE__);
		return "unknow";
	}

	if (0 == strcasecmp(product_id, FW_OFILM_STR))
	{
		return "ofilm";
	}
	else if(0 == strcasecmp(product_id, FW_EELY_STR))
	{
		return "eely";
	}
	else if(0 == strcasecmp(product_id, FW_TRULY_STR))
	{
		return "truly";
	}
	else if(0 == strcasecmp(product_id, FW_JUNDA_STR))
	{
		return "junda";
	}
	else if(0 == strcasecmp(product_id, FW_LENSONE_STR))
	{
		return "lensone";
	}
	else
	{
		return "unknow";
	}
}
*/

static int synaptics_get_f34_addr(struct synaptics_rmi4_data *rmi4_data, unsigned char*f34_address)
{
	int retval;
	unsigned short addr;
	struct synaptics_rmi4_fn_desc rmi_fd;

	for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE)
	{
		retval = rmi4_data->i2c_read(rmi4_data, addr, (unsigned char *)&rmi_fd, sizeof(rmi_fd));
		if (retval < 0) {
			tp_log_warning("Failed to read map register\n");
			return retval;
		}

		if (SYNAPTICS_RMI4_F34 == rmi_fd.fn_number) {
			tp_log_info("Found F%02x\n", rmi_fd.fn_number);
			*f34_address = rmi_fd.ctrl_base_addr;
			return NO_ERR;
		}
	}
	tp_log_warning("Failed to get f34 addr\n");
	return -EINVAL;
}

unsigned char s3718_config_id[4];

int synaptics_s3718_set_appinfo(struct synaptics_rmi4_data *rmi4_data)
{
	int rc = 0;
	int ret=0;
	unsigned char f34_ctrl_base_addr = 0;
	const char *module_name = get_cof_module_name(fwu->rmi4_data->rmi4_mod_info.product_id_string);
	struct synaptics_rmi4_device_info *rmi;
	rmi = &(rmi4_data->rmi4_mod_info);
	tp_log_info("%s:synaptics_s3718_set_appinfo called\n",__func__);

	rc = synaptics_get_f34_addr(rmi4_data, &f34_ctrl_base_addr);
	if (rc < 0){
		tp_log_err("%s:failed to scan pdt\n",__func__);
		goto exit;
	}

	rc = rmi4_data->i2c_read(rmi4_data, f34_ctrl_base_addr, s3718_config_id, sizeof(s3718_config_id));
	if (rc < 0) {
		tp_log_err("%s:Could not read configid\n",__func__);
		goto exit;
	}

	tp_log_info("%s:config ID 0x%02X, 0x%02X, 0x%02X, 0x%02X,addr = 0x%02x\n",
					__func__,s3718_config_id[0], s3718_config_id[1], s3718_config_id[2], s3718_config_id[3], f34_ctrl_base_addr);
	if (rmi4_data->board->appinfo_display_flag) {
		snprintf(touch_s3718_info,sizeof(touch_s3718_info),"synaptics_%s_%s.%x%02x",rmi->product_id_string, module_name,s3718_config_id[2], s3718_config_id[3]);
	}else {
		snprintf(touch_s3718_info,sizeof(touch_s3718_info),"synaptics_%s_%s.%x%02x",get_ic_name(rmi4_data->board->ic_type), module_name, s3718_config_id[2], s3718_config_id[3]);
	}

	snprintf(fwu->rmi4_data->tp_chip_info, sizeof(fwu->rmi4_data->tp_chip_info),
	"%s-%x%02x",
	module_name,
	s3718_config_id[2],
	s3718_config_id[3]);

	ret = app_info_set("touch_panel", touch_s3718_info);
	if (ret < 0) 
	{
		tp_log_err("%s(line %d): error,ret=%d\n",__func__,__LINE__,ret);
		goto exit;
	}
	return 0;
exit:
	ret = app_info_set("touch_panel", "synaptics_S3718_X.X");
	if (ret < 0)
	{
		tp_log_err("%s(line %d): app_info_set fail,ret=%d\n"
				,__func__,__LINE__,ret);
	}
	return 0;
}

/* Used to read configid */
int synaptics_get_s3718_configid(u8 *buf, size_t buf_size)
{
	tp_log_info("%s called\n",__func__);

	snprintf(buf, buf_size, "%02x %02x %02x %02x\n",
				s3718_config_id[0],
				s3718_config_id[1],
				s3718_config_id[2],
				s3718_config_id[3]);

	tp_log_info("buf = %s\n", buf);
	return 0;
}

static void fwu_parse_partition_table(const unsigned char *partition_table,
		struct block_count *blkcount, struct physical_address *phyaddr)
{
	unsigned char ii;
	unsigned char index;
	unsigned char offset;
	unsigned short partition_length;
	unsigned short physical_address;
	struct partition_table *ptable;
	//struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	for (ii = 0; ii < fwu->partitions; ii++) {
		index = ii * 8 + 2;
		ptable = (struct partition_table *)&partition_table[index];
		partition_length = ptable->partition_length_15_8 << 8 |ptable->partition_length_7_0;
		physical_address = ptable->start_physical_address_15_8 << 8 |ptable->start_physical_address_7_0;
		tp_log_debug("%s: Partition entry %d:\n",__func__, ii);
		for (offset = 0; offset < 8; offset++) {
			tp_log_debug("%s: 0x%02x\n",	__func__,	partition_table[index + offset]);
		}
		switch (ptable->partition_id) {
		case CORE_CODE_PARTITION:
			blkcount->ui_firmware = partition_length;
			phyaddr->ui_firmware = physical_address;
			tp_log_debug("%s: Core code block count: %d\n",	__func__, blkcount->ui_firmware);
			break;
		case CORE_CONFIG_PARTITION:
			blkcount->ui_config = partition_length;
			phyaddr->ui_config = physical_address;
			tp_log_debug("%s: Core config block count: %d\n",	__func__, blkcount->ui_config);
			break;
		case DISPLAY_CONFIG_PARTITION:
			blkcount->dp_config = partition_length;
			phyaddr->dp_config = physical_address;
			tp_log_debug("%s: Display config block count: %d\n",	__func__, blkcount->dp_config);
			break;
		case FLASH_CONFIG_PARTITION:
			blkcount->fl_config = partition_length;
			tp_log_debug("%s: Flash config block count: %d\n",	__func__, blkcount->fl_config);
			break;
		case GUEST_CODE_PARTITION:
			blkcount->guest_code = partition_length;
			phyaddr->guest_code = physical_address;
			tp_log_debug("%s: Guest code block count: %d\n",	__func__, blkcount->guest_code);
			break;
		case GUEST_SERIALIZATION_PARTITION:
			blkcount->pm_config = partition_length;
			tp_log_debug("%s: Guest serialization block count: %d\n",	__func__, blkcount->pm_config);
			break;
		case GLOBAL_PARAMETERS_PARTITION:
			blkcount->bl_config = partition_length;
			tp_log_debug("%s: Global parameters block count: %d\n",	__func__, blkcount->bl_config);
			break;
		case DEVICE_CONFIG_PARTITION:
			blkcount->lockdown = partition_length;
			tp_log_debug("%s: Device config block count: %d\n",	__func__, blkcount->lockdown);
			break;
		};
	}

	return;
}

static void fwu_parse_image_header_10_bl_container(const unsigned char *image)
{
	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int container_id;
	unsigned int length;
	const unsigned char *content;
	struct container_descriptor *descriptor;

	num_of_containers = (fwu->img.bootloader.size - 4) / 4;

	for (ii = 1; ii <= num_of_containers; ii++) {
		addr = le_to_uint(fwu->img.bootloader.data + (ii * 4));
		descriptor = (struct container_descriptor *)(image + addr);
		container_id = descriptor->container_id[0] |descriptor->container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);
		switch (container_id) {
		case BL_CONFIG_CONTAINER:
		case GLOBAL_PARAMETERS_CONTAINER:
			fwu->img.bl_config.data = content;
			fwu->img.bl_config.size = length;
			break;
		case BL_LOCKDOWN_INFO_CONTAINER:
		case DEVICE_CONFIG_CONTAINER:
			fwu->img.lockdown.data = content;
			fwu->img.lockdown.size = length;
			break;
		default:
			break;
		};
	}

	return;
}

static void fwu_parse_image_header_10(void)
{
	unsigned char ii;
	unsigned char num_of_containers;
	unsigned int addr;
	unsigned int offset;
	unsigned int container_id;
	unsigned int length;
	const unsigned char *image;
	const unsigned char *content;
	struct container_descriptor *descriptor;
	struct image_header_10 *header;

	image = fwu->image;
	header = (struct image_header_10 *)image;

	fwu->img.checksum = le_to_uint(header->checksum);

	/* address of top level container */
	offset = le_to_uint(header->top_level_container_start_addr);
	descriptor = (struct container_descriptor *)(image + offset);

	/* address of top level container content */
	offset = le_to_uint(descriptor->content_address);
	num_of_containers = le_to_uint(descriptor->content_length) / 4;

	for (ii = 0; ii < num_of_containers; ii++) {
		addr = le_to_uint(image + offset);
		offset += 4;
		descriptor = (struct container_descriptor *)(image + addr);
		container_id = descriptor->container_id[0] |descriptor->container_id[1] << 8;
		content = image + le_to_uint(descriptor->content_address);
		length = le_to_uint(descriptor->content_length);
		switch (container_id) {
		case UI_CONTAINER:
		case CORE_CODE_CONTAINER:
			fwu->img.ui_firmware.data = content;
			fwu->img.ui_firmware.size = length;
			break;
		case UI_CONFIG_CONTAINER:
		case CORE_CONFIG_CONTAINER:
			fwu->img.ui_config.data = content;
			fwu->img.ui_config.size = length;
			break;
		case BL_CONTAINER:
			fwu->img.bl_version = *content;
			fwu->img.bootloader.data = content;
			fwu->img.bootloader.size = length;
			fwu_parse_image_header_10_bl_container(image);
			break;
		case GUEST_CODE_CONTAINER:
			fwu->img.contains_guest_code = true;
			fwu->img.guest_code.data = content;
			fwu->img.guest_code.size = length;
			break;
		case DISPLAY_CONFIG_CONTAINER:
			fwu->img.contains_disp_config = true;
			fwu->img.dp_config.data = content;
			fwu->img.dp_config.size = length;
			break;
		case FLASH_CONFIG_CONTAINER:
			fwu->img.contains_flash_config = true;
			fwu->img.fl_config.data = content;
			fwu->img.fl_config.size = length;
			break;
		case GENERAL_INFORMATION_CONTAINER:
			fwu->img.contains_firmware_id = true;
			fwu->img.firmware_id = le_to_uint(content + 4);
			break;
		default:
			break;
		}
	}

	return;
}

static void fwu_parse_image_header_05_06(void)
{
	int retval;
	const unsigned char *image;
	struct image_header_05_06 *header;

	image = fwu->image;
	header = (struct image_header_05_06 *)image;

	fwu->img.checksum = le_to_uint(header->checksum);

	fwu->img.bl_version = header->header_version;

	fwu->img.contains_bootloader = header->options_bootloader;
	if (fwu->img.contains_bootloader)
		fwu->img.bootloader_size = le_to_uint(header->bootloader_size);

	fwu->img.ui_firmware.size = le_to_uint(header->firmware_size);
	if (fwu->img.ui_firmware.size) {
		fwu->img.ui_firmware.data = image + IMAGE_AREA_OFFSET;
		if (fwu->img.contains_bootloader)
			fwu->img.ui_firmware.data += fwu->img.bootloader_size;
	}

	if ((fwu->img.bl_version == BL_V6) && header->options_tddi)
		fwu->img.ui_firmware.data = image + IMAGE_AREA_OFFSET;

	fwu->img.ui_config.size = le_to_uint(header->config_size);
	if (fwu->img.ui_config.size) {
		fwu->img.ui_config.data = fwu->img.ui_firmware.data +fwu->img.ui_firmware.size;
	}

	if (fwu->img.contains_bootloader || header->options_tddi)
		fwu->img.contains_disp_config = true;
	else
		fwu->img.contains_disp_config = false;

	if (fwu->img.contains_disp_config) {
		fwu->img.disp_config_offset = le_to_uint(header->dsp_cfg_addr);
		fwu->img.dp_config.size = le_to_uint(header->dsp_cfg_size);
		fwu->img.dp_config.data = image + fwu->img.disp_config_offset;
	} else {
		retval = secure_memcpy(fwu->img.cstmr_product_id,sizeof(fwu->img.cstmr_product_id),header->cstmr_product_id,sizeof(header->cstmr_product_id),PRODUCT_ID_SIZE);
		if (retval < 0) {
			tp_log_warning("%s: Failed to copy custom product ID string\n",__func__);
		}
		fwu->img.cstmr_product_id[PRODUCT_ID_SIZE] = 0;
	}

	fwu->img.contains_firmware_id = header->options_firmware_id;
	if (fwu->img.contains_firmware_id)
		fwu->img.firmware_id = le_to_uint(header->firmware_id);

	retval = secure_memcpy(fwu->img.product_id,
			sizeof(fwu->img.product_id),
			header->product_id,
			sizeof(header->product_id),
			PRODUCT_ID_SIZE);
	if (retval < 0) {
		tp_log_warning("%s: Failed to copy product ID string\n",__func__);
	}
	fwu->img.product_id[PRODUCT_ID_SIZE] = 0;

	fwu->img.lockdown.size = LOCKDOWN_SIZE;
	fwu->img.lockdown.data = image + IMAGE_AREA_OFFSET - LOCKDOWN_SIZE;

	return;
}

static int fwu_parse_image_info(void)
{
	struct image_header_10 *header;

	header = (struct image_header_10 *)fwu->image;

	memset(&fwu->img, 0x00, sizeof(fwu->img));

	tp_log_info("header->major_header_version is %d", header->major_header_version);

	switch (header->major_header_version) {
	case IMAGE_HEADER_VERSION_10:
		fwu_parse_image_header_10();
		break;
	case IMAGE_HEADER_VERSION_05:
	case IMAGE_HEADER_VERSION_06:
		fwu_parse_image_header_05_06();
		break;
	default:
		tp_log_warning("%s: Unsupported image file format (0x%02x)\n",__func__, header->major_header_version);
		return -EINVAL;
	}

	if (fwu->bl_version == BL_V7) {
		if (!fwu->img.contains_flash_config) {
			tp_log_warning("%s: No flash config found in firmware image\n",	__func__);
			return -EINVAL;
		}

		fwu_parse_partition_table(fwu->img.fl_config.data,&fwu->img.blkcount, &fwu->img.phyaddr);

		fwu_compare_partition_tables();
	} else {
		fwu->new_partition_table = false;
	}
	tp_log_info("fwu_parse_image_info out\n");
	return 0;
}

static int fwu_read_flash_status(void)
{
	int retval;
	unsigned char status;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu->fn_ptr->read(rmi4_data,
			fwu->f34_fd.data_base_addr + fwu->off.flash_status,
			&status,
			sizeof(status));
	if (retval < 0) {
		tp_log_err("Failed to read F01 device status\n");
		return retval;
	}

	fwu->in_bl_mode = status >> 7;

	if (fwu->bl_version == BL_V5)
		fwu->flash_status = (status >> 4) & MASK_3BIT;
	else if (fwu->bl_version == BL_V6)
		fwu->flash_status = status & MASK_3BIT;
	else if (fwu->bl_version == BL_V7)
		fwu->flash_status = status & MASK_5BIT;

	if (fwu->flash_status != 0x00) {
		tp_log_err("%s: Flash status = %d, command = 0x%02x\n", __func__, fwu->flash_status, fwu->command);
	}

	retval = fwu->fn_ptr->read(rmi4_data,
			fwu->f34_fd.data_base_addr + fwu->off.flash_cmd,
			&command,
			sizeof(command));
	if (retval < 0) {
		tp_log_err("%s: Failed to read flash command\n", __func__);
		return retval;
	}

	if (fwu->bl_version == BL_V5)
		fwu->command = command & MASK_4BIT;
	else if (fwu->bl_version == BL_V6)
		fwu->command = command & MASK_6BIT;
	else if (fwu->bl_version == BL_V7)
		fwu->command = command;

	return 0;
}

static int fwu_wait_for_idle(int timeout_ms, bool poll)
{
	int count = 0;
	int timeout_count = ((timeout_ms * 1000) / MAX_SLEEP_TIME_US) + 1;

	do {
		usleep_range(MIN_SLEEP_TIME_US, MAX_SLEEP_TIME_US);

		count++;
		if (count == timeout_count)
		{
			tp_log_warning("%s: Timed equeal count:%d  timeout_count:%d \n", __func__,count,timeout_count);
			fwu_read_flash_status();
		}

		if ((fwu->command == CMD_IDLE) && (fwu->flash_status == 0x00))
			return 0;
	} while (count < timeout_count);

	tp_log_err("%s: Timed out waiting for idle status\n", __func__);

	return -ETIMEDOUT;
}

static int fwu_write_f34_v7_command_single_transaction(unsigned char cmd)
{
	int retval;
	unsigned char base;
	struct f34_v7_data_1_5 data_1_5;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	memset(data_1_5.data, 0x00, sizeof(data_1_5.data));

	switch (cmd) {
	case CMD_ERASE_ALL:
		data_1_5.partition_id = CORE_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE_AP;
		break;
	case CMD_ERASE_UI_FIRMWARE:
		data_1_5.partition_id = CORE_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_BL_CONFIG:
		data_1_5.partition_id = GLOBAL_PARAMETERS_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_UI_CONFIG:
		data_1_5.partition_id = CORE_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_DISP_CONFIG:
		data_1_5.partition_id = DISPLAY_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_FLASH_CONFIG:
		data_1_5.partition_id = FLASH_CONFIG_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ERASE_GUEST_CODE:
		data_1_5.partition_id = GUEST_CODE_PARTITION;
		data_1_5.command = CMD_V7_ERASE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		data_1_5.partition_id = BOOTLOADER_PARTITION;
		data_1_5.command = CMD_V7_ENTER_BL;
		break;
	};

	data_1_5.payload_0 = fwu->bootloader_id[0];
	data_1_5.payload_1 = fwu->bootloader_id[1];

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.partition_id,
			data_1_5.data,
			sizeof(data_1_5.data));
	if (retval < 0) {
		tp_log_err("%s: Failed to write single transaction command\n",__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_v7_command(unsigned char cmd)
{
	int retval;
	unsigned char base;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_WRITE_FW:
	case CMD_WRITE_CONFIG:
	case CMD_WRITE_GUEST_CODE:
		command = CMD_V7_WRITE;
		break;
	case CMD_READ_CONFIG:
		command = CMD_V7_READ;
		break;
	case CMD_ERASE_ALL:
		command = CMD_V7_ERASE_AP;
		break;
	case CMD_ERASE_UI_FIRMWARE:
	case CMD_ERASE_BL_CONFIG:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_FLASH_CONFIG:
	case CMD_ERASE_GUEST_CODE:
		command = CMD_V7_ERASE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		command = CMD_V7_ENTER_BL;
		break;
	default:
		tp_log_err("%s: Invalid command 0x%02x\n",__func__, cmd);
		return -EINVAL;
	};

	fwu->command = command;

	switch (cmd) {
	case CMD_ERASE_ALL:
	case CMD_ERASE_UI_FIRMWARE:
	case CMD_ERASE_BL_CONFIG:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_FLASH_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ENABLE_FLASH_PROG:
		retval = fwu_write_f34_v7_command_single_transaction(cmd);
		if (retval < 0)
			return retval;
		else
			return 0;
	default:
		break;
	};

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.flash_cmd,
			&command,
			sizeof(command));
	if (retval < 0) {
		tp_log_err("%s: Failed to write flash command\n",__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_v5v6_command(unsigned char cmd)
{
	int retval;
	unsigned char base;
	unsigned char command;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_IDLE:
		command = CMD_V5V6_IDLE;
		break;
	case CMD_WRITE_FW:
		command = CMD_V5V6_WRITE_FW;
		break;
	case CMD_WRITE_CONFIG:
		command = CMD_V5V6_WRITE_CONFIG;
		break;
	case CMD_WRITE_LOCKDOWN:
		command = CMD_V5V6_WRITE_LOCKDOWN;
		break;
	case CMD_WRITE_GUEST_CODE:
		command = CMD_V5V6_WRITE_GUEST_CODE;
		break;
	case CMD_READ_CONFIG:
		command = CMD_V5V6_READ_CONFIG;
		break;
	case CMD_ERASE_ALL:
		command = CMD_V5V6_ERASE_ALL;
		break;
	case CMD_ERASE_UI_CONFIG:
		command = CMD_V5V6_ERASE_UI_CONFIG;
		break;
	case CMD_ERASE_DISP_CONFIG:
		command = CMD_V5V6_ERASE_DISP_CONFIG;
		break;
	case CMD_ERASE_GUEST_CODE:
		command = CMD_V5V6_ERASE_GUEST_CODE;
		break;
	case CMD_ENABLE_FLASH_PROG:
		command = CMD_V5V6_ENABLE_FLASH_PROG;
		break;
	default:
		tp_log_err("%s: Invalid command 0x%02x\n",__func__, cmd);
		return -EINVAL;
	}

	switch (cmd) {
	case CMD_ERASE_ALL:
	case CMD_ERASE_UI_CONFIG:
	case CMD_ERASE_DISP_CONFIG:
	case CMD_ERASE_GUEST_CODE:
	case CMD_ENABLE_FLASH_PROG:
		retval = fwu->fn_ptr->write(rmi4_data,base + fwu->off.payload,fwu->bootloader_id,sizeof(fwu->bootloader_id));
		if (retval < 0) {
			tp_log_err("%s: Failed to write bootloader ID\n",	__func__);
			return retval;
		}
		break;
	default:
		break;
	};

	fwu->command = command;

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.flash_cmd,
			&command,
			sizeof(command));
	if (retval < 0) {
		tp_log_err("%s: Failed to write command 0x%02x\n",__func__, command);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_command(unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7)
		retval = fwu_write_f34_v7_command(cmd);
	else
		retval = fwu_write_f34_v5v6_command(cmd);

	return retval;
}

static int fwu_write_f34_v7_partition_id(unsigned char cmd)
{
	int retval;
	unsigned char base;
	unsigned char partition;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	switch (cmd) {
	case CMD_WRITE_FW:
		partition = CORE_CODE_PARTITION;
		break;
	case CMD_WRITE_CONFIG:
	case CMD_READ_CONFIG:
		if (fwu->config_area == UI_CONFIG_AREA)
			partition = CORE_CONFIG_PARTITION;
		else if (fwu->config_area == DP_CONFIG_AREA)
			partition = DISPLAY_CONFIG_PARTITION;
		else if (fwu->config_area == PM_CONFIG_AREA)
			partition = GUEST_SERIALIZATION_PARTITION;
		else if (fwu->config_area == BL_CONFIG_AREA)
			partition = GLOBAL_PARAMETERS_PARTITION;
		else if (fwu->config_area == FLASH_CONFIG_AREA)
			partition = FLASH_CONFIG_PARTITION;
		break;
	case CMD_WRITE_GUEST_CODE:
		partition = GUEST_CODE_PARTITION;
		break;
	case CMD_ERASE_ALL:
		partition = CORE_CODE_PARTITION;
		break;
	case CMD_ERASE_BL_CONFIG:
		partition = GLOBAL_PARAMETERS_PARTITION;
		break;
	case CMD_ERASE_UI_CONFIG:
		partition = CORE_CONFIG_PARTITION;
		break;
	case CMD_ERASE_DISP_CONFIG:
		partition = DISPLAY_CONFIG_PARTITION;
		break;
	case CMD_ERASE_FLASH_CONFIG:
		partition = FLASH_CONFIG_PARTITION;
		break;
	case CMD_ERASE_GUEST_CODE:
		partition = GUEST_CODE_PARTITION;
		break;
	case CMD_ENABLE_FLASH_PROG:
		partition = BOOTLOADER_PARTITION;
		break;
	default:
		tp_log_err("%s: Invalid command 0x%02x\n",__func__, cmd);
		return -EINVAL;
	};

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.partition_id,
			&partition,
			sizeof(partition));
	if (retval < 0) {
		tp_log_err("%s: Failed to write partition ID\n",__func__);
		return retval;
	}

	return 0;
}

static int fwu_write_f34_partition_id(unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7)
		retval = fwu_write_f34_v7_partition_id(cmd);
	else
		retval = 0;

	return retval;
}

static int fwu_read_f34_v7_partition_table(void)
{
	int retval;
	unsigned char base;
	unsigned char length[2];
	unsigned short block_number = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	fwu->config_area = FLASH_CONFIG_AREA;

	retval = fwu_write_f34_partition_id(CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.block_number,
			(unsigned char *)&block_number,
			sizeof(block_number));
	if (retval < 0) {
		tp_log_err("%s: Failed to write block number\n",__func__);
		return retval;
	}

	length[0] = (unsigned char)(fwu->flash_config_length & MASK_8BIT);
	length[1] = (unsigned char)(fwu->flash_config_length >> 8);

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.transfer_length,
			length,
			sizeof(length));
	if (retval < 0) {
		tp_log_err("%s: Failed to write transfer length\n",__func__);
		return retval;
	}

	retval = fwu_write_f34_command(CMD_READ_CONFIG);
	if (retval < 0) {
		tp_log_err("%s: Failed to write command\n",__func__);
		return retval;
	}

	retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
	if (retval < 0) {
		tp_log_err("%s: Failed to wait for idle status\n",__func__);
		return retval;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
			base + fwu->off.payload,
			fwu->read_config_buf,
			fwu->partition_table_bytes);
	if (retval < 0) {
		tp_log_err("%s: Failed to read block data\n",__func__);
		return retval;
	}

	return 0;
}

static int fwu_read_f34_v7_queries(void)
{
	int retval;
	unsigned char ii;
	unsigned char base;
	unsigned char index;
	unsigned char offset;
	unsigned char *ptable;
	struct f34_v7_query_0 query_0;
	struct f34_v7_query_1_7 query_1_7;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.query_base_addr;

	retval = fwu->fn_ptr->read(rmi4_data,
			base,
			query_0.data,
			sizeof(query_0.data));
	if (retval < 0) {
		tp_log_err("%s: Failed to read query 0\n",__func__);
		return retval;
	}

	offset = query_0.subpacket_1_size + 1;

	retval = fwu->fn_ptr->read(rmi4_data,
			base + offset,
			query_1_7.data,
			sizeof(query_1_7.data));
	if (retval < 0) {
		tp_log_err("%s: Failed to read queries 1 to 7\n",__func__);
		return retval;
	}

	fwu->bootloader_id[0] = query_1_7.bl_minor_revision;
	fwu->bootloader_id[1] = query_1_7.bl_major_revision;

	fwu->block_size = query_1_7.block_size_15_8 << 8 |
			query_1_7.block_size_7_0;

	fwu->flash_config_length = query_1_7.flash_config_length_15_8 << 8 |
			query_1_7.flash_config_length_7_0;

	fwu->payload_length = query_1_7.payload_length_15_8 << 8 |
			query_1_7.payload_length_7_0;

	fwu->off.flash_status = V7_FLASH_STATUS_OFFSET;
	fwu->off.partition_id = V7_PARTITION_ID_OFFSET;
	fwu->off.block_number = V7_BLOCK_NUMBER_OFFSET;
	fwu->off.transfer_length = V7_TRANSFER_LENGTH_OFFSET;
	fwu->off.flash_cmd = V7_COMMAND_OFFSET;
	fwu->off.payload = V7_PAYLOAD_OFFSET;

	fwu->flash_properties.has_disp_config = query_1_7.has_display_config;
	fwu->flash_properties.has_pm_config = query_1_7.has_guest_serialization;
	fwu->flash_properties.has_bl_config = query_1_7.has_global_parameters;

	fwu->has_guest_code = query_1_7.has_guest_code;

	index = sizeof(query_1_7.data) - V7_PARTITION_SUPPORT_BYTES;

	fwu->partitions = 0;
	for (offset = 0; offset < V7_PARTITION_SUPPORT_BYTES; offset++) {
		for (ii = 0; ii < 8; ii++) {
			if (query_1_7.data[index + offset] & (1 << ii))fwu->partitions++;
		}

		tp_log_debug("%s: Supported partitions: 0x%02x\n",__func__, query_1_7.data[index + offset]);
	}

	fwu->partition_table_bytes = fwu->partitions * 8 + 2;

	kfree(fwu->read_config_buf);
	fwu->read_config_buf = kzalloc(fwu->partition_table_bytes, GFP_KERNEL);
	if (!fwu->read_config_buf) {
		tp_log_err("%s: Failed to alloc mem for fwu->read_config_buf\n",__func__);
		fwu->read_config_buf_size = 0;
		return -ENOMEM;
	}
	fwu->read_config_buf_size = fwu->partition_table_bytes;
	ptable = fwu->read_config_buf;

	retval = fwu_read_f34_v7_partition_table();
	if (retval < 0) {
		tp_log_err("%s: Failed to read partition table\n",__func__);
		return retval;
	}

	fwu_parse_partition_table(ptable, &fwu->blkcount, &fwu->phyaddr);

	return 0;
}

static int fwu_read_f34_v5v6_queries(void)
{
	int retval;
	unsigned char count;
	unsigned char base;
	unsigned char buf[10];
	struct f34_v5v6_flash_properties_2 properties_2;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.query_base_addr;

	retval = fwu->fn_ptr->read(rmi4_data,
			base + V5V6_BOOTLOADER_ID_OFFSET,
			fwu->bootloader_id,
			sizeof(fwu->bootloader_id));
	if (retval < 0) {
		tp_log_err("%s: Failed to read bootloader ID\n",__func__);
		return retval;
	}

	if (fwu->bl_version == BL_V5) {
		fwu->off.properties = V5_PROPERTIES_OFFSET;
		fwu->off.block_size = V5_BLOCK_SIZE_OFFSET;
		fwu->off.block_count = V5_BLOCK_COUNT_OFFSET;
		fwu->off.block_number = V5_BLOCK_NUMBER_OFFSET;
		fwu->off.payload = V5_BLOCK_DATA_OFFSET;
	} else if (fwu->bl_version == BL_V6) {
		fwu->off.properties = V6_PROPERTIES_OFFSET;
		fwu->off.properties_2 = V6_PROPERTIES_2_OFFSET;
		fwu->off.block_size = V6_BLOCK_SIZE_OFFSET;
		fwu->off.block_count = V6_BLOCK_COUNT_OFFSET;
		fwu->off.gc_block_count = V6_GUEST_CODE_BLOCK_COUNT_OFFSET;
		fwu->off.block_number = V6_BLOCK_NUMBER_OFFSET;
		fwu->off.payload = V6_BLOCK_DATA_OFFSET;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
			base + fwu->off.block_size,
			buf,
			2);
	if (retval < 0) {
		tp_log_err("%s: Failed to read block size info\n",__func__);
		return retval;
	}

	batohs(&fwu->block_size, &(buf[0]));

	if (fwu->bl_version == BL_V5) {
		fwu->off.flash_cmd = fwu->off.payload + fwu->block_size;
		fwu->off.flash_status = fwu->off.flash_cmd;
	} else if (fwu->bl_version == BL_V6) {
		fwu->off.flash_cmd = V6_FLASH_COMMAND_OFFSET;
		fwu->off.flash_status = V6_FLASH_STATUS_OFFSET;
	}

	retval = fwu->fn_ptr->read(rmi4_data,
			base + fwu->off.properties,
			fwu->flash_properties.data,
			sizeof(fwu->flash_properties.data));
	if (retval < 0) {
		tp_log_err("%s: Failed to read flash properties\n",__func__);
		return retval;
	}

	count = 4;

	if (fwu->flash_properties.has_pm_config)
		count += 2;

	if (fwu->flash_properties.has_bl_config)
		count += 2;

	if (fwu->flash_properties.has_disp_config)
		count += 2;

	retval = fwu->fn_ptr->read(rmi4_data,
			base + fwu->off.block_count,
			buf,
			count);
	if (retval < 0) {
		tp_log_err("Failed to read block count info\n");
		return retval;
	}

	batohs(&fwu->blkcount.ui_firmware, &(buf[0]));
	batohs(&fwu->blkcount.ui_config, &(buf[2]));

	count = 4;

	if (fwu->flash_properties.has_pm_config) {
		batohs(&fwu->blkcount.pm_config, &(buf[count]));
		count += 2;
	}

	if (fwu->flash_properties.has_bl_config) {
		batohs(&fwu->blkcount.bl_config, &(buf[count]));
		count += 2;
	}

	if (fwu->flash_properties.has_disp_config)
		batohs(&fwu->blkcount.dp_config, &(buf[count]));

	fwu->has_guest_code = false;

	if (fwu->flash_properties.has_query4) {
		retval = fwu->fn_ptr->read(rmi4_data,base + fwu->off.properties_2,properties_2.data,sizeof(properties_2.data));
		if (retval < 0) {
			tp_log_err("%s: Failed to read flash properties 2\n",	__func__);
			return retval;
		}

		if (properties_2.has_guest_code) {
			retval = fwu->fn_ptr->read(rmi4_data,	base + fwu->off.gc_block_count,	buf,	2);
			if (retval < 0) {tp_log_err(	"%s: Failed to read guest code block count\n",		__func__);return retval;
			}

			batohs(&fwu->blkcount.guest_code, &(buf[0]));
			fwu->has_guest_code = true;
		}
	}

	return 0;
}

static int fwu_read_f34_queries(void)
{
	int retval;

	memset(&fwu->blkcount, 0x00, sizeof(fwu->blkcount));
	memset(&fwu->phyaddr, 0x00, sizeof(fwu->phyaddr));

	if (fwu->bl_version == BL_V7)
		retval = fwu_read_f34_v7_queries();
	else
		retval = fwu_read_f34_v5v6_queries();

	return retval;
}

static int fwu_write_f34_v7_blocks(unsigned char *block_ptr,
		unsigned short block_cnt, unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char length[2];
	unsigned short transfer;
	unsigned short max_transfer;
	unsigned short remaining = block_cnt;
	unsigned short block_number = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	retval = fwu_write_f34_partition_id(command);
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.block_number,
			(unsigned char *)&block_number,
			sizeof(block_number));
	if (retval < 0) {
		tp_log_err("Failed to read flash status\n");
		return retval;
	}

	if (fwu->payload_length > (PAGE_SIZE / fwu->block_size))
		max_transfer = PAGE_SIZE / fwu->block_size;
	else
		max_transfer = fwu->payload_length;

	do {
		if (remaining / max_transfer)
			transfer = max_transfer;
		else
			transfer = remaining;

		length[0] = (unsigned char)(transfer & MASK_8BIT);
		length[1] = (unsigned char)(transfer >> 8);

		retval = fwu->fn_ptr->write(rmi4_data,base + fwu->off.transfer_length,length,sizeof(length));
		if (retval < 0) {
			tp_log_err("%s: Failed to write transfer length (%d blocks remaining)\n",	__func__, remaining);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			tp_log_err("%s: Failed to write command (%d blocks remaining)\n",	__func__, remaining);
			return retval;
		}

		retval = fwu->fn_ptr->write(rmi4_data,base + fwu->off.payload,block_ptr,transfer * fwu->block_size);
		if (retval < 0) {
			tp_log_err("%s: Failed to write block data (%d blocks remaining)\n",	__func__, remaining);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			tp_log_err("%s: Failed to wait for idle status (%d blocks remaining)\n",	__func__, remaining);
			return retval;
		}

		block_ptr += (transfer * fwu->block_size);
		remaining -= transfer;
	} while (remaining);

	return 0;
}

static int fwu_write_f34_v5v6_blocks(unsigned char *block_ptr,
		unsigned short block_cnt, unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char block_number[] = {0, 0};
	unsigned short blk;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	block_number[1] |= (fwu->config_area << 5);

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.block_number,
			block_number,
			sizeof(block_number));
	if (retval < 0) {
		tp_log_err("%s: Failed to write block number\n",__func__);
		return retval;
	}

	for (blk = 0; blk < block_cnt; blk++) {
		retval = fwu->fn_ptr->write(rmi4_data,base + fwu->off.payload,block_ptr,fwu->block_size);
		if (retval < 0) {
			tp_log_err("%s: Failed to write block data (block %d)\n",	__func__, blk);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			tp_log_err("%s: Failed to write command for block %d\n",	__func__, blk);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			tp_log_err("%s: Failed to wait for idle status (block %d)\n",	__func__, blk);
			return retval;
		}

		block_ptr += fwu->block_size;
	}

	return 0;
}

static int fwu_write_f34_blocks(unsigned char *block_ptr,
		unsigned short block_cnt, unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7)
		retval = fwu_write_f34_v7_blocks(block_ptr, block_cnt, cmd);
	else
		retval = fwu_write_f34_v5v6_blocks(block_ptr, block_cnt, cmd);

	return retval;
}

static int fwu_read_f34_v7_blocks(unsigned short block_cnt,
		unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char length[2];
	unsigned short transfer;
	unsigned short max_transfer;
	unsigned short remaining = block_cnt;
	unsigned short block_number = 0;
	unsigned short index = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	retval = fwu_write_f34_partition_id(command);
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.block_number,
			(unsigned char *)&block_number,
			sizeof(block_number));
	if (retval < 0) {
		tp_log_err("%s: Failed to write block number\n",__func__);
		return retval;
	}

	if (fwu->payload_length > (PAGE_SIZE / fwu->block_size))
		max_transfer = PAGE_SIZE / fwu->block_size;
	else
		max_transfer = fwu->payload_length;

	do {
		if (remaining / max_transfer)
			transfer = max_transfer;
		else
			transfer = remaining;

		length[0] = (unsigned char)(transfer & MASK_8BIT);
		length[1] = (unsigned char)(transfer >> 8);

		retval = fwu->fn_ptr->write(rmi4_data,base + fwu->off.transfer_length,length,sizeof(length));
		if (retval < 0) {
			tp_log_err("%s: Failed to write transfer length (%d blocks remaining)\n", __func__, remaining);
			return retval;
		}

		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			tp_log_err("%s: Failed to write command (%d blocks remaining)\n",	__func__, remaining);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			tp_log_err("%s: Failed to wait for idle status (%d blocks remaining)\n", __func__, remaining);
			return retval;
		}

		retval = fwu->fn_ptr->read(rmi4_data,base + fwu->off.payload,&fwu->read_config_buf[index],transfer * fwu->block_size);
		if (retval < 0) {
			tp_log_err("%s: Failed to read block data (%d blocks remaining)\n", __func__, remaining);
			return retval;
		}

		index += (transfer * fwu->block_size);
		remaining -= transfer;
	} while (remaining);

	return 0;
}

static int fwu_read_f34_v5v6_blocks(unsigned short block_cnt,
		unsigned char command)
{
	int retval;
	unsigned char base;
	unsigned char block_number[] = {0, 0};
	unsigned short blk;
	unsigned short index = 0;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	base = fwu->f34_fd.data_base_addr;

	block_number[1] |= (fwu->config_area << 5);

	retval = fwu->fn_ptr->write(rmi4_data,
			base + fwu->off.block_number,
			block_number,
			sizeof(block_number));
	if (retval < 0) {
		tp_log_err("%s: Failed to write block number\n",__func__);
		return retval;
	}

	for (blk = 0; blk < block_cnt; blk++) {
		retval = fwu_write_f34_command(command);
		if (retval < 0) {
			tp_log_err("%s: Failed to write read config command\n",	__func__);
			return retval;
		}

		retval = fwu_wait_for_idle(WRITE_WAIT_MS, true);
		if (retval < 0) {
			tp_log_err("%s: Failed to wait for idle status\n",	__func__);
			return retval;
		}

		retval = fwu->fn_ptr->read(rmi4_data,base + fwu->off.payload,&fwu->read_config_buf[index],fwu->block_size);
		if (retval < 0) {
			tp_log_err("%s: Failed to read block data (block %d)\n", __func__, blk);
			return retval;
		}

		index += fwu->block_size;
	}

	return 0;
}

static int fwu_read_f34_blocks(unsigned short block_cnt, unsigned char cmd)
{
	int retval;

	if (fwu->bl_version == BL_V7)
		retval = fwu_read_f34_v7_blocks(block_cnt, cmd);
	else
		retval = fwu_read_f34_v5v6_blocks(block_cnt, cmd);

	return retval;
}

static enum flash_area fwu_go_nogo(void)
{
	int retval;
	enum flash_area flash_area = NONE;
	unsigned char config_id[4];
	unsigned int device_config_id;
	unsigned int image_config_id;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	if (fwu->force_update) {
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	/* Update both UI and config if device is in bootloader mode */
	if (fwu->in_bl_mode) {
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	/* Get device config ID */
	retval = fwu->fn_ptr->read(rmi4_data,fwu->f34_fd.ctrl_base_addr,config_id,sizeof(config_id));
	if (retval < 0) {
		tp_log_err("%s: Failed to read device config ID\n",__func__);
		flash_area = NONE;
		goto exit;
	}
	device_config_id = be_to_uint(config_id);
	tp_log_info("Device config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
			config_id[0],
			config_id[1],
			config_id[2],
			config_id[3]);

        snprintf(fwu->rmi4_data->tp_chip_info, sizeof(fwu->rmi4_data->tp_chip_info),
	        "%s-%x%02x",
	        get_cof_module_name(fwu->rmi4_data->rmi4_mod_info.product_id_string),
	        config_id[2],
	        config_id[3]);

	/* Get image config ID */
	image_config_id = be_to_uint(fwu->img.ui_config.data);
	tp_log_info("Image config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
			fwu->img.ui_config.data[0],
			fwu->img.ui_config.data[1],
			fwu->img.ui_config.data[2],
			fwu->img.ui_config.data[3]);

	tp_log_info("Device config ID %d, .img config ID %d\n", device_config_id, image_config_id);

	if (image_config_id != device_config_id) {
		tp_log_info("img configid is different from device configid\n");
		flash_area = UI_FIRMWARE;
		goto exit;
	}

	flash_area = NONE;

exit:
	if (flash_area == NONE)
		tp_log_info("Nothing needs to be updated\n");
	else
		tp_log_info("Update %s block\n", flash_area == UI_FIRMWARE ? "UI FW" : "CONFIG");
	return flash_area;
}

static int fwu_scan_pdt(void)
{
	int retval;
	unsigned char ii;
	unsigned char intr_count = 0;
	unsigned char intr_off;
	unsigned char intr_src;
	unsigned short addr;
	bool f01found = false;
	bool f34found = false;
	bool f35found = false;
	struct synaptics_rmi4_fn_desc rmi_fd;
	fwu->in_ub_mode = false;

	tp_log_info("fwu_scan_pdt called\n");

	for (addr = PDT_START; addr > PDT_END; addr -= PDT_ENTRY_SIZE) {
		retval = fwu->fn_ptr->read(fwu->rmi4_data,	addr, (unsigned char *)&rmi_fd, sizeof(rmi_fd));
		if (retval < 0)
			return retval;

		if (rmi_fd.fn_number) {
			tp_log_info("Found F%02x\n", rmi_fd.fn_number);
			switch (rmi_fd.fn_number) {
			case SYNAPTICS_RMI4_F01:
				f01found = true;
				fwu->f01_fd.query_base_addr = rmi_fd.query_base_addr;
				fwu->f01_fd.ctrl_base_addr = rmi_fd.ctrl_base_addr;
				fwu->f01_fd.data_base_addr = rmi_fd.data_base_addr;
				fwu->f01_fd.cmd_base_addr = rmi_fd.cmd_base_addr;
				break;
			case SYNAPTICS_RMI4_F34:
				f34found = true;
				fwu->f34_fd.query_base_addr =	rmi_fd.query_base_addr;
				fwu->f34_fd.ctrl_base_addr = rmi_fd.ctrl_base_addr;
				fwu->f34_fd.data_base_addr = rmi_fd.data_base_addr;
			switch (rmi_fd.fn_version) {
				case F34_V0:
					fwu->bl_version = BL_V5;
					break;
				case F34_V1:
					fwu->bl_version = BL_V6;
					break;
				case F34_V2:
					fwu->bl_version = BL_V7;
					break;
				default:
					tp_log_err("%s: Unrecognized F34 version\n", __func__);
					return -EINVAL;
				}
			fwu->intr_mask = 0;
			intr_src = rmi_fd.intr_src_count;
			intr_off = intr_count % 8;
			for (ii = intr_off; ii < (intr_src + intr_off); ii++) {
				fwu->intr_mask |= 1 << ii;
			}
			break;
			case SYNAPTICS_RMI4_F35:
				f35found = true;
				fwu->f35_fd.query_base_addr =	rmi_fd.query_base_addr;
				fwu->f35_fd.ctrl_base_addr = rmi_fd.ctrl_base_addr;
				fwu->f35_fd.data_base_addr = rmi_fd.data_base_addr;
				fwu->f35_fd.cmd_base_addr = rmi_fd.cmd_base_addr;
				break;
			}
		} else {
			break;
		}
	}

	if (!f01found || !f34found) {
		tp_log_err("%s: Failed to find both F01 and F34\n", __func__);
		if (!f35found) {
			tp_log_err("%s: Failed to find F35\n", __func__);
			return -EINVAL;
		} else {
			fwu->in_ub_mode = true;
			tp_log_debug("%s: In microbootloader mode\n",	__func__);
			fwu_recovery_check_status();
			return 0;
		}
	}

	return 0;
}

static int fwu_enter_flash_prog(void)
{
	int retval;
	struct f01_device_control f01_device_control;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	retval = fwu_read_flash_status();
	if (retval < 0)
		return retval;

	if (fwu->in_bl_mode)
		return 0;

	//retval = rmi4_data->irq_enable(rmi4_data, false, true);
	if (retval < 0)
		return retval;

	msleep(INT_DISABLE_WAIT_MS);

	retval = fwu_write_f34_command(CMD_ENABLE_FLASH_PROG);
	if (retval < 0)
		return retval;

	retval = fwu_wait_for_idle(ENABLE_WAIT_MS, true);
	if (retval < 0)
		return retval;

	if (!fwu->in_bl_mode) {
		tp_log_err("%s: BL mode not entered\n", __func__);
		return -EINVAL;
	}

	retval = fwu_scan_pdt();
	if (retval < 0)
		return retval;

	retval = fwu->fn_ptr->read(rmi4_data,
			fwu->f01_fd.ctrl_base_addr,
			f01_device_control.data,
			sizeof(f01_device_control.data));
	if (retval < 0) {
		tp_log_err("%s: Failed to read F01 device control\n", __func__);
		return retval;
	}

	f01_device_control.nosleep = true;
	f01_device_control.sleep_mode = SLEEP_MODE_NORMAL;

	retval = fwu->fn_ptr->write(rmi4_data,
			fwu->f01_fd.ctrl_base_addr,
			f01_device_control.data,
			sizeof(f01_device_control.data));
	if (retval < 0) {
		tp_log_err("%s: Failed to write F01 device control\n", __func__);
		return retval;
	}

	msleep(ENTER_FLASH_PROG_WAIT_MS);

	return retval;
}

int synaptics_s3718_fw_upgrade(unsigned char *fw_data)
{
	int retval;

	tp_log_warning("%s(line %d): begin\n",__func__,__LINE__);
	if (!fwu) {
		tp_log_err("%s(line %d): fwu is null\n",__func__,__LINE__);
		return -ENODEV;
	}

	if (!fwu->initialized) {
		tp_log_err("%s(line %d): fwu is not init\n",__func__,__LINE__);
		return -ENODEV;
	}
	
	if (fwu->rmi4_data->board->esd_support) {
		synaptics_dsx_esd_suspend();
	}

	fwu->ext_data_source = fw_data;
	fwu->config_area = UI_CONFIG_AREA;

	// wake touch panel up
	pm_runtime_get_sync(&fwu->rmi4_data->i2c_client->dev);

	retval = fwu_start_reflash();

#ifdef CONFIG_HUAWEI_DSM
	/* if fw upgrade err, report err */
	if(retval<0) {
		//synp_tp_report_dsm_err(DSM_TP_FWUPDATE_ERROR_NO, retval);
	}
#endif/*CONFIG_HUAWEI_DSM*/

	pm_runtime_put(&fwu->rmi4_data->i2c_client->dev);


	if (fwu->rmi4_data->board->esd_support) {
		synaptics_dsx_esd_resume();
	}
	
	return retval;
}


static ssize_t fwu_sysfs_image_size_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned long size;

	retval = sstrtoul(buf, 10, &size);
	if (retval) {
		tp_log_err("%s(%d):parse file size fail, buf=%s, retval=%d\n", 
			__func__, __LINE__, buf, retval);
		return retval;
	}

	tp_log_info("%s(%d):firmware size:%ld\n", __func__, __LINE__, size);
	if (!fwu) {
		tp_log_err("%s(%d): fwu is null\n", __func__, __LINE__);
		return count;
	}
	
	fwu->image_size = size;
	fwu->data_pos = 0;

	if (fwu->ext_data_source)
		kfree(fwu->ext_data_source);

	fwu->ext_data_source = kzalloc(fwu->image_size, GFP_KERNEL);
	if (!fwu->ext_data_source) {
		tp_log_err("%s: Failed to alloc mem for image data\n",
				__func__);
		return count;
	}

	return count;
}

static ssize_t fwu_sysfs_do_reflash_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval = 0;
	unsigned int input;
	int need_update;

	tp_log_info("%s(%d)start manual update\n", __func__, __LINE__);
	if (sscanf(buf, "%u", &input) != 1) {
		retval = -EINVAL;
		goto exit;
	}

	if (input & LOCKDOWN) {
		fwu->do_lockdown = true;
		input &= ~LOCKDOWN;
	}

	if ((input != NORMAL) && (input != FORCE)) {
		retval = -EINVAL;
		goto exit;
	}

	if (input == FORCE)
		fwu->force_update = true;

	if (fwu->ext_data_source) {
		fwu->image = fwu->ext_data_source;
	} else {
		tp_log_err("%s: firmware data is null\n", __func__);
		goto exit;
	}

	pm_runtime_get_sync(&fwu->rmi4_data->i2c_client->dev);

	/*
	*synaptics_check_fw_s3718_version will do some parse fw image work here
	*/
	need_update = synaptics_check_fw_s3718_version();

	if (fwu->force_update || need_update) {
		retval = synaptics_fw_s3718_update();
		if (retval) {
			tp_log_err("%s:failed update s3718 fw\n",__func__);
		} else {
			tp_log_info("%s:successfully s3718 update fw\n",__func__);
		}
	}
#ifdef CONFIG_APP_INFO
	synaptics_s3718_set_appinfo(fwu->rmi4_data);
#endif
	pm_runtime_put(&fwu->rmi4_data->i2c_client->dev);

	retval = count;

exit:
	if (fwu->ext_data_source)
		kfree(fwu->ext_data_source);
	fwu->ext_data_source = NULL;
	fwu->force_update = FORCE_UPDATE;
	fwu->do_lockdown = DO_LOCKDOWN;

	tp_log_info("%s(%d)start manual update finish\n", __func__, __LINE__);
	return retval;
}

static ssize_t fwu_sysfs_show_image(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	if (count < fwu->config_size) {
#ifdef CONFIG_64BIT
		tp_log_err("%s: Not enough space (%lu bytes) in buffer\n",
				__func__, count);
#else
		tp_log_err("%s: Not enough space (%d bytes) in buffer\n",
				__func__, count);
#endif
		return -EINVAL;
	}

	memcpy(buf, fwu->read_config_buf, fwu->config_size);

	return fwu->config_size;
}

static ssize_t fwu_sysfs_store_image(struct file *data_file,
		struct kobject *kobj, struct bin_attribute *attributes,
		char *buf, loff_t pos, size_t count)
{
	tp_log_info("%s(%d)store image data\n", __func__, __LINE__);
	if (!fwu->ext_data_source) {
		tp_log_err("%s(%d):ext_data_source has not initialized\n", __func__, __LINE__);	
		return count;
	}

	memcpy((void *)(&fwu->ext_data_source[fwu->data_pos]),
			(const void *)buf,
			count);

	fwu->data_pos += count;

	return count;
}

static ssize_t fwu_sysfs_firmware_version_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	tp_log_info("%s %d:Device config ID = 0x%02x 0x%02x 0x%02x 0x%02x\n",
			__func__,__LINE__,
			s3718_config_id[0],
			s3718_config_id[1],
			s3718_config_id[2],
			s3718_config_id[3]);														

	return snprintf(buf, PAGE_SIZE, "%x%02x",  s3718_config_id[2], s3718_config_id[3]);
}

static struct bin_attribute dev_attr_data = {
	.attr = {
		.name = "data",
		.mode = (S_IRUGO | S_IWUSR|S_IWGRP),
	},
	.size = 0,
	.read = fwu_sysfs_show_image,
	.write = fwu_sysfs_store_image,
};

static struct device_attribute attrs[] = {
	__ATTR(dorecovery, S_IWUSR|S_IWGRP,
			synaptics_rmi4_show_error,
			fwu_sysfs_do_recovery_store),
	__ATTR(doreflash, S_IWUSR|S_IWGRP,
			synaptics_rmi4_show_error,
			fwu_sysfs_do_reflash_store),
	__ATTR(imagesize, S_IWUSR|S_IWGRP,
			synaptics_rmi4_show_error,
			fwu_sysfs_image_size_store),
	__ATTR(firmwareVersion, S_IRUGO,
			fwu_sysfs_firmware_version_show,
			synaptics_rmi4_store_error),
};


static int fwu_check_ui_firmware_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.ui_firmware.size / fwu->block_size;

	if (block_count != fwu->blkcount.ui_firmware) {
		tp_log_err("%s: UI firmware size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_ui_configuration_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.ui_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.ui_config) {
		tp_log_err("%s: UI configuration size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_dp_configuration_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.dp_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.dp_config) {
		tp_log_err("%s: Display configuration size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_bl_configuration_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.bl_config.size / fwu->block_size;

	if (block_count != fwu->blkcount.bl_config) {
		tp_log_err("%s: Bootloader configuration size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_check_guest_code_size(void)
{
	unsigned short block_count;

	block_count = fwu->img.guest_code.size / fwu->block_size;
	if (block_count != fwu->blkcount.guest_code) {
		tp_log_err("%s: Guest code size mismatch\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fwu_write_firmware(void)
{
	unsigned short firmware_block_count;

	firmware_block_count = fwu->img.ui_firmware.size / fwu->block_size;

	return fwu_write_f34_blocks((unsigned char *)fwu->img.ui_firmware.data,
			firmware_block_count, CMD_WRITE_FW);
}

static int fwu_erase_configuration(void)
{
	int retval;

	switch (fwu->config_area) {
	case UI_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_UI_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case DP_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_DISP_CONFIG);
		if (retval < 0)
			return retval;
		break;
	case BL_CONFIG_AREA:
		retval = fwu_write_f34_command(CMD_ERASE_BL_CONFIG);
		if (retval < 0)
			return retval;
		break;
	}

	tp_log_debug(
			"%s: Erase command written\n",
			__func__);
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;

	tp_log_debug(
			"%s: Idle status detected\n",
			__func__);

	return retval;
}

static int fwu_erase_guest_code(void)
{
	int retval;

	retval = fwu_write_f34_command(CMD_ERASE_GUEST_CODE);
	if (retval < 0)
		return retval;

	tp_log_debug(
			"%s: Erase command written\n",
			__func__);
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;

	tp_log_debug(
			"%s: Idle status detected\n",
			__func__);

	return 0;
}

static int fwu_erase_all(void)
{
	int retval;

	if (fwu->bl_version == BL_V7) {
		retval = fwu_write_f34_command(CMD_ERASE_UI_FIRMWARE);
		if (retval < 0)
			return retval;

		tp_log_debug("%s: Erase command written\n", __func__);
		retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
		if (retval < 0)
			return retval;

		tp_log_debug("%s: Idle status detected\n", __func__);

		fwu->config_area = UI_CONFIG_AREA;
		retval = fwu_erase_configuration();
		if (retval < 0)
			return retval;
	} else {
		retval = fwu_write_f34_command(CMD_ERASE_ALL);
		if (retval < 0)
			return retval;

		tp_log_debug("%s: Erase all command written\n", __func__);
		retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
		if (retval < 0)
			return retval;

		tp_log_debug("%s: Idle status detected\n", __func__);
	}

	if (fwu->flash_properties.has_disp_config) {
		fwu->config_area = DP_CONFIG_AREA;
		retval = fwu_erase_configuration();
		if (retval < 0)
			return retval;
	}

	if (fwu->new_partition_table && fwu->has_guest_code) {
		retval = fwu_erase_guest_code();
		if (retval < 0)
			return retval;
	}

	return 0;
}

static int fwu_write_configuration(void)
{
	return fwu_write_f34_blocks((unsigned char *)fwu->config_data,
			fwu->config_block_count, CMD_WRITE_CONFIG);
}

static int fwu_write_ui_configuration(void)
{
	fwu->config_area = UI_CONFIG_AREA;
	fwu->config_data = fwu->img.ui_config.data;
	fwu->config_size = fwu->img.ui_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration();
}

static int fwu_write_dp_configuration(void)
{
	fwu->config_area = DP_CONFIG_AREA;
	fwu->config_data = fwu->img.dp_config.data;
	fwu->config_size = fwu->img.dp_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	return fwu_write_configuration();
}

static int fwu_write_flash_configuration(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	fwu->config_area = FLASH_CONFIG_AREA;
	fwu->config_data = fwu->img.fl_config.data;
	fwu->config_size = fwu->img.fl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	if (fwu->config_block_count != fwu->blkcount.fl_config) {
		tp_log_err("%s: Flash configuration size mismatch\n", __func__);
		return -EINVAL;
	}

	retval = fwu_write_f34_command(CMD_ERASE_FLASH_CONFIG);
	if (retval < 0)
		return retval;

	tp_log_debug(
			"%s: Erase flash configuration command written\n", __func__);
	retval = fwu_wait_for_idle(ERASE_WAIT_MS, true);
	if (retval < 0)
		return retval;

	tp_log_debug("%s: Idle status detected\n", __func__);

	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;

	rmi4_data->reset_device(rmi4_data);

	return 0;
}

static int fwu_write_guest_code(void)
{
	int retval;
	unsigned short guest_code_block_count;

	guest_code_block_count = fwu->img.guest_code.size / fwu->block_size;

	retval = fwu_write_f34_blocks((unsigned char *)fwu->img.guest_code.data,
			guest_code_block_count, CMD_WRITE_GUEST_CODE);
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_write_partition_table(void)
{
	int retval;
	unsigned short block_count;

	block_count = fwu->blkcount.bl_config;
	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_size = fwu->block_size * block_count;
	kfree(fwu->read_config_buf);
	fwu->read_config_buf = kzalloc(fwu->config_size, GFP_KERNEL);
	if (!fwu->read_config_buf) {
		tp_log_err("%s: Failed to alloc mem for fwu->read_config_buf\n", __func__);
		fwu->read_config_buf_size = 0;
		return -ENOMEM;
	}
	fwu->read_config_buf_size = fwu->config_size;

	retval = fwu_read_f34_blocks(block_count, CMD_READ_CONFIG);
	if (retval < 0)
		return retval;

	retval = fwu_erase_configuration();
	if (retval < 0)
		return retval;

	retval = fwu_write_flash_configuration();
	if (retval < 0)
		return retval;

	fwu->config_area = BL_CONFIG_AREA;
	fwu->config_data = fwu->read_config_buf;
	fwu->config_size = fwu->img.bl_config.size;
	fwu->config_block_count = fwu->config_size / fwu->block_size;

	retval = fwu_write_configuration();
	if (retval < 0)
		return retval;

	return 0;
}

static int fwu_do_reflash(void)
{
	int retval;

	if (!fwu->new_partition_table) {
		retval = fwu_check_ui_firmware_size();
		if (retval < 0)
			return retval;

		retval = fwu_check_ui_configuration_size();
		if (retval < 0)
			return retval;

		if (fwu->flash_properties.has_disp_config &&fwu->img.contains_disp_config) {
			retval = fwu_check_dp_configuration_size();
			if (retval < 0)return retval;
		}

		if (fwu->has_guest_code && fwu->img.contains_guest_code) {
			retval = fwu_check_guest_code_size();
			if (retval < 0)return retval;
		}
	} else {
		retval = fwu_check_bl_configuration_size();
		if (retval < 0)
			return retval;
	}

	retval = fwu_erase_all();
	if (retval < 0)
		return retval;

	if (fwu->new_partition_table) {
		retval = fwu_write_partition_table();
		if (retval < 0)
			return retval;
		tp_log_info("%s: Partition table programmed\n", __func__);
	}

	retval = fwu_write_firmware();
	if (retval < 0) {
		tp_log_err("fwu_write_firmware failed\n");
		return retval;
	}

	tp_log_info("%s: Firmware programmed\n", __func__);

	fwu->config_area = UI_CONFIG_AREA;
	retval = fwu_write_ui_configuration();
	if (retval < 0) {
		tp_log_err("fwu_write_ui_configuration failed. \n");
		return retval;
	}

	tp_log_info("%s: Configuration programmed\n", __func__);

	if (fwu->flash_properties.has_disp_config &&
			fwu->img.contains_disp_config) {
		retval = fwu_write_dp_configuration();
		if (retval < 0) {
			tp_log_err("fwu_write_dp_configuration failed.\n");
			return retval;
		}

		tp_log_info("%s: Display configuration programmed\n", __func__);
	}

	if (fwu->new_partition_table) {
		if (fwu->has_guest_code && fwu->img.contains_guest_code) {
			retval = fwu_write_guest_code();
			if (retval < 0)return retval;
			tp_log_info("%s: Guest code programmed\n", __func__);
		}
	}

	return retval;
}

static int fwu_start_reflash(void)
{
	int retval = 0;

	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;
	tp_log_info("fwu_start_reflash begin\n");

	retval = fwu_enter_flash_prog();
	if (retval < 0) {
		rmi4_data->reset_device(rmi4_data);
		return retval;
	}
	retval = fwu_do_reflash();
	if (retval < 0) {
		fwu->rmi4_data->tp_fw_update_fail = true;
		tp_log_err("fwu_do_reflash is failed");
	}
	tp_log_info("reset_device\n");
	rmi4_data->reset_device(rmi4_data);

	retval = fwu_read_flash_status();
	rmi4_data->reset_device(rmi4_data);

	if (retval < 0)
		tp_log_err("fwu_read_flash_status read error\n");
	else
		tp_log_info("fwu->in_bl_mode is %s\n", fwu->in_bl_mode?"in bl mode":"normal mode");
	
	return retval;
}

static int fwu_recovery_check_status(void)
{
	int retval;
	unsigned char data_base;
	unsigned char status;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	data_base = fwu->f35_fd.data_base_addr;

	retval = fwu->fn_ptr->read(rmi4_data,
			data_base + F35_ERROR_CODE_OFFSET,
			&status,
			1);
	if (retval < 0) {
		tp_log_err("%s: Failed to read status\n",__func__);
		return retval;
	}

	status = status & MASK_5BIT;

	if (status != 0x00) {
		tp_log_err("%s: Recovery mode status = %d\n",__func__, status);
		return -EINVAL;
	}

	return 0;
}

static int fwu_recovery_erase_completion(void)
{
	int retval;
	unsigned char data_base;
	unsigned char command;
	unsigned char status;
	unsigned int timeout = F35_ERASE_ALL_WAIT_MS / 20;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	data_base = fwu->f35_fd.data_base_addr;

	do {
		command = 0x01;
		retval = fwu->fn_ptr->write(rmi4_data,
				fwu->f35_fd.cmd_base_addr,
				&command,
				sizeof(command));
		if (retval < 0) {
			tp_log_err("%s: Failed to issue command\n",
					__func__);
			return retval;
		}

		do {
			retval = fwu->fn_ptr->read(rmi4_data,
					fwu->f35_fd.cmd_base_addr,
					&command,
					sizeof(command));
			if (retval < 0) {
				tp_log_err("%s: Failed to read command status\n",
						__func__);
				return retval;
			}

			if ((command & 0x01) == 0x00)
				break;

			msleep(20);
			timeout--;
		} while (timeout > 0);

		if (timeout == 0)
			goto exit;

		retval = fwu->fn_ptr->read(rmi4_data,
				data_base + F35_FLASH_STATUS_OFFSET,
				&status,
				sizeof(status));
		if (retval < 0) {
			tp_log_err("%s: Failed to read flash status\n",
					__func__);
			return retval;
		}

		if ((status & 0x01) == 0x00)
			break;

		msleep(20);
		timeout--;
	} while (timeout > 0);

exit:
	if (timeout == 0) {
		tp_log_err("%s: Timed out waiting for flash erase completion\n",
				__func__);
		return -ETIMEDOUT;
	}

	return 0;
}

static int fwu_recovery_erase_all(void)
{
	int retval;
	unsigned char ctrl_base;
	unsigned char command = CMD_F35_ERASE_ALL;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	ctrl_base = fwu->f35_fd.ctrl_base_addr;

	retval = fwu->fn_ptr->write(rmi4_data,
			ctrl_base + F35_CHUNK_COMMAND_OFFSET,
			&command,
			sizeof(command));
	if (retval < 0) {
		tp_log_err("%s: Failed to issue erase all command\n",
				__func__);
		return retval;
	}

	if (fwu->f35_fd.cmd_base_addr) {
		retval = fwu_recovery_erase_completion();
		if (retval < 0){
			tp_log_err("%s: fwu_recovery_erase_completion fail,retval=%d\n",
				__func__,retval);
			return retval;
		}
		else {
			msleep(F35_ERASE_ALL_WAIT_MS);
		}

		retval = fwu_recovery_check_status();
		if (retval < 0)
		{
			tp_log_err("%s: fwu_recovery_check_status fail,retval=%d\n",
				__func__,retval);
			return retval;
		}
	}
	return 0;
}

static int fwu_recovery_write_chunk(void)
{
	int retval;
	unsigned char ctrl_base;
	unsigned char chunk_number[] = {0, 0};
	unsigned char chunk_spare;
	unsigned char chunk_size;
	unsigned char buf[F35_CHUNK_SIZE + 1];
	unsigned short chunk;
	unsigned short chunk_total;
	unsigned short bytes_written = 0;
	unsigned char *chunk_ptr = (unsigned char *)fwu->image;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	ctrl_base = fwu->f35_fd.ctrl_base_addr;

	retval = fwu->fn_ptr->write(rmi4_data,
			ctrl_base + F35_CHUNK_NUM_LSB_OFFSET,
			chunk_number,
			sizeof(chunk_number));
	if (retval < 0) {
		tp_log_err("%s: Failed to write chunk number\n",
				__func__);
		return retval;
	}

	buf[sizeof(buf) - 1] = CMD_F35_WRITE_CHUNK;

	chunk_total = fwu->image_size / F35_CHUNK_SIZE;
	chunk_spare = fwu->image_size % F35_CHUNK_SIZE;
	if (chunk_spare)
		chunk_total++;

	for (chunk = 0; chunk < chunk_total; chunk++) {
		if (chunk_spare && chunk == chunk_total - 1)
			chunk_size = chunk_spare;
		else
			chunk_size = F35_CHUNK_SIZE;

		memset(buf, 0x00, F35_CHUNK_SIZE);
		secure_memcpy(buf, sizeof(buf), chunk_ptr,
					fwu->image_size - bytes_written,
					chunk_size);

		retval = fwu->fn_ptr->write(rmi4_data,
				ctrl_base + F35_CHUNK_DATA_OFFSET,
				buf,
				sizeof(buf));
		if (retval < 0) {
			tp_log_err("%s: Failed to write chunk data (chunk %d)\n",
					__func__, chunk);
			return retval;
		}
		chunk_ptr += chunk_size;
		bytes_written += chunk_size;
	}

	retval = fwu_recovery_check_status();
	if (retval < 0) {
		tp_log_err("%s: Failed to write chunk data\n",
				__func__);
		return retval;
	}

	return 0;
}

static int fwu_recovery_reset(void)
{
	int retval;
	unsigned char ctrl_base;
	unsigned char command = CMD_F35_RESET;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	ctrl_base = fwu->f35_fd.ctrl_base_addr;

	retval = fwu->fn_ptr->write(rmi4_data,
			ctrl_base + F35_CHUNK_COMMAND_OFFSET,
			&command,
			sizeof(command));
	if (retval < 0) {
		tp_log_err("%s: Failed to issue reset command\n",
				__func__);
		return retval;
	}

	msleep(F35_RESET_WAIT_MS);

	return 0;
}
static int fwu_start_recovery(void)
{
	int retval;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;

	rmi4_data->firmware_updating = true;

	tp_log_info("%s(%d):synap,Start of recovery process\n", __func__,__LINE__);

	retval = rmi4_data->irq_enable(rmi4_data, false);
	if (retval < 0) {
		tp_log_err("%s: Failed to disable interrupt\n",
				__func__);
		goto exit;
	}

	retval = fwu_recovery_erase_all();
	if (retval < 0) {
		tp_log_err("%s: Failed to do erase all in recovery mode\n",
				__func__);
		goto exit;
	}

	tp_log_info("%s(%d):synap,External flash erased\n", __func__,__LINE__);

	retval = fwu_recovery_write_chunk();
	if (retval < 0) {
		tp_log_err("%s: Failed to write chunk data in recovery mode\n",
				__func__);
		goto exit;
	}

	tp_log_info("%s(%d): synap,Chunk data programmed\n", __func__,__LINE__);

	retval = fwu_recovery_reset();
	if (retval < 0) {
		tp_log_err("%s: Failed to reset device in recovery mode\n",
				__func__);
		goto exit;
	}

	tp_log_info("%s(%d): synap,Recovery mode reset issued,reset device\n", __func__,__LINE__);

	rmi4_data->reset_device(rmi4_data);

	retval = rmi4_data->irq_enable(rmi4_data, true);
	if (retval < 0) {
		tp_log_err("%s: Failed to enable interrupt\n",__func__);
		goto exit;
	}

	retval = 0;

exit:
	tp_log_info("%s(%d): synap,End of recovery process\n", __func__,__LINE__);

	rmi4_data->firmware_updating = false;

	return retval;
}

int synaptics_fw_s3718_update(void)
{
	int retval;

	if (!fwu)
		return -ENODEV;

	if (!fwu->initialized)
		return -ENODEV;

	if (fwu->in_ub_mode)
		return -ENODEV;

	if (fwu->rmi4_data->board->esd_support) {
		synaptics_dsx_esd_suspend();
	}

	retval = fwu_start_reflash();

	fwu->image = NULL;

	if (fwu->rmi4_data->board->esd_support) {
		synaptics_dsx_esd_resume();
	}

	return retval;
}

#ifdef DO_STARTUP_FW_UPDATE
static void fwu_startup_fw_update_work(struct work_struct *work)
{


	static unsigned char do_once = 1;
#ifdef WAIT_FOR_FB_READY
	unsigned int timeout;
	struct synaptics_rmi4_data *rmi4_data = fwu->rmi4_data;
#endif

	if (!do_once)
		return;
	do_once = 0;

#ifdef WAIT_FOR_FB_READY
	timeout = FB_READY_TIMEOUT_S * 1000 / FB_READY_WAIT_MS + 1;

	while (!rmi4_data->fb_ready) {
		msleep(FB_READY_WAIT_MS);
		timeout--;
		if (timeout == 0) {
			tp_log_err("%s: Timed out waiting for FB ready\n",	__func__);
			return;
		}
	}
#endif

	synaptics_fw_updater(NULL);
	return;
}
#endif

int synaptics_get_fw_data_s3718_boot(void)
{
	int retval;
	struct device *dev = &fwu->rmi4_data->i2c_client->dev;
	const char *module_name;

	if(true==fwu->rmi4_data->recover_up)
	{
		snprintf(fwu->image_name,25,"%s","milan_4322_ihex.bin");
		tp_log_info("%s(%d): recover_up true,firmware_name:%s\n", __func__,__LINE__,fwu->image_name);
	}
	else 
	{
		module_name = get_cof_module_name(fwu->rmi4_data->rmi4_mod_info.product_id_string);
		tp_log_info("%s(%d):productname:%s module_name:%s\n", __func__,__LINE__, fwu->rmi4_data->board->product_name,module_name);
		strncpy(fwu->firmware_name, fwu->rmi4_data->board->product_name,strlen(fwu->rmi4_data->board->product_name));
		strncat(fwu->firmware_name,"_",1);
		strncat(fwu->firmware_name,module_name,strlen(module_name));
		strncat(fwu->firmware_name,"_fw.img",strlen("_fw.img"));
		strncpy(fwu->image_name, fwu->firmware_name, MAX_IMAGE_NAME_LEN);
	}
	tp_log_info("%s(%d):firmware_name:%s\n",__func__,__LINE__,fwu->firmware_name);
	retval = request_firmware(&fwu->fw_entry_boot, fwu->image_name, dev);
	if (retval != 0) {
		tp_log_err("Firmware image %s not available\n", fwu->firmware_name);
		return retval;
	}

	if (fwu->fw_entry_boot == NULL) {
		tp_log_err("fw is null\n");
		return -EINVAL;
	}

	fwu->image = fwu->fw_entry_boot->data;
	fwu->image_size =  fwu->fw_entry_boot->size;

	tp_log_info("%s(%d):Firmware get successful,Firmware image size = %ld\n",__func__,__LINE__, fwu->fw_entry_boot->size);
	return NO_ERR;
}


static ssize_t fwu_sysfs_do_recovery_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int retval;
	unsigned int input;

	if (sscanf(buf, "%u", &input) != 1) {
		retval = -EINVAL;
		tp_log_err("%s: sscanf fail\n",__func__);
		goto exit;
	}

	if (!fwu->in_ub_mode) {
		tp_log_err("%s: Not in microbootloader mode\n",__func__);
		retval = -EINVAL;
		goto exit;
	}

	if (!fwu->ext_data_source)
		return -EINVAL;
	else
		fwu->image = fwu->ext_data_source;

	retval = fwu_start_recovery();
	if (retval < 0) {
		tp_log_err("%s: synap,Failed to do recovery\n",__func__);
		goto exit;
	}

	retval = count;

exit:
	kfree(fwu->ext_data_source);
	fwu->ext_data_source = NULL;
	fwu->image = NULL;
	return retval;
}

void synaptics_fw_data_s3718_release(void)
{
	tp_log_info("release fw resource\n");

	if(fwu->firmware_name)
		kfree(fwu->firmware_name);
	
	if(fwu->image_name)
		kfree(fwu->image_name);

	if (fwu && fwu->fn_ptr) {
		kfree(fwu->fn_ptr);
		fwu->fn_ptr = NULL;
	}
	if (fwu && fwu->fw_entry_sd) {
		vfree(fwu->fw_entry_sd);
		fwu->fw_entry_sd = NULL;
	}
	if (fwu && fwu->fw_entry_boot) {
		release_firmware(fwu->fw_entry_boot);
		fwu->fw_entry_boot = NULL;
	}
	if (fwu) {
		kfree(fwu);
		fwu = NULL;
	}
}

bool synaptics_check_fw_s3718_version(void)
{
	enum flash_area flash_area;
	int retval = 0;

	tp_log_info("synaptics_check_fw_s3718_version in\n");

	retval = fwu_parse_image_info();
	if (retval < 0) {
		tp_log_err("fwu_parse_image_info error\n");
		return false;
	}

	if (fwu->bl_version != fwu->img.bl_version) {
		tp_log_err("%s: Bootloader version mismatch\n",__func__);
		retval = -EINVAL;
	}

	if (!fwu->force_update && fwu->new_partition_table) {
		tp_log_err("%s: Partition table mismatch\n",__func__);
		retval = -EINVAL;
	}

	retval = fwu_read_flash_status();
	if (retval < 0)
		return false;

	if (fwu->in_bl_mode) {
		tp_log_info("%s: Device in bootloader mode\n",__func__);
	}

	flash_area = fwu_go_nogo();
	if (flash_area == UI_FIRMWARE) {
		tp_log_info("update fw\n");
		return true;
	} else {
		tp_log_info("no need to update fw\n");
		return false;
	}
}

static void fwu_startup_fw_s3718_update_work(struct work_struct *work)
{
	int retval=0;
	bool need_update = false;

	tp_log_info("%s:start\n",__func__);
//	msleep(1000);
	pm_runtime_get_sync(&fwu->rmi4_data->i2c_client->dev);
	fwu->rmi4_data->firmware_updating = true;
	retval = synaptics_get_fw_data_s3718_boot();
	if (retval) {
		tp_log_err("%s:load fw data from s3718 bootimage error\n",__func__);
		goto exit;
	}
	tp_log_info("%s:synaptics_fw_s3718_upgrade retval:%d \n",__func__,retval);
	if(true==fwu->rmi4_data->recover_up)
	{
		tp_log_info("%s(%d):fwu_start_recovery start\n",__func__,__LINE__);
		retval=fwu_start_recovery();
		if(retval>=0)
		{
			fwu->rmi4_data->recover_up=false;
			fwu->rmi4_data->flash_prog_mode=false;
		}
		goto exit;
	}
	   
	need_update = synaptics_check_fw_s3718_version();

	if (fwu->force_update || need_update) {
		retval = synaptics_fw_s3718_update();
		if (retval || fwu->rmi4_data->tp_fw_update_fail) {
			tp_log_err("%s:failed update s3718 fw\n",__func__);
		} else {
			tp_log_info("%s:successfully s3718 update fw\n",__func__);
		}
	}
exit:
	fwu->rmi4_data->firmware_updating = false;
#ifdef CONFIG_APP_INFO
	synaptics_s3718_set_appinfo(fwu->rmi4_data);
#endif
	pm_runtime_put(&fwu->rmi4_data->i2c_client->dev);

	if (fwu && fwu->fw_entry_boot) {
			release_firmware(fwu->fw_entry_boot);
			fwu->fw_entry_boot = NULL;
	}
//	synaptics_fw_data_s3718_release();
	return;
}

int synaptics_fw_data_s3718_init(struct synaptics_rmi4_data *rmi4_data)
{
	int retval;
	struct pdt_properties pdt_props;
	int attr_count = 0;
	tp_log_info("%s: in\n", __func__);
	if (fwu) {
		tp_log_err("%s: Handle already exists\n",__func__);
		return 0;
	}

	fwu = kzalloc(sizeof(*fwu), GFP_KERNEL);
	if (!fwu) {
		tp_log_err( "%s: Failed to alloc mem for fwu\n",__func__);
		retval = -ENOMEM;
		goto exit;
	}

	fwu->fn_ptr = kzalloc(sizeof(*(fwu->fn_ptr)), GFP_KERNEL);
	if (!fwu->fn_ptr) {
		tp_log_err( "%s: Failed to alloc mem for fn_ptr\n",__func__);
		retval = -ENOMEM;
		goto exit_free_fwu;
	}

	fwu->image_name = kzalloc(MAX_IMAGE_NAME_LEN, GFP_KERNEL);
	if (!fwu->image_name) {
		tp_log_err( "%s: Failed to alloc mem for image name\n",__func__);
		retval = -ENOMEM;
		goto exit_free_fn_ptr;
	}

	fwu->firmware_name = kzalloc(MAX_IMAGE_NAME_LEN, GFP_KERNEL);
	if (!fwu->firmware_name) {
		tp_log_err("%s: Failed to alloc mem for firmware_name_used\n",
				__func__);
		retval = -ENOMEM;
		goto exit_free_mem;
	}

	fwu->rmi4_data = rmi4_data;
	fwu->fn_ptr->read = rmi4_data->i2c_read;
	fwu->fn_ptr->write = rmi4_data->i2c_write;

	retval = fwu->fn_ptr->read(rmi4_data,
			PDT_PROPS,
			pdt_props.data,
			sizeof(pdt_props.data));
	if (retval < 0) {
		tp_log_debug( "%s: Failed to read PDT properties, assuming 0x00\n",__func__);
	} else if (pdt_props.has_bsr) {
		tp_log_err( "%s: Reflash for LTS not currently supported\n",__func__);
		retval = -ENODEV;
		goto exit_free_mem_firmname;
	}

	retval = fwu_scan_pdt();
	if (retval < 0)
		goto exit_free_mem_firmname;

	if (!fwu->in_ub_mode) {
		retval = fwu_read_f34_queries();
		if (retval < 0)
			goto exit_free_mem_firmname;
	}

	fwu->force_update = FORCE_UPDATE;
	fwu->do_lockdown = DO_LOCKDOWN;
	fwu->initialized = true;

	retval = sysfs_create_bin_file(&rmi4_data->input_dev->dev.kobj,
			&dev_attr_data);
	if (retval < 0) {
		tp_log_err("%s: Failed to create sysfs bin file\n",	__func__);
		goto exit_free_mem;
	}

	for (attr_count = 0; attr_count < ARRAY_SIZE(attrs); attr_count++) {
		retval = sysfs_create_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
		if (retval < 0) {
			tp_log_err("%s: Failed to create sysfs attributes\n", __func__);
			retval = -ENODEV;
			goto exit_remove_attrs;
		}
	}
	tp_log_info("%s: end, everything ok\n", __func__);

	return 0;
exit_remove_attrs:
	for (attr_count--; attr_count >= 0; attr_count--) {
		sysfs_remove_file(&rmi4_data->input_dev->dev.kobj,
				&attrs[attr_count].attr);
	}

	sysfs_remove_bin_file(&rmi4_data->input_dev->dev.kobj, &dev_attr_data);

exit_free_mem_firmname:
	kfree(fwu->firmware_name);
	
exit_free_mem:
	kfree(fwu->image_name);

exit_free_fn_ptr:
	kfree(fwu->fn_ptr);

exit_free_fwu:
	kfree(fwu);
	fwu = NULL;

exit:
	return retval;
}

int synap_update_work_init(struct synaptics_rmi4_data *rmi4_data)
{
	tp_log_info("%s: s3718 synap_update_work_init\n",__func__);
	synaptics_fw_data_s3718_init(rmi4_data);
	
	fwu->fwu_workqueue = create_singlethread_workqueue("fwu_workqueue");
	if(NULL == fwu->fwu_workqueue){
		tp_log_err("%s: failed to create workqueue for fw upgrade\n", __func__);
		synaptics_fw_data_s3718_release();
		return 0;
	}
	INIT_DELAYED_WORK(&fwu->fwu_work, fwu_startup_fw_s3718_update_work);
		queue_delayed_work(fwu->fwu_workqueue,
			&fwu->fwu_work,
			msecs_to_jiffies(STARTUP_FW_UPDATE_DELAY_MS));

	tp_log_info("%s: s3718 synap_update_work_init end\n",__func__);
	return 0;
}


void synaptics_rmi4_fwu_s3718_attn(struct synaptics_rmi4_data *rmi4_data,
		unsigned char intr_mask)
{
	int ret;
	if (!fwu)
		return;

	if (fwu->intr_mask & intr_mask)
	{
		ret = fwu_read_flash_status();
		if(ret)
			tp_log_err("fwu_read_flash_status read error ret = %d\n", ret);
	}
	return;
}
static int file_open_firmware(u8 **buf)
{
	struct inode *inode = NULL;
	mm_segment_t oldfs;
	uint32_t	length;
	struct file *filp;
	const char filename[]="/sdcard/update/synaptics.img";
	const char filename_bak[]="/cache/synaptics.img";
	/* open file */
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(filename, O_RDONLY, S_IRUSR);
	if (IS_ERR(filp)) {

		printk("%s: file %s filp_open error,try %s\n", __FUNCTION__,filename,filename_bak);
		filp = filp_open(filename_bak, O_RDONLY, S_IRUSR);
		if (IS_ERR(filp)) {
			printk("%s: file %s filp_open error\n", __FUNCTION__,filename_bak);
			set_fs(oldfs);
			return -1;
		}
	}

	if (!(filp)->f_op) {
		printk("%s: File Operation Method Error\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	inode = (filp)->f_path.dentry->d_inode;
	if (!inode) {
		printk("%s: Get inode from filp failed\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	/* file's size */
	length = i_size_read(inode->i_mapping->host);
	if (!( length > 0 && length < FIRMWARE_LEN )) {
		printk("file size error\n");
		filp_close((filp), NULL);
		set_fs(oldfs);
		return -1;
	}

	/* allocation buff size */
	*buf = vmalloc(length+(length%2));		/* buf size if even */
	if (!(*buf)) {
		printk("alloctation memory failed\n");
		filp_close((filp), NULL);
		set_fs(oldfs);
		return -1;
	}

	/* read data */
	if ((filp)->f_op->read((filp), (*buf), length, &(filp)->f_pos) != length) {
		printk("%s: file read error\n", __FUNCTION__);
		filp_close((filp), NULL);
		set_fs(oldfs);
		return -1;
	}

	set_fs(oldfs);
	filp_close((filp), NULL);
	return 0;
}

/*    get the firmware data form file in sd card.
 */
int synaptics_get_fw_data_s3718_sd(void)
{
	int retval;

	tp_log_info("synaptics_get_fw_data_sd called\n");

	retval = file_open_firmware(&(fwu->fw_entry_sd));
	if (retval != 0) {
		tp_log_err("file_open_firmware error, code = %d\n", retval);
		return retval;
	}

	fwu->image = fwu->fw_entry_sd;

	if (fwu->image)
		tp_log_info("get sd entry OK++++++\n");

	tp_log_info("synaptics_get_fw_data_sd done\n");
	return NO_ERR;
}
