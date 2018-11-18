

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/kernel.h>/* add for log */
#include <linux/ctype.h>/* add for tolower */

#include <linux/spinlock.h>/* add for spinlock */
#include <linux/netlink.h>/* add for thread */
#include <uapi/linux/netlink.h>/* add for netlink */
#include <linux/kthread.h>/* add for thread */

#include "nf_hw_common.h"
#include "nf_hw_hook.h"
#include "nf_app_dl_monitor.h"
#include "nf_ad_filter.h"

#define REPORTCMD NETLINK_DOWNLOADEVENT_TO_USER

static spinlock_t	g_netlink_lock;/* lock for netlink array*/
static u64 g_lastrepot = 0;

struct tag_hw_msg2knl {
	struct nlmsghdr hdr;
	int opt;
	char data[1];
};

struct appdload_nl_packet_msg {
	   int event;
	   char url[1];
};

static uid_t find_skb_uid(struct sk_buff *skb)
{
	const struct file *filp = NULL;
	struct sock *sk = NULL;
	uid_t sock_uid = 0xffffffff;

	if (!skb)
		return sock_uid;
	sk = skb->sk;
	if (!sk)
		return 0xffffffff;
	if (sk && sk->sk_state == TCP_TIME_WAIT)
		return sock_uid;
	filp = sk->sk_socket ? sk->sk_socket->file : NULL;
	if (filp)
		sock_uid = from_kuid(&init_user_ns, filp->f_cred->fsuid);
	else
		return 0xffffffff;
	return sock_uid;
}

static struct sock *g_hw_nlfd;
static unsigned int g_uspa_pid;

void proc_cmd(int cmd, int opt, const char *data)
{
	if (NETLINK_SET_AD_RULE_TO_KERNEL == cmd)
		add_ad_rule(data, opt == 0 ? false : true);
	else if (NETLINK_CLR_AD_RULE_TO_KERNEL == cmd)
		clear_ad_rule(opt, data);
	else if (NETLINK_OUTPUT_AD_TO_KERNEL == cmd)
		output_ad_rule();
	else if (NETLINK_SET_APPDL_RULE_TO_KERNEL == cmd)
		add_appdl_rule(data, opt == 0 ? false : true);
	else if (NETLINK_CLR_APPDL_RULE_TO_KERNEL == cmd)
		clear_appdl_rule(opt, data);
	else if (NETLINK_OUTPUT_APPDL_TO_KERNEL == cmd)
		output_appdl_rule();
	else if (NETLINK_APPDL_CALLBACK_TO_KERNEL == cmd)
		download_notify(opt, data);
	else
		pr_info("hwad:kernel_hw_receive cmd=%d\n", cmd);
}

/* receive cmd for netd */
static void kernel_hw_receive(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh;
	struct tag_hw_msg2knl *hmsg = NULL;
	struct sk_buff *skb;
	char *data;

	skb = skb_get(__skb);
	if (skb->len >= NLMSG_HDRLEN) {
		nlh = nlmsg_hdr(skb);
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) &&
				(skb->len >= nlh->nlmsg_len)) {
			if (NETLINK_REG_TO_KERNEL == nlh->nlmsg_type)
				g_uspa_pid = nlh->nlmsg_pid;
			else if (NETLINK_UNREG_TO_KERNEL == nlh->nlmsg_type)
				g_uspa_pid = 0;
			else {
				hmsg = (struct tag_hw_msg2knl *)nlh;
				data = (char *)&(hmsg->data[0]);
				proc_cmd(nlh->nlmsg_type, hmsg->opt, data);
			}
		}
	}
}

