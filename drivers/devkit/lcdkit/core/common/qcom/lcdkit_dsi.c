#include "mdss_dsi.h"
#include "lcdkit_dbg.h"
#include "lcdkit_common.h"
#include "lcdkit_dbg.h"
#include "huawei_ts_kit.h"
#include <linux/lcdkit_dsm.h>
#include <linux/regulator/driver.h>

extern int is_device_reboot;
extern char *saved_command_line;
extern const char *default_panel_name;

struct platform_device *lcdkit_dsi_ctrl_pdev;

bool enable_PT_test = 0;
module_param_named(enable_PT_test, enable_PT_test, bool, S_IRUGO | S_IWUSR);

bool lcdkit_is_default_panel(void)
{
	if(default_panel_name == NULL)
	{
		LCDKIT_ERR("The point of panel name is NULL!");
		return false;
	}

	if (!strncmp(default_panel_name,
            "AUO_OTM1901A 5.2' VIDEO TFT 1080 x 1920 DEFAULT",
            strlen(default_panel_name)))
	{
		return true;
	}

	return false;
}

/* a requirement about the production line test the leaky current of LCD  */
static bool lcdkit_is_factory_mode(void)
{
	static enum lcd_run_mode_enum lcd_run_mode = LCD_RUN_MODE_INIT;

	if(LCD_RUN_MODE_INIT == lcd_run_mode)
	{
		lcd_run_mode = LCD_RUN_MODE_NORMAL;

		if(saved_command_line != NULL)
		{
			if(strstr(saved_command_line,
                    "androidboot.huawei_swtype=factory") != NULL)
			{
				lcd_run_mode = LCD_RUN_MODE_FACTORY;
			}
		}

		LCDKIT_WARNING("lcd run mode is %d\n", lcd_run_mode);
	}

	return (LCD_RUN_MODE_FACTORY == lcd_run_mode) ? 1 : 0;

}

static void mdss_dsi_parse_lane_swap(struct device_node *np, char *dlane_swap)
{
	const char *data;

	*dlane_swap = DSI_LANE_MAP_0123;
	data = of_get_property(np, "hw,lcdkit-lane-map", NULL);
	if (data) {
		if (!strcmp(data, "lane_map_3012"))
			*dlane_swap = DSI_LANE_MAP_3012;
		else if (!strcmp(data, "lane_map_2301"))
			*dlane_swap = DSI_LANE_MAP_2301;
		else if (!strcmp(data, "lane_map_1230"))
			*dlane_swap = DSI_LANE_MAP_1230;
		else if (!strcmp(data, "lane_map_0321"))
			*dlane_swap = DSI_LANE_MAP_0321;
		else if (!strcmp(data, "lane_map_1032"))
			*dlane_swap = DSI_LANE_MAP_1032;
		else if (!strcmp(data, "lane_map_2103"))
			*dlane_swap = DSI_LANE_MAP_2103;
		else if (!strcmp(data, "lane_map_3210"))
			*dlane_swap = DSI_LANE_MAP_3210;
	}
}

void lcdkit_set_dsi_ctrl_pdev(struct platform_device *pdev)
{
	static char already_set = 0;

	/* judge if already set or not*/
	if (already_set)
	{
		LCDKIT_ERR("dsi ctrl pdev already set\n");
	}
	else
	{
		lcdkit_dsi_ctrl_pdev = pdev;
		already_set = 1;

        LCDKIT_DEBUG("set dsi ctrl pdev: %p.\n", pdev);
	}

	return;
}

/* get global ctrl_pdata pointer */
struct platform_device *lcdkit_get_dsi_ctrl_pdev(void)
{
    LCDKIT_DEBUG("get dsi ctrl pdev: %p.\n", lcdkit_dsi_ctrl_pdev);

	return lcdkit_dsi_ctrl_pdev;
}

void *lcdkit_get_dsi_ctrl_pdata(void)
{
    return platform_get_drvdata(lcdkit_get_dsi_ctrl_pdev());
}

void *lcdkit_get_dsi_panel_pdata(void)
{
    struct mdss_dsi_ctrl_pdata *ctrl = lcdkit_get_dsi_ctrl_pdata();
    return &ctrl->panel_data.panel_info;
}

