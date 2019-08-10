/* kernel\drivers\video\msm\mdss\lcd_hw_debug.c
 * this file is used by the driver team to change the
 * LCD init parameters by putting a config file in the mobile,
 * this function can make the LCD parameter debug easier.
 *
 * Copyright (C) 2010 HUAWEI Technology Co., ltd.
 */

#include "hw_lcd_debug.h"
#include <asm/uaccess.h>
#include <linux/hw_lcd_common.h>

#ifdef CONFIG_HUAWEI_DSM
#include <dsm/dsm_pub.h>
#include <linux/sched.h>
#include "mdss_panel.h"

struct dsm_client *lcd_dclient = NULL;
#endif

#define IS_VALID_CHAR(_ch) ((_ch >= '0' && _ch <= '9')?1:\
	(_ch >= 'a' && _ch <= 'f')?1:(_ch >= 'A' && _ch <= 'F'))?1:0

#define HEX_BASE ((char)16)

static char hex_char_to_value(char ch){
	switch (ch){
	case 'a' ... 'f':
		ch = 10 + (ch - 'a');
		break;
	case 'A' ... 'F':
		ch = 10 + (ch - 'A');
		break;
	case '0' ... '9':
		ch = ch - '0';
		break;
	}
	return ch;
}

static int fget_dtsi_style(unsigned char * buf, int max_num, int fd,off_t *fd_seek)
{
	int cur_seek=*fd_seek;
	unsigned char ch = '\0';
	unsigned char last_char = 'Z';
	int j =0;

	sys_lseek(fd, (off_t)0,0);

	while (j < max_num){
		if ((unsigned)sys_read(fd, &ch, 1) != 1) {
			LCD_LOG_INFO("\n%s: its end of file %d : len = %d\n", __func__, __LINE__, j);
			return j;
		}
		else
		{
			if (!IS_VALID_CHAR(ch)){
				last_char = 'Z';
				cur_seek++;
				sys_lseek(fd, (off_t)cur_seek,0);
				continue;
			}

			if (last_char != 'Z'){
				/*two char value is possible like F0, so make it a single char*/
				--j;
				buf[j] = (buf[j] * HEX_BASE) + hex_char_to_value(ch);
				last_char = 'Z';
			}else{
				buf[j]= hex_char_to_value(ch);
				last_char = buf[j];
			}

			j++;
			cur_seek++;
			sys_lseek(fd, (off_t)cur_seek,0);
		}
	}

	if (j >= max_num){
		LCD_LOG_ERR("%s:Buffer is not enough", __func__);
		j *= -1;
	}

	*fd_seek=cur_seek;
	return j;
}

static bool lcd_resolve_dtsi_config_file(int fd, void **para_table,uint32_t *para_num)
{
	off_t fd_seek=0;
	int num = 0;
	unsigned char *lcd_config_table = NULL;
lcd_config_table = kzalloc(HW_LCD_CONFIG_TABLE_MAX_NUM, 0);

	if(NULL ==  lcd_config_table){
		goto kalloc_err;
	}

	sys_lseek(fd, (off_t)0, 0);

	num = fget_dtsi_style(lcd_config_table, HW_LCD_CONFIG_TABLE_MAX_NUM, fd, &fd_seek);
	if (num <= 0){
		LCD_LOG_INFO("%s read failed with error return:%d", __func__, num);
		goto debug_init_read_fail;
	}

	*para_num = num;
	*para_table = lcd_config_table;
	return TRUE;

debug_init_read_fail:
	kfree(lcd_config_table);
	lcd_config_table = NULL;

kalloc_err:
	para_table = NULL;
	*para_num = 0;
	return FALSE;
}

bool lcd_debug_malloc_dtsi_para(void **para_table, uint32_t *para_num)
{
	int ret = 0 ;
	int fd = 0 ;
	void * table_tmp = NULL;
	int num_tmp =0 ;
	mm_segment_t fs;

	if(NULL==para_table){
		return FALSE;
	}

	fs = get_fs();     /* save previous value */
	set_fs (get_ds()); /* use kernel limit */

	fd = sys_open((const char __force *) HW_LCD_INIT_TEST_PARAM, O_RDONLY, 0);
	if (fd < 0)
	{
		LCD_LOG_INFO("%s: %s file doesn't exsit\n", __func__, HW_LCD_INIT_TEST_PARAM);
		set_fs(fs);
		return FALSE;
	}

	LCD_LOG_INFO( "%s: Config file %s opened. \n", __func__, HW_LCD_INIT_TEST_PARAM);

	//resolve the config file
	ret = lcd_resolve_dtsi_config_file(fd, &table_tmp,&num_tmp);
	sys_close(fd);
	set_fs(fs);

	*para_table = table_tmp;
	*para_num = (uint32_t)num_tmp;

	if (FALSE == ret){
		LCD_LOG_ERR("%s failed to read the init code into memory\n",__func__);
		return FALSE;
	}
	*para_table = table_tmp;

	LCD_LOG_INFO("%s init code is copied into memory\n",__func__);
	return TRUE;
}

