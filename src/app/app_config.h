#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "FreeRTOS.h"
#include "task.h"
#include "xil_types.h"

#define APP_DMA_TRANS_LENGTH          256U
#define APP_DMA_FRAME_BYTES           (APP_DMA_TRANS_LENGTH * sizeof(u64))
#define APP_DMA_PINGPANG_BYTES        (2U * APP_DMA_FRAME_BYTES)

#define APP_DMA_TASK_STACK_WORDS      1024U
#define APP_TCP_TASK_STACK_WORDS      2048U
#define APP_STATS_TASK_STACK_WORDS    512U

#define APP_DMA_TASK_PRIORITY         (tskIDLE_PRIORITY + 4)
#define APP_TCP_TASK_PRIORITY         (tskIDLE_PRIORITY + 3)
#define APP_STATS_TASK_PRIORITY       (tskIDLE_PRIORITY + 1)

#define APP_TCP_POLL_INTERVAL_MS      2U
#define APP_STATS_INTERVAL_MS         2000U

#define APP_LOCAL_MAC0                0x00
#define APP_LOCAL_MAC1                0x01
#define APP_LOCAL_MAC2                0x02
#define APP_LOCAL_MAC3                0x03
#define APP_LOCAL_MAC4                0x04
#define APP_LOCAL_MAC5                0x05

#define APP_LOCAL_IP                  "192.168.100.100"
#define APP_LOCAL_NETMASK             "255.255.255.0"
#define APP_LOCAL_GATEWAY             "192.168.100.1"
#define APP_REMOTE_IP                 "192.168.100.99"
#define APP_REMOTE_PORT               8080U

#endif
