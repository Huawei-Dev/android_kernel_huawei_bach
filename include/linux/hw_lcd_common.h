/* Copyright (c), Code HUAWEI. All rights reserved.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef HW_LCD_COMMON_H
#define HW_LCD_COMMON_H

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#endif

/* Add dynamic_log interface */
#define LCD_ERR  1
#define LCD_INFO 2
#define LCD_DBG  3

extern int lcd_debug_mask ;

#ifdef CONFIG_HUAWEI_DSM
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
extern struct lcd_pwr_status_t lcd_pwr_status;
#define  LCD_PWR_STAT_GOOD 0x000f
#endif

#ifndef LCD_LOG_ERR
#define LCD_LOG_ERR( x...)					\
do{											\
	if( lcd_debug_mask >= LCD_ERR )			\
	{										\
		printk(KERN_ERR "[LCD_ERR] " x);	\
	}										\
											\
}while(0)
#endif

#ifndef LCD_LOG_INFO
#define LCD_LOG_INFO( x...)					\
do{											\
	if( lcd_debug_mask >= LCD_INFO )		\
	{										\
		printk(KERN_ERR "[LCD_INFO] " x);	\
	}										\
											\
}while(0)
#endif

#ifndef LCD_LOG_DBG
#define LCD_LOG_DBG( x...)					\
do{											\
	if( lcd_debug_mask >= LCD_DBG )			\
	{										\
		printk(KERN_ERR "[LCD_DBG] " x);	\
	}										\
											\
}while(0)
#endif

/* LCD_MDELAY will select mdelay or msleep according value */
#define LCD_MDELAY(time_ms)   	\
	do							\
	{ 							\
		if (time_ms>10)			\
			msleep(time_ms);	\
		else					\
			mdelay(time_ms);	\
	}while(0)

#endif

#ifdef CONFIG_HUAWEI_DSM
extern struct dsm_client *lcd_dclient;
int lcd_report_dsm_err(int type,  int err_value,int add_value);
int mdss_record_dsm_err(u32 *dsi_status);
void lcd_dcm_pwr_status_handler(unsigned long data);
void mdp_underrun_dsm_report(unsigned long num,unsigned long underrun_cnt);
int lcd_report_dsm_status_err(int type, char *err_value, char *add_value);
int lcd_report_dsm_labibb_err(int type, int err_value, int add_value);
#endif

