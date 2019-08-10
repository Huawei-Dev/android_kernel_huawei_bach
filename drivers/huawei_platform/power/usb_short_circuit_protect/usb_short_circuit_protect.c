/*
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/notifier.h>
#include <linux/wakelock.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <dsm/dsm_pub.h>
#include <linux/delay.h>
#include <dsm/dsm_pub.h>



struct uscp_device_info
{
    struct device   *dev;
    struct workqueue_struct *uscp_wq;
    struct work_struct uscp_check_wk;
    struct notifier_block   usb_nb;
    struct power_supply     *usb_psy;
    struct power_supply     *batt_psy;
    struct power_supply     *bms_psy;
    struct hrtimer timer;
    int gpio_uscp;
    int uscp_threshold_tusb;
    int open_mosfet_temp;
    int close_mosfet_temp;
    int interval_switch_temp;
    int check_interval;
    int keep_check_cnt;
    int no_need_uscp;//support uscp or not
};

static struct dsm_dev dsm_uscp =
{
    .name = "dsm_usb_short_circuit_protect",
    .fops = NULL,
    .buff_size = 1024,
};
#define		TRUE					1
#define		FALSE					0
#define		INVALID_DELTA_TEMP		0
#define		USCP_DEFAULT_CHK_CNT	(-1)
#define		USCP_START_CHK_CNT		0
#define		USCP_END_CHK_CNT		1001
#define		USCP_CHK_CNT_STEP		1
#define		USCP_INSERT_CHG_CNT		1100
#define		FAST_MONITOR_INTERVAL	300  /*300 ms*/
#define		NORMAL_MONITOR_INTERVAL	10000  /*10000 ms*/
#define		GPIO_HIGH				1
#define		GPIO_LOW				0
#define		INTERVAL_0				0
#define		RETRY_CNT				3
#define		USB_TEMP_CNT			2
#define		TUSB_TEMP_UPPER_LIMIT	100
#define		TUSB_TEMP_LOWER_LIMIT	(-30)
#define		COVERSE_TEMP_UNIT		10
#define		INVALID_BATT_TEMP		(-255)
#define 	ADC_VOL_INIT			(-1)
#define		AVG_COUNT					3
#define		FG_NOT_READY					0
#define     DEFAULT_TUSB_THRESHOLD       40


