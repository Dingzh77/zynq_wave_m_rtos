#include <stdio.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "xil_printf.h"
#include "xparameters.h"

#include "../bsp/bsp_adc.h"
#include "../bsp/bsp_ccd.h"
#include "../bsp/bsp_cfg_state.h"
#include "../drivers/pl/cycle_dma.h"
#include "../drivers/pl/pl_rst_n.h"
#include "../middleware/buff/ring_buff.h"
#include "../middleware/net/tcp_client.h"

#define DMA_TRANS_LENGTH        256U
#define DMA_FRAME_BYTES         (DMA_TRANS_LENGTH * sizeof(u64))
#define DMA_PINGPANG_BYTES      (2U * DMA_FRAME_BYTES)

#define DMA_TASK_STACK_WORDS    1024U
#define TCP_TASK_STACK_WORDS    2048U
#define STATS_TASK_STACK_WORDS  512U

#define DMA_TASK_PRIORITY       (tskIDLE_PRIORITY + 4)
#define TCP_TASK_PRIORITY       (tskIDLE_PRIORITY + 3)
#define STATS_TASK_PRIORITY     (tskIDLE_PRIORITY + 1)

static struct netif ps_netif;
static struct tcp_pcb *tcp_pcb_ptr = NULL;
static ring_buff_t g_tx_ring;
static SemaphoreHandle_t g_ring_mutex = NULL;
static TaskHandle_t g_dma_task_handle = NULL;

static u64 trans_buf[2U * DMA_TRANS_LENGTH] __attribute__((aligned(4096)));

static volatile u32 g_dma_done_cnt = 0;
static volatile u32 g_tcp_send_ok_cnt = 0;
static volatile u32 g_tcp_send_buf_err_cnt = 0;
static volatile u32 g_tcp_send_other_err_cnt = 0;

static lwip_tcp_client_cfg_t net_cfg = {
    .netif          = &ps_netif,
    .tcp_pcb        = &tcp_pcb_ptr,
    .local_mac      = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05},
    .local_ip       = "192.168.100.100",
    .local_netmask  = "255.255.255.0",
    .local_gateway  = "192.168.100.1",
    .remote_ip      = "192.168.100.99",
    .remote_port    = 8080,
    .emac_baseaddr  = XPAR_XEMACPS_0_BASEADDR
};

extern void vPortInstallFreeRTOSVectorTable(void);

static void dma_task(void *arg);
static void tcp_task(void *arg);
static void stats_task(void *arg);

int main(void)
{
    BaseType_t ok;

    vPortInstallFreeRTOSVectorTable();

    ring_buff_init(&g_tx_ring);
    g_ring_mutex = xSemaphoreCreateMutex();
    if (g_ring_mutex == NULL) {
        xil_printf("ring mutex create failed\r\n");
        return -1;
    }

    ok = xTaskCreate(dma_task,
                     "dma_task",
                     DMA_TASK_STACK_WORDS,
                     NULL,
                     DMA_TASK_PRIORITY,
                     &g_dma_task_handle);
    if (ok != pdPASS) {
        xil_printf("dma task create failed\r\n");
        return -1;
    }

    ok = xTaskCreate(tcp_task,
                     "tcp_task",
                     TCP_TASK_STACK_WORDS,
                     NULL,
                     TCP_TASK_PRIORITY,
                     NULL);
    if (ok != pdPASS) {
        xil_printf("tcp task create failed\r\n");
        return -1;
    }

    ok = xTaskCreate(stats_task,
                     "stats_task",
                     STATS_TASK_STACK_WORDS,
                     NULL,
                     STATS_TASK_PRIORITY,
                     NULL);
    if (ok != pdPASS) {
        xil_printf("stats task create failed\r\n");
        return -1;
    }

    vTaskStartScheduler();

    for (;;) {
    }
}

