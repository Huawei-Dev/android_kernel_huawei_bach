#include <linux/module.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/ip.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <net/icmp.h>
#include <linux/ieee80211.h>
#include <linux/types.h>
#include <linux/skbuff.h>
#include <linux/ipv6.h>
#include <hw_wifi.h>

#define MAX_MSG_LENGTH 30
#define IPADDR(addr) \
        ((unsigned char*)&addr)[0], \
        ((unsigned char*)&addr)[1], \
        ((unsigned char*)&addr)[2], \
        ((unsigned char*)&addr)[3]

#define IPV6_ADDRESS_SIZEINBYTES 0x10
#define IPV6_ADDRESS_PER_UNIT 0x8
#define IPV6_DESTOPTS_HDR_OPTIONSIZE 0x8

struct ieee8021x_hdr {
	u8 version;
	u8 type;
	__be16 length;
};

typedef struct IPV6RoutingHeaderFormatTag
{
	uint8_t ucNextHeader;
	uint8_t ucRoutingType;
	uint8_t ucNumAddresses;
	uint8_t ucNextAddress;
	uint32_t ulReserved;
	//uint8_t aucAddressList[0];

}IPV6RoutingHeader;

typedef struct IPV6FragmentHeaderFormatTag
{
	uint8_t ucNextHeader;
	uint8_t ucReserved;
	uint16_t usFragmentOffset;
	uint32_t  ulIdentification;
}IPV6FragmentHeader;

typedef struct IPV6DestOptionsHeaderFormatTag
{
	uint8_t ucNextHeader;
	uint8_t ucHdrExtLen;
	uint8_t ucDestOptions[6];
	//uint8_t udExtDestOptions[0];
}IPV6DestOptionsHeader;

typedef struct IPV6HopByHopOptionsHeaderFormatTag
{
	uint8_t ucNextHeader;
	uint8_t ucMisc[3];
	uint32_t ulJumboPayloadLen;
}IPV6HopByHopOptionsHeader;

typedef struct IPV6AuthenticationHeaderFormatTag
{
	uint8_t ucNextHeader;
	uint8_t ucLength;
	uint16_t usReserved;
	uint32_t  ulSecurityParametersIndex;
	//uint8_t  ucAuthenticationData[0];

}IPV6AuthenticationHeader;
//#endif



/***************************************************************************
*Function: 	wlan_send_nl_event
*Description: 	send the port number to the userspace use uevent.
*Input: 		struct net_device *net_dev: dhd net device.
			u16 port: port number.
*Output: 	null
*Return:		null
***************************************************************************/
#if 0
static void wlan_send_nl_event(struct net_device *net_dev,  u16 port)
{
	struct device* dev = NULL;
	char *uevent[2];
	char msg[MAX_MSG_LENGTH];

	dev = &(net_dev->dev);
	memset(msg, 0, sizeof(msg));
	snprintf(msg, sizeof(msg), "WIFI_WAKE_PORT=%d",port);
	pr_err(WIFI_WAKESRC_TAG"%s: send msg: %s\n", __FUNCTION__, msg);
	uevent[0] = msg;
	uevent[1] = NULL;
	kobject_uevent_env(&(dev->kobj), KOBJ_CHANGE, (char**)&uevent);

	return;
}
#endif

/***************************************************************************
*Function: 	parse_ipv4_packet
*Description: 	if the packet is tcp/ip type, print ip type, ip address, ip port.
*Input: 		struct sk_buff *skb
*Output: 	null
*Return:		null
***************************************************************************/
static void parse_ipv4_packet(struct sk_buff *skb)
{
	const struct iphdr *iph;
	int iphdr_len = 0;
	struct tcphdr *th;
	struct udphdr *uh;
	struct icmphdr *icmph;

	iph = (struct iphdr *)skb->data;
	if(NULL == iph)
	    return;
		
	iphdr_len = iph->ihl*4;

	pr_err(WIFI_WAKESRC_TAG"receive ipv4 packet. src ip:%d.%d.%d.%d, dst ip:%d.%d.%d.%d\n", IPADDR(iph->saddr), IPADDR(iph->daddr));
	
	switch(iph->protocol){
	case IPPROTO_UDP:
	    {
	        uh = (struct udphdr *)(skb->data + iphdr_len);
	        pr_err(WIFI_WAKESRC_TAG"receive UDP packet, src port:%d, dst port:%d.\n", ntohs(uh->source), ntohs(uh->dest));
	        //wlan_send_nl_event(skb->dev, ntohs(uh->dest));
	        break;
	    }
	case IPPROTO_TCP:
	    {
	        th = (struct tcphdr *)(skb->data + iphdr_len);
	        pr_err(WIFI_WAKESRC_TAG"receive TCP packet, src port:%d, dst port:%d.\n", ntohs(th->source), ntohs(th->dest));
	        //wlan_send_nl_event(skb->dev, ntohs(th->dest));
	        break;
	    }
	case IPPROTO_ICMP:
	    {
	        icmph = (struct icmphdr *)(skb->data + iphdr_len);
	        pr_err(WIFI_WAKESRC_TAG"receive ICMP packet, type(%d):%s, code:%d.\n", icmph->type,
			    ((icmph->type == 0)?"ping reply":((icmph->type == 8)?"ping request":"other icmp pkt")), icmph->code);
	        break;
	    }
	case IPPROTO_IGMP:
	    {
	        pr_err(WIFI_WAKESRC_TAG"receive IGMP packet.\n");
	        break;
	    }
	default:
	    {
	        pr_err(WIFI_WAKESRC_TAG"receive other IPv4 packet.\n");
            break;
		}
	}
	return;
}