static int protect_enable = FALSE;
static int protect_dmd_notify_enable = TRUE;
static int is_uscp_mode = 0;
static int T_A_TABLE[][2] =
{
    {-40, 1696},
    {-39, 1690},
    {-38, 1684},
    {-37, 1678},
    {-36, 1672},
    {-35, 1665},
    {-34, 1658},
    {-33, 1651},
    {-32, 1644},
    {-31, 1636},
    {-30, 1628},
    {-29, 1619},
    {-28, 1611},
    {-27, 1602},
    {-26, 1593},
    {-25, 1583},
    {-24, 1573},
    {-23, 1563},
    {-22, 1553},
    {-21, 1542},
    {-20, 1531},
    {-19, 1519},
    {-18, 1508},
    {-17, 1496},
    {-16, 1484},
    {-15, 1471},
    {-14, 1458},
    {-13, 1445},
    {-12, 1431},
    {-11, 1418},
    {-10, 1404},
    {-9,  1389},
    {-8,  1375},
    {-7,  1360},
    {-6,  1345},
    {-5,  1329},
    {-4,  1314},
    {-3,  1298},
    {-2,  1282},
    {-1,  1266},
    {0,   1249},
    {1,   1233},
    {2,   1216},
    {3,   1199},
    {4,   1182},
    {5,   1165},
    {6,   1148},
    {7,   1131},
    {8,   1113},
    {9,   1096},
    {10,  1078},
    {11,  1061},
    {12,  1043},
    {13,  1025},
    {14,  1008},
    {15,  990 },
    {16,  973 },
    {17,  955 },
    {18,  938 },
    {19,  920 },
    {20,  903 },
    {21,  886 },
    {22,  869 },
    {23,  852 },
    {24,  835 },
    {25,  818 },
    {26,  802 },
    {27,  785 },
    {28,  769 },
    {29,  753 },
    {30,  737 },
    {31,  721 },
    {32,  705 },
    {33,  690 },
    {34,  675 },
    {35,  660 },
    {36,  645 },
    {37,  631 },
    {38,  617 },
    {39,  603 },
    {40,  589 },
    {41,  575 },
    {42,  562 },
    {43,  549 },
    {44,  536 },
    {45,  523 },
    {46,  511 },
    {47,  499 },
    {48,  487 },
    {49,  475 },
    {50,  463 },
    {51,  452 },
    {52,  441 },
    {53,  430 },
    {54,  420 },
    {55,  410 },
    {56,  400 },
    {57,  390 },
    {58,  380 },
    {59,  371 },
    {60,  361 },
    {61,  353 },
    {62,  344 },
    {63,  335 },
    {64,  327 },
    {65,  319 },
    {66,  311 },
    {67,  304 },
    {68,  296 },
    {69,  289 },
    {70,  282 },
    {71,  275 },
    {72,  268 },
    {73,  261 },
    {74,  255 },
    {75,  249 },
    {76,  243 },
    {77,  237 },
    {78,  231 },
    {79,  225 },
    {80,  220 },
    {81,  214 },
    {82,  209 },
    {83,  204 },
    {84,  199 },
    {85,  194 },
    {86,  190 },
    {87,  185 },
    {88,  181 },
    {89,  176 },
    {90,  172 },
    {91,  168 },
    {92,  164 },
    {93,  160 },
    {94,  156 },
    {95,  152 },
    {96,  149 },
    {97,  145 },
    {98,  142 },
    {99,  138 },
    {100, 135 },
    {101, 132 },
    {102, 129 },
    {103, 126 },
    {104, 123 },
    {105, 120 },
    {106, 117 },
    {107, 115 },
    {108, 112 },
    {109, 109 },
    {110, 107 },
    {111, 105 },
    {112, 102 },
    {113, 100 },
    {114, 98  },
    {115, 95  },
    {116, 93  },
    {117, 91  },
    {118, 89  },
    {119, 87  },
    {120, 85  },
    {121, 83  },
    {122, 82  },
    {123, 80  },
    {124, 78  },
    {125, 76  },

};

static struct dsm_client *uscp_client = NULL;
static struct uscp_device_info* g_di = NULL;
static struct wake_lock uscp_wakelock;
extern int get_loginfo_int(struct power_supply *psy, int propery);
extern int get_pmi_sub_voltage(void);
extern int usb_charger_register_notifier(struct notifier_block *nb);
static bool is_factory_mode = false;
static int __init early_parse_factory_mode(char *cmdline)
{
    if ((cmdline) && !strncmp(cmdline, "factory", strlen("factory"))) {
        is_factory_mode = true;
    }

    return 0;
}
early_param("androidboot.huawei_swtype", early_parse_factory_mode);
static void uscp_wake_lock(void)
{
    if(!wake_lock_active(&uscp_wakelock))
    {
        pr_info("wake lock\n");
        wake_lock(&uscp_wakelock);
    }
}

static void uscp_wake_unlock(void)
{
    if(wake_lock_active(&uscp_wakelock))
    {
        pr_info("wake unlock\n");
        wake_unlock(&uscp_wakelock);
    }
}