/* notify event to netd  */
static int notify_event(int event, int pid, char *url)
{
	int ret;
	int size;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct appdload_nl_packet_msg *packet = NULL;
	int urllen = strlen(url);

	if (!pid || !g_hw_nlfd || 0 == g_uspa_pid) {
		pr_info("hwad:cannot notify pid 0 or nlfd 0\n");
		ret = -1;
		goto end;
	}
	size = sizeof(struct appdload_nl_packet_msg) + urllen + 1;
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb) {
		pr_info("hwad: alloc skb fail\n");
		ret = -1;
		goto end;
	}
	nlh = nlmsg_put(skb, 0, 0, 0, size, 0);
	if (!nlh) {
		pr_info("hwad: notify_event fail\n");
		kfree_skb(skb);
		skb = NULL;
		ret = -1;
		goto end;
	}
	packet = nlmsg_data(nlh);
	memset(packet, 0, size);
	memcpy(packet->url, url, urllen);
	packet->event = event;
	spin_lock_bh(&g_netlink_lock);
	ret = netlink_unicast(g_hw_nlfd, skb, pid, MSG_DONTWAIT);
	spin_unlock_bh(&g_netlink_lock);
end:
	return ret;
}

static void netlink_init(void)
{
	struct netlink_kernel_cfg hwcfg = {
		.input = kernel_hw_receive,
	};
	g_hw_nlfd = netlink_kernel_create(&init_net, NETLINK_HW_NF, &hwcfg);
	if (!g_hw_nlfd)
		pr_info("hwad: netlink_init failed NETLINK_HW_NF\n");
}

static inline void send_reject_error(unsigned int uid, struct sk_buff *skb)
{
	if (skb->sk) {
		skb->sk->sk_err = ECONNRESET;
		skb->sk->sk_error_report(skb->sk);
		pr_info("hwad:reject: uid=%d sk_err=%d for skb=%p sk=%p\n",
			uid, skb->sk->sk_err, skb, skb->sk);
	}
}

unsigned int download_app_pro(struct sk_buff *skb, unsigned int uid,
			      const char *data, int dlen, char *ip)
{
	char *tempurl = get_url_path(data, dlen);/* new 501 */
	int lact = 0;
	struct dl_info *node;
	char *buf;
	int iret = 0;
	char *url;
	int opt;

	if (!tempurl)
		return NF_ACCEPT;
	opt = get_select(skb);
	if (opt != DLST_NOT) {
		kfree(tempurl);
		if (opt == DLST_REJECT)
			return NF_DROP;
		else if (opt == DLST_ALLOW)
			return NF_ACCEPT;
		else if (opt == DLST_WAIT)
			return 0xffffffff;
	}
	node = get_download_monitor(skb, uid, tempurl);
	kfree(tempurl);
	if (!node) {
		pr_info("hwad:get_download_monitor=NULL\n ");
		return NF_ACCEPT;
	}
	if (node->bwait)
		return 0xffffffff;
	url = get_url_form_data(data, dlen);
	if (!url) {
		free_node(node);
		return NF_ACCEPT;
	}
	buf = get_report_msg(node->dlid, uid, url, ip);
	kfree(url);
	if (!buf) {
		free_node(node);
		return NF_ACCEPT;
	}
	iret = notify_event(REPORTCMD, g_uspa_pid, buf);
	kfree(buf);/* free 801 */
	if (iret < 0) {
		free_node(node);
		return NF_ACCEPT;
	}
	if (!in_irq() && !in_interrupt()) {
		opt = wait_user_event(node);
	} else {
		pr_info("hwad:in_irq=%d in_interrupt=%d",
			(int)in_irq(), (int)in_interrupt());
	}
	if (opt != DLST_NOT) {
		if (opt == DLST_ALLOW)
			return NF_ACCEPT;
		return NF_DROP;
	}
	return 0xffffffff;
}

