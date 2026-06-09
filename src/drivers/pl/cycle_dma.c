#include "xil_printf.h"

#include "cycle_dma.h"


volatile u8 cycle_dma_int_flag = 0;
static TaskHandle_t s_dma_event_task = NULL;

/* 模块内部保存 buffer 信息 */
static u64 *s_dma_buf_base = NULL;
static u32  s_dma_frame_length = 0;
static u32  s_dma_frame_bytes = 0;

u32 g_done_buf_id = DMA_BUF_PANG;

void cycle_dma_set_event_task(TaskHandle_t task)
{
    s_dma_event_task = task;
}

/**
 * @brief 初始化循环 DMA 中断
 * @param gic_ins_ptr GIC 控制器实例指针
 * @note 为 PL_INT_ID0 和 PL_INT_ID1 两个中断源配置上升沿触发
 */
void cycle_dma_int_init(XScuGic *gic_ins_ptr)
{
    pl2ps_int_init(gic_ins_ptr, PL_INT_ID0, TRIGGER_RISE);
    pl2ps_int_init(gic_ins_ptr, PL_INT_ID1, TRIGGER_RISE);
}

/**
 * @brief 获取循环 DMA 传输长度
 * @return 当前 FIFO 中的传输长度（单位：字节）
 * @note 读取 PL_FIFO_LEN_REG 寄存器获得当前已配置的传输长度
 */
u32 get_cycle_dma_trans_length(void)
{
    return pl_reg_read(PL_CYCLE_DMA_REGISTER_BASE_ADDR, PL_FIFO_LEN_REG);
}

/**
 * @brief 启动循环 DMA 传输
 * @param start_addr     源地址（DMA 读取的起始地址）
 * @param trans_length   传输长度（单位：字节）
 * @note 先配置传输地址，再配置传输长度，写入长度寄存器后 DMA 自动启动
 */
void cycle_dma_start_trans(u32 start_addr, u32 trans_length)
{
    pl_reg_write(PL_CYCLE_DMA_REGISTER_BASE_ADDR, PL_TRANS_ADDR_REG, start_addr);
    pl_reg_write(PL_CYCLE_DMA_REGISTER_BASE_ADDR, PL_TRANS_LEN_REG, trans_length);
}

/**
 * @brief 清除循环 DMA 中断
 * @param int_item 中断标识
 * @note 向 PL_INT_CLR_REG 写入指定中断号，清除对应的中断挂起状态
 */
void cycle_dma_int_clear(u8 int_item)
{
    pl_reg_write(PL_CYCLE_DMA_REGISTER_BASE_ADDR, PL_INT_CLR_REG, (u32)int_item);
}

/**
 * @brief pl DMA 中断处理函数
 * @param CallbackRef 中断回调参数，实际为中断 ID（PL_INT_ID0 或 PL_INT_ID1）
 * @note 根据中断 ID 设置对应的标志位，供主循环判断和处理
 */
void pl_int_handler(void *CallbackRef)
{
    u16 int_id = (u16)(uintptr_t)CallbackRef;
    BaseType_t higher_priority_task_woken = pdFALSE;
    u32 notify_bits = 0U;

    if (int_id == PL_INT_ID0) {
        cycle_dma_int_flag |= DMA_INT_READY;
        notify_bits = DMA_INT_READY;
    }
    else if (int_id == PL_INT_ID1) {
        cycle_dma_int_flag |= DMA_INT_DONE;

        /* 记录本次DMA完成的buffer id */
        g_done_buf_id = (g_done_buf_id == DMA_BUF_PING) ? DMA_BUF_PANG : DMA_BUF_PING;
        notify_bits = DMA_INT_DONE;
    }
    else {
//        xil_printf("PLX Interrupt\r\n");
    }

    if ((s_dma_event_task != NULL) && (notify_bits != 0U)) {
        xTaskNotifyFromISR(s_dma_event_task,
                           notify_bits,
                           eSetBits,
                           &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}

/* 绑定 buffer
 * buf_base      : ping-pang总缓冲区首地址
 * frame_length  : 单帧 u64 个数，例如 256
 */
void cycle_dma_bind_buffer(u64 *buf_base, u32 frame_length)
{
    s_dma_buf_base = buf_base;
    s_dma_frame_length = frame_length;
    s_dma_frame_bytes = frame_length * sizeof(u64);
}

UINTPTR cycle_dma_get_ping_addr(void)
{
    return (UINTPTR)&s_dma_buf_base[0];
}

UINTPTR cycle_dma_get_pang_addr(void)
{
    return (UINTPTR)&s_dma_buf_base[s_dma_frame_length];
}

UINTPTR cycle_dma_get_buf_addr(u32 buf_id)
{
    return (buf_id == DMA_BUF_PING) ? cycle_dma_get_ping_addr()
                                    : cycle_dma_get_pang_addr();
}

u64 *cycle_dma_get_ping_ptr(void)
{
    return &s_dma_buf_base[0];
}

u64 *cycle_dma_get_pang_ptr(void)
{
    return &s_dma_buf_base[s_dma_frame_length];
}

u64 *cycle_dma_get_buf_ptr(u32 buf_id)
{
    return (buf_id == DMA_BUF_PING) ? cycle_dma_get_ping_ptr()
                                    : cycle_dma_get_pang_ptr();
}

u32 cycle_dma_get_frame_length(void)
{
    return s_dma_frame_length;
}

u32 cycle_dma_get_frame_bytes(void)
{
    return s_dma_frame_bytes;
}

void cycle_dma_invalidate_buf(u32 buf_id)
{
    if (s_dma_buf_base == NULL || s_dma_frame_bytes == 0U) {
        return;
    }

    Xil_DCacheInvalidateRange(cycle_dma_get_buf_addr(buf_id), s_dma_frame_bytes);
}