static void print_cmds(struct dsi_cmd_desc *cmds, int cnt)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < cnt; ++i){
		printk("%02x ", cmds->dchdr.dtype);
		printk("%02x ", cmds->dchdr.last);
		printk("%02x ", cmds->dchdr.vc);
		printk("%02x ", cmds->dchdr.ack);
		printk("%02x ", cmds->dchdr.wait);
		printk("%04x ", cmds->dchdr.dlen);
		for (j = 0; j < cmds->dchdr.dlen; ++j){
			printk("%02x ", cmds->payload[j]);
		}

		printk("\n");
		cmds++;
	}
}

int hw_parse_dsi_cmds(struct dsi_panel_cmds *pcmds)
{
	int blen = 0, len = 0;
	char *buf = NULL, *bp = NULL;
	struct dsi_ctrl_hdr *dchdr;
	int i = 0, cnt = 0;

	memset(pcmds, 0, sizeof(struct dsi_panel_cmds));

	if (!lcd_debug_malloc_dtsi_para((void **)&buf, &blen)) {
		LCD_LOG_ERR("%s: failed\n", __func__);
		return -ENOMEM;
	}

	/* scan dcs commands */
	bp = buf;
	len = blen;
	cnt = 0;
	while (len > sizeof(*dchdr)) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		dchdr->dlen = ntohs(dchdr->dlen);
		if (dchdr->dlen > len) {
			LCD_LOG_ERR("%s: dtsi cmd=%x error, len=%d",__func__, dchdr->dtype, dchdr->dlen);
			return -ENOMEM;
		}
		bp += sizeof(*dchdr);
		len -= sizeof(*dchdr);
		bp += dchdr->dlen;
		len -= dchdr->dlen;
		cnt++;
	}

	if (len != 0) {
		LCD_LOG_ERR("%s: dcs_cmd=%x len=%d error!",__func__, buf[0], blen);
		kfree(buf);
		return -ENOMEM;
	}

	pcmds->cmds = kzalloc(cnt * sizeof(struct dsi_cmd_desc),GFP_KERNEL);
	if (!pcmds->cmds){
		kfree(buf);
		return -ENOMEM;
	}

	pcmds->cmd_cnt = cnt;
	pcmds->buf = buf;
	pcmds->blen = blen;

	bp = buf;
	len = blen;
	for (i = 0; i < cnt; i++) {
		dchdr = (struct dsi_ctrl_hdr *)bp;
		len -= sizeof(*dchdr);
		bp += sizeof(*dchdr);
		pcmds->cmds[i].dchdr = *dchdr;
		pcmds->cmds[i].payload = bp;
		bp += dchdr->dlen;
		len -= dchdr->dlen;
	}

	print_cmds(pcmds->cmds, pcmds->cmd_cnt);

	LCD_LOG_INFO("%s: dcs_cmd=%x len=%d, cmd_cnt=%d\n", __func__,
		pcmds->buf[0], pcmds->blen, pcmds->cmd_cnt);

	return 0;
}

bool hw_free_dsi_cmds(struct dsi_panel_cmds *pcmds)
{

	if (pcmds->buf)
		kfree(pcmds->buf);

	if (pcmds->cmds)
		kfree(pcmds->cmds);
	LCD_LOG_INFO("The new LCD config region has been freed\n");
	return TRUE;
}

