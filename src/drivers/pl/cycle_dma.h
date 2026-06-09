#ifndef _CYCLE_DMA_H_
#define _CYCLE_DMA_H_

#include "xparameters.h"
#include "xstatus.h"
#include "xil_cache.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pl2ps_interrupt.h"
#include "axi_to_pl.h"

// 寄存器地址定义
#define PL_CYCLE_DMA_REGISTER_BASE_ADDR   0x43000000
// #define PL_CYCLE_DMA_DATA_BASE_ADDR       0x00000000

// 寄存器偏移定义
#define PL_FIFO_LEN_REG       0x04//read only
#define PL_TRANS_ADDR_REG     0x08
#define PL_TRANS_LEN_REG      0x0C
#define PL_INT_CLR_REG        0x10

#define DMA_INT_READY         0x01
#define DMA_INT_DONE          0x02

#define DMA_BUF_PING          0U
#define DMA_BUF_PANG          1U

extern volatile u8 cycle_dma_int_flag;
extern u32 g_done_buf_id;

/* =========================================================
 * Function Prototypes
 * ========================================================= */

void cycle_dma_int_init(XScuGic *gic_ins_ptr);
void cycle_dma_set_event_task(TaskHandle_t task);
u32 get_cycle_dma_trans_length(void);
void cycle_dma_start_trans(u32 start_addr, u32 trans_length);
void cycle_dma_int_clear(u8 int_item);

/* 新增：绑定 ping-pang 缓冲区 */
void cycle_dma_bind_buffer(u64 *buf_base, u32 frame_length);

/* 新增：buffer 相关工具函数 */
UINTPTR cycle_dma_get_ping_addr(void);
UINTPTR cycle_dma_get_pang_addr(void);
UINTPTR cycle_dma_get_buf_addr(u32 buf_id);

u64 *cycle_dma_get_ping_ptr(void);
u64 *cycle_dma_get_pang_ptr(void);
u64 *cycle_dma_get_buf_ptr(u32 buf_id);

u32  cycle_dma_get_frame_length(void);
u32  cycle_dma_get_frame_bytes(void);

void cycle_dma_invalidate_buf(u32 buf_id);


#endif
