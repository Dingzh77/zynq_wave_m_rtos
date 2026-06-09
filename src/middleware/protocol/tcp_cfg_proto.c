#include <string.h>
#include "xil_printf.h"
#include "xil_io.h"

#include "../../bsp/bsp_adc.h"
#include "../../bsp/bsp_ccd.h"
#include "../../bsp/bsp_cfg_state.h"

#include "tcp_cfg_proto.h"
#include "../net/tcp_client.h"

//接收缓冲区结构体
typedef struct {
    u8  buf[TCP_CFG_RX_BUF_SIZE];       // 接收缓冲区
    u16 len;                            // 缓冲区中当前有效数据的字节数
} tcp_cfg_rx_ctx_t;

static tcp_cfg_rx_ctx_t g_tcp_cfg_rx_ctx = {0};

//帧头中的 magic 字段，使用大端字节序表示 "CFG1"，以便在数据流中快速识别帧的起始位置
static const u8 g_cfg_magic_be[4] = {
    0x43, 0x46, 0x47, 0x31   /* "CFG1" */
};


//从数据流中消费（处理掉）前面 n 个字节后，需要把剩下的 len - n 个字节移到缓冲区开头，并更新 len。
static void tcp_cfg_consume_bytes(tcp_cfg_rx_ctx_t *ctx, u16 n)
{
    if (ctx == 0 || n == 0U) {
        return;
    }

    if (n >= ctx->len) {
        ctx->len = 0U;
        return;
    }
    //dest, src, count
    memmove(ctx->buf, &ctx->buf[n], ctx->len - n);
    ctx->len = (u16)(ctx->len - n);
}


//查找帧头 magic 第一次出现的位置
static int tcp_cfg_find_magic(const u8 *buf, u16 len)
{
    u16 i;

    if (buf == 0 || len < 4U) {
        return -1;
    }

    for (i = 0; i <= (u16)(len - 4U); i++) {
        if ((buf[i + 0] == g_cfg_magic_be[0]) &&
            (buf[i + 1] == g_cfg_magic_be[1]) &&
            (buf[i + 2] == g_cfg_magic_be[2]) &&
            (buf[i + 3] == g_cfg_magic_be[3])) {
            return (int)i;
        }
    }

    return -1;
}

