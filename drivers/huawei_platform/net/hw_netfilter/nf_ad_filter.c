

#include <linux/module.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>/* add for log */
#include <linux/ctype.h>/* add for tolower */
#include <linux/slab.h>
#include <linux/jhash.h>

#include "nf_hw_common.h"
#include "nf_ad_filter.h"
struct ad_url {
	struct list_head list;
	char *url;
	int len;
};

struct ad_rule {
	struct list_head list;
	unsigned int uid;
	struct ad_url urllist;
};

static struct list_head g_adhead[MAX_HASH];
static spinlock_t g_adlock[MAX_HASH];/* lock for g_adhead array*/

#define MAX_RECENT_DROP 128

static spinlock_t g_droplock;
static struct ad_url g_droparr[MAX_RECENT_DROP];
static unsigned int g_curcount;

bool is_droplist(char *url, int len)
{
	int i;

	if (!url)
		return false;
	spin_lock_bh(&g_droplock);
	for (i = 0; i < MAX_RECENT_DROP; i++) {
		if (g_droparr[i].url && g_droparr[i].len == len) {
			if (memcmp(g_droparr[i].url, url, len) == 0) {
				spin_unlock_bh(&g_droplock);
				return true;
			}
		}
	}
	spin_unlock_bh(&g_droplock);
	return false;
}

void add_to_droplist(char *url, int len)
{
	unsigned int idx = 0;
	char *temp;

	if (!url)
		return false;
	if (is_droplist(url, len))
		return;
	temp = kmalloc(len + 1, GFP_ATOMIC);
	if (NULL == temp)
		return;
	temp[len] = 0;
	memcpy(temp, url, len);
	spin_lock_bh(&g_droplock);
	idx = g_curcount++;
	if (g_curcount >= MAX_RECENT_DROP)
		g_curcount = 0;
	kfree(g_droparr[idx].url);
	g_droparr[idx].url = temp;
	g_droparr[idx].len = len;
	spin_unlock_bh(&g_droplock);
}


void add_url_to_node(struct ad_rule *node, const char *url, int len)
{
	struct ad_url *nd = NULL;
	int urllen = len / 2;
	char *urlstr = kmalloc(urllen + 2, GFP_ATOMIC);/* new 101 */

	if (NULL == urlstr)
		return;
	hex_to_byte(url, len, urlstr, urllen + 2);
	urlstr[urllen] = 0;
	list_for_each_entry(nd, &node->urllist.list, list) {
		if (urllen == nd->len) {
			if (memcmp(nd->url, urlstr, urllen) == 0) {
				pr_info("hwad: already in list url=%s", url);
				kfree(urlstr);/* kfree 101 */
				return;
			}
		}
	}
	nd = kmalloc(sizeof(struct ad_url), GFP_ATOMIC);/* new 102 */
	if (NULL == nd) {
		kfree(urlstr);/* kfree 101 */
		return;
	}
	nd->url = urlstr;
	nd->len = urllen;
	list_add(&nd->list, &node->urllist.list);
}

void add_ad_rule_to_list(unsigned int uid, const char *url, int len)
{
	struct ad_rule *node = NULL;
	unsigned int hid = get_hash_id(uid);

	if (hid >= MAX_HASH) {
		pr_info("hwad:url to list>=MAX_HASH id=%u\n", hid);
		return;
	}
	spin_lock_bh(&g_adlock[hid]);
	list_for_each_entry(node, &g_adhead[hid], list) {
		if (node->uid == uid) {
			add_url_to_node(node, url, len);
			spin_unlock_bh(&g_adlock[hid]);
			return;
		}
	}
	node = kmalloc(sizeof(struct ad_rule), GFP_ATOMIC);/* new 103 */
	if (NULL == node) {
		spin_unlock_bh(&g_adlock[hid]);
		return;
	}
	pr_info("hwad:add_ad_rule_to_list newuid=%u url=<%s>\n", uid, url);
	INIT_LIST_HEAD(&node->urllist.list);
	node->uid = uid;
	add_url_to_node(node, url, len);
	list_add(&node->list, &g_adhead[hid]);
	spin_unlock_bh(&g_adlock[hid]);
}