#ifdef CONFIG_HUAWEI_DSM
int mdss_record_dsm_err(u32 *dsi_status)
{
	if( NULL == lcd_dclient )
	{
		LCD_LOG_ERR("%s: there is no lcd_dclient!\n", __func__);
		return -1;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(lcd_dclient))
	{
		/* buffer is busy */
		LCD_LOG_ERR("%s: buffer is busy!\n", __func__);
		return -1;
	}

	LCD_LOG_INFO("%s: entry!\n", __func__);
	if (dsi_status[0])
		dsm_client_record(lcd_dclient, "DSI_ACK_ERR_STATUS is wrong ,err number :%x\n", dsi_status[0]);

	if (dsi_status[1] & 0x0111)
		dsm_client_record(lcd_dclient, "DSI_TIMEOUT_STATUS is wrong ,err number :%x\n", dsi_status[1]);

	if (dsi_status[2] & 0x011111)
		dsm_client_record(lcd_dclient, "DSI_DLN0_PHY_ERR is wrong ,err number :%x\n", dsi_status[2]);
	//Disable check reg 00c because the register can not show dsi status accurately
	if (dsi_status[3] & 0xcccc4489)
		return 0;
		//dsm_client_record(lcd_dclient, "DSI_FIFO_STATUS is wrong ,err number :%x\n", dsi_status[3]);
	if (dsi_status[4] & 0x80000000)
		dsm_client_record(lcd_dclient, "DSI_STATUS is wrong ,err number :%x\n", dsi_status[4]);

	dsm_client_notify(lcd_dclient, DSM_LCD_MDSS_DSI_ISR_ERROR_NO);

	return 0;
}
/* remove APR web LCD report log information  */
int lcd_report_dsm_err(int type, int err_value,int add_value)
{
	/* we will ignore lcd error 20100 for 0x51 */
	if ((DSM_LCD_MIPI_ERROR_NO == type && 0x51 == add_value)|| DSM_LCD_MDSS_DSI_ISR_ERROR_NO == type)
	{
		return 0;
	}

	LCD_LOG_INFO("%s: entry! type:%d\n", __func__, type);


	if( NULL == lcd_dclient )
	{
		LCD_LOG_ERR("%s: there is not lcd_dclient!\n", __func__);
		return -1;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(lcd_dclient))
	{
		/* buffer is busy */
		LCD_LOG_ERR("%s: buffer is busy!\n", __func__);
		return -1;
	}

	/* lcd report err according to err type */
	switch(type)
	{
		case DSM_LCD_STATUS_ERROR_NO:
			dsm_client_record(lcd_dclient, "lcd register %x status wrong, value :%x\n",add_value, err_value);
			break;
		case DSM_LCD_MIPI_ERROR_NO:
			dsm_client_record(lcd_dclient, "mipi transmit register %x time out ,err number :%x\n", add_value, err_value );
			break;
		/* add for lcd esd */
		case DSM_LCD_ESD_STATUS_ERROR_NO:
			dsm_client_record(lcd_dclient, "LCD STATUS ERROR %x read data = %x\n", add_value, err_value );
			break;
		case DSM_LCD_ESD_RESET_ERROR_NO:
			dsm_client_record(lcd_dclient, "LCD RESET register %x read data =%x\n", add_value, err_value );
			break;
		case DSM_LCD_MDSS_IOMMU_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss iommu attach/detach or map memory fail (%d)\n", err_value);
			break;
		case DSM_LCD_MDSS_PIPE_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss pipe status error (%d)\n",err_value);
			break;
		case DSM_LCD_MDSS_PINGPONG_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss wait pingpong time out (%d)\n",err_value);
			break;
		case DSM_LCD_MDSS_VSP_VSN_ERROR_NO:
			dsm_client_record(lcd_dclient, "get vsp/vsn(%d) register fail (%d) \n", add_value, err_value);
			break;
		case DSM_LCD_MDSS_ROTATOR_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss rotator queue fail (%d) \n",err_value);
			break;
		case DSM_LCD_MDSS_FENCE_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss sync_fence_wait fail (%d) \n", err_value);
			break;
		case DSM_LCD_MDSS_CMD_STOP_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss stop cmd time out (%d) \n", err_value);
			break;
		case DSM_LCD_MDSS_VIDEO_DISPLAY_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss commit without wait! ctl=%d", err_value);
			break;
		case DSM_LCD_MDSS_MDP_CLK_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss mdp clk can't be turned off\n", err_value);
			break;
		case DSM_LCD_MDSS_MDP_BUSY_ERROR_NO:
			dsm_client_record(lcd_dclient, "mdss mdp dma tx time out (%d)\n", err_value);
			break;
		default:
			break;
	}

	dsm_client_notify(lcd_dclient, type);
	return 0;
}

/* just report status regs error */
int lcd_report_dsm_status_err(int type, char *err_value, char *add_value)
{
	LCD_LOG_INFO("%s: entry! type:%d\n", __func__, type);

	if( NULL == lcd_dclient )
	{
		LCD_LOG_ERR("%s: there is not lcd_dclient!\n", __func__);
		return -1;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(lcd_dclient))
	{
		/* buffer is busy */
		LCD_LOG_ERR("%s: buffer is busy!\n", __func__);
		return -1;
	}
	dsm_client_record(lcd_dclient, "lcd register %s status wrong, value :%s\n", add_value, err_value);

	dsm_client_notify(lcd_dclient, type);
	return 0;
}