void dump_ipv6_addr(unsigned short *addr)
{
	int i =0;

	for (i = 0; i < IPV6_ADDRESS_PER_UNIT; i++) {
		pr_err(WIFI_WAKESRC_TAG ":%x", ntohs(addr[i]));
	}
	pr_err("\n");
}

static uint8_t *get_next_ipv6_chain_header(uint8_t **headerscan, uint8_t *headtype, int8_t *done, uint16_t *payload_len)
{
	uint16_t next_header_offset = 0;
	uint8_t * payload_ptr = *headerscan;
	uint8_t * return_header_ptr = *headerscan;

	if(headerscan == NULL || (*payload_len == 0) || (*done)){
		return NULL;
	}
	*done = 0;

	switch(*headtype){
	case NEXTHDR_HOP:
		{
			pr_err(WIFI_WAKESRC_TAG"IPv6 HopByHop Header.\n");
			next_header_offset += sizeof(IPV6HopByHopOptionsHeader);
		}
		break;
	case NEXTHDR_ROUTING:
		{
			IPV6RoutingHeader *pstIpv6RoutingHeader = (IPV6RoutingHeader *)payload_ptr;
			pr_err(WIFI_WAKESRC_TAG"IPv6 Routing Header\n");
			next_header_offset += sizeof(IPV6RoutingHeader);
			next_header_offset += pstIpv6RoutingHeader->ucNumAddresses * IPV6_ADDRESS_SIZEINBYTES;
		}
		break;
	case NEXTHDR_FRAGMENT:
		{
			pr_err(WIFI_WAKESRC_TAG"IPv6 Fragmentation Header\n");
			next_header_offset += sizeof(IPV6FragmentHeader);
		}
		break;
	case NEXTHDR_DEST:
		{
			IPV6DestOptionsHeader *pstIpv6DestOptsHdr = (IPV6DestOptionsHeader *)payload_ptr;
			int nTotalOptions = pstIpv6DestOptsHdr->ucHdrExtLen;
			pr_err(WIFI_WAKESRC_TAG"IPv6 DestOpts Header Header\n");
			next_header_offset += sizeof(IPV6DestOptionsHeader);
			next_header_offset += nTotalOptions * IPV6_DESTOPTS_HDR_OPTIONSIZE ;
		}
		break;
	case NEXTHDR_AUTH:
		{
			IPV6AuthenticationHeader *pstIpv6AuthHdr = (IPV6AuthenticationHeader *)payload_ptr;
			int nHdrLen = pstIpv6AuthHdr->ucLength;
			pr_err(WIFI_WAKESRC_TAG"IPv6 Authentication Header\n");
			next_header_offset += nHdrLen * 4;
		}
		break;
	case NEXTHDR_TCP:
	case NEXTHDR_UDP:
		{
			pr_err(WIFI_WAKESRC_TAG"tcp/udp Header: %d.\n", *headtype);
			*done = 1;
		}
		break;
	case NEXTHDR_ICMP:
		{
			pr_err(WIFI_WAKESRC_TAG"icmp Header: %d.\n", *headtype);
			*done = 1;
		}
		break;
	default:
		*done = 1;
		break;
	}

	if (next_header_offset == 0) {
		pr_err(WIFI_WAKESRC_TAG"Receive wrong package, do not parse continuously\n");
		*done = 1;
	}
	if (*done == 0) {
		if (*payload_len <= next_header_offset) {
			*done = true;
		} else {
			*headtype = *payload_ptr;
			payload_ptr += next_header_offset;
			(*payload_len) -= next_header_offset;
		}
	}

	*headerscan = payload_ptr;
	return return_header_ptr;
}

