#include "app_stats_task.h"

#include "task.h"
#include "xil_printf.h"

#include "app_config.h"
#include "app_context.h"

static void app_stats_task(void *arg);

BaseType_t app_stats_task_start(void)
{
    return xTaskCreate(app_stats_task,
                       "stats_task",
                       APP_STATS_TASK_STACK_WORDS,
                       NULL,
                       APP_STATS_TASK_PRIORITY,
                       NULL);
}

static void app_stats_task(void *arg)
{
    (void)arg;

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(APP_STATS_INTERVAL_MS));

        if (xSemaphoreTake(g_app_ctx.ring_mutex, pdMS_TO_TICKS(20)) == pdTRUE) {
            xil_printf("dma_done=%lu, tx_ok=%lu, tx_buf_err=%lu, tx_other_err=%lu, ring_cnt=%lu, ring_drop=%lu\r\n",
                       (unsigned long)g_app_ctx.stats.dma_done,
                       (unsigned long)g_app_ctx.stats.tcp_send_ok,
                       (unsigned long)g_app_ctx.stats.tcp_send_buf_err,
                       (unsigned long)g_app_ctx.stats.tcp_send_other_err,
                       (unsigned long)ring_buff_count(&g_app_ctx.tx_ring),
                       (unsigned long)ring_buff_drop_count(&g_app_ctx.tx_ring));
            xSemaphoreGive(g_app_ctx.ring_mutex);
        }
    }
}