static void dma_task(void *arg)
{
    u32 trans_length;
    u32 trans_start_addr;

    (void)arg;

    cycle_dma_bind_buffer(trans_buf, DMA_TRANS_LENGTH);
    trans_start_addr = (u32)cycle_dma_get_ping_addr();

    pl_reset_n();
    bsp_ccd_init(&ccd_cfg_handle);
    bsp_adc_init(&adc_cfg_handle);

    cycle_dma_set_event_task(g_dma_task_handle);
    cycle_dma_int_init(NULL);

    xil_printf("dma task started, frame_bytes=%lu pingpang_bytes=%lu\r\n",
               (unsigned long)DMA_FRAME_BYTES,
               (unsigned long)DMA_PINGPANG_BYTES);

    for (;;) {
        u32 events = 0U;

        (void)xTaskNotifyWait(0U,
                              DMA_INT_READY | DMA_INT_DONE,
                              &events,
                              pdMS_TO_TICKS(100));

        if ((events & DMA_INT_READY) || (cycle_dma_int_flag & DMA_INT_READY)) {
            cycle_dma_int_flag &= (u8)(~DMA_INT_READY);

            trans_length = get_cycle_dma_trans_length();
            cycle_dma_start_trans(trans_start_addr, trans_length);
            cycle_dma_int_clear(DMA_INT_READY);
        }

        if ((events & DMA_INT_DONE) || (cycle_dma_int_flag & DMA_INT_DONE)) {
            cycle_dma_int_flag &= (u8)(~DMA_INT_DONE);
            cycle_dma_int_clear(DMA_INT_DONE);

            cycle_dma_invalidate_buf(g_done_buf_id);
            g_dma_done_cnt++;

            if (xSemaphoreTake(g_ring_mutex, portMAX_DELAY) == pdTRUE) {
                (void)ring_buff_push_payload(&g_tx_ring,
                                             cycle_dma_get_buf_ptr(g_done_buf_id),
                                             (u16)cycle_dma_get_frame_bytes());
                xSemaphoreGive(g_ring_mutex);
            }
        }
    }
}

static void tcp_task(void *arg)
{
    (void)arg;

    if (lwip_tcp_client_init(&net_cfg) != 0) {
        xil_printf("lwip/tcp init failed\r\n");
        vTaskDelete(NULL);
    }

    lwip_tcp_client_start_input_thread(net_cfg.netif);

    for (;;) {
        lwip_tcp_client_link(net_cfg.tcp_pcb,
                             net_cfg.remote_ip,
                             net_cfg.remote_port);

        if (xSemaphoreTake(g_ring_mutex, pdMS_TO_TICKS(2)) == pdTRUE) {
            ring_buff_view_t view;

            if (ring_buff_peek(&g_tx_ring, &view)) {
                err_t err = tcp_client_send_data(view.data, view.len);

                if (err == ERR_OK) {
                    g_tcp_send_ok_cnt++;
                    ring_buff_pop(&g_tx_ring);
                } else if (err == ERR_BUF) {
                    g_tcp_send_buf_err_cnt++;
                } else if (err != ERR_CONN) {
                    g_tcp_send_other_err_cnt++;
                    ring_buff_pop(&g_tx_ring);
                }
            }

            xSemaphoreGive(g_ring_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(2));
    }
}

static void stats_task(void *arg)
{
    (void)arg;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(2000));

        if (xSemaphoreTake(g_ring_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            xil_printf("dma_done=%lu, tx_ok=%lu, tx_buf_err=%lu, tx_other_err=%lu, ring_cnt=%lu, ring_drop=%lu\r\n",
                       (unsigned long)g_dma_done_cnt,
                       (unsigned long)g_tcp_send_ok_cnt,
                       (unsigned long)g_tcp_send_buf_err_cnt,
                       (unsigned long)g_tcp_send_other_err_cnt,
                       (unsigned long)ring_buff_count(&g_tx_ring),
                       (unsigned long)ring_buff_drop_count(&g_tx_ring));
            xSemaphoreGive(g_ring_mutex);
        }
    }
}

void vApplicationMallocFailedHook(void)
{
    xil_printf("malloc failed\r\n");
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void)task;
    xil_printf("stack overflow: %s\r\n", task_name);
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
