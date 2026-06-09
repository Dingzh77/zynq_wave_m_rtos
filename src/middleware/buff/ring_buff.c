#include <string.h>

#include "ring_buff.h"
#include "lwip/def.h"   /* 提供 lwip_htonl / lwip_htons */

/**
 * @brief 初始化环形缓冲区
 * 
 * 将整个结构体清零，包括槽位数组、head/tail/count、next_frame_id、drop_cnt。
 * 
 * @param rb 指向 ring_buff_t 的指针（不能为 NULL，若为 NULL 则直接返回）
 */
void ring_buff_init(ring_buff_t *rb)
{
    if (rb == NULL) {
        return;   /* 空指针不操作 */
    }

    memset(rb, 0, sizeof(*rb));
}

/**
 * @brief 检查缓冲区是否为空
 * @param rb 指向 ring_buff_t 的指针
 * @return 1 为空，0 非空（或指针无效）
 */
int ring_buff_is_empty(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 1;
    }
    return (rb->count == 0U);
}

/**
 * @brief 检查缓冲区是否为满
 * @param rb 指向 ring_buff_t 的指针
 * @return 1 为满，0 未满（或指针无效）
 */
int ring_buff_is_full(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 1;
    }
    return (rb->count >= RING_BUFF_DEPTH);
}

/**
 * @brief 获取当前缓冲区中的帧数量
 * @param rb 指向 ring_buff_t 的指针
 * @return 帧数量（指针无效时返回0）
 */
u32 ring_buff_count(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 0U;
    }
    return rb->count;
}

/**
 * @brief 获取丢帧计数
 * @param rb 指向 ring_buff_t 的指针
 * @return 丢弃的帧数（指针无效时返回0）
 */
u32 ring_buff_drop_count(const ring_buff_t *rb)
{
    if (rb == NULL) {
        return 0U;
    }
    return rb->drop_cnt;
}

/**
 * @brief 将负载数据封装成帧并推入环形缓冲区
 * 
 * 实现步骤：
 * 1. 参数有效性检查（rb、payload、payload_len）
 * 2. 检查负载长度是否超过 RING_BUFF_PAYLOAD_MAX
 * 3. 检查缓冲区是否已满
 * 4. 获取 tail 指向的槽位
 * 5. 构建帧头部（使用网络字节序）
 * 6. 拷贝头部和负载到槽位 data 数组
 * 7. 填写槽位的元信息（len, frame_id）
 * 8. 更新 next_frame_id 并移动 tail 指针（环形）
 * 9. 增加 count
 * 
 * @param rb          指向 ring_buff_t 的指针
 * @param payload     负载数据指针
 * @param payload_len 负载长度（必须 >0 且 <= RING_BUFF_PAYLOAD_MAX）
 * @return 1 成功，0 失败（失败时 drop_cnt 会增加）
 */
int ring_buff_push_payload(ring_buff_t *rb, const void *payload, u16 payload_len)
{
    ring_buff_slot_t *slot;
    adc_frame_header_t header;

    /* 参数合法性检查 */
    if (rb == NULL || payload == NULL || payload_len == 0U) {
        return 0;
    }

    /* 负载长度超限：丢弃并统计 */
    if (payload_len > RING_BUFF_PAYLOAD_MAX) {
        rb->drop_cnt++;
        return 0;
    }

    /* 缓冲区已满：丢弃并统计 */
    if (ring_buff_is_full(rb)) {
        rb->drop_cnt++;
        return 0;
    }

    /* 获取当前 tail 指向的槽位 */
    slot = &rb->slots[rb->tail];

    /* 构建帧头部（转为网络字节序，即大端） */
    header.magic       = lwip_htonl(ADC_FRAME_MAGIC);
    header.frame_id    = lwip_htons(rb->next_frame_id);
    header.payload_len = lwip_htons(payload_len);

    /* 拷贝头部和负载到槽位数据区 */
    memcpy(slot->data, &header, sizeof(header));
    memcpy(slot->data + sizeof(header), payload, payload_len);

    /* 记录槽位元信息 */
    slot->len       = (u16)(sizeof(header) + payload_len);
    slot->frame_id  = rb->next_frame_id;
    // slot->src_buf_id = src_buf_id;

    /* 更新下一个帧序号 */
    rb->next_frame_id++;

    /* 移动 tail 指针（环形） */
    rb->tail++;
    if (rb->tail >= RING_BUFF_DEPTH) {
        rb->tail = 0U;
    }

    /* 增加帧计数 */
    rb->count++;

    return 1;
}

/**
 * @brief 查看队头帧的信息（只读，不弹出）
 * 
 * @param rb   指向 ring_buff_t 的指针
 * @param view 输出参数，用于接收队头帧的视图
 * @return 1 成功，0 失败（缓冲区为空或参数无效）
 * 
 * @note 返回的 view.data 指向槽位内部的数组，调用 pop 后该指针可能失效。
 *       如需长期保存数据，请拷贝到自己的缓冲区。
 */
int ring_buff_peek(const ring_buff_t *rb, ring_buff_view_t *view)
{
    const ring_buff_slot_t *slot;

    if (rb == NULL || view == NULL || ring_buff_is_empty(rb)) {
        return 0;
    }

    slot = &rb->slots[rb->head];

    view->data      = slot->data;
    view->len       = slot->len;
    view->frame_id  = slot->frame_id;

    return 1;
}

/**
 * @brief 弹出队头帧（从缓冲区中移除）
 * 
 * 仅移动 head 指针并减少 count，不实际清除槽位数据。
 * 
 * @param rb 指向 ring_buff_t 的指针
 * 
 * @warning 如果缓冲区为空，此函数不做任何操作。
 *          通常在 peek 获取数据并处理完成后调用。
 */
void ring_buff_pop(ring_buff_t *rb)
{
    if (rb == NULL || ring_buff_is_empty(rb)) {
        return;
    }

    /* 移动 head 指针（环形） */
    rb->head++;
    if (rb->head >= RING_BUFF_DEPTH) {
        rb->head = 0U;
    }

    /* 减少帧计数 */
    rb->count--;
}