static void tcp_cfg_handle_write_block(u16 seq, const u8 *payload, u16 payload_len)
{
    tcp_cfg_write_block_t raw;
    u16 reg_mask;
    u16 ccd_control;
    u16 ccd_integtim;
    u16 ccd_sps;
    u16 ccd_waittim;
    u16 adc_control;

    u32 ccd_mode_raw;
    u32 adc_mode_raw;

    if (payload_len != sizeof(tcp_cfg_write_block_t)) {
        xil_printf("CFG write_block invalid payload_len=%u\r\n",
                   (unsigned int)payload_len);
        return;
    }

    memcpy(&raw, payload, sizeof(raw));

    reg_mask     = lwip_ntohs(raw.reg_mask);
    ccd_control  = lwip_ntohs(raw.ccd_control);
    ccd_integtim = lwip_ntohs(raw.ccd_integtim);
    ccd_sps      = lwip_ntohs(raw.ccd_sps);
    ccd_waittim  = lwip_ntohs(raw.ccd_waittim);
    adc_control  = lwip_ntohs(raw.adc_control);

    /* 过滤非法位 */
    ccd_control &= CCD_CONTROL_ALLOWED_MASK;
    adc_control &= ADC_CONTROL_ALLOWED_MASK;

    /* ------------------------------
     * CCD控制寄存器：更新软件shadow + 写硬件寄存器
     * ------------------------------ */
    if (reg_mask & CFG_MASK_CCD_CONTROL) {

        ccd_cfg_handle.enable =
            (((ccd_control & CCD_CTRL_ENABLE_MASK) >> CCD_CTRL_ENABLE_SHIFT) != 0U);

        ccd_cfg_handle.ccd1_gain =
            (u32)((ccd_control & CCD_CTRL_CCD1_GAIN_MASK) >> CCD_CTRL_CCD1_GAIN_SHIFT);

        ccd_cfg_handle.ccd2_gain =
            (u32)((ccd_control & CCD_CTRL_CCD2_GAIN_MASK) >> CCD_CTRL_CCD2_GAIN_SHIFT);

        ccd_mode_raw =
            (u32)((ccd_control & CCD_CTRL_MODE_MASK) >> CCD_CTRL_MODE_SHIFT);

        /* 只接受当前定义过的模式值 */
         if ((ccd_mode_raw == (u32)CCD_MODE_INNER_AUTO) ||
             (ccd_mode_raw == (u32)CCD_MODE_EXTER_ONCE)) {
             ccd_cfg_handle.mode = (ccd_mode_t)ccd_mode_raw;
         } else {
//             xil_printf("CFG warning: invalid CCD mode=%lu, keep old mode=%d\r\n",
//                        (unsigned long)ccd_mode_raw,
//                        (int)pl_ccd_cfg.mode);
         }

        ccd_cfg_handle.sudo =
            (((ccd_control & CCD_CTRL_SUDO_MASK) >> CCD_CTRL_SUDO_SHIFT) != 0U);

        pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR,
                     PL_CCD_CONTROL_REG,
                     (u32)ccd_control);
    }

    /* ------------------------------
     * CCD数据寄存器：更新软件shadow + 写硬件寄存器
     * ------------------------------ */
    if (reg_mask & CFG_MASK_CCD_INTEGTIM) {
        g_ccd_integtim_us = ccd_integtim;

        pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR,
                     PL_CCD_INTEGTIM_REG,
                     (u32)ccd_integtim);
    }

    if (reg_mask & CFG_MASK_CCD_SPS) {
        g_ccd_sps_us = ccd_sps;

        pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR,
                     PL_CCD_SPS_REG,
                     (u32)ccd_sps);
    }

    if (reg_mask & CFG_MASK_CCD_WAITTIM) {
        g_ccd_waittim_us = ccd_waittim;

        pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR,
                     PL_CCD_WAITTIM_REG,
                     (u32)ccd_waittim);
    }

    /* ------------------------------
     * ADC控制寄存器：更新软件shadow + 写硬件寄存器
     * ------------------------------ */
    if (reg_mask & CFG_MASK_ADC_CONTROL) {

        adc_mode_raw =
            (u32)((adc_control & ADC_CTRL_MODE_MASK) >> ADC_CTRL_MODE_SHIFT);

        adc_cfg_handle.mode = (adc_mode_t)adc_mode_raw;

        adc_cfg_handle.three_channel =
            (u32)((adc_control & ADC_CTRL_CH_MASK) >> ADC_CTRL_CH_SHIFT);

        adc_cfg_handle.init_err_clr =
            (((adc_control & ADC_CTRL_INIT_ERR_CLR_MASK) >> ADC_CTRL_INIT_ERR_CLR_SHIFT) != 0U);

        pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR,
                     PL_ADC_CONTROL_REG,
                     (u32)adc_control);
    }

    xil_printf("CFG apply ok: seq=%u mask=0x%04x ccd_ctrl=0x%04x integ=%u sps=%u wait=%u adc_ctrl=0x%04x\r\n",
               (unsigned int)seq,
               (unsigned int)reg_mask,
               (unsigned int)ccd_control,
               (unsigned int)g_ccd_integtim_us,
               (unsigned int)g_ccd_sps_us,
               (unsigned int)g_ccd_waittim_us,
               (unsigned int)adc_control);
}

static void tcp_cfg_handle_frame(u8 cmd, u16 seq, const u8 *payload, u16 payload_len)
{
    switch (cmd) {

        case TCP_CFG_CMD_WRITE_BLOCK:
            tcp_cfg_handle_write_block(seq, payload, payload_len);
            break;

        default:
            xil_printf("CFG unknown cmd=0x%02x seq=%u payload_len=%u\r\n",
                       (unsigned int)cmd,
                       (unsigned int)seq,
                       (unsigned int)payload_len);
            break;
    }
}

