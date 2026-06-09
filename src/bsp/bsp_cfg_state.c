#include <stdbool.h>
#include "bsp_cfg_state.h"

/* ---------------------------------------------------------
 * 软件侧配置
 * --------------------------------------------------------- */

/* CCD 控制配置 shadow */
ccd_ctrl_cfg_t ccd_cfg_handle = {
    .enable    = false,
    .ccd1_gain = 1,
    .ccd2_gain = 1,
    .mode      = CCD_MODE_INNER_AUTO,
    .sudo      = false
};

/* ADC 控制配置 shadow */
adc_ctrl_cfg_t adc_cfg_handle = {
    .mode          = ADC_MODE_SHA,
    .three_channel = 0,
    .init_err_clr  = false
};

/* CCD 数据寄存器 shadow */
u16 g_ccd_integtim_us = 100;
u16 g_ccd_sps_us      = 1000;
u16 g_ccd_waittim_us  = 0;
