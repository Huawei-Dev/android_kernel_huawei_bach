/*
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 * Not a Contribution.
 *
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/spmi.h>
#include <linux/pm.h>
#include <soc/qcom/smem.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/io.h>
#include <linux/of_address.h>

/*S1 && S2 have two types of voltage, called "HV_FALSE" & "HV_TRUE",
and they have different base voltages and steps. So it need to read
the type of voltage firstly, according the type, get the VMIN voltae,
second,it need to read the STEP count,lastly count the result:
VOLTAGE = PMIC_MSS(CX)_TYPE_HV_FALSE(TRUE)_VMIN +
   PMIC_MSS(CX)_TYPE_HV_FALSE(TRUE)_STEP * PMIC_MSS(CX)_VALUE_REG
MX only have one type of voltage, the method is as same as S1 & S2*/
struct pmic_para_struct
{
    u16 mss_type_reg;
    u16 mss_value_reg;
    u32 mss_false_vmin;
    u32 mss_false_step;
    u32 mss_true_vmin;
    u32 mss_true_step;
    u16 cx_type_reg;
    u16 cx_value_reg;
    u32 cx_false_vmin;
    u32 cx_false_step;
    u32 cx_true_vmin;
    u32 cx_true_step;
    u16 mx_value_reg;
    u16 reverse;
    u32 mx_vmin;
    u32 mx_step;
};

struct cpu_info_struct {
    struct spmi_device *spmi;
    struct pmic_para_struct pmic_para;
    struct notifier_block panic_blk;
};

typedef struct
{
    unsigned int mx_mode;
    unsigned int cx_mode;
} railway_mx_cx_corner_info;

/*this union will save the mode info which will get from imem.*/
union uni_corner_info
{
    railway_mx_cx_corner_info corner_info;
    unsigned long corner_value;
};

enum cx_type {
    TYPE_HV_FALSE,
    TYPE_HV_TRUE
};

struct mode_info_struct
{
    int mode_num;
    char * mode_name;
};

static const struct mode_info_struct mode_info[] =
{
 {0,"RAILWAY_NO_REQUEST"},
 {1,"RAILWAY_RETENTION"},
 {2,"RAILWAY_SVS_LOW"},
 {3,"RAILWAY_SVS_SOC"},
 {4,"RAILWAY_SVS_HIGH"},
 {5,"RAILWAY_NOMINAL"},
 {6,"RAILWAY_NOMINAL_HIGH"},
 {7,"RAILWAY_TURBO"},
 {8,"RAILWAY_SUPER_TURBO"},
 {9,"RAILWAY_SUPER_TURBO_NO_CPR"},
 {10,"RAILWAY_SUPER_TURBO_HIGH"},
};

static char * cpuinfo_change_mode_tostr(int mode)
{
    int count = sizeof(mode_info)/sizeof(struct mode_info_struct);
    if(mode < 0 || mode >= count)
    {
        return NULL;
    }

    return mode_info[mode].mode_name;
}

/*this function will get the mx/cx mode from imem, and the value will be
record by the rpm railway.*/
static void cpuinfo_get_mx_cx_mode(int *mx_mode, int *cx_mode)
{
    struct device_node *np = NULL;
    static void *corner_addr = NULL;
    union uni_corner_info corner={0};

    np = of_find_compatible_node(NULL,
                                 NULL,
                                 "qcom,msm-imem-cpuinfo-addr");

    if (!np)
    {
        printk(KERN_CRIT "get mx/cx from imem error!\n");
        return;
    }

    corner_addr = of_iomap(np, 0);
    if (!corner_addr)
    {
        printk(KERN_CRIT "unable to map imem msm-imem-cpuinfo-addr offset\n");
        return;
    }

    printk(KERN_CRIT "corner addr:%lx\n",corner_addr);
    corner.corner_value = __raw_readq(corner_addr);

    *mx_mode = corner.corner_info.mx_mode;
    *cx_mode = corner.corner_info.cx_mode;

    return;
}

/*show the mss voltage*/
static void cpuinfo_show_mss(struct cpu_info_struct *cpuinfo, u8 mss_type, u8 mss_value)
{
    int mss_voltage = 0;

    if(mss_type == TYPE_HV_FALSE)
    {
        mss_voltage = cpuinfo->pmic_para.mss_false_vmin + cpuinfo->pmic_para.mss_false_step*mss_value;
    }
    else
    {
        mss_voltage = cpuinfo->pmic_para.mss_true_vmin + cpuinfo->pmic_para.mss_true_step*mss_value;
    }

    printk(KERN_CRIT "mss_voltage is:%duV\n",mss_voltage);

    return;
}