//从接收缓冲区里，一帧一帧地拆出完整命令来处理。
//对于不完整的帧（半包）则等后续数据到齐后再处理；
//对于无效数据则丢弃掉以实现重同步。
static void tcp_cfg_try_parse_frames(tcp_cfg_rx_ctx_t *ctx)
{
    while (1) {
        tcp_cfg_header_t hdr_raw;
        u32 magic;
        u8  cmd;
        u16 payload_len;
        u16 seq;
        u16 total_len;//头长度 + payload长度
        int pos;

        if (ctx == 0) {
            return;
        }

        if (ctx->len < (u16)sizeof(tcp_cfg_header_t)) {//10 字节帧头都不够，继续等
            return;
        }

        /* 先找 magic，做重同步 */
        pos = tcp_cfg_find_magic(ctx->buf, ctx->len);

        if (pos < 0) {
            /* 当前缓存里没有 magic，保留最后3字节 */
            if (ctx->len > 3U) {
                memmove(ctx->buf, &ctx->buf[ctx->len - 3U], 3U);
                ctx->len = 3U;
            }
            return;
        }

        //如果帧头不在开头，就丢弃掉前面无效的数据，继续处理后面的数据
        if (pos > 0) {
            tcp_cfg_consume_bytes(ctx, (u16)pos);
            if (ctx->len < (u16)sizeof(tcp_cfg_header_t)) {
                return;
            }
        }

        //把前 10 个字节拷贝到 hdr_raw 结构体里
        memcpy(&hdr_raw, ctx->buf, sizeof(hdr_raw));

        //大小端转换
        magic       = lwip_ntohl(hdr_raw.magic);
        cmd         = hdr_raw.cmd;
        payload_len = lwip_ntohs(hdr_raw.payload_len);
        seq         = lwip_ntohs(hdr_raw.seq);

        if (magic != TCP_CFG_MAGIC) {
            tcp_cfg_consume_bytes(ctx, 1U);
            continue;
        }

        if (payload_len > TCP_CFG_MAX_PAYLOAD_LEN) {
            xil_printf("CFG payload too large: %u\r\n", (unsigned int)payload_len);
            tcp_cfg_consume_bytes(ctx, 1U);
            continue;
        }

        total_len = (u16)(sizeof(tcp_cfg_header_t) + payload_len);

        if (ctx->len < total_len) {
            /* 半包，继续等 */
            return;
        }

        tcp_cfg_handle_frame(cmd,
                             seq,
                             &ctx->buf[sizeof(tcp_cfg_header_t)],//跳过前面的头部，从 payload 起始位置开始，把 payload 地址传进去
                             payload_len);

        tcp_cfg_consume_bytes(ctx, total_len);
    }
}

//从TCP数据流中喂入字节数据，追加到接收缓冲区末尾，并尝试解析出完整的帧来处理
void tcp_cfg_feed_bytes(const u8 *data, u16 data_len)
{
    while (data_len > 0U) {
        u16 room;
        u16 copy_len;

        room = (u16)(TCP_CFG_RX_BUF_SIZE - g_tcp_cfg_rx_ctx.len);

        if (room == 0U) {
            /* 缓冲满了，强制重同步 */
            if (g_tcp_cfg_rx_ctx.len > 3U) {
                memmove(g_tcp_cfg_rx_ctx.buf,
                        &g_tcp_cfg_rx_ctx.buf[g_tcp_cfg_rx_ctx.len - 3U],
                        3U);
                g_tcp_cfg_rx_ctx.len = 3U;
            } else {
                g_tcp_cfg_rx_ctx.len = 0U;
            }

            room = (u16)(TCP_CFG_RX_BUF_SIZE - g_tcp_cfg_rx_ctx.len);
        }

        copy_len = (data_len < room) ? data_len : room;

        //尾插法把新数据追加到缓冲区末尾
        memcpy(&g_tcp_cfg_rx_ctx.buf[g_tcp_cfg_rx_ctx.len], data, copy_len);
        g_tcp_cfg_rx_ctx.len = (u16)(g_tcp_cfg_rx_ctx.len + copy_len);

        //更新指针和剩余长度，继续处理后续数据
        data += copy_len;
        data_len = (u16)(data_len - copy_len);

        tcp_cfg_try_parse_frames(&g_tcp_cfg_rx_ctx);
    }
}