void add_ad_uid_rule(const char *rule, int len)
{
	/* 1001:url9,url10; 15 */
	const char *p = NULL;
	unsigned int uid;
	int temlen = 0;
	int i = 0;

	if (NULL == rule)
		return;
	p = strstr(rule, ":");
	uid = simple_strtoull(rule, &p, 10);
	p++;
	i = p - rule;
	while (i < len) {
		if (rule[i] == ',') {
			temlen = rule + i - p;
			if (temlen > 0)
				add_ad_rule_to_list(uid, p, temlen);
			p = rule + i + 1;
		}
		i++;
	}
	if (p - rule < len) {
		temlen = rule + len - p;
		add_ad_rule_to_list(uid, p, temlen);
	}
}

void add_ad_rule(const char *rule, bool reset)
{
	/* 1001:url9,url10;10004:url6,url7,url8;*/
	const char *p = rule;
	const char *temp;
	int len = strlen(rule);

	if (NULL == rule)
		return;
	if (reset)
		clear_ad_rule(1, "");
	while (*p != 0) {
		temp = strstr(p, ";");
		if (!temp)
			break;
		if (temp - p > 0) {
			add_ad_uid_rule(p, temp - p);
			if (temp - rule >= len)
				break;
			p = temp + 1;
			if (*p == 0)
				break;
		} else {
			break;
		}
	}
}

void clear_node_ad_rule(struct ad_rule *node)
{
	struct ad_url *temp = NULL;
	struct ad_url *next = NULL;

	if (NULL == node)
		return;
	list_for_each_entry_safe(temp, next, &node->urllist.list, list) {
		list_del(&temp->list);
		temp->len = 0;
		kfree(temp->url);/* kfree 101 */
		kfree(temp);/* kfree 102 */
	}
	list_del(&node->list);
	kfree(node);/* kfree 103 */
}

void clear_uid_ad_rule(int uid)
{
	struct ad_rule *node = NULL;
	struct ad_rule *next = NULL;
	unsigned int hid = get_hash_id(uid);

	if (hid >= MAX_HASH)
		return;
	spin_lock_bh(&g_adlock[hid]);
	list_for_each_entry_safe(node, next, &g_adhead[hid], list) {
		if (uid == node->uid)
			clear_node_ad_rule(node);
	}
	spin_unlock_bh(&g_adlock[hid]);
}

void clear_ad_rule_string(const char *rule)
{
	/* 1001;10004;10064; */
	const char *p;
	const char *temp;
	int uid = 0;

	if (NULL == rule)
		return;
	temp = rule;
	p = rule;
	while (p && *p != 0) {
		p = strstr(temp, ";");
		if (p) {
			uid = simple_strtoull(temp, &p, 10);
			clear_uid_ad_rule(uid);
			temp = p + 1;
		}
	}
}

void clear_ad_rule(int opt, const char *data)
{
	int i = 0;
	struct ad_rule *node = NULL;
	struct ad_rule *next = NULL;

	if (NULL == data)
		return;
	if (0 == opt) {
		clear_ad_rule_string(data);
		return;
	}
	for (i = 0; i < MAX_HASH; i++) {
		spin_lock_bh(&g_adlock[i]);
		list_for_each_entry_safe(node, next, &g_adhead[i], list) {
			clear_node_ad_rule(node);
			}
		spin_unlock_bh(&g_adlock[i]);
	}
}

bool match_ad_uid(unsigned int uid)
{
	struct ad_rule *node = NULL;
	unsigned int hid = get_hash_id(uid);

	if (hid >= MAX_HASH) {
		pr_info("hwad:match_ad_uid>=MAX_hid=%uuid=%u\n", hid, uid);
		return false;
	}
	spin_lock_bh(&g_adlock[hid]);
	list_for_each_entry(node, &g_adhead[hid], list) {
		if (node->uid == uid) {
			spin_unlock_bh(&g_adlock[hid]);
			return true;
		}
	}
	spin_unlock_bh(&(g_adlock[hid]));
	return false;
}

