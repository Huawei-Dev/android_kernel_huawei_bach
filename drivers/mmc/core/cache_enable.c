/*
 *  linux/drivers/mmc/core/cache_enable.c
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *  Copyright (C) 2005-2007 Pierre Ossman, All Rights Reserved.
 *  MMCv4 support Copyright (C) 2006 Philip Langdale, All Rights Reserved.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/mmc/card.h>
#include <linux/string.h>
#define CID_MANFID_SAMSUNG	0x15
extern char *saved_command_line;

static bool is_batterystate_exist(void)
{
	if(strstr(saved_command_line, "androidboot.huawei_battery_state=exist") != NULL) {
		pr_info("%s: battery state exist\n", __func__);
		return true;
	}else{
		pr_info("%s: battery state no exist\n", __func__);
		return false;
	}
}
static bool is_normal_mode(void)
{
	if(NULL != strstr(saved_command_line,"androidboot.huawei_swtype=factory")) {
		pr_info("%s: factory runmode\n", __func__);
		return false;
	}else{
		pr_info("%s: normal runmode\n", __func__);
		return true;
	}
}
int mmc_screen_test_cache_enable(struct mmc_card *card)
{
	int need_cache_on = 1;
	bool runmode_normal = true;

	if(card == NULL)
	{
		printk(KERN_INFO "card is null and then return\n");
		return need_cache_on;
	}

	if(is_normal_mode() || is_batterystate_exist() )
	{
		pr_info(KERN_INFO "need_cache_on\n");
		need_cache_on = 1;
	}
	else
	{
		pr_info(KERN_INFO "need_cache_off\n");
		need_cache_on = 0;
	}
	pr_info(KERN_INFO "runmode_normal=%d is_batterystate_exist=%d card->cid.manfid=%x\n", runmode_normal, is_batterystate_exist(), card->cid.manfid);
	return need_cache_on;
}