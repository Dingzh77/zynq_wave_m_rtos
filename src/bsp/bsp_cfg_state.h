#ifndef _BSP_CFG_STATE_H_
#define _BSP_CFG_STATE_H_

#include "xil_types.h"

#include "bsp_adc.h"
#include "bsp_ccd.h"


/* ---------------------------------------------------------
 * 软件侧配置 shadow
 * 用于：
 * 1. 下行配置写入时同步维护
 * 2. 上行状态同步时直接读取
 * --------------------------------------------------------- */

/* CCD 控制配置 shadow */
extern ccd_ctrl_cfg_t ccd_cfg_handle;

/* ADC 控制配置 shadow */
extern adc_ctrl_cfg_t adc_cfg_handle;

/* CCD 数据寄存器 shadow */
extern u16 g_ccd_integtim_us;
extern u16 g_ccd_sps_us;
extern u16 g_ccd_waittim_us;

#endif /* _PL_CFG_STATE_H_ */