static void charge_type_handler(struct uscp_device_info* di, enum power_supply_type type)
{
    int interval = 0;

    if ((!protect_enable)||(NULL == di))
        return;
    if ((POWER_SUPPLY_TYPE_USB_DCP == type)||(POWER_SUPPLY_TYPE_USB == type) || (POWER_SUPPLY_TYPE_USB_CDP == type))
    {
        if (hrtimer_active(&(di->timer)))
        {
            pr_info("timer already armed , do nothing\n");
        }
        else
        {
            pr_info("start uscp check\n");
            interval = INTERVAL_0;
            /*record 30 seconds after the charger just insert; 30s = (1100 - 1001 + 1)*300ms */
            di->keep_check_cnt = USCP_INSERT_CHG_CNT;
            hrtimer_start(&di->timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
        }
    }
    else
    {
        pr_info("charger type = %d, do nothing\n", type);
    }
}

static int usb_notifier_call(struct notifier_block *usb_nb, unsigned long event, void *data)
{
    struct uscp_device_info *di = container_of(usb_nb, struct uscp_device_info, usb_nb);
    enum power_supply_type type = ((enum power_supply_type)event);
    pr_err("%s:%d\n",__func__,type);

    charge_type_handler(di, type);
    return NOTIFY_OK;
}

static int get_one_adc_sample(void)
{
    int i =0;
    const int retry_times = RETRY_CNT;
    int T_sample = ADC_VOL_INIT;
    for (i = 0; i < retry_times; ++i)
    {
        T_sample = get_pmi_sub_voltage();

        if (T_sample < 0)
        {
            pr_err("adc read fail!\n");
        }
        else
        {
             break;
        }
    }
    return T_sample;
}
static int adc_to_temp(int adc_value)
{
    int table_size = sizeof(T_A_TABLE)/sizeof(T_A_TABLE[0]);
    int high = table_size - 1;
    int low = 0;
    int mid = 0;

    if (adc_value >= T_A_TABLE[0][1])
        return T_A_TABLE[0][0];
    if (adc_value <= T_A_TABLE[table_size - 1][1])
        return T_A_TABLE[table_size - 1][0];
    /*use binary search*/
    while (low < high)
    {
        pr_debug("low = %d,high = %d!\n", low, high);
        mid = (low + high) / 2;
        if (0 == mid)
            return T_A_TABLE[1][0];
        if (adc_value > T_A_TABLE[mid][1])
        {
            if (adc_value < T_A_TABLE[mid - 1][1])
                return T_A_TABLE[mid][0];
            high = mid - 1;
        }
        else if(adc_value < T_A_TABLE[mid][1])
        {
            if (adc_value >= T_A_TABLE[mid + 1][1])
                return T_A_TABLE[mid + 1][0];
            low = mid + 1;
        }
        else
            return T_A_TABLE[mid][0];
    }
    pr_err("transform error!\n");
    return 0;
}

static int get_usb_temp_value(void)
{
    int i = 0;
    int cnt = 0;
    int adc_temp = 0;
    const int sample_num = AVG_COUNT; // use 3 samples to get an average value
    int sum = 0;
    int temp = 0;

    for (i = 0; i < sample_num; ++i)
    {
        adc_temp = get_one_adc_sample();
        if (adc_temp >= 0)
        {
            sum += adc_temp;
            ++cnt;
        }
        else
        {
            pr_err(" get temperature fail!\n");
        }
    }
    if (cnt > 0)
    {
        temp = adc_to_temp(sum/cnt);
        return temp;
    }
    else
    {
        pr_err("use 0 as default temperature!\n");
        return 0;
    }
}
static int get_batt_temp_value(void)
{
    int rc = 0;
    union power_supply_propval ret = {0, };

    if((g_di == NULL)||(g_di->batt_psy == NULL) ||(g_di->bms_psy == NULL))
    {
        pr_err(" %s g_di is NULL!\n",__func__);
        return INVALID_BATT_TEMP;
    }

    rc = g_di->batt_psy->get_property(g_di->batt_psy, POWER_SUPPLY_PROP_PROFILE_STATUS, &ret);
    if((rc != 0)||(ret.intval == FG_NOT_READY))
    {
        pr_err(" %s  get profile error!\n",__func__);
        return INVALID_BATT_TEMP;
    }

    rc = g_di->batt_psy->get_property(g_di->batt_psy, POWER_SUPPLY_PROP_TEMP, &ret);
    if(rc)
    {
        pr_err(" %s  get temp error!\n",__func__);
        return INVALID_BATT_TEMP;
    }

    return ret.intval/COVERSE_TEMP_UNIT;

}

static void set_interval(struct uscp_device_info* di, int temp)
{
    if(NULL == di)
    {
        pr_err(" %s di is NULL!\n",__func__);
        return;
    }

    if (temp > di->interval_switch_temp) {
        di->check_interval = FAST_MONITOR_INTERVAL;
        di->keep_check_cnt = USCP_START_CHK_CNT;
        is_uscp_mode = TRUE;
        pr_info("cnt = %d!\n", di->keep_check_cnt);
    } else {
        if (di->keep_check_cnt > USCP_END_CHK_CNT) {
            /*check the temperature per 0.3 second for 100 times ,when the charger just insert.*/
            pr_info("cnt = %d!\n", di->keep_check_cnt);
            di->keep_check_cnt -= USCP_CHK_CNT_STEP;
            di->check_interval = FAST_MONITOR_INTERVAL;
            is_uscp_mode = FALSE;
        } else if (di->keep_check_cnt == USCP_END_CHK_CNT) {
            /* reset the flag when the temperature status is stable*/
            pr_info("cnt = %d!\n", di->keep_check_cnt);
            di->keep_check_cnt = USCP_DEFAULT_CHK_CNT;
            di->check_interval = NORMAL_MONITOR_INTERVAL;
            is_uscp_mode = FALSE;
            uscp_wake_unlock();
        } else if (di->keep_check_cnt >= USCP_START_CHK_CNT) {
            pr_info("cnt = %d!\n", di->keep_check_cnt);
            di->keep_check_cnt = di->keep_check_cnt + USCP_CHK_CNT_STEP;
            di->check_interval = FAST_MONITOR_INTERVAL;
            is_uscp_mode = TRUE;
        } else {
            di->check_interval = NORMAL_MONITOR_INTERVAL;
            is_uscp_mode = FALSE;
        }
    }
}
static void protection_process(struct uscp_device_info* di, int tbatt, int tusb)
{
    int gpio_value = 0;
    int tdiff = 0;

    if(NULL == di)
    {
        pr_err(" %s di is NULL!\n",__func__);
        return;
    }
    tdiff = tusb - tbatt;

    gpio_value = gpio_get_value(di->gpio_uscp);
    if ((INVALID_BATT_TEMP != tbatt) &&(tusb >= di->uscp_threshold_tusb) && (tdiff >= di->open_mosfet_temp)) {
        uscp_wake_lock();
        gpio_set_value(di->gpio_uscp, GPIO_HIGH);/*open mosfet*/
        pr_err("pull up gpio_uscp!gpio value:%d\n",gpio_value);
    } else if (tdiff <= di->close_mosfet_temp) {
        gpio_set_value(di->gpio_uscp, GPIO_LOW);/*close mosfet*/
        pr_info("pull down gpio_uscp!gpio value:%d\n",gpio_value);
    } else {
        /*do nothing*/
    }
}
static void check_temperature(struct uscp_device_info* di)
{
    int tusb = 0;
    int tbatt = 0;
    int tdiff = 0;

    if(NULL == di)
    {
        pr_err(" %s di is NULL!\n",__func__);
        return;
    }

    tusb = get_usb_temp_value();
    tbatt = get_batt_temp_value();

    pr_info("tusb = %d, tbatt = %d\n", tusb, tbatt);
    tdiff = tusb - tbatt;
    if(INVALID_BATT_TEMP == tbatt)
    {
        tdiff = INVALID_DELTA_TEMP;
        pr_err("get battery adc temp err,not care!!!\n");
    }

    if ((tusb >= di->uscp_threshold_tusb) && (tdiff >= di->open_mosfet_temp)) {
        if (protect_dmd_notify_enable) {
            if (!dsm_client_ocuppy(uscp_client)) {
                pr_info("record and notify\n");
                dsm_client_record(uscp_client, "usb short happened!\n");
                dsm_client_notify(uscp_client, ERROR_NO_USB_SHORT_PROTECT);
                protect_dmd_notify_enable = FALSE;
            }
        }
    }

    set_interval(di, tdiff);
    protection_process(di, tbatt, tusb);
}
static void uscp_check_work(struct work_struct *work)
{
    struct uscp_device_info *di = container_of(work,struct uscp_device_info, uscp_check_wk);
    int interval = 0;
    int type = 0;
    type = get_loginfo_int(di->usb_psy, POWER_SUPPLY_PROP_TYPE);
    
#ifdef CONFIG_HLTHERM_RUNTEST
    pr_info("Disable HLTHERM protect\n");
    return;
#endif

    if (((USCP_DEFAULT_CHK_CNT == di->keep_check_cnt) && (POWER_SUPPLY_TYPE_UNKNOWN == type)))
    {
        protect_dmd_notify_enable = TRUE;
        gpio_set_value(di->gpio_uscp, GPIO_LOW);//close mosfet
        di->keep_check_cnt = USCP_DEFAULT_CHK_CNT;
        di->check_interval = NORMAL_MONITOR_INTERVAL;
        is_uscp_mode = FALSE;
        di->keep_check_cnt = USCP_INSERT_CHG_CNT;
        pr_info("chargertype is %d,stop checking\n", type);
        return;
    }

    check_temperature(di);
    interval = di->check_interval;

    hrtimer_start(&di->timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);

}

static enum hrtimer_restart uscp_timer_func(struct hrtimer *timer)
{
    struct uscp_device_info *di = NULL;

    di = container_of(timer, struct uscp_device_info, timer);
    queue_work(di->uscp_wq, &di->uscp_check_wk);
    return HRTIMER_NORESTART;
}

static void check_ntc_error(void)
{
    int temp = 0;
    int sum = 0;
    int i = 0;

    for (i = 0; i < USB_TEMP_CNT; ++i)
    {
        sum += get_usb_temp_value();
    }
    temp = sum / USB_TEMP_CNT;
    if (temp > TUSB_TEMP_UPPER_LIMIT || temp < TUSB_TEMP_LOWER_LIMIT)
    {
        if (!dsm_client_ocuppy(uscp_client))
        {
            pr_info("ntc error notify\n");
            dsm_client_record(uscp_client, "ntc error happened!\n");
            dsm_client_notify(uscp_client, ERROR_NO_USB_SHORT_PROTECT_NTC);
        }
        protect_enable = FALSE;
    }
    else
    {
        pr_info("enable usb short protect\n");
        protect_enable = TRUE;
    }
}
static int uscp_probe(struct platform_device *pdev)
{
    struct device_node* np = NULL;
    struct uscp_device_info* di = NULL;
    enum power_supply_type type = POWER_SUPPLY_TYPE_UNKNOWN;
    struct power_supply *usb_psy = NULL;
    struct power_supply *batt_psy = NULL;
    struct power_supply *bms_psy = NULL;
    int ret = 0;
    int batt_present = TRUE;
    int need_uscp = TRUE;

    usb_psy = power_supply_get_by_name("usb");
    if (!usb_psy) {
        pr_err("usb supply not found deferring probe\n");
        return -EPROBE_DEFER;
    }
    batt_psy = power_supply_get_by_name("battery");
    if (!batt_psy) {
        pr_err("batt supply not found deferring probe\n");
        return -EPROBE_DEFER;
    }
    bms_psy = power_supply_get_by_name("bms");
    if (!bms_psy) {
        pr_err("bms supply not found deferring probe\n");
        return -EPROBE_DEFER;
    }

    np = pdev->dev.of_node;
    if(NULL == np)
    {
        pr_err("np is NULL\n");
        return -1;
    }
    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di)
    {
        pr_err("di is NULL\n");
        return -ENOMEM;

    }
    di->dev = &pdev->dev;
    dev_set_drvdata(&(pdev->dev), di);
    g_di = di;
    if (!uscp_client)
    {
        uscp_client = dsm_register_client(&dsm_uscp);
    }
    if (NULL == uscp_client)
    {
        pr_err("uscp register dsm fail\n");
        ret = -EINVAL;
        goto free_mem;
    }

    di->usb_psy = usb_psy;
    di->batt_psy = batt_psy;
    di->bms_psy = bms_psy;
    is_uscp_mode = FALSE;
    di->keep_check_cnt = USCP_INSERT_CHG_CNT;

    di->gpio_uscp = of_get_named_gpio(np, "gpio_usb_short_circuit_protect",0);
    if (!gpio_is_valid(di->gpio_uscp))
    {
        pr_err("gpio_uscp is not valid\n");
        ret = -EINVAL;
        goto free_mem;
    }
    pr_info("gpio_uscp = %d\n", di->gpio_uscp);

    ret = gpio_request(di->gpio_uscp, "usb_short_circuit_protect");
    if (ret)
    {
        pr_err("could not request gpio_uscp\n");
        ret = -EINVAL;
        goto free_mem;
    }
    gpio_direction_output(di->gpio_uscp, GPIO_LOW);

    ret = of_property_read_u32(np, "no_need_uscp", &(di->no_need_uscp));
    if (ret)
    {
        pr_err("get open_mosfet_temp info fail!\n");
        ret = -EINVAL;
        goto free_gpio;
    }
    pr_info("no_need_uscp = %d\n", di->no_need_uscp);
    ret = of_property_read_u32(np, "uscp_threshold_tusb", &(di->uscp_threshold_tusb));
    if (ret)
    {
        di->uscp_threshold_tusb = DEFAULT_TUSB_THRESHOLD;
        pr_err("get uscp_threshold_tusb info fail!use default threshold = %d\n",di->uscp_threshold_tusb);
    }
    pr_info("uscp_threshold_tusb = %d\n", di->uscp_threshold_tusb);
    ret = of_property_read_u32(np, "open_mosfet_temp", &(di->open_mosfet_temp));
    if (ret)
    {
        pr_err("get open_mosfet_temp info fail!\n");
        ret = -EINVAL;
        goto free_gpio;
    }
    pr_info("open_mosfet_temp = %d\n", di->open_mosfet_temp);
    ret = of_property_read_u32(np, "close_mosfet_temp", &(di->close_mosfet_temp));
    if (ret)
    {
        pr_err("get close_mosfet_temp info fail!\n");
        ret = -EINVAL;
        goto free_gpio;
    }
    pr_info("close_mosfet_temp = %d\n", di->close_mosfet_temp);
    ret = of_property_read_u32(np, "interval_switch_temp", &(di->interval_switch_temp));
    if (ret)
    {
        pr_err("get interval_switch_temp info fail!\n");
        ret = -EINVAL;
        goto free_gpio;
    }
    pr_info("interval_switch_temp = %d\n", di->interval_switch_temp);

    check_ntc_error();
    batt_present = get_loginfo_int(batt_psy, POWER_SUPPLY_PROP_PRESENT);

    if((is_factory_mode)&& (di->no_need_uscp == TRUE))
    {
        need_uscp = FALSE; //qcom not need uscp in factory mode
    }

   if ((!batt_present) ||(FALSE == need_uscp)) {
        pr_err("battery is not exist or no need uscp in factory mode, disable usb short protect!\n");
        protect_enable = FALSE;
    }
    if (!protect_enable)
    {
        goto free_gpio;
    }
    wake_lock_init(&uscp_wakelock, WAKE_LOCK_SUSPEND, "usb_short_circuit_protect_wakelock");
    di->uscp_wq = create_singlethread_workqueue("usb_short_circuit_protect_wq");
    INIT_WORK(&di->uscp_check_wk, uscp_check_work);
    hrtimer_init(&di->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    di->timer.function = uscp_timer_func;
    di->usb_nb.notifier_call = usb_notifier_call;
    ret = usb_charger_register_notifier(&di->usb_nb);
    if (ret < 0)
    {
       pr_err("hisi_charger_type_notifier_register failed\n");
       ret = -EINVAL;
        goto free_gpio;
    }

    type = get_loginfo_int(usb_psy, POWER_SUPPLY_PROP_TYPE);

    pr_info("usb type = %d\n", type);
    charge_type_handler(di, type);

    pr_info("uscp probe ok!\n");
    return 0;

free_gpio:
  gpio_free(di->gpio_uscp);
free_mem:
    kfree(di);
    g_di = NULL;
    return ret;
}
static int uscp_remove(struct platform_device *pdev)
{
    struct uscp_device_info *di = dev_get_drvdata(&pdev->dev);

    gpio_free(di->gpio_uscp);
    kfree(di);
    g_di = NULL;

    return 0;
}
static struct of_device_id uscp_match_table[] =
{
    {
        .compatible = "huawei,usb_short_circuit_protect",
        .data = NULL,
    },
    {
    },
};
static struct platform_driver uscp_driver = {
    .probe = uscp_probe,
	.remove = uscp_remove,
    .driver = {
        .name = "huawei,usb_short_circuit_protect",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(uscp_match_table),
    },
};

static int __init uscp_init(void)
{
    return platform_driver_register(&uscp_driver);
}

device_initcall_sync(uscp_init);

static void __exit uscp_exit(void)
{
    platform_driver_unregister(&uscp_driver);
}

module_exit(uscp_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:uscp");
MODULE_AUTHOR("HUAWEI Inc");
