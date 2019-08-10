#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include <linux/pm_runtime.h>
#include <misc/app_info.h>
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#endif
#include <huawei_platform/touchscreen/hw_tp_common.h>
#include <linux/platform_device.h>

#define DERVER_VERSION_MAJOR 		2
#define DERVER_VERSION_MINOR 		0
#define CUSTOMER_ID 				0
#define MODULE_ID					0
#define PLATFORM_ID					0
#define PLATFORM_MODULE				0
#define ENGINEER_ID					0
#define ic2120						1
#define QUALCOM		1
#define PLAT      QUALCOM

#define CLOCK_INTERRUPT
#define ILI_UPDATE_FW
#define TOUCH_PROTOCOL_B
#define REPORT_THREAD
#define HALL_CHECK
#define TOOL
#define SENSOR_TEST
#define REPORT_PRESSURE
//#define RESET_GPIO  12
//#define GESTURE


#define ROI_DATA_LENGTH 98
#define CHANGE_REPORT_RATE_RESULT_SUCCESS	0xAA
#define CHANGE_REPORT_RATE_RESULT_60HZ_FAIL	0xA1
#define CHANGE_REPORT_RATE_RESULT_120HZ_FAIL	0xA2
#define CHANGE_REPORT_RATE_SUCCESS		0
#define CHANGE_REPORT_RATE_60HZ_FAIL	-1
#define CHANGE_REPORT_RATE_120HZ_FAIL	-2
#define CHANGE_REPORT_RATE_FAIL			-3
#define TRANSMIT_ERROR					3
#define UPGRADE_OK							2
#define NOT_NEED_UPGRADE					1
#define FW_SIZE								(56*1024)
#define SENSOR_TEST_SET_CDC_INITIAL	0xF1
#define SENSOR_TEST_TRCRQ_TRCST_TESET	0x20
#define SENSOR_TEST_COMMAND		0x00
#define SENSOR_TEST_TEAD_DATA_SELECT_CONTROL	0xF6
#define SENSOR_TEST_GET_CDC_RAW_DATA	0xF2
#define POLLINT_INT_TIMES	300

#define FW_OK							0x80
#define AP_STARTADDR					0x00
#define AP_ENDADDR						0xDFFF
#define UPGRADE_TRANSMIT_LEN		256
#define FW_VERSION1					0xD100
#define FW_VERSION2					0xD101
#define FW_VERSION3					0xD102

#define PRODUCT_ID_STARTADDR			0xE000
#define PRODUCT_ID_ENDADDR			0xE006

#define SECTOR_SIZE						0x1000
#define SECTOR_ENDADDR				0xD000

#define REG_LEN							4
#define REG_START_DATA				0x25

#define ENTER_ICE_MODE				0x181062
#define ENTER_ICE_MODE_NO_DATA		0X0

#define EXIT_ICE_MODE					0x1810621B

#define REG_FLASH_CMD					0x041000
#define REG_FLASH_CMD_DATA_ERASE		0x20
#define REG_FLASH_CMD_DATA_PROGRAMME		0x02
#define REG_FLASH_CMD_READ_FLASH_STATUS	0x5
#define REG_FLASH_CMD_WRITE_ENABLE			0x6
#define REG_FLASH_CMD_MEMORY_READ			0x3B
#define REG_FLASH_CMD_RELEASE_FROM_POWER_DOWN		0xab

#define REG_PGM_NUM					0x041004
#define REG_PGM_NUM_TRIGGER_KEY	0x66aa5500
#define REG_PGM_NUM_32				0x66aa551F
#define REG_PGM_NUM_256				0x66aa55FF

#define REG_READ_NUM					0x041009
#define REG_READ_NUM_1				0x0

#define REG_CHK_EN						0x04100B
#define REG_CHK_EN_PARTIAL_READ 		0x3

#define REG_TIMING_SET					0x04100d
#define REG_TIMING_SET_10MS			0x00

#define REG_CHK_FLAG					0x041011
#define FLASH_READ_DATA				0x041012
#define FLASH_STATUS					0x041013
#define REG_PGM_DATA					0x041020

#define WDTRLDT                            			0x5200C
#define WDTRLDT_CLOSE                            	0
#define WDTCNT1                     				0x52020
#define WDTCNT1_OPEN                     		1
#define WDTCNT1_CLOSE                     		0