static void get_ipv6_protocal_ports(uint8_t *payload, uint16_t payload_len, uint8_t next_header, uint16_t *src_port, uint16_t *des_port)
{
	int8_t done = 0;
	uint8_t *headerscan = payload;
	uint8_t *payload_header = NULL;
	uint8_t headtype;

	if(!payload || payload_len == 0)
		return;

	headtype = next_header;
	while(!done){
		payload_header = get_next_ipv6_chain_header(&headerscan, &headtype, &done, &payload_len);
		if(done){
			if((payload_header != NULL) &&
				(headtype == NEXTHDR_TCP || headtype == NEXTHDR_UDP)){
				*src_port = *((uint16_t *)payload_header);
				*des_port = *((uint16_t *)(payload_header+2));
				pr_err(WIFI_WAKESRC_TAG"src_port:0x%x, des_port:0x%x.\n", ntohs(*src_port), ntohs(*des_port));
			}
			break;
		}
	}
}

static void parse_ipv6_packet(struct sk_buff *skb)
{
	struct ipv6hdr *nh;
	uint16_t src_port;
	uint16_t des_port;
	uint8_t *payload;

	/*
	*This code may cause crash, so it should be closed  temporarily
	*payload = nh + sizeof(struct ipv6hdr);
	*pointer payload may illegal
	*/
	return;
	nh = (struct ipv6hdr *)skb->data;
	pr_err(WIFI_WAKESRC_TAG"version: %d, payload length: %d, nh->nexthdr: %d. \n", nh->version, ntohs(nh->payload_len), nh->nexthdr);
	pr_err(WIFI_WAKESRC_TAG"ipv6 src addr: ");
	dump_ipv6_addr((unsigned short *)&(nh->saddr));
	pr_err(WIFI_WAKESRC_TAG"ipv6 dst addr: ");
	dump_ipv6_addr((unsigned short *)&(nh->daddr));
	payload = (uint8_t *)((uint8_t *)nh + sizeof(struct ipv6hdr));

	get_ipv6_protocal_ports(payload, nh->payload_len, nh->nexthdr, &src_port, &des_port);

	return;
}

/***************************************************************************
*Function: 	parse_arp_packet
*Description: 	if the packet if 802.11 type, print the type name and sub type name.
*Input: 		struct sk_buff *skb
*Output: 	null
*Return:		null
***************************************************************************/
static void parse_arp_packet(struct sk_buff *skb)
{
	const struct iphdr *iph;
	int iphdr_len = 0;
	struct arphdr *arp;

	iph = (struct iphdr *)skb->data;
	iphdr_len = iph->ihl*4;
	arp = (struct arphdr *)(skb->data + iphdr_len);
	pr_err(WIFI_WAKESRC_TAG"receive ARP packet, hardware type:%d, protocol type:%d, opcode:%d.\n", ntohs(arp->ar_hrd), ntohs(arp->ar_pro), ntohs(arp->ar_op));

	return;
}

/***************************************************************************
*Function: 	parse_8021x_packet
*Description:	if the packet if 802.1x type, print the type name and sub type name.
*Input:		struct sk_buff *skb
*Output: 	null
*Return:		null
***************************************************************************/
static void parse_8021x_packet(struct sk_buff *skb)
{
	struct ieee8021x_hdr *hdr = (struct ieee8021x_hdr *)(skb->data);

	pr_err(WIFI_WAKESRC_TAG"receive 802.1x frame: version:%d, type:%d, length:%d\n", hdr->version, hdr->type, ntohs(hdr->length));

	return;
}


/***************************************************************************
*Function: 	parse_packet
*Description: 	parse the packet from TL when system waked up by wifi. identify the packet type.
*Input: 		struct sk_buff *skb
*Output: 	null
*Return:		null
***************************************************************************/
void parse_packet(struct sk_buff *skb)
{
	__u16 type;

	type = be16_to_cpu(skb->protocol);
	
	switch(type){
	case ETH_P_IP:
	    {
		     parse_ipv4_packet(skb);
			 break;
		}	 
	case ETH_P_IPV6:
	    {
		    pr_err(WIFI_WAKESRC_TAG"receive ipv6 packet.\n");
	        parse_ipv6_packet(skb);
			break;
		}
	case ETH_P_ARP:
	    {
		    parse_arp_packet(skb);
			break;
		}
	case ETH_P_PAE:
        {
		    parse_8021x_packet(skb); 
			break;
		}
	default:
	    {
		    pr_err(WIFI_WAKESRC_TAG"receive other packet.\n");
	        break;
		}
	}

	return;
}



