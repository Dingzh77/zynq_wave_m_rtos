#include "tcp_client.h"

#include "FreeRTOS.h"
#include "task.h"
#include "lwip/sys.h"
#include "lwip/tcpip.h"
#include "xil_printf.h"

#include "../protocol/tcp_cfg_proto.h"

#define TCP_RETRY_INTERVAL_MS       1000U
#define TCP_CONNECT_TIMEOUT_MS      5000U
#define TCP_INPUT_THREAD_STACK      1024U
#define TCP_INPUT_THREAD_PRIORITY   (tskIDLE_PRIORITY + 2)

struct tcp_pcb *client_pcb = NULL;
volatile u8 g_client_connected = 0;

static volatile int s_tcpip_ready = 0;

static void tcpip_init_done_cb(void *arg)
{
    (void)arg;
    s_tcpip_ready = 1;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcp_client_err(void *arg, err_t err);

static void tcp_client_err(void *arg, err_t err)
{
    (void)arg;

    g_client_connected = 0;
    client_pcb = NULL;

    xil_printf("TCP error callback: %d\r\n", err);
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    (void)arg;

    if (err != ERR_OK) {
        g_client_connected = 0;
        client_pcb = NULL;

        if (tpcb != NULL) {
            tcp_abort(tpcb);
        }

        xil_printf("Connection error: %d\r\n", err);
        return err;
    }

    client_pcb = tpcb;
    g_client_connected = 1;

    tcp_arg(tpcb, NULL);
    tcp_recv(tpcb, tcp_client_recv_data);
    tcp_err(tpcb, tcp_client_err);

    xil_printf("Client connect success\r\n");
    return ERR_OK;
}

err_t tcp_client_recv_data(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    struct pbuf *q;

    (void)arg;

    if (err != ERR_OK) {
        if (p != NULL) {
            pbuf_free(p);
        }

        g_client_connected = 0;
        client_pcb = NULL;

        if (tpcb != NULL) {
            tcp_recv(tpcb, NULL);
            tcp_abort(tpcb);
        }

        xil_printf("TCP receive error: %d\r\n", err);
        return err;
    }

    if (p == NULL) {
        g_client_connected = 0;
        client_pcb = NULL;

        xil_printf("TCP peer closed connection\r\n");

        if (tpcb != NULL) {
            tcp_recv(tpcb, NULL);
            tcp_close(tpcb);
        }

        return ERR_OK;
    }

    for (q = p; q != NULL; q = q->next) {
        tcp_cfg_feed_bytes((const u8 *)q->payload, q->len);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

err_t tcp_client_send_data(const void *sendBuf, unsigned int sendLen)
{
    err_t err;

    if ((sendBuf == NULL) || (sendLen == 0U)) {
        return ERR_ARG;
    }

    LOCK_TCPIP_CORE();

    if ((client_pcb == NULL) || !g_client_connected) {
        UNLOCK_TCPIP_CORE();
        return ERR_CONN;
    }

    if (sendLen > tcp_sndbuf(client_pcb)) {
        UNLOCK_TCPIP_CORE();
        return ERR_BUF;
    }

    err = tcp_write(client_pcb, sendBuf, sendLen, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        err = tcp_output(client_pcb);
    }

    UNLOCK_TCPIP_CORE();
    return err;
}

err_t tcp_client_init(struct tcp_pcb **pcb, const char *RemoteIpStr, u16 port)
{
    err_t err;
    ip_addr_t remote_ip;

    if ((pcb == NULL) || (RemoteIpStr == NULL)) {
        return ERR_ARG;
    }

    LOCK_TCPIP_CORE();

    if (*pcb != NULL) {
        tcp_abort(*pcb);
        *pcb = NULL;
    }

    if (!ipaddr_aton(RemoteIpStr, &remote_ip)) {
        xil_printf("Invalid remote IP: %s\r\n", RemoteIpStr);
        UNLOCK_TCPIP_CORE();
        return ERR_ARG;
    }

    *pcb = tcp_new();
    if (*pcb == NULL) {
        xil_printf("tcp_new() failed\r\n");
        UNLOCK_TCPIP_CORE();
        return ERR_MEM;
    }

    g_client_connected = 0;
    client_pcb = *pcb;

    tcp_arg(*pcb, NULL);
    tcp_err(*pcb, tcp_client_err);

    err = tcp_connect(*pcb, &remote_ip, port, tcp_client_connected);
    if (err != ERR_OK) {
        xil_printf("Error on tcp_connect: %d\r\n", err);
        tcp_abort(*pcb);
        *pcb = NULL;
        client_pcb = NULL;
    }

    UNLOCK_TCPIP_CORE();
    return err;
}

int lwip_tcp_client_init(lwip_tcp_client_cfg_t *cfg)
{
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;

    if ((cfg == NULL) || (cfg->netif == NULL) || (cfg->tcp_pcb == NULL)) {
        xil_printf("lwip_tcp_client_init: invalid param\r\n");
        return -1;
    }

    s_tcpip_ready = 0;
    tcpip_init(tcpip_init_done_cb, NULL);
    while (!s_tcpip_ready) {
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    if (!ipaddr_aton(cfg->local_ip, &ipaddr)) {
        xil_printf("invalid local ip\r\n");
        return -2;
    }

    if (!ipaddr_aton(cfg->local_netmask, &netmask)) {
        xil_printf("invalid netmask\r\n");
        return -3;
    }

    if (!ipaddr_aton(cfg->local_gateway, &gw)) {
        xil_printf("invalid gateway\r\n");
        return -4;
    }

    LOCK_TCPIP_CORE();

    if (xemac_add(cfg->netif,
                  &ipaddr,
                  &netmask,
                  &gw,
                  cfg->local_mac,
                  cfg->emac_baseaddr) == NULL) {
        UNLOCK_TCPIP_CORE();
        xil_printf("xemac_add failed\r\n");
        return -5;
    }

    netif_set_default(cfg->netif);
    netif_set_up(cfg->netif);

    UNLOCK_TCPIP_CORE();

    *(cfg->tcp_pcb) = NULL;
    g_client_connected = 0;
    client_pcb = NULL;

    xil_printf("lwip tcp client init ok\r\n");
    xil_printf("local ip : %s\r\n", cfg->local_ip);
    xil_printf("server   : %s:%u\r\n", cfg->remote_ip, (unsigned int)cfg->remote_port);

    return 0;
}

void lwip_tcp_client_start_input_thread(struct netif *netif)
{
    sys_thread_new("xemacif_input",
                   (void (*)(void *))xemacif_input_thread,
                   netif,
                   TCP_INPUT_THREAD_STACK,
                   TCP_INPUT_THREAD_PRIORITY);
}

void lwip_tcp_client_poll(struct netif *netif)
{
    if (netif != NULL) {
        (void)xemacif_input(netif);
    }
}

void lwip_tcp_client_link(struct tcp_pcb **pcb,
                          const char *remote_ip,
                          u16 remote_port)
{
    static u8 last_client_connected = 0U;
    static u8 need_send_cfg_state = 0U;
    static u16 cfg_state_seq = 1U;
    static TickType_t last_retry_tick = 0U;
    static TickType_t connect_start_tick = 0U;
    TickType_t now = xTaskGetTickCount();

    if (g_client_connected) {
        last_retry_tick = now;
        connect_start_tick = now;

        if (!last_client_connected) {
            need_send_cfg_state = 1U;
            last_client_connected = 1U;
        }

        if (need_send_cfg_state) {
            err_t err = tcp_cfg_send_state_report(cfg_state_seq);

            if (err == ERR_OK) {
                need_send_cfg_state = 0U;
                cfg_state_seq++;
                xil_printf("CFG state report sent\r\n");
            }
        }

        return;
    }

    last_client_connected = 0U;

    if ((pcb != NULL) && (*pcb != NULL) &&
        ((now - connect_start_tick) >= pdMS_TO_TICKS(TCP_CONNECT_TIMEOUT_MS))) {
        LOCK_TCPIP_CORE();
        tcp_abort(*pcb);
        UNLOCK_TCPIP_CORE();
        *pcb = NULL;
        client_pcb = NULL;
        xil_printf("TCP connection failed\r\n");
    }

    if ((pcb != NULL) && (*pcb == NULL) &&
        ((now - last_retry_tick) >= pdMS_TO_TICKS(TCP_RETRY_INTERVAL_MS))) {
        last_retry_tick = now;
        connect_start_tick = now;

        if (tcp_client_init(pcb, remote_ip, remote_port) == ERR_OK) {
            xil_printf("TCP connection request has been sent\r\n");
        } else {
            xil_printf("TCP connection initiation failed\r\n");
            *pcb = NULL;
            client_pcb = NULL;
        }
    }
}
