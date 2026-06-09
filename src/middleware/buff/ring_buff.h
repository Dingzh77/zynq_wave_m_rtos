/**
 * @file ring_buff.h
 * @brief 环形缓冲区模块 - 用于存储带协议头的帧数据
 * 
 * 该模块实现了一个固定深度的环形缓冲区，每个槽位存储一个完整的数据帧。
 * 帧格式：[adc_frame_header_t][payload]
 * 用户只需提供负载数据，模块自动添加头部（魔数、帧ID、负载长度）。
 * 
 * @note 本模块未提供内部锁保护。如果在中断服务例程(ISR)和主循环中同时访问，
 *       必须由调用者自行保护临界区（如关中断、使用互斥量等）。
 */

#ifndef _RING_BUFF_H_
#define _RING_BUFF_H_

#include "xil_types.h"          // 提供 u8, u16, u32 等类型定义
#include "../protocol/tcp_frame_proto.h"    // 提供 adc_frame_header_t, ADC_FRAME_MAGIC 等

/* 环形缓冲区的深度（最多能存放的帧数）*/
#ifndef RING_BUFF_DEPTH
#define RING_BUFF_DEPTH          8U
#endif

/* 每个帧中负载（payload）的最大字节数 */
#ifndef RING_BUFF_PAYLOAD_MAX
#define RING_BUFF_PAYLOAD_MAX    2048U
#endif

/* 每个槽位数据区的总大小 = 头部大小 + 最大负载大小 */
#define RING_BUFF_FRAME_MAX      (sizeof(adc_frame_header_t) + RING_BUFF_PAYLOAD_MAX)

/**
 * @brief 环形缓冲区的一个槽位（存储一帧数据）
 */
typedef struct {
    u8  data[RING_BUFF_FRAME_MAX];  /**< 存储完整帧数据（头部+负载）的缓冲区 */
    u16 len;                        /**< 该帧的实际总长度（头部+负载） */
    u16 frame_id;                   /**< 该帧的序号（由环形缓冲区分配） */
} ring_buff_slot_t;

/**
 * @brief 环形缓冲区控制结构体
 */
typedef struct {
    ring_buff_slot_t slots[RING_BUFF_DEPTH]; /**< 槽位数组 */
    u32 head;           /**< 队头索引（指向最早存入的帧） */
    u32 tail;           /**< 队尾索引（指向下一个可写入的位置） */
    u32 count;          /**< 当前缓冲区中的帧数量 */
    u16 next_frame_id;  /**< 下一个要分配的帧序号（每次push后自增） */
    u32 drop_cnt;       /**< 因缓冲区满或负载过大而丢弃的帧计数 */
} ring_buff_t;

/**
 * @brief 只读视图结构体，用于安全地查看队头帧而不暴露内部槽位
 * 
 * @note 通过 peek 获取的 data 指针指向槽位内部数组。
 *       在调用 pop 之后，该指针可能被后续写入覆盖，如需长期保存请拷贝数据。
 */
typedef struct {
    const u8 *data;     /**< 指向帧数据的指针（包含头部+负载） */
    u16 len;            /**< 帧的总长度 */
    u16 frame_id;       /**< 帧序号 */
} ring_buff_view_t;

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================
 * Function Prototypes
 * ========================================================= */

void ring_buff_init(ring_buff_t *rb);
int  ring_buff_is_empty(const ring_buff_t *rb);
int  ring_buff_is_full(const ring_buff_t *rb);
u32  ring_buff_count(const ring_buff_t *rb);
u32  ring_buff_drop_count(const ring_buff_t *rb);
int  ring_buff_push_payload(ring_buff_t *rb, const void *payload, u16 payload_len);
int  ring_buff_peek(const ring_buff_t *rb, ring_buff_view_t *view);
void ring_buff_pop(ring_buff_t *rb);

#ifdef __cplusplus
}
#endif

#endif /* _RING_BUFF_H_ */