static void lcdkit_ctrl_gpio_init(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
    LCDKIT_DEBUG("te: ctrl %d: lcdkit %d\n",
        ctrl_pdata->disp_te_gpio, lcdkit_info.panel_infos.gpio_lcd_te);

    LCDKIT_DEBUG("bkl: ctrl %d: lcdkit %d\n",
        ctrl_pdata->bklt_en_gpio, lcdkit_info.panel_infos.gpio_lcd_bl);

    LCDKIT_DEBUG("enable: ctrl %d: lcdkit %d\n",
        ctrl_pdata->disp_en_gpio, lcdkit_info.panel_infos.gpio_lcd_en);

    LCDKIT_DEBUG("mode: ctrl %d: lcdkit %d\n",
        ctrl_pdata->mode_gpio, lcdkit_info.panel_infos.gpio_lcd_mode);

    LCDKIT_DEBUG("reset: ctrl %d: lcdkit %d\n",
        ctrl_pdata->rst_gpio, lcdkit_info.panel_infos.gpio_lcd_reset);

    if (lcdkit_info.panel_infos.gpio_lcd_te)
    {
    	ctrl_pdata->disp_te_gpio = lcdkit_info.panel_infos.gpio_lcd_te;
	    if (!gpio_is_valid(ctrl_pdata->disp_te_gpio))
		    LCDKIT_ERR(":%d, TE gpio not specified\n", __LINE__);
    }

    if (lcdkit_info.panel_infos.gpio_lcd_bl)
    {
        ctrl_pdata->bklt_en_gpio = lcdkit_info.panel_infos.gpio_lcd_bl;
        if (!gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		    LCDKIT_ERR(": bklt_en gpio not specified\n");
    }

    if (lcdkit_info.panel_infos.gpio_lcd_en)
    {
        ctrl_pdata->disp_en_gpio = lcdkit_info.panel_infos.gpio_lcd_en;
        if (!gpio_is_valid(ctrl_pdata->disp_en_gpio))
		    LCDKIT_ERR(": disp_en gpio not specified\n");
    }

    if (lcdkit_info.panel_infos.gpio_lcd_mode)
    {
        ctrl_pdata->mode_gpio = lcdkit_info.panel_infos.gpio_lcd_mode;
        if (!gpio_is_valid(ctrl_pdata->mode_gpio))
		    LCDKIT_ERR(": mode gpio not specified\n");
    }

    if (lcdkit_info.panel_infos.gpio_lcd_reset)
    {
        ctrl_pdata->rst_gpio = lcdkit_info.panel_infos.gpio_lcd_reset;
        if (!gpio_is_valid(ctrl_pdata->rst_gpio))
		    LCDKIT_ERR(":%d, reset gpio not specified\n", __LINE__);
    }

	return;
}

static int lcdkit_gpio_config(unsigned gpio, const char *label)
{
	int rc = 0;

   	if (gpio_is_valid(gpio))
    {
    	rc = gpio_request(gpio, label);
    	if (rc) {
    		LCDKIT_ERR("request gpio %s:%d failed, rc=%d\n", label, gpio, rc);
    		return rc;
    	}
   	}

	return rc;
}

static int lcdkit_enable_gpio(unsigned gpio)
{
    int rc;

    if (gpio_is_valid(gpio))
    {
        rc = gpio_direction_output(gpio, 1);
        if (rc) {
            LCDKIT_ERR(": unable to set gpio dir \n");
            return rc;
		}
	}

	return rc;
}

static int lcdkit_disable_gpio(unsigned gpio)
{
	if (gpio_is_valid(gpio))
    {
	    gpio_set_value(gpio, 0);
		gpio_free(gpio);
	}

    return 0;
}

static void lcdkit_dump_regulator(struct regulator *regulator)
{
    if (!regulator)
        return;

    LCDKIT_DEBUG("always_on          = 0x%d\n", regulator->always_on);
    LCDKIT_DEBUG("bypass             = 0x%d\n", regulator->bypass);
    LCDKIT_DEBUG("uA_load            = 0x%d\n", regulator->uA_load);
    LCDKIT_DEBUG("min_uV             = 0x%d\n", regulator->min_uV);
    LCDKIT_DEBUG("max_uV             = 0x%d\n", regulator->max_uV);
    LCDKIT_DEBUG("enabled            = 0x%d\n", regulator->enabled);

    if (regulator->supply_name)
    {
        LCDKIT_DEBUG("supply_name    = %s\n",   regulator->supply_name);
    }

    return;
}

static void lcdkit_dump_dss_vreg(struct dss_vreg *vreg)
{
    if (!vreg)
        return;

    lcdkit_dump_regulator((void*)vreg->vreg);

    LCDKIT_DEBUG("name               = %s\n",   vreg->vreg_name);
    LCDKIT_DEBUG("min_voltage        = 0x%d\n", vreg->min_voltage);
    LCDKIT_DEBUG("max_voltage        = 0x%d\n", vreg->max_voltage);
    LCDKIT_DEBUG("enable_load        = 0x%d\n", vreg->enable_load);
    LCDKIT_DEBUG("disable_load       = 0x%d\n", vreg->disable_load);
    LCDKIT_DEBUG("pre_on_sleep       = 0x%d\n", vreg->pre_on_sleep);
    LCDKIT_DEBUG("post_on_sleep      = 0x%d\n", vreg->post_on_sleep);
    LCDKIT_DEBUG("pre_off_sleep      = 0x%d\n", vreg->pre_off_sleep);
    LCDKIT_DEBUG("post_off_sleep     = 0x%d\n", vreg->post_off_sleep);
}

static int lcdkit_get_regulator_index(struct dss_vreg *vreg, int cnt, char* name)
{
    int i;
    for (i = 0; i < cnt; i++)
    {
        if (0 == strncmp(vreg[i].vreg_name, name, strlen(name)))
        {
            LCDKIT_DEBUG("get %d regulator name is : %s.\n", i, vreg[i].vreg_name);
            return i;
        }
    }

    return i;
}

//mdss_dsi_get_dt_vreg_data
static void lcdkit_regulator_init(struct dss_vreg *vreg, int cnt)
{
    int i;
    for (i = 0; i < cnt; i++)
    {
        if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_VDD_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_info.panel_infos.lcdanalog_vcc);
            update_value(vreg[i].max_voltage, lcdkit_info.panel_infos.lcdanalog_vcc);
            update_value(vreg[i].post_on_sleep, lcdkit_info.panel_infos.delay_af_vci_on);
            update_value(vreg[i].post_off_sleep, lcdkit_info.panel_infos.delay_af_vci_off);

            lcdkit_dump_dss_vreg(&vreg[i]);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_VDDIO_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_info.panel_infos.lcdio_vcc);
            update_value(vreg[i].max_voltage, lcdkit_info.panel_infos.lcdio_vcc);
            update_value(vreg[i].post_on_sleep, lcdkit_info.panel_infos.delay_af_iovcc_on);
            update_value(vreg[i].post_off_sleep, lcdkit_info.panel_infos.delay_af_iovcc_off);

            lcdkit_dump_dss_vreg(&vreg[i]);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_BIAS_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_info.panel_infos.lcd_bias);
            update_value(vreg[i].max_voltage, lcdkit_info.panel_infos.lcd_bias);
            update_value(vreg[i].post_on_sleep, lcdkit_info.panel_infos.delay_af_bias_on);
            update_value(vreg[i].post_off_sleep, lcdkit_info.panel_infos.delay_af_bias_off);

            lcdkit_dump_dss_vreg(&vreg[i]);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_LAB_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_info.panel_infos.lcd_vsp);
            update_value(vreg[i].max_voltage, lcdkit_info.panel_infos.lcd_vsp);
            update_value(vreg[i].post_on_sleep, lcdkit_info.panel_infos.delay_af_vsp_on);
            update_value(vreg[i].post_off_sleep, lcdkit_info.panel_infos.delay_af_vsp_off);

            lcdkit_dump_dss_vreg(&vreg[i]);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_IBB_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_info.panel_infos.lcd_vsn);
            update_value(vreg[i].max_voltage, lcdkit_info.panel_infos.lcd_vsn);
            update_value(vreg[i].post_on_sleep, lcdkit_info.panel_infos.delay_af_vsn_on);
            update_value(vreg[i].post_off_sleep, lcdkit_info.panel_infos.delay_af_vsn_off);

            lcdkit_dump_dss_vreg(&vreg[i]);
        }
    }
}