bool match_ab_string(const char *str, const char *a,
		     int al, const char *b, int bl)
{
	const char *ps;

	ps = strfinds2(str, a, al);
	if (NULL == ps)
		return false;
	ps += al;
	return match_url_string(ps, b, bl);
}
/* Description */
/*1. xxx								*/
/*  1.1 abcd bc							*/
/*  1.2 abcde abcde						*/
/*2. *xxx								*/
/*  2.1 abcdef *bc						*/
/*  2.2 abcdef *cdef					*/
/*  2.3 abcdef *abcdef					*/
/*3. xxx*								*/
/*  3.1 abcdef bc*						*/
/*  3.2 abcdef abc*						*/
/*  3.3 abcdef abcdef*					*/
/*4. xxx*yyy							*/
/*  4.1 abcdef ab*f						*/
/*  4.2 abcdefg bc*de					*/
/*  4.3 abcdef ab*def					*/
/*  4.4 abcdef abc*def					*/
/*5. xxx*yyy*xxx*zzz					*/
/*  5.1 abcdefghigk ab*f*igk			*/
/*  5.2 abcdefghigk *ab*f*				*/
/*  5.3 abcdefghigk a*c*e*g*i*k			*/
/*  5.4 abcdefghigk *a*c*e*g*i*k*		*/
/*http://xxx/adhhhh.html*/
bool match_url_string(const char *url, const char *filter, int len)
{
	const char *ps = strfindpos(filter, "*", len);

	if (NULL == ps) {
		/*1. xxx*/
		if (strfinds2(url, filter, len))
			return true;
		return false;
	}
	if (filter == ps) {
		/*2. *xxx*/
		return match_url_string(url, filter+1, len-1);
	} else if (filter + len -1  == ps) {
		/*3. xxx**/
		if (strfinds2(url, filter, len-1))
			return true;
		else
			return false;
	} else {
		/*4. xxx*yyy*/
		return match_ab_string(url,
			filter, ps -filter,
			ps + 1, filter+len-ps-1);
	}
}


bool match_uid_url(struct ad_rule *node, const char *url)
{
	struct ad_url *temp_node = NULL;
	bool bret = false;

	list_for_each_entry(temp_node, &node->urllist.list, list) {
		if (0 != temp_node->len) {
			if (match_url_string(url, temp_node->url,
					temp_node->len)) {
				pr_info("hwad: match_url_string true rule=%s\n",
						temp_node->url);
				bret = true;
				break;
			}
		}
	}
	return bret;
}

bool match_ad_url(unsigned int uid, const char *data, int datalen)
{
	struct ad_rule *node = NULL;
	unsigned int hid = get_hash_id(uid);
	bool bret = false;
	char *tempurl = NULL;

	if (0xffffffff == uid)
		return false;
	if (hid >= MAX_HASH)
		return bret;
	tempurl = get_url_form_data(data, datalen);

	if (NULL == tempurl) {
		tempurl = kmalloc(datalen + 1, GFP_ATOMIC);
		if (NULL == tempurl)
			return bret;
		memcpy(tempurl, data, datalen);
		*(tempurl + datalen) = 0;
		pr_info("hwad:match_ad_url multi part\n");
	}
	spin_lock_bh(&g_adlock[hid]);
	list_for_each_entry(node, &g_adhead[hid], list) {
		if (node->uid == uid) {
			bret = match_uid_url(node, tempurl);
			break;
		}
	}
	spin_unlock_bh(&g_adlock[hid]);
	kfree(tempurl);
	return bret;
}

void output_ad_rule(void)
{
	int i = 0;
	struct ad_rule *node = NULL;
	struct ad_url *temp = NULL;

	pr_info("hwad:output_ad_rule start\n");
	for (i = 0; i < MAX_HASH; i++) {
		spin_lock_bh(&g_adlock[i]);
		list_for_each_entry(node, &g_adhead[i], list) {
			pr_info("hwad:output_ad_rule uid=%d\n", node->uid);
			list_for_each_entry(temp, &node->urllist.list, list) {
				pr_info("hwad:output_ad_rule=%s len=%d\n",
						temp->url, temp->len);
			}
			pr_info("\n");
		}
		spin_unlock_bh(&g_adlock[i]);
	}
	pr_info("hwad: output_ad_rule  end\n");
}

void init_ad(void)
{
	int i;

	g_curcount = 0;
	for (i = 0; i < MAX_HASH; i++) {
		INIT_LIST_HEAD(&g_adhead[i]);
		spin_lock_init(&g_adlock[i]);
	}
	for (i = 0; i < MAX_RECENT_DROP; i++) {
		g_droparr[i].url = NULL;
		g_droparr[i].len = 0;
	}
	spin_lock_init(&g_droplock);
}

void uninit_ad(void)
{
	int i;

	clear_ad_rule(1, "");
	for (i = 0; i < MAX_RECENT_DROP; i++) {
		kfree(g_droparr[i].url);
		g_droparr[i].url = NULL;
		g_droparr[i].len = 0;
	}
}
