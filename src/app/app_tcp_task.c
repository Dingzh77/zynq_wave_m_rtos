#include "app_tcp_task.h"

#include "task.h"
#include "xil_printf.h"

#include "app_config.h"
#include "app_context.h"
#include "../middleware/net/tcp_client.h"

static void app_tcp_task(void *arg);

BaseType_t app_tcp_task_start(void)
{
    return xTaskCreate(app_tcp_task,
                       "tcp_task",
                       APP_TCP_TASK_STACK_WORDS,
                       NULL,
                       APP_TCP_TASK_PRIORITY,
                       NULL);
}

static void app_tcp_task(void *arg)
{
    (void)arg;

    if (lwip_tcp_client_init(&g_app_ctx.net_cfg) != 0) {
        xil_printf("lwip/tcp init failed\r\n");
        vTaskDelete(NULL);
    }

    lwip_tcp_client_start_input_thread(g_app_ctx.net_cfg.netif);

    for (;;) {
        lwip_tcp_client_link(g_app_ctx.net_cfg.tcp_pcb,
                             g_app_ctx.net_cfg.remote_ip,
                             g_app_ctx.net_cfg.remote_port);

        if (xSemaphoreTake(g_app_ctx.ring_mutex, pdMS_TO_TICKS(APP_TCP_POLL_INTERVAL_MS)) == pdTRUE) {
            ring_buff_view_t view;

            if (ring_buff_peek(&g_app_ctx.tx_ring, &view)) {
                err_t err = tcp_client_send_data(view.data, view.len);

                if (err == ERR_OK) {
                    g_app_ctx.stats.tcp_send_ok++;
                    ring_buff_pop(&g_app_ctx.tx_ring);
                } else if (err == ERR_BUF) {
                    g_app_ctx.stats.tcp_send_buf_err++;
                } else if (err != ERR_CONN) {
                    g_app_ctx.stats.tcp_send_other_err++;
                    ring_buff_pop(&g_app_ctx.tx_ring);
                }
            }

            xSemaphoreGive(g_app_ctx.ring_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(APP_TCP_POLL_INTERVAL_MS));
    }
}