static int lcdkit_regulator_config(struct platform_device *ctrl_pdev,
    struct dss_vreg *vreg, int cnt, char* name, int enable)
{
    int ret = 0;
    ret = lcdkit_get_regulator_index(vreg, cnt, name);
    if (ret >= cnt)
    {
        LCDKIT_ERR(": failed to get regulator index, rc=%d\n", ret);
        return -EINVAL;
    }

    LCDKIT_DEBUG("get %s regulator index: %d.\n", name, ret);

    ret = msm_dss_config_vreg(&ctrl_pdev->dev, &vreg[ret], 1, enable);
    if (ret)
    {
        LCDKIT_ERR(": failed to enable vregs for %s\n", name);
    	return ret;
    }

    return ret;
}

static int lcdkit_power_config(struct platform_device *ctrl_pdev,
    struct mdss_dsi_ctrl_pdata *ctrl_pdata, struct device_node *pan_node)
{
    int rc;

	rc = mdss_dsi_get_dt_vreg_data(&ctrl_pdev->dev, pan_node,
		&ctrl_pdata->panel_power_data, DSI_PANEL_PM);
	if (rc) {
		LCDKIT_ERR(": '%s' get_dt_vreg_data failed.rc=%d\n",
                __mdss_dsi_pm_name(DSI_PANEL_PM), rc);
		return rc;
	}

