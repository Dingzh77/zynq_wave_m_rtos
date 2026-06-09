#ifndef _TCP_FRAME_PROTO_H_
#define _TCP_FRAME_PROTO_H_

#include "xil_types.h"
#include "lwip/err.h"
#include "lwip/def.h"   /* lwip_htonl / lwip_htons */

#define ADC_FRAME_MAGIC  0x5A4E5951U

#pragma pack(push, 1)
typedef struct {
    u32 magic;        /* 固定帧头 */
    u16 frame_id;     /* 帧序号 */
    u16 payload_len;  /* payload长度 */
} adc_frame_header_t;
#pragma pack(pop)


#endif