#define CLOSE_10K_WDT1					0x42000
#define CLOSE_10K_WDT1_VALUE			0x0f154900

#define CLOSE_10K_WDT2					0x42014
#define CLOSE_10K_WDT2_VALUE			0x02

#define CLOSE_10K_WDT3					0x42000
#define CLOSE_10K_WDT3_VALUE			0x00000000

#define DATA_SHIFT_0					0x000000FF
#define DATA_SHIFT_8					0x0000FF00
#define DATA_SHIFT_16					0x00FF0000
#define DATA_SHIFT_24					0xFF000000

#ifdef TP_LOG_NAME
#undef TP_LOG_NAME
#define TP_LOG_NAME "[ILITEK]"
#endif

struct key_info {
	int id;
	int x;
	int y;
	int status;
	int flag;
};

struct touch_info {
	int id;
	int x;
	int y;
	int status;
	int flag;
};

struct ilitek_platform_data {
	bool x_flip;
	bool y_flip;
	bool swap_axes;
	bool reg_en;
	unsigned char grip_left_lsb;
	unsigned char grip_left_msb;
	unsigned char grip_right_lsb;
	unsigned char grip_right_msb;
	unsigned short ub_i2c_addr;
	unsigned short i2c_addr;
	int irq_gpio;
	int power_gpio;
	int power_on_state;
	int reset_gpio;
	int vci_gpio;
	unsigned int ic_type;
	int reset_on_state;
	unsigned long irq_flags;
	unsigned int panel_x;
	unsigned int panel_y;
	unsigned int power_delay_ms;
	unsigned int reset_delay_ms;
	unsigned int reset_active_ms;
	unsigned int sleep_strategy;
	unsigned int debug_data_type;
	struct touch_panel_reg_operate reg_operate;
#ifdef CONFIG_HUAWEI_KERNEL
	unsigned char *regulator_vdd;
	unsigned char *regulator_vbus;
	unsigned int lcd_x;
	unsigned int lcd_y;
	unsigned int lcd_all;
	int dtz_x0;
	int dtz_y0;
	int dtz_x1;
	int dtz_y1;
	const char *product_name;
#ifdef ENABLE_VIRTUAL_KEYS
	struct syanptics_virtual_keys vkeys;
#endif
	bool gesture_enabled;
	bool glove_enabled;
	unsigned int easy_wakeup_supported_gestures;
	bool glove_supported;
	bool esd_support;
	bool low_power_support;
	bool glove_edge_switch_supported;
	bool grip_algorithm_supported;
	bool grip_algorithm_enabled;
	struct syanptics_wakeup_keys *wakeup_keys;
	unsigned char fast_relax_gesture;
	bool holster_supported;
	bool roi_supported;
#endif
	int (*gpio_config)(int gpio, bool configure, int dir, int state);
	struct synaptics_dsx_cap_button_map *cap_button_map;
	bool shutdown_flag;
	bool appinfo_display_flag;
};

struct i2c_data {
	struct input_dev *input_dev;
	struct i2c_client *client;
	struct task_struct *thread;
	int max_x;
	int max_y;
	int min_x;
	int min_y;
	int max_tp;
	int max_btn;
	int x_ch;
	int x_allnode_ch;
	int y_ch;
	int valid_i2c_register;
	int valid_input_register;
	int stop_polling;
	struct semaphore wr_sem;
	int protocol_ver;
	int set_polling_mode;
	int valid_irq_request;
	struct regulator *vdd;
	struct regulator *vcc_i2c;
	int irq;
	int irq_gpio;
	int vci_gpio;
	u32 irq_gpio_flags;
	int rst;
	u32 reset_gpio_flags;
	unsigned char firmware_ver[4];
	int reset_request_success;
	struct workqueue_struct *irq_work_queue;
	struct work_struct irq_work;
	struct timer_list timer;
	int report_status;
	int reset_gpio;
	int irq_status;
	struct mutex mutex;
	struct mutex roi_mutex;
	struct completion complete;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	int keyflag;
	int keycount;
	int key_xlen;
	int key_ylen;
	struct key_info keyinfo[10];
	int touch_flag ;
	struct touch_info touchinfo[10];
	bool firmware_updating;
	u32 *ilitek_rx_cap_max;
	u32 *ilitek_tx_cap_max;
	short ilitek_deltarawimage_max;
	short ilitek_open_threshold;
	short ilitek_short_threshold;
	u32 *ilitek_full_raw_max_cap;
	u32 *ilitek_full_raw_min_cap;
	unsigned char *product_id;