/*show the cx voltage and mode*/
static void cpuinfo_show_cx(struct cpu_info_struct *cpuinfo, u8 cx_type, u8 cx_value, int mode)
{
    int cx_voltage = 0;
    char *mode_str = NULL;

    if(cx_type == TYPE_HV_FALSE)
    {
        cx_voltage = cpuinfo->pmic_para.cx_false_vmin + cpuinfo->pmic_para.cx_false_step*cx_value;
    }
    else
    {
        cx_voltage = cpuinfo->pmic_para.cx_true_vmin + cpuinfo->pmic_para.cx_true_step*cx_value;
    }

    printk(KERN_CRIT "cx_voltage is:%duV\n",cx_voltage);

    mode_str = cpuinfo_change_mode_tostr(mode);
    if(NULL == mode_str)
    {
        printk(KERN_CRIT "cx_mode is:UNKNOWN\n");
    }
    else
    {
        printk(KERN_CRIT "cx_mode is:%s\n",mode_str);
    }

    return;
}

/*show the mx voltage and mode*/
static void cpuinfo_show_mx(struct cpu_info_struct *cpuinfo, u8 mx_value, int mode)
{
    int mx_voltage = 0;
    char *mode_str = NULL;

    mx_voltage = cpuinfo->pmic_para.mx_vmin + cpuinfo->pmic_para.mx_step*mx_value;

    printk(KERN_CRIT "mx_voltage is:%duV\n",mx_voltage);
    mode_str = cpuinfo_change_mode_tostr(mode);
    if(NULL == mode_str)
    {
        printk(KERN_CRIT "mx_mode is:UNKNOWN\n");
    }
    else
    {
        printk(KERN_CRIT "mx_mode is:%s\n",mode_str);
    }

    return;
}

/*get the mx/cx/mss voltage from pmic by spmi bus,*/
static int mx_cx_voltage_get(struct notifier_block *this,
                              unsigned long event, void *ptr)
{
    int rc = 0;
    u8 cx_type = 0;
    u8 cx_value = 0;
    u8 mx_value = 0;
    u8 mss_type = 0;
    u8 mss_value = 0;
    int mx_mode = 0;
    int cx_mode = 0;

    struct cpu_info_struct *cpuinfo = NULL;
    cpuinfo = container_of(this, struct cpu_info_struct, panic_blk);
    printk(KERN_ERR "mx_cx_voltage_get start\n");

    rc = spmi_ext_register_readl(cpuinfo->spmi->ctrl, cpuinfo->spmi->sid, cpuinfo->pmic_para.cx_type_reg, &cx_type, 1);
    if (rc) {
        dev_err(&cpuinfo->spmi->dev, "Unable to read cx type reg rc: %d\n", rc);
        goto fail;
    }

    rc = spmi_ext_register_readl(cpuinfo->spmi->ctrl, cpuinfo->spmi->sid, cpuinfo->pmic_para.cx_value_reg, &cx_value, 1);
    if (rc) {
        dev_err(&cpuinfo->spmi->dev, "Unable to read cx type reg rc: %d\n", rc);
        goto fail;
    }

    rc = spmi_ext_register_readl(cpuinfo->spmi->ctrl, cpuinfo->spmi->sid, cpuinfo->pmic_para.mx_value_reg, &mx_value, 1);
    if (rc) {
        dev_err(&cpuinfo->spmi->dev, "Unable to read mx value reg rc: %d\n", rc);
        goto fail;
    }

    rc = spmi_ext_register_readl(cpuinfo->spmi->ctrl,cpuinfo->spmi->sid, cpuinfo->pmic_para.mss_type_reg, &mss_type, 1);
    if (rc) {
        dev_err(&cpuinfo->spmi->dev, "Unable to read mss reg rc: %d\n", rc);
        goto fail;
    }

    rc =spmi_ext_register_readl(cpuinfo->spmi->ctrl, cpuinfo->spmi->sid, cpuinfo->pmic_para.mss_value_reg, &mss_value, 1);
    if (rc) {
        dev_err(&cpuinfo->spmi->dev, "Unable to read mss reg rc: %d\n", rc);
        goto fail;
    }

    cpuinfo_get_mx_cx_mode(&mx_mode, &cx_mode);
    cpuinfo_show_mx(cpuinfo, mx_value, mx_mode);
    cpuinfo_show_cx(cpuinfo, cx_type, cx_value, cx_mode);
    cpuinfo_show_mss(cpuinfo, mss_type, mss_value);

    return 0;

fail:
    return rc;
}

