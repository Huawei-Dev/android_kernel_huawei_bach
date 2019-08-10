/*
 * Protect lru of task support. It's between normal lru and mlock,
 * that means we will reclaim protect lru pages as late as possible.
 *
 * Copyright (c) 2016 Hisilicon.
 *
 * Authors:
 * Shaobo Feng <fengshaobo@huawei.com>
 * Xishi Qiu <qiuxishi@huawei.com>
 * Yisheng Xie <xieyisheng1@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/utsname.h>
#include <linux/xattr.h>
#include <linux/swap.h>
#include <linux/mm_inline.h>
#include <asm/uaccess.h>

#include "internal.h"
#include <linux/protect_lru.h>

#define MAX_BUFFER_LEN          256
#define MAX_LEN                 (10 + MAX_BUFFER_LEN)
#define MAX_LEVEL               3
#define FPROTECTLRUSET          0x545E


static inline bool check_protect_page(struct page *page)
{
	/* Skip non-lru page */
	if (!PageLRU(page))
		return false;

	return check_file_page(page);
}

void protect_lru_set_from_file(struct page *page)
{
	struct inode *inode;
	int num = 0;

	/*lint -save -e730 -e676*/
	if (unlikely(!protect_lru_enable))
		return;

	if (page->mapping) {
		inode = page->mapping->host;
		if (inode)
			num = inode->i_protect;
	}
	/*
	 * Add the protect flag by file.
	 * Its priority is higher than process.
	 * We cannot change the prot level dynamically.
	 */
	if (unlikely(num != 0)) {
		struct zone *zone;
		struct lruvec *lruvec;

		zone = page_zone(page);
		lruvec = mem_cgroup_page_lruvec(page, zone);
		if (check_file_page(page)
		    && lruvec->heads[num - 1].max_pages) {
			SetPageActive(page);
			SetPageProtect(page);
			set_page_num(page, num);
		}
	}
	/*lint -restore*/
}
EXPORT_SYMBOL(protect_lru_set_from_file);

void protect_lru_set_from_process(struct page *page)
{
	struct mm_struct *mm = current->mm;

	/*lint -save -e730 -e613*/
	if (unlikely(protect_lru_enable && mm && (mm->protect != 0))) {
		struct zone *zone;
		struct lruvec *lruvec;
		int num;

		zone = page_zone(page);
		lruvec = mem_cgroup_page_lruvec(page, zone);
		num = mm->protect;
		if (check_file_page(page)
		    && lruvec->heads[num - 1].max_pages
		    && !PageProtect(page)) {
			SetPageActive(page);
			SetPageProtect(page);
			set_page_num(page, num);
		}
	}
	/*lint -restore*/
}
EXPORT_SYMBOL(protect_lru_set_from_process);

void add_page_to_protect_lru_list(struct page *page, struct lruvec *lruvec, bool lru_head)
{
	int nr_pages, num = 0;
	enum lru_list lru;
	struct list_head *head;

	if (!check_protect_page(page))
		return;

	lru = page_lru(page);
	/*lint -save -e64 -e730 -e737 -e679 -e644*/
	if (unlikely(PageProtect(page))) {
		nr_pages = hpage_nr_pages(page);
		num = get_page_num(page) - 1;
		head = &lruvec->heads[num].protect_page[lru].lru;
		lruvec->heads[num].pages += nr_pages;
		__mod_zone_page_state(page_zone(page),
				NR_PROTECT_LRU_BASE + lru, nr_pages);
	} else
		head = &lruvec->heads[PROTECT_HEAD_END].protect_page[lru].lru;

	if (lru_head)
		/* Move to the head for both normal and prot page */
		list_move(&page->lru, head);
	else if (unlikely(PageProtect(page))) {
		/* Move to the tail for prot page */
		head = &lruvec->heads[num + 1].protect_page[lru].lru;
		list_move_tail(&page->lru, head);
	} else {
		/* Move to the tail for normal page */
		head = &lruvec->lists[lru];
		list_move_tail(&page->lru, head);
	}
	/*lint -restore*/
}
EXPORT_SYMBOL(add_page_to_protect_lru_list);

