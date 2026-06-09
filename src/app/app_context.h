#ifndef APP_CONTEXT_H
#define APP_CONTEXT_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "xil_types.h"

#include "../middleware/buff/ring_buff.h"
#include "../middleware/net/tcp_client.h"
#include "app_config.h"

typedef struct {
    volatile u32 dma_done;
    volatile u32 tcp_send_ok;
    volatile u32 tcp_send_buf_err;
    volatile u32 tcp_send_other_err;
} app_stats_t;

typedef struct {
    struct netif netif;
    struct tcp_pcb *tcp_pcb;
    lwip_tcp_client_cfg_t net_cfg;
    ring_buff_t tx_ring;
    SemaphoreHandle_t ring_mutex;
    TaskHandle_t dma_task_handle;
    u64 dma_buf[2U * APP_DMA_TRANS_LENGTH] __attribute__((aligned(4096)));
    app_stats_t stats;
} app_context_t;

extern app_context_t g_app_ctx;

int app_context_init(void);

#endif