static int cpuinfo_get_device_tree_data(struct spmi_device *spmi, struct cpu_info_struct *cpuinfo)
{
    int rc = 0;
    u16 temp_reg_data[2];
    u32 temp_para_data[4];
    struct device_node *of_node = spmi->dev.of_node;

    if(NULL == spmi || NULL == cpuinfo) {
        rc = -ENOMEM;
        goto fail;
    }

    of_node = spmi->dev.of_node;

    memset(temp_reg_data, 0, sizeof(temp_reg_data));
    rc = of_property_read_u16_array(of_node, "mss-reg", temp_reg_data, 2);
    if (rc) {
        goto fail;
    };
    cpuinfo->pmic_para.mss_type_reg = temp_reg_data[0];
    cpuinfo->pmic_para.mss_value_reg = temp_reg_data[1];

    memset(temp_para_data, 0, sizeof(temp_para_data));
    rc = of_property_read_u32_array(of_node, "mss-para", temp_para_data, 4);
    if (rc) {
        goto fail;
    };
    cpuinfo->pmic_para.mss_false_vmin = temp_para_data[0];
    cpuinfo->pmic_para.mss_false_step = temp_para_data[1];
    cpuinfo->pmic_para.mss_true_vmin = temp_para_data[2];
    cpuinfo->pmic_para.mss_true_step = temp_para_data[3];

    memset(temp_reg_data, 0, sizeof(temp_reg_data));
    rc = of_property_read_u16_array(of_node, "cx-reg", temp_reg_data, 2);
    if (rc) {
        goto fail;
    };
    cpuinfo->pmic_para.cx_type_reg = temp_reg_data[0];
    cpuinfo->pmic_para.cx_value_reg = temp_reg_data[1];

    memset(temp_para_data, 0, sizeof(temp_para_data));
    rc = of_property_read_u32_array(of_node, "cx-para", temp_para_data, 4);
    if (rc) {
        goto fail;
    };
    cpuinfo->pmic_para.cx_false_vmin = temp_para_data[0];
    cpuinfo->pmic_para.cx_false_step = temp_para_data[1];
    cpuinfo->pmic_para.cx_true_vmin = temp_para_data[2];
    cpuinfo->pmic_para.cx_true_step = temp_para_data[3];

    memset(temp_reg_data, 0, sizeof(temp_reg_data));
    rc = of_property_read_u16_array(of_node, "mx-reg", temp_reg_data, 1);
    if (rc) {
        goto fail;
    };
    cpuinfo->pmic_para.mx_value_reg = temp_reg_data[0];

    memset(temp_para_data, 0, sizeof(temp_para_data));
    rc = of_property_read_u32_array(of_node, "mx-para", temp_para_data, 2);
    if (rc) {
        goto fail;
    };
    cpuinfo->pmic_para.mx_vmin = temp_para_data[0];
    cpuinfo->pmic_para.mx_step = temp_para_data[1];

    return 0;

fail:
    return rc;
}

static int cpuinfo_probe(struct spmi_device *spmi)
{
    int rc = 0;
    struct cpu_info_struct *cpuinfo = NULL;
    printk(KERN_ERR "cpuinfo_probe start\n");
    cpuinfo = devm_kzalloc(&spmi->dev, sizeof(struct cpu_info_struct), GFP_KERNEL);

    if (!cpuinfo) {
        dev_err(&spmi->dev, "Can't allocate cpuinfo\n");
        return -ENOMEM;
    }

    cpuinfo->spmi = spmi;

    rc = cpuinfo_get_device_tree_data(spmi, cpuinfo);
    if(rc) {
        printk(KERN_ERR "can't find pmic params\n");
        return rc;
    }

    /*register to the panic notifier list, when the device crash, it could be execute*/
    cpuinfo->panic_blk.notifier_call = mx_cx_voltage_get;
    atomic_notifier_chain_register(&panic_notifier_list,
                                   &cpuinfo->panic_blk);

    return 0;
}

static int cpuinfo_remove(struct spmi_device *pdev)
{
    struct cpu_info_struct * cpuinfo;

    cpuinfo = platform_get_drvdata(pdev);

    if(cpuinfo)
    {
        atomic_notifier_chain_unregister(&panic_notifier_list,
                                                &cpuinfo->panic_blk);
    }

    return 0;
}

static const struct of_device_id cpuinfo_match_table[] = {
    {
        .compatible = "huawei,cpuinfo",
    },
    {}
};

static struct spmi_driver cpuinfo_driver = {
    .probe = cpuinfo_probe,
    .remove = cpuinfo_remove,
    .driver = {
        .name = "cpuinfo",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(cpuinfo_match_table),
    },
};

static int __init hw_cpuinfo_init(void)
{
    int err = 0;

    printk(KERN_ERR "hw_cpuinfo_init start\n");
    err = spmi_driver_register(&cpuinfo_driver);
    if (err){
        printk(KERN_ERR "cpuinfo drv regiset error %d\n", err);
    }

    return err;
}

static void __exit hw_cpuinfo_exit(void)
{
    spmi_driver_unregister(&cpuinfo_driver);
}

MODULE_AUTHOR("huawei");
MODULE_DESCRIPTION("get cpuinfo");
MODULE_LICENSE("GPL");

module_init(hw_cpuinfo_init);
module_exit(hw_cpuinfo_exit);