void del_page_from_protect_lru_list(struct page *page, struct lruvec *lruvec)
{
	int nr_pages, num;
	enum lru_list lru;

	if (!check_protect_page(page))
		return;

	/*lint -save -e64 -e730 -e737*/
	if (unlikely(PageProtect(page))) {
		lru = page_lru(page);
		nr_pages = hpage_nr_pages(page);
		num = get_page_num(page) - 1;
		lruvec->heads[num].pages -= nr_pages;
		__mod_zone_page_state(page_zone(page),
				NR_PROTECT_LRU_BASE + lru, -nr_pages);
		/*lint -restore*/
	}
}
EXPORT_SYMBOL(del_page_from_protect_lru_list);

void protect_lruvec_init(struct lruvec *lruvec)
{
	enum lru_list lru;
	struct page *page;
	int i;

	for (i = 0; i < PROTECT_HEAD_MAX; i++) {
		/*lint -save -e64*/
		for_each_evictable_lru(lru) {
		/*lint -restore*/
			page = &lruvec->heads[i].protect_page[lru];
			/*
			 * The address is not from direct mapping,
			 * so can't use __va()/__pa(), page_to_pfn()...
			 */
			init_page_count(page);
			page_mapcount_reset(page);
			SetPageReserved(page);
			SetPageLRU(page);
			INIT_LIST_HEAD(&page->lru);
			/* Only protect file list */
			if (lru >= LRU_INACTIVE_FILE)
				list_add_tail(&page->lru, &lruvec->lists[lru]);
		}
		lruvec->heads[i].max_pages = 0;
		lruvec->heads[i].pages = 0;
	}
}
EXPORT_SYMBOL(protect_lruvec_init);

int ioctl_protect_lru_set(struct file *filp, unsigned long arg)
{
	struct inode *inode;
	int ret = 0;

	inode = file_inode(filp);
	if (!inode) {
		ret = -EINVAL;
		goto out;
	}

	/*lint -save -e568 -e685 -e574 -e737*/
	if (arg <= PROTECT_HEAD_END) {
		inode->i_protect = (int)arg;
		pr_info("protect_lru: set file %s, num=%lu\n",
			filp->f_path.dentry->d_name.name, arg);
	} else
		ret = -EINVAL;
	/*lint -restore*/

out:
	return ret;
}
EXPORT_SYMBOL(ioctl_protect_lru_set);

int ioctl_protect_lru_get(struct file *filp)
{
	struct inode *inode;
	int ret = 0, num;

	inode = file_inode(filp);
	if (!inode) {
		ret = -EINVAL;
		goto out;
	}

	num = inode->i_protect;
	pr_info("protect_lru: get file %s, num=%d\n",
		filp->f_path.dentry->d_name.name, num);

out:
	return ret;
}
EXPORT_SYMBOL(ioctl_protect_lru_get);

struct page *protect_lru_move_and_shrink(struct page *page)
{
	struct zone *zone;
	struct lruvec *lruvec;
	unsigned long flags;

	/*lint -save -e730*/
	if (unlikely(!protect_lru_enable))
		return page;
	/*lint -restore*/

	if (!page)
		return NULL;

	zone = page_zone(page);
	lruvec = mem_cgroup_page_lruvec(page, zone);

	if (protect_file_is_full(lruvec)) {
		/*lint -save -e550 -e747*/
		spin_lock_irqsave(&zone->lru_lock, flags);
		shrink_protect_file(lruvec, false);
		/*lint -restore*/
		spin_unlock_irqrestore(&zone->lru_lock, flags);
	}

	if (PageProtect(page))
		mark_page_accessed(page);

	return page;
}
EXPORT_SYMBOL(protect_lru_move_and_shrink);

static unsigned long protect_max_mbytes[PROTECT_HEAD_END];
static unsigned long protect_cur_mbytes[PROTECT_HEAD_END];
static unsigned long zero;
static unsigned long one = 1;
static unsigned long fifty = 50;
static unsigned long one_hundred = 100;
unsigned long protect_lru_enable __read_mostly = 1;
unsigned long protect_reclaim_ratio = 50;

