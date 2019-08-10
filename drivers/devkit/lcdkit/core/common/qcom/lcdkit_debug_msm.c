
#include "mdss_dsi.h"
#include <linux/mdss_io_util.h>
#include "lcdkit_dbg.h"
#include "lcdkit_common.h"
#include <linux/lcdkit_dsm.h>

struct platform_device *lcdkit_get_dsi_ctrl_pdev(void);
void lcdkit_debug_set_vsp_vsn(void *preg, int cnt)
{
    int i, rc;
    struct dss_vreg *vreg = preg;
    struct platform_device* pdev = lcdkit_get_dsi_ctrl_pdev();

    for (i = 0; i < cnt; i++)
    {
        #if 0
        if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_VDD_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdanalog_vcc);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdanalog_vcc);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_VDDIO_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdio_vcc);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdio_vcc);
        }
        #endif
        if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_BIAS_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdkit_panel_bias);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdkit_panel_bias);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_LAB_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdkit_panel_vsp);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdkit_panel_vsp);
        }
        else if (0 == strncmp(vreg[i].vreg_name,
            LCDKIT_VREG_IBB_NAME, strlen(vreg[i].vreg_name)))
        {
            update_value(vreg[i].min_voltage, lcdkit_dbg.lcdkit_panel_vsn);
            update_value(vreg[i].max_voltage, lcdkit_dbg.lcdkit_panel_vsn);
        }
    }

    rc = msm_dss_config_vreg(&pdev->dev, vreg, cnt, 1);
    if (rc)
    {
        LCDKIT_ERR(": failed to init regulator, rc=%d\n", rc);
    }

    return ;

}
