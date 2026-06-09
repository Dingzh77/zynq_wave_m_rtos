#ifndef _TCP_CFG_PROTO_H_
#define _TCP_CFG_PROTO_H_

#include "xil_types.h"
#include "lwip/def.h"   /* lwip_htonl / lwip_htons / lwip_ntohl / lwip_ntohs */
#include "lwip/err.h"

#define TCP_CFG_MAGIC              0x43464731U   /* "CFG1" */
#define TCP_CFG_RX_BUF_SIZE        128U
#define TCP_CFG_MAX_PAYLOAD_LEN    32U

/* 命令ID */
#define TCP_CFG_CMD_WRITE_BLOCK    0x01U   /* PC -> Board */
#define TCP_CFG_CMD_STATE_REPORT   0x81U   /* Board -> PC */

/* reg_mask 位定义 */
#define CFG_MASK_CCD_CONTROL       (1U << 0)
#define CFG_MASK_CCD_INTEGTIM      (1U << 1)
#define CFG_MASK_CCD_SPS           (1U << 2)
#define CFG_MASK_CCD_WAITTIM       (1U << 3)
#define CFG_MASK_ADC_CONTROL       (1U << 4)

/* 合法位掩码 */
#define CCD_CONTROL_ALLOWED_MASK   0x801FU
#define ADC_CONTROL_ALLOWED_MASK   0x8003U

#pragma pack(push, 1)
typedef struct {
    u32 magic;         /* 固定帧头，网络字节序 */
    u8  cmd;           /* 命令ID */
    u8  reserved;      /* 保留 */
    u16 payload_len;   /* payload长度，网络字节序 */
    u16 seq;           /* 序号，网络字节序 */
} tcp_cfg_header_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    u16 reg_mask;       /* 网络字节序 */
    u16 ccd_control;    /* 网络字节序 */
    u16 ccd_integtim;   /* 网络字节序 */
    u16 ccd_sps;        /* 网络字节序 */
    u16 ccd_waittim;    /* 网络字节序 */
    u16 adc_control;    /* 网络字节序 */
} tcp_cfg_write_block_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    u16 ccd_control;    /* 网络字节序 */
    u16 ccd_integtim;   /* 网络字节序 */
    u16 ccd_sps;        /* 网络字节序 */
    u16 ccd_waittim;    /* 网络字节序 */
    u16 adc_control;    /* 网络字节序 */
} tcp_cfg_state_payload_t;
#pragma pack(pop)

void tcp_cfg_feed_bytes(const u8 *data, u16 data_len);
err_t tcp_cfg_send_state_report(u16 seq);

#endif