/* just report vsp/vsn error */
int lcd_report_dsm_labibb_err(int type, int err_value, int add_value)
{
	LCD_LOG_INFO("%s: entry! type:%d\n", __func__, type);

	if( NULL == lcd_dclient )
	{
		LCD_LOG_ERR("%s: there is not lcd_dclient!\n", __func__);
		return -1;
	}

	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(lcd_dclient))
	{
		/* buffer is busy */
		LCD_LOG_ERR("%s: buffer is busy!\n", __func__);
		return -1;
	}
	dsm_client_record(lcd_dclient, "lcd vsp/vsn voltage is not in the normal range, register 0x%x status wrong, value :0x%x\n", add_value, err_value);

	dsm_client_notify(lcd_dclient, type);
	return 0;
}

/* remove APR web LCD report log information  */

/*
*
*bit 0  do unblank
*bit 1  lcd on
*bit 2  set frame
*bit 3  set backlgiht
*/
/*if did the operation the bit will be set to 1 or the bit is 0*/
void lcd_dcm_pwr_status_handler(unsigned long data)
{
	/*optimize 20110 report strategy for device monitor.*/
	if(lcd_pwr_status.lcd_dcm_pwr_status != LCD_PWR_STAT_GOOD)
	{
		show_state_filter(TASK_UNINTERRUPTIBLE);
		dsm_client_record(lcd_dclient, "lcd power status wrong, value :%x\n",lcd_pwr_status.lcd_dcm_pwr_status);
		dsm_client_record(lcd_dclient, "lcd power status :bit 0  do unblank\n");
		dsm_client_record(lcd_dclient, "lcd power status :bit 1  lcd on\n");
		dsm_client_record(lcd_dclient, "lcd power status :bit 2  set frame\n");
		dsm_client_record(lcd_dclient, "lcd power status :bit 3  set backlgiht\n");
		dsm_client_record(lcd_dclient, "lcd power status :if did the operation the bit will be set to 1 or the bit is 0\n");
		dsm_client_record(lcd_dclient,"unblank at [%d-%d-%d]%d:%d:%d:%d\n",lcd_pwr_status.tm_unblank.tm_year + 1900,lcd_pwr_status.tm_unblank.tm_mon+1,
						lcd_pwr_status.tm_unblank.tm_mday,lcd_pwr_status.tm_unblank.tm_hour,lcd_pwr_status.tm_unblank.tm_min,lcd_pwr_status.tm_unblank.tm_sec,lcd_pwr_status.tvl_unblank.tv_usec%1000);
		dsm_client_record(lcd_dclient,"lcd on at [%d-%d-%d]%d:%d:%d:%d\n",lcd_pwr_status.tm_lcd_on.tm_year + 1900,lcd_pwr_status.tm_lcd_on.tm_mon+1,
						lcd_pwr_status.tm_lcd_on.tm_mday,lcd_pwr_status.tm_lcd_on.tm_hour,lcd_pwr_status.tm_lcd_on.tm_min,lcd_pwr_status.tm_lcd_on.tm_sec,lcd_pwr_status.tvl_lcd_on.tv_usec%1000);
		dsm_client_record(lcd_dclient,"set frame at [%d-%d-%d]%d:%d:%d:%d\n",lcd_pwr_status.tm_set_frame.tm_year + 1900,lcd_pwr_status.tm_set_frame.tm_mon+1,
						lcd_pwr_status.tm_set_frame.tm_mday,lcd_pwr_status.tm_set_frame.tm_hour,lcd_pwr_status.tm_set_frame.tm_min,lcd_pwr_status.tm_set_frame.tm_sec,lcd_pwr_status.tvl_set_frame.tv_usec%1000);
		dsm_client_record(lcd_dclient,"set backlight at [%d-%d-%d]%d:%d:%d:%d\n",lcd_pwr_status.tm_backlight.tm_year + 1900,lcd_pwr_status.tm_backlight.tm_mon+1,
						lcd_pwr_status.tm_backlight.tm_mday,lcd_pwr_status.tm_backlight.tm_hour,lcd_pwr_status.tm_backlight.tm_min,lcd_pwr_status.tm_backlight.tm_sec,lcd_pwr_status.tvl_backlight.tv_usec%1000);
		dsm_client_notify(lcd_dclient, DSM_LCD_POWER_STATUS_ERROR_NO_QCOM);
	}
	lcd_pwr_status.lcd_dcm_pwr_status = 0;
}
/* remove APR web LCD report log information  */

void mdp_underrun_dsm_report(unsigned long num,unsigned long underrun_cnt)
{
	/* try to get permission to use the buffer */
	if(dsm_client_ocuppy(lcd_dclient))
	{
		/* buffer is busy */
		LCD_LOG_ERR("%s: buffer is busy!\n", __func__);
		return;
	}
	dsm_client_record(lcd_dclient, "Lcd underrun detected for ctl=%d,count=%d \n"      \
		,num,underrun_cnt);
	dsm_client_notify(lcd_dclient, DSM_LCD_MDSS_UNDERRUN_ERROR_NO);
}
#endif
