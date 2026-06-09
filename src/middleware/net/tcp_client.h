#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "lwip/tcp.h"
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#include "lwip/init.h"
#include "lwip/ip.h"
#include "lwip/dhcp.h"
#include "lwip/priv/tcp_priv.h"
#include "lwip/tcpip.h"
#include "netif/xadapter.h"

typedef struct {
    struct netif *netif;
    struct tcp_pcb **tcp_pcb;

    u8_t  local_mac[6];
    const char *local_ip;
    const char *local_netmask;
    const char *local_gateway;

    const char *remote_ip;
    u16_t remote_port;

    u32_t emac_baseaddr;
} lwip_tcp_client_cfg_t;

extern volatile u8_t g_client_connected;
extern struct tcp_pcb *client_pcb;

#define u16 u16_t
#define u8  u8_t

/* =========================================================
 * Function Prototypes
 * ========================================================= */

err_t tcp_client_init(struct tcp_pcb **pcb, const char *RemoteIpStr, u16 port);
err_t tcp_client_recv_data(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
err_t tcp_client_send_data(const void *sendBuf, unsigned int sendLen);

int lwip_tcp_client_init(lwip_tcp_client_cfg_t *cfg);
void lwip_tcp_client_poll(struct netif *netif);
void lwip_tcp_client_start_input_thread(struct netif *netif);
void lwip_tcp_client_link(struct tcp_pcb **pcb,
                          const char *remote_ip,
                          u16 remote_port);

#endif
