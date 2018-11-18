
#ifndef __LCDKIT_DSM_H_
#define __LCDKIT_DSM_H_


#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#define  LCD_PWR_STAT_GOOD  0x000f
struct lcd_pwr_status_t
{
	int panel_power_on;
	int lcd_dcm_pwr_status;
	struct timer_list lcd_dsm_t;
	struct tm tm_unblank;
	struct timeval tvl_unblank;
	struct tm tm_lcd_on;
	struct timeval tvl_lcd_on;
	struct tm tm_set_frame;
	struct timeval tvl_set_frame;
	struct tm tm_backlight;
	struct timeval tvl_backlight;
};

extern int lcd_dcm_pwr_status;
extern struct dsm_client *lcd_dclient;
extern struct lcd_pwr_status_t lcd_pwr_status;

int lcdkit_report_dsm_err(int type, char* reg_name, int read_value,int expect_value);
int lcdkit_record_dsm_err(u32 *dsi_status);
void lcdkit_dcm_pwr_status_handler(unsigned long data);
void lcdkit_underrun_dsm_report(unsigned long num,unsigned long underrun_cnt,
        int cpu_freq,unsigned long clk_axi,unsigned long clk_ahb);
#endif

#endif