unsigned int hw_match_httppack(struct sk_buff *skb, unsigned int uid,
			       const char *data, int dlen, char *ip)
{
	unsigned int iret = NF_ACCEPT;
	bool bmatch = false;
	char *tempurl = NULL;

	if (match_appdl_uid(uid)) {
		if (match_appdl_url(data, dlen)) {
			bmatch = true;
			iret = download_app_pro(skb, uid, data, dlen, ip);
		}
	}
	if (!bmatch && match_ad_uid(uid)) {
		if (match_ad_url(uid, data, dlen)) {
			char *buf = NULL;
			int ret;

			/* limit the report time as 1 second*/
			if (get_cur_time() - g_lastrepot > 1000) {
				g_lastrepot = get_cur_time();
				/* report the ad block to systemmanager */
				tempurl = get_url_form_data(data, dlen);
				if (!tempurl) {
					pr_info("hwad:notify_event ad tempurl=NULL\n");
					return iret;
				}
				buf = get_report_msg(100, uid, tempurl, ip);
				if (buf) {
					ret = notify_event(REPORTCMD, g_uspa_pid, buf);
					if (iret < 0)
						pr_info("hwad:notify ret=%d\n", ret);
					kfree(buf);
				} else {
					pr_info("hwad:report to systemmanager false\n");
				}
				kfree(tempurl);
			}
			iret = NF_DROP;
		}
	}
	if (iret == NF_DROP || uid == 0xffffffff) {
		tempurl = get_url_path(data, dlen);
		if (!tempurl)
			return iret;
		if (is_droplist(tempurl, strlen(tempurl))) {
			iret = NF_DROP;
		} else {
			if (tempurl)
				add_to_droplist(tempurl, strlen(tempurl));
		}
		kfree(tempurl);
	}
	return iret;
}
char g_get_s[] = {'G', 'E', 'T', 0};
char g_post_s[] = {'P', 'O', 'S', 'T', 0};
char g_http_s[] = {'H', 'T', 'T', 'P', 0};

static unsigned int net_hw_hook_localout(const struct nf_hook_ops *ops,
					 struct sk_buff *skb,
					 const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcp = NULL;
	char *pstart = NULL;
	unsigned int iret = NF_ACCEPT;
	int dlen = 0;
	char *data;

	tcp = tcp_hdr(skb);
	iph = ip_hdr(skb);
	if (!iph || !tcp) {
		pr_info("\nhwad:net_hw_hook_localout drop NULL==iph\n");
		return NF_ACCEPT;
	}
	pstart = skb->data;
	if (pstart) {
		struct tcphdr *th;

		th = (struct tcphdr *)((__u32 *)iph + iph->ihl);
		data = (char *)((__u32 *)th + th->doff);
		dlen = skb->len - (data - (char *)iph);
		if (dlen < 5)
			return NF_ACCEPT;
		if (strfindpos(data, g_get_s, 5) ||
				strfindpos(data, g_post_s, 5)) {
			unsigned int uid = 0;
			char ip[32];

			sprintf(ip, "%d.%d.%d.%d:%d",
				(iph->daddr&0x000000FF)>>0,
				(iph->daddr&0x0000FF00)>>8,
				(iph->daddr&0x00FF0000)>>16,
				(iph->daddr&0xFF000000)>>24,
				htons(tcp->dest));
			uid = find_skb_uid(skb);
			iret = hw_match_httppack(skb, uid, data, dlen, ip);
			if (NF_DROP == iret) {
				send_reject_error(uid, skb);
			}
		}
	}
	if (iret == 0xffffffff)
		iret = NF_DROP;
	return iret;
}

static struct nf_hook_ops net_hooks[] = {
	{
		.hook		= net_hw_hook_localout,
		.owner		= THIS_MODULE,
		.pf		= PF_INET,
		.hooknum	= NF_INET_LOCAL_OUT,
		.priority	= NF_IP_PRI_FILTER - 1,
	},
};

static int __init nf_init(void)
{
	int ret = 0;

	init_appdl();
	init_ad();
	ret = nf_register_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	if (ret) {
		pr_info("hwad:nf_init ret=%d  ", ret);
		return -1;
		}
	netlink_init();
	spin_lock_init(&g_netlink_lock);
	pr_info("hwad:nf_init success\n");
	return 0;
}

static void __exit nf_exit(void)
{
	uninit_ad();
	uninit_appdl();
	nf_unregister_hooks(net_hooks, ARRAY_SIZE(net_hooks));
}

module_init(nf_init);
module_exit(nf_exit);

MODULE_LICENSE("Dual BSD");
MODULE_AUTHOR("c00179874");
MODULE_DESCRIPTION("HW Netfilter NF_HOOK");
MODULE_VERSION("1.0.1");
MODULE_ALIAS("HW Netfilter 01");