static u16 tcp_cfg_build_ccd_control_word(void)
{
    u16 reg_val = 0U;

    reg_val |= ((ccd_cfg_handle.enable ? 1U : 0U) << CCD_CTRL_ENABLE_SHIFT) & CCD_CTRL_ENABLE_MASK;
    reg_val |= ((ccd_cfg_handle.ccd1_gain & 0x1U) << CCD_CTRL_CCD1_GAIN_SHIFT) & CCD_CTRL_CCD1_GAIN_MASK;
    reg_val |= ((ccd_cfg_handle.ccd2_gain & 0x1U) << CCD_CTRL_CCD2_GAIN_SHIFT) & CCD_CTRL_CCD2_GAIN_MASK;
    reg_val |= (((u16)ccd_cfg_handle.mode & 0x3U) << CCD_CTRL_MODE_SHIFT) & CCD_CTRL_MODE_MASK;
    reg_val |= ((ccd_cfg_handle.sudo ? 1U : 0U) << CCD_CTRL_SUDO_SHIFT) & CCD_CTRL_SUDO_MASK;

    reg_val &= CCD_CONTROL_ALLOWED_MASK;
    return reg_val;
}

static u16 tcp_cfg_build_adc_control_word(void)
{
    u16 reg_val = 0U;

    reg_val |= (((u16)adc_cfg_handle.mode & 0x1U) << ADC_CTRL_MODE_SHIFT) & ADC_CTRL_MODE_MASK;
    reg_val |= ((adc_cfg_handle.three_channel & 0x1U) << ADC_CTRL_CH_SHIFT) & ADC_CTRL_CH_MASK;
    reg_val |= ((adc_cfg_handle.init_err_clr ? 1U : 0U) << ADC_CTRL_INIT_ERR_CLR_SHIFT) & ADC_CTRL_INIT_ERR_CLR_MASK;

    reg_val &= ADC_CONTROL_ALLOWED_MASK;
    return reg_val;
}

err_t tcp_cfg_send_state_report(u16 seq)
{
    tcp_cfg_header_t hdr;
    tcp_cfg_state_payload_t payload;
    u8 tx_buf[sizeof(tcp_cfg_header_t) + sizeof(tcp_cfg_state_payload_t)];
    u16 ccd_control;
    u16 adc_control;
    err_t err;

    ccd_control = tcp_cfg_build_ccd_control_word();
    adc_control = tcp_cfg_build_adc_control_word();

    hdr.magic       = lwip_htonl(TCP_CFG_MAGIC);
    hdr.cmd         = TCP_CFG_CMD_STATE_REPORT;
    hdr.reserved    = 0U;
    hdr.payload_len = lwip_htons((u16)sizeof(tcp_cfg_state_payload_t));
    hdr.seq         = lwip_htons(seq);

    payload.ccd_control  = lwip_htons(ccd_control);
    payload.ccd_integtim = lwip_htons(g_ccd_integtim_us);
    payload.ccd_sps      = lwip_htons(g_ccd_sps_us);
    payload.ccd_waittim  = lwip_htons(g_ccd_waittim_us);
    payload.adc_control  = lwip_htons(adc_control);

    memcpy(&tx_buf[0], &hdr, sizeof(hdr));
    memcpy(&tx_buf[sizeof(hdr)], &payload, sizeof(payload));

    err = tcp_client_send_data(tx_buf, sizeof(tx_buf));

    if (err == ERR_OK) {
        xil_printf("STATE report sent: seq=%u ccd_ctrl=0x%04x integ=%u sps=%u wait=%u adc_ctrl=0x%04x\r\n",
                   (unsigned int)seq,
                   (unsigned int)ccd_control,
                   (unsigned int)g_ccd_integtim_us,
                   (unsigned int)g_ccd_sps_us,
                   (unsigned int)g_ccd_waittim_us,
                   (unsigned int)adc_control);
    }

    return err;
}
