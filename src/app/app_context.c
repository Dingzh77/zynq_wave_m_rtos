#include "app_context.h"

#include "xil_printf.h"
#include "xparameters.h"

#include "app_config.h"

app_context_t g_app_ctx;

int app_context_init(void)
{
    ring_buff_init(&g_app_ctx.tx_ring);

    g_app_ctx.ring_mutex = xSemaphoreCreateMutex();
    if (g_app_ctx.ring_mutex == NULL) {
        xil_printf("ring mutex create failed\r\n");
        return -1;
    }

    g_app_ctx.tcp_pcb = NULL;
    g_app_ctx.dma_task_handle = NULL;

    g_app_ctx.net_cfg.netif = &g_app_ctx.netif;
    g_app_ctx.net_cfg.tcp_pcb = &g_app_ctx.tcp_pcb;
    g_app_ctx.net_cfg.local_mac[0] = APP_LOCAL_MAC0;
    g_app_ctx.net_cfg.local_mac[1] = APP_LOCAL_MAC1;
    g_app_ctx.net_cfg.local_mac[2] = APP_LOCAL_MAC2;
    g_app_ctx.net_cfg.local_mac[3] = APP_LOCAL_MAC3;
    g_app_ctx.net_cfg.local_mac[4] = APP_LOCAL_MAC4;
    g_app_ctx.net_cfg.local_mac[5] = APP_LOCAL_MAC5;
    g_app_ctx.net_cfg.local_ip = APP_LOCAL_IP;
    g_app_ctx.net_cfg.local_netmask = APP_LOCAL_NETMASK;
    g_app_ctx.net_cfg.local_gateway = APP_LOCAL_GATEWAY;
    g_app_ctx.net_cfg.remote_ip = APP_REMOTE_IP;
    g_app_ctx.net_cfg.remote_port = APP_REMOTE_PORT;
    g_app_ctx.net_cfg.emac_baseaddr = XPAR_XEMACPS_0_BASEADDR;

    return 0;
}