    // using value defined in user xml files
    lcdkit_regulator_init(ctrl_pdata->panel_power_data.vreg_config,
    		ctrl_pdata->panel_power_data.num_vreg);

    if (HYBRID == g_tskit_ic_type)
    {
        if (lcdkit_vci_is_regulator_ctrl_power())
        {
            rc = lcdkit_regulator_config(ctrl_pdev,
                    ctrl_pdata->panel_power_data.vreg_config,
            		ctrl_pdata->panel_power_data.num_vreg,
            		LCDKIT_VREG_VDD_NAME, 1);
            if (rc) {
                LCDKIT_ERR(": failed to init regulator: %s, rc=%d\n",
                        LCDKIT_VREG_VDD_NAME, rc);
            	return rc;
            }
        }
        else
        {
            rc = lcdkit_gpio_config(lcdkit_info.panel_infos.gpio_lcd_vci,
                        LCDKIT_VREG_VDD_NAME);
            if (rc) {
                LCDKIT_ERR(": failed to init gpio, rc=%d\n", rc);
            	return rc;
            }
        }
    }


    if (lcdkit_iovcc_is_regulator_ctrl_power())
    {
    	rc = lcdkit_regulator_config(ctrl_pdev,
            ctrl_pdata->panel_power_data.vreg_config,
    		ctrl_pdata->panel_power_data.num_vreg,
    		LCDKIT_VREG_VDDIO_NAME, 1);
    	if (rc) {
    		LCDKIT_ERR(": failed to init regulator: %s, rc=%d\n",
                    LCDKIT_VREG_VDDIO_NAME, rc);
    		return rc;
    	}
    }
    else
    {
        rc = lcdkit_gpio_config(lcdkit_info.panel_infos.gpio_lcd_iovcc,
                    LCDKIT_VREG_VDDIO_NAME);
    	if (rc) {
    		LCDKIT_ERR(": failed to init gpio, rc=%d\n", rc);
    		return rc;
    	}
    }

