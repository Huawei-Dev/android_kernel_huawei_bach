#ifndef __HW_WIFI_H__
#define __HW_WIFI_H__

#define WIFI_WAKESRC_TAG "WIFI wake src: "
extern volatile bool g_wifi_firstwake;

extern void parse_packet(struct sk_buff *skb);

#endif