static int sysctl_protect_max_mbytes_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *length, loff_t *ppos)
{
	struct zone *zone;
	struct lruvec *lruvec;
	unsigned long prot_pages, total_prot_pages, flags;
	int i, ret;

	ret = proc_doulongvec_minmax(table, write, buffer, length, ppos);
	if (ret)
		return ret;

	if (write) {
		if (!mem_cgroup_disabled())
			return -EINVAL;

		/* Skip the last head, because it is non-prot head */
		for (i = 0; i < PROTECT_HEAD_END; i++) {
			total_prot_pages = protect_max_mbytes[i] << (20 - PAGE_SHIFT);
			for_each_populated_zone(zone) {
				prot_pages = (u64)zone->managed_pages * total_prot_pages
						/ totalram_pages;
				lruvec = &zone->lruvec;
				lruvec->heads[i].max_pages = prot_pages;
			}
		}

		for_each_populated_zone(zone) {
			lruvec = &zone->lruvec;
			while (protect_file_is_full(lruvec)) {
				/*lint -save -e550 -e747*/
				spin_lock_irqsave(&zone->lru_lock, flags);
				shrink_protect_file(lruvec, false);
				/*lint -restore*/
				spin_unlock_irqrestore(&zone->lru_lock, flags);
				if (signal_pending(current))
					return ret;
				cond_resched();
			}
		}
	}

	return ret;
}

static int sysctl_protect_cur_mbytes_handler(struct ctl_table *table, int write,
		void __user *buffer, size_t *length, loff_t *ppos)
{
	struct zone *zone;
	struct lruvec *lruvec;
	int i;

	if (!mem_cgroup_disabled())
		return -EINVAL;

	/* Skip the last head, because it is non-prot head */
	for (i = 0; i < PROTECT_HEAD_END; i++) {
		protect_cur_mbytes[i] = 0;
		for_each_populated_zone(zone) {
			lruvec = &zone->lruvec;
			protect_cur_mbytes[i] += lruvec->heads[i].pages;
		}
		protect_cur_mbytes[i] >>= (20 - PAGE_SHIFT);
	}

	return proc_doulongvec_minmax(table, write, buffer, length, ppos);
}

/*lint -save -e679 -e826 -e747*/
void shrink_protect_file(struct lruvec *lruvec, bool force)
{
	int i, count;
	unsigned long cur, max;
	struct list_head *head;
	struct page *tail;

	/* Skip the last head, because it is non-prot head */
	for (i = 0; i < PROTECT_HEAD_END; i++) {
		cur = lruvec->heads[i].pages;
		max = lruvec->heads[i].max_pages;
		if (!cur)
			continue;
		if (!force && cur <= max)
			continue;

		/* Shrink the inactive list first */
		head = &lruvec->heads[i + 1].protect_page[LRU_INACTIVE_FILE].lru;
		tail = list_entry(head->prev, struct page, lru);
		if (PageReserved(tail))
			head = &lruvec->heads[i + 1].protect_page[LRU_ACTIVE_FILE].lru;

		/* Move 32 tail pages every time */
		count = SWAP_CLUSTER_MAX;
		while (count--) {
			tail = list_entry(head->prev, struct page, lru);
			if (PageReserved(tail))
				break;
			del_page_from_protect_lru_list(tail, lruvec);
			ClearPageProtect(tail);
			set_page_num(tail, 0);
			add_page_to_protect_lru_list(tail, lruvec, true);
		}
	}
}
/*lint -restore*/
EXPORT_SYMBOL(shrink_protect_file);

bool protect_file_is_full(struct lruvec *lruvec)
{
	int i;
	unsigned long cur, max;

	/* Skip the last head, because it is non-prot head */
	for (i = 0; i < PROTECT_HEAD_END; i++) {
		cur = lruvec->heads[i].pages;
		max = lruvec->heads[i].max_pages;
		if (cur && cur > max)
			return true;
	}

	return false;
}
EXPORT_SYMBOL(protect_file_is_full);