	unsigned char ilitek_roi_data[ROI_DATA_LENGTH];
	int ilitek_roi_enabled;
	
	int glove_status;
	int hall_status;
	int hall_x0;
	int hall_y0;
	int hall_x1;
	int hall_y1;

	int function_ctrl;
	bool force_upgrade;
};



#define TX 27
#define RX 15
#define VIRTUAL_FUN_1	1
#define VIRTUAL_FUN_2	2
#define VIRTUAL_FUN_3	3
#define VIRTUAL_FUN		VIRTUAL_FUN_1
#define BTN_DELAY_TIME	500

#define TOUCH_POINT    0x80
#define TOUCH_KEY      0xC0
#define RELEASE_KEY    0x40
#define RELEASE_POINT    0x00
#define DRIVER_VERSION "aimvF"

#define KEYPAD01_X1	0
#define KEYPAD01_X2	1000
#define KEYPAD02_X1	1000
#define KEYPAD02_X2	2000
#define KEYPAD03_X1	2000
#define KEYPAD03_X2	3000
#define KEYPAD04_X1	3000
#define KEYPAD04_X2	3968
#define KEYPAD_Y	2100

#define ILITEK_I2C_RETRY_COUNT			3
#define ILITEK_I2C_DRIVER_NAME			"ilitek_i2c"
#define ILITEK_FILE_DRIVER_NAME			"ilitek_file"

#define ILITEK_TP_CMD_READ_DATA			    0x10
#define ILITEK_TP_CMD_READ_SUB_DATA		    0x11
#define ILITEK_TP_CMD_GET_RESOLUTION		0x20
#define ILITEK_TP_CMD_GET_KEY_INFORMATION	0x22
#define ILITEK_TP_CMD_SLEEP                 0x02
#define ILITEK_TP_CMD_GET_FIRMWARE_VERSION	0x21
#define ILITEK_TP_CMD_READ_DATA_CONTROL		0xF6
#define ILITEK_TP_CMD_GET_PROTOCOL_VERSION	0x22
#define ILITEK_TP_CMD_SOFTRESET				0x60
#define	ILITEK_TP_CMD_CALIBRATION			0xCC
#define	ILITEK_TP_CMD_CALIBRATION_STATUS	0xCD
#define ILITEK_TP_CMD_ERASE_BACKGROUND		0xCE
#define ILITEK_TP_CMD_READ_ROI_DATA			    0x0E
#define ILITEK_MAX_CAP_LIMIT								300
#define ILITEK_MIN_CAP_LIMIT								100
#define ILITEK_SHORT_LIMIT								10
#define ILITEK_NOISE_LIMIT						30
#define ILITEK_OPEN_LIMIT						3200
#define ILITEK_TX_LIMIT							25
#define ILITEK_RX_LIMIT							40
MODULE_AUTHOR("Steward_Fu");
MODULE_DESCRIPTION("ILITEK I2C touch driver for Android platform");
MODULE_LICENSE("GPL");

int ilitek_poll_int(void) ;
int ilitek_i2c_read_tp_info(void);
int ilitek_i2c_transfer(struct i2c_client*, struct i2c_msg*, int);
int ilitek_i2c_suspend(struct i2c_client*, pm_message_t);
int ilitek_i2c_resume(struct i2c_client*);
int ilitek_i2c_read(struct i2c_client *client, uint8_t *data, int length);
int ilitek_i2c_write_and_read(struct i2c_client *client, uint8_t *cmd,
		int write_len, int delay, uint8_t *data, int read_len);
int ilitek_i2c_write(struct i2c_client *client, uint8_t * cmd, int length);
int inwrite(unsigned int address);
int outwrite(unsigned int address, unsigned int data, int size);
void ilitek_i2c_irq_enable(void);
void ilitek_i2c_irq_disable(void);
void ilitek_reset(int reset_gpio);
void ilitek_set_finish_init_flag(void);