    if (lcdkit_is_oled_panel() || lcdkit_is_default_panel())
    {
        LCDKIT_INFO("Bypass the LAB/IBB power on when the board is "
                    "without lcd panel or oled panel!\n");
    }
    else if(!lcdkit_bias_is_used_ctrl_power())
    {
        LCDKIT_INFO("Bias power control is diabled!\n");
    }
    else
    {
        if (lcdkit_bias_is_regulator_ctrl_power())
        {
            #if 0
        	rc = lcdkit_regulator_config(ctrl_pdev,
                ctrl_pdata->panel_power_data.vreg_config,
        		ctrl_pdata->panel_power_data.num_vreg,
        		LCDKIT_VREG_BIAS_NAME, 1);
        	if (rc) {
        		LCDKIT_ERR(": failed to init regulator: %s, rc=%d\n",
                        LCDKIT_VREG_BIAS_NAME, rc);
        		return rc;
        	}
            #endif

        	rc = lcdkit_regulator_config(ctrl_pdev,
                ctrl_pdata->panel_power_data.vreg_config,
        		ctrl_pdata->panel_power_data.num_vreg,
				LCDKIT_VREG_LAB_NAME, 1);
        	if (rc) {
        		LCDKIT_ERR(": failed to init regulator: %s, rc=%d\n",
                        LCDKIT_VREG_LAB_NAME, rc);
        		return rc;
        	}

        	rc = lcdkit_regulator_config(ctrl_pdev,
                ctrl_pdata->panel_power_data.vreg_config,
        		ctrl_pdata->panel_power_data.num_vreg,
				LCDKIT_VREG_IBB_NAME, 1);
        	if (rc) {
        		LCDKIT_ERR(": failed to init regulator: %s, rc=%d\n",
                        LCDKIT_VREG_IBB_NAME, rc);
        		return rc;
        	}

        }
        else
        {
            #if 0
            rc = lcdkit_gpio_config(lcdkit_info.gpio_lcd_bias,
                    LCDKIT_VREG_BIAS_NAME);
        	if (rc) {
        		LCDKIT_ERR(": failed to init bias gpio: %d, rc=%d\n",
                        lcdkit_info.gpio_lcd_bias, rc);
        		return rc;
        	}
            #endif

            rc = lcdkit_gpio_config(lcdkit_info.panel_infos.gpio_lcd_vsp,
                    LCDKIT_VREG_LAB_NAME);
        	if (rc) {
        		LCDKIT_ERR(": failed to init vsp gpio: %d, rc=%d\n",
                        lcdkit_info.panel_infos.gpio_lcd_vsp, rc);
        		return rc;
        	}
            rc = lcdkit_gpio_config(lcdkit_info.panel_infos.gpio_lcd_vsn,
                    LCDKIT_VREG_IBB_NAME);
        	if (rc) {
        		LCDKIT_ERR(": failed to init vsn gpio: %d, rc=%d\n",
                        lcdkit_info.panel_infos.gpio_lcd_vsn, rc);
        		return rc;
        	}

        }
    }

    return rc;
}

static int lcdkit_regulator_clean(struct dss_vreg *vreg, int cnt, char* name)
{
    int ret = 0;
    ret = lcdkit_get_regulator_index(vreg, cnt, name);
    if (ret >= cnt)
    {
        LCDKIT_ERR(": failed to get regulator index, rc=%d\n", ret);
        return -EINVAL;
    }

    if(vreg[ret].vreg->rdev->use_count > 1)
	{
        vreg[ret].vreg->rdev->use_count = 1;
	    LCDKIT_INFO("Exit PT test, for disable the regulator, clean the count.\n");
    }

    return 0;
}

static int lcdkit_regulator_enable(struct dss_vreg *vreg, int cnt,
    char* name, int enable)
{
    int ret = 0;
    ret = lcdkit_get_regulator_index(vreg, cnt, name);
    if (ret >= cnt)
    {
        LCDKIT_ERR(": failed to get regulator index, rc=%d\n", ret);
        return -EINVAL;
    }

    lcdkit_dump_dss_vreg(&vreg[ret]);

    ret = msm_dss_enable_vreg(&vreg[ret], 1, enable);
    if (ret)
    {
        LCDKIT_ERR(": failed to enable vregs for %s\n", name);
    	return ret;
    }

    return ret;
}

u32 lcdkit_get_panel_off_reset_high(void)
{
    return lcdkit_info.panel_infos.panel_off_reset_high;
}

static int lcdkit_is_enter_sleep_mode(void)
{
   return (ts_kit_gesture_func || g_tskit_pt_station_flag);
}