struct ctl_table protect_lru_table[] = {
/*lint -save -e785*/
	{
		.procname	= "protect_max_mbytes",
		.data		= &protect_max_mbytes,
		.maxlen		= sizeof(protect_max_mbytes),
		.mode		= 0640,
		.proc_handler	= sysctl_protect_max_mbytes_handler,
		.extra1         = &zero,
		.extra2         = &one_hundred,
	},
	{
		.procname	= "protect_cur_mbytes",
		.data		= &protect_cur_mbytes,
		.maxlen		= sizeof(protect_cur_mbytes),
		.mode		= 0440,
		.proc_handler	= sysctl_protect_cur_mbytes_handler,
	},
	{
		.procname	= "protect_lru_enable",
		.data		= &protect_lru_enable,
		.maxlen		= sizeof(unsigned long),
		.mode		= 0640,
		.proc_handler	= proc_doulongvec_minmax,
		.extra1         = &zero,
		.extra2         = &one,
	},
	{
		.procname	= "protect_reclaim_ratio",
		.data		= &protect_reclaim_ratio,
		.maxlen		= sizeof(unsigned long),
		.mode		= 0640,
		.proc_handler	= proc_doulongvec_minmax,
		.extra1		= &fifty,
	},
	{ }
/*lint -restore*/
};

static int __init protect_lru_init(void)
{
	struct zone *zone;
	struct lruvec *lruvec;
	unsigned long prot_pages, total_prot_pages;
	int i;

	/* Set the default value of protect max */
	for (i = 0; i < PROTECT_HEAD_END; i++) {
		total_prot_pages = protect_max_mbytes[i] << (20 - PAGE_SHIFT);
		for_each_populated_zone(zone) {
			prot_pages = (u64)zone->managed_pages * total_prot_pages
					/ totalram_pages;
			lruvec = &zone->lruvec;
			lruvec->heads[i].max_pages = prot_pages;
		}
	}

	return 0;
}

/*lint -e528 -esym(528,*)*/
module_init(protect_lru_init);
/*lint -e528 +esym(528,*)*/

/*
 * Format: 'fd level path'
 * */
/*lint -save -e838 -e732 -e747 -e713 -e715 -e438 -e550*/
static ssize_t protect_write(struct file *file, const char __user * buffer,
                       size_t count, loff_t *ppos)
{
	char proctectData[MAX_LEN] = {0};
	int ret = 0, index = 0, fd = 0, level = 0;
	struct fd f;
	char *p = NULL, *start = NULL, *path = NULL, *pStrLevel = NULL;


	pr_info("set protect lru:begin\n");
	if (count > (sizeof(proctectData) - 1)) {
		pr_err("set protect lru:error count too large\n");
		return -EINVAL;
	}

	if (copy_from_user(proctectData, buffer, count)) {
		pr_err("set protect lru:error copy\n");
		return -EFAULT;
	}

	proctectData[count] = '\0';
	start = strstrip(proctectData);
	if (strlen(start) < 1) {
		pr_err("set protect lru:zero len\n");
		return -EINVAL;
	}

	p = start;
	while ((*p) != '\0') {
		if (*p != ' ') {
			p++;
			continue;
		}

		if (index == 0) {
			index ++;
			*p = '\0';
			p++;
			while (' ' == *p)
				p++;
			pStrLevel = p;
		} else if (index == 1) {
			index ++;
			*p = '\0';
			p++;
			while (' ' == *p)
				p++;
			break;
		}
	}

	if (2 != index) {
		pr_err("set protect lru:incorrect content!\n");
		return -EINVAL;
	}

	path = p;
	ret = kstrtoint(start, 10, &fd);
	if (0 != ret) {
		pr_err("set protect lru:get fd error!\n");
		return -EINVAL;
	}

	ret = kstrtoint(pStrLevel, 10, &level);
	if (0 != ret) {
		pr_err("set protect lru:get level error!\n");
		return -EINVAL;
	}

	f = fdget(fd);
	if (!f.file) {
		pr_err("set protect lru:get file fail\n");
		return -EBADF;
	}

	if (0 <= level && level <= MAX_LEVEL) {
		ret = do_vfs_ioctl(f.file, fd, FPROTECTLRUSET, level);
	} else {
		pr_err("set protect lru: level is not right\n");
		fdput(f);
		return -EINVAL;
	}
	fdput(f);

	pr_info("set protect lru:end\n");
	return count;
}
/*lint -restore*/

/*lint -save -e785*/
static const struct file_operations protect_proc_fops = {
	.write  = protect_write,
	.llseek = noop_llseek,
};
/*lint -restore*/

static int __init proc_protect_init(void)
{
	proc_create("protect_lru", 0220, NULL, &protect_proc_fops);
	return 0;
}

/*lint -e528 -esym(528,*)*/
fs_initcall(proc_protect_init);
/*lint -e528 +esym(528,*)*/
