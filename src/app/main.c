#include "FreeRTOS.h"
#include "task.h"
#include "xil_printf.h"

#include "app_context.h"
#include "app_dma_task.h"
#include "app_stats_task.h"
#include "app_tcp_task.h"

extern void vPortInstallFreeRTOSVectorTable(void);

static int app_start_tasks(void)
{
    if (app_dma_task_start() != pdPASS) {
        xil_printf("dma task create failed\r\n");
        return -1;
    }

    if (app_tcp_task_start() != pdPASS) {
        xil_printf("tcp task create failed\r\n");
        return -1;
    }

    if (app_stats_task_start() != pdPASS) {
        xil_printf("stats task create failed\r\n");
        return -1;
    }

    return 0;
}

int main(void)
{
    vPortInstallFreeRTOSVectorTable();

    if (app_context_init() != 0) {
        return -1;
    }

    if (app_start_tasks() != 0) {
        return -1;
    }

    vTaskStartScheduler();

    for (;;) {
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