static int mdss_dsi_panel_power_off(struct mdss_panel_data *pdata)
{
	int ret = 0;
    int power_down_flag = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		LCDKIT_ERR(": Invalid input data\n");
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

    if (!lcdkit_get_panel_off_reset_high())
    {
        if (!is_device_reboot)
        {
    		ret = mdss_dsi_panel_reset(pdata, 0);
    		if (ret) {
    			LCDKIT_WARNING(": Panel reset failed. rc=%d\n", ret);
    			ret = 0;
    		}
        }

    	if (mdss_dsi_pinctrl_set_state(ctrl_pdata, false))
    		LCDKIT_DEBUG("reset disable: pinctrl not enabled\n");
    }
    else
    {
        LCDKIT_INFO("Keep special lcd reset high.\n");
    }

    if (!lcdkit_is_enter_sleep_mode())
    {
        if (lcdkit_is_oled_panel() || lcdkit_is_default_panel())
        {
            LCDKIT_INFO("Bypass the LAB/IBB power off when the board is "
                        "without lcd panel or oled panel.\n");
        }
        else if(!lcdkit_bias_is_used_ctrl_power())
        {
            LCDKIT_INFO("Bias power control is diabled!\n");
        }
        else
        {
            if(lcdkit_is_factory_mode())
    		{
    	        //enable VSP/VSN power test
    	    	if(enable_PT_test)
                {
                    LCDKIT_INFO("enter PT test, enable VSP/VSN power\n");
    				power_down_flag = false;
    			}
    			else
    			{
                    if (lcdkit_bias_is_regulator_ctrl_power())
                    {
                        ret = lcdkit_regulator_clean(
                                    ctrl_pdata->panel_power_data.vreg_config,
                                    ctrl_pdata->panel_power_data.num_vreg,
                                    LCDKIT_VREG_IBB_NAME);
                    	if (ret)
                        {
                    		LCDKIT_ERR(": failed to clean vregs for %s\n",
                                    LCDKIT_VREG_IBB_NAME);
                    		return ret;
                    	}

                        ret = lcdkit_regulator_clean(
                                    ctrl_pdata->panel_power_data.vreg_config,
                                    ctrl_pdata->panel_power_data.num_vreg,
                                    LCDKIT_VREG_LAB_NAME);
                    	if (ret)
                        {
                    		LCDKIT_ERR(": failed to clean vregs for %s\n",
                                    LCDKIT_VREG_LAB_NAME);
                    		return ret;
                    	}
                    }

                    power_down_flag = true;
    			}
    		}
            else
            {
                power_down_flag = true;
            }

            if (power_down_flag)
            {
                if (lcdkit_bias_is_regulator_ctrl_power())
                {
                    ret = lcdkit_regulator_enable(
                                ctrl_pdata->panel_power_data.vreg_config,
                                ctrl_pdata->panel_power_data.num_vreg,
                                LCDKIT_VREG_IBB_NAME, 0);
                	if (ret)
                    {
                		LCDKIT_ERR(": failed to enable vregs for %s\n",
                                LCDKIT_VREG_IBB_NAME);
                		return ret;
                	}

                    ret = lcdkit_regulator_enable(
                                ctrl_pdata->panel_power_data.vreg_config,
                                ctrl_pdata->panel_power_data.num_vreg,
                                LCDKIT_VREG_LAB_NAME, 0);
                	if (ret)
                    {
                		LCDKIT_ERR(": failed to enable vregs for %s\n",
                                LCDKIT_VREG_LAB_NAME);
                		return ret;
                	}

                    #if 0
                    ret = lcdkit_regulator_enable(
                                ctrl_pdata->panel_power_data.vreg_config,
                                ctrl_pdata->panel_power_data.num_vreg,
                                LCDKIT_VREG_BIAS_NAME, 0);
                	if (ret)
                    {
                		LCDKIT_ERR(": failed to enable vregs for %s\n",
                                LCDKIT_VREG_BIAS_NAME);
                		return ret;
                	}
                    #endif

                }
                else
                {
                    ret = lcdkit_disable_gpio(lcdkit_info.panel_infos.gpio_lcd_vsn);
                	if (ret) {
                		LCDKIT_ERR(": failed to enable vsn gpio:%d!\n",
                                lcdkit_info.panel_infos.gpio_lcd_vsn);
                		return ret;
                	}

                    lcdkit_delay(lcdkit_info.panel_infos.delay_af_vsn_off);

                    ret = lcdkit_disable_gpio(lcdkit_info.panel_infos.gpio_lcd_vsp);
                	if (ret) {
                		LCDKIT_ERR(": failed to enable vsp gpio:%d!\n",
                                lcdkit_info.panel_infos.gpio_lcd_vsp);
                		return ret;
                	}

                    lcdkit_delay(lcdkit_info.panel_infos.delay_af_vsp_off);

                    #if 0
                    ret = lcdkit_disable_power_gpio(lcdkit_info.gpio_lcd_bias);
                	if (ret) {
                		LCDKIT_ERR(": failed to enable bias gpio:%d!\n",
                                lcdkit_info.gpio_lcd_bias);
                		return ret;
                	}

                    lcdkit_delay(lcdkit_info.delay_af_bias_off);
                    #endif

                }
            }

        }

        if (lcdkit_iovcc_is_regulator_ctrl_power())
        {
            ret = lcdkit_regulator_enable(
                    ctrl_pdata->panel_power_data.vreg_config,
                    ctrl_pdata->panel_power_data.num_vreg,
                    LCDKIT_VREG_VDDIO_NAME, 0);
        	if (ret)
            {
        		LCDKIT_ERR(": failed to enable vregs for %s\n",
                        LCDKIT_VREG_VDDIO_NAME);
        		return ret;
        	}
        }
        else
        {
            ret = lcdkit_disable_gpio(lcdkit_info.panel_infos.gpio_lcd_iovcc);
        	if (ret) {
        		LCDKIT_ERR(": failed to enable iovcc gpio:%d!\n",
                        lcdkit_info.panel_infos.gpio_lcd_iovcc);
        		return ret;
        	}

            lcdkit_delay(lcdkit_info.panel_infos.delay_af_iovcc_off);
        }

        if (HYBRID == g_tskit_ic_type)
        {
            if (lcdkit_vci_is_regulator_ctrl_power())
            {
                ret = lcdkit_regulator_enable(
                            ctrl_pdata->panel_power_data.vreg_config,
                            ctrl_pdata->panel_power_data.num_vreg,
                            LCDKIT_VREG_VDD_NAME, 0);
                if (ret)
                {
                    LCDKIT_ERR(": failed to enable vregs for %s\n",
                                LCDKIT_VREG_VDD_NAME);
                	return ret;
                }
            }
            else
            {
                ret = lcdkit_disable_gpio(lcdkit_info.panel_infos.gpio_lcd_vci);
                if (ret)
                {
                    LCDKIT_ERR(": failed to enable lcd vci gpio:%d!\n",
                                lcdkit_info.panel_infos.gpio_lcd_vci);
                	return ret;
                }

                lcdkit_delay(lcdkit_info.panel_infos.delay_af_vci_off);
            }
        }
    }

	return ret;
}

