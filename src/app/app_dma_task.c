#include "app_dma_task.h"

#include "task.h"
#include "xil_printf.h"

#include "app_config.h"
#include "app_context.h"
#include "../bsp/bsp_adc.h"
#include "../bsp/bsp_ccd.h"
#include "../bsp/bsp_cfg_state.h"
#include "../drivers/pl/cycle_dma.h"
#include "../drivers/pl/pl_rst_n.h"

static void app_dma_task(void *arg);

BaseType_t app_dma_task_start(void)
{
    return xTaskCreate(app_dma_task,
                       "dma_task",
                       APP_DMA_TASK_STACK_WORDS,
                       NULL,
                       APP_DMA_TASK_PRIORITY,
                       &g_app_ctx.dma_task_handle);
}

static void app_dma_task(void *arg)
{
    u32 trans_length;
    u32 trans_start_addr;

    (void)arg;

    cycle_dma_bind_buffer(g_app_ctx.dma_buf, APP_DMA_TRANS_LENGTH);
    trans_start_addr = (u32)cycle_dma_get_ping_addr();

    pl_reset_n();
    bsp_ccd_init(&ccd_cfg_handle);
    bsp_adc_init(&adc_cfg_handle);

    cycle_dma_set_event_task(g_app_ctx.dma_task_handle);
    cycle_dma_int_init(NULL);

    xil_printf("dma task started, frame_bytes=%lu pingpang_bytes=%lu\r\n",
               (unsigned long)APP_DMA_FRAME_BYTES,
               (unsigned long)APP_DMA_PINGPANG_BYTES);

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
            g_app_ctx.stats.dma_done++;

            if (xSemaphoreTake(g_app_ctx.ring_mutex, portMAX_DELAY) == pdTRUE) {
                (void)ring_buff_push_payload(&g_app_ctx.tx_ring,
                                             cycle_dma_get_buf_ptr(g_done_buf_id),
                                             (u16)cycle_dma_get_frame_bytes());
                xSemaphoreGive(g_app_ctx.ring_mutex);
            }
        }
    }
}