static int mdss_dsi_panel_power_on(struct mdss_panel_data *pdata)
{
	int ret = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		LCDKIT_ERR(": Invalid input data\n");
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

    LCDKIT_DEBUG("enter!");

    if (!lcdkit_is_enter_sleep_mode())
    {
        if (HYBRID == g_tskit_ic_type)
        {
            if (lcdkit_vci_is_regulator_ctrl_power())
            {
                ret = lcdkit_regulator_enable(
                            ctrl_pdata->panel_power_data.vreg_config,
                            ctrl_pdata->panel_power_data.num_vreg,
                            LCDKIT_VREG_VDD_NAME, 1);
                if (ret)
                {
                    LCDKIT_ERR(": failed to enable vregs for %s\n",
                            LCDKIT_VREG_VDD_NAME);
                	return ret;
                }
            }
            else
            {
                ret = lcdkit_enable_gpio(lcdkit_info.panel_infos.gpio_lcd_vci);
                if (ret)
                {
                    LCDKIT_ERR(": failed to enable lcd vci gpio:%d!\n",
                            lcdkit_info.panel_infos.gpio_lcd_vci);
                    return ret;
                }

                lcdkit_delay(lcdkit_info.panel_infos.delay_af_vci_on);
            }
        }

        if (lcdkit_iovcc_is_regulator_ctrl_power())
        {
            ret = lcdkit_regulator_enable(
                        ctrl_pdata->panel_power_data.vreg_config,
                        ctrl_pdata->panel_power_data.num_vreg,
                        LCDKIT_VREG_VDDIO_NAME, 1);
        	if (ret)
            {
        		LCDKIT_ERR(": failed to enable vregs for %s\n",
                        LCDKIT_VREG_VDDIO_NAME);
        		return ret;
        	}
        }
        else
        {
            ret = lcdkit_enable_gpio(lcdkit_info.panel_infos.gpio_lcd_iovcc);
        	if (ret) {
        		LCDKIT_ERR(": failed to enable iovcc gpio:%d!\n",
                        lcdkit_info.panel_infos.gpio_lcd_iovcc);
        		return ret;
        	}

            lcdkit_delay(lcdkit_info.panel_infos.delay_af_iovcc_on);
        }

    	if( (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
            && lcdkit_info.panel_infos.first_reset)
    	{
    	    mdss_dsi_panel_reset(&ctrl_pdata->panel_data, 1);
        }

        if (lcdkit_is_oled_panel() || lcdkit_is_default_panel())
        {
            LCDKIT_INFO("Bypass the LAB/IBB power on when the board is "
                        "without lcd panel or oled panel!\n");
        }
        else if(!lcdkit_bias_is_used_ctrl_power())
        {
            LCDKIT_INFO("Bias power control is diabled!\n");
        }
        else
        {
            if (lcdkit_bias_is_regulator_ctrl_power())
            {
                if (is_lcdkit_vsp_vsn_enable())
                {
                    LCDKIT_DEBUG("enter debug!");
                    lcdkit_debug_set_vsp_vsn(
                            ctrl_pdata->panel_power_data.vreg_config,
                		    ctrl_pdata->panel_power_data.num_vreg);
                }

                #if 0
                ret = lcdkit_regulator_enable(
                        ctrl_pdata->panel_power_data.vreg_config,
                        ctrl_pdata->panel_power_data.num_vreg,
                        LCDKIT_VREG_BIAS_NAME, 1);
            	if (ret)
                {
            		LCDKIT_ERR(": failed to enable vregs for %s\n",
                            LCDKIT_VREG_BIAS_NAME);
            		return ret;
            	}
                #endif

                ret = lcdkit_regulator_enable(
                        ctrl_pdata->panel_power_data.vreg_config,
                        ctrl_pdata->panel_power_data.num_vreg,
                        LCDKIT_VREG_LAB_NAME, 1);
            	if (ret)
                {
            		LCDKIT_ERR(": failed to enable vregs for %s\n",
                            LCDKIT_VREG_LAB_NAME);
            		return ret;
            	}

                ret = lcdkit_regulator_enable(
                        ctrl_pdata->panel_power_data.vreg_config,
                        ctrl_pdata->panel_power_data.num_vreg,
                        LCDKIT_VREG_IBB_NAME, 1);
            	if (ret)
                {
            		LCDKIT_ERR(": failed to enable vregs for %s\n",
                        LCDKIT_VREG_IBB_NAME);
            		return ret;
            	}
            }
            else
            {
                #if 0
                ret = lcdkit_enable_power_gpio(lcdkit_info.gpio_lcd_bias);
            	if (ret) {
            		LCDKIT_ERR(": failed to enable bias gpio:%d!\n",
                            lcdkit_info.gpio_lcd_bias);
            		return ret;
            	}

                lcdkit_delay(lcdkit_info.delay_af_bias_on);
                #endif

                ret = lcdkit_enable_gpio(lcdkit_info.panel_infos.gpio_lcd_vsp);
            	if (ret) {
            		LCDKIT_ERR(": failed to enable vsp gpio:%d!\n",
                            lcdkit_info.panel_infos.gpio_lcd_vsp);
            		return ret;
            	}

                lcdkit_delay(lcdkit_info.panel_infos.delay_af_vsp_on);

                ret = lcdkit_enable_gpio(lcdkit_info.panel_infos.gpio_lcd_vsn);
            	if (ret) {
            		LCDKIT_ERR(": failed to enable vsn gpio:%d!\n",
                            lcdkit_info.panel_infos.gpio_lcd_vsn);
            		return ret;
            	}

                lcdkit_delay(lcdkit_info.panel_infos.delay_af_vsn_on);
            }
        }
    }

	/*
	 * If continuous splash screen feature is enabled, then we need to
	 * request all the GPIOs that have already been configured in the
	 * bootloader. This needs to be done irresepective of whether
	 * the lp11_init flag is set or not.
	 */
	//second_reset flag add here, is it correct?
	if ((pdata->panel_info.cont_splash_enabled ||
		!pdata->panel_info.mipi.lp11_init) || (lcdkit_info.panel_infos.second_reset)){
		if (mdss_dsi_pinctrl_set_state(ctrl_pdata, true))
			LCDKIT_DEBUG("reset enable: pinctrl not enabled\n");

		ret = mdss_dsi_panel_reset(pdata, 1);
		if (ret)
			LCDKIT_ERR(": Panel reset failed. rc=%d\n", ret);
	}

	return ret;
}

