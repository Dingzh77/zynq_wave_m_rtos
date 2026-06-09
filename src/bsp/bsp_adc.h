#ifndef _BSP_ADC_H_
#define _BSP_ADC_H_

#include "xparameters.h"
#include "xstatus.h"
#include "../drivers/pl/axi_to_pl.h"

/* =========================================================
 * Mode Definition
 * ========================================================= */

/**
 * @brief ADC 工作模式枚举
 * @note 对应 ADC 控制寄存器中的模式位（bit0），共 1 位
 */
typedef enum {
	ADC_MODE_SHA = 0,  /**< 模式 1：单采样模式 */
	ADC_MODE_CDS = 1   /**< 模式 2：相关双采样模式 */
} adc_mode_t;

/* =========================================================
 * Config Struct
 * ========================================================= */

/**
 * @brief ADC 控制配置结构体
 * @note 用于一次性配置 ADC 控制寄存器的各个字段
 */
typedef struct {
    adc_mode_t mode;            /**< 工作模式，1bit，对应枚举值 */
    u32        three_channel;   /**< 1：3通道，0：2通道 */
    bool       init_err_clr;    /**< 初始化错误清除，true: 清除，false: 不清除 */
} adc_ctrl_cfg_t;

/* =========================================================
 * Register Engine 地址映射
 * ========================================================= */

/**
 * @brief PL 寄存器引擎基地址
 * @note 通过 AXI 总线与 PL 交互的基地址
 */
#define  PL_REGISTER_ENGINE_BASE_ADDR 	0x40000000

/* =========================================================
 * Register Engine 配置寄存器地址偏移
 * ========================================================= */
#define PL_ADC_CONTROL_REG  		    0x14   /**< ADC 控制寄存器偏移 */

/* =========================================================
 * ADC 控制寄存器位域定义
 * ========================================================= */

/* 模式位 */
#define ADC_CTRL_MODE_SHIFT            0
#define ADC_CTRL_MODE_MASK             BIT(ADC_CTRL_MODE_SHIFT)

/* 通道位 */
#define ADC_CTRL_CH_SHIFT              1
#define ADC_CTRL_CH_MASK               BIT(ADC_CTRL_CH_SHIFT)

/* 初始化错误清除位 */
#define ADC_CTRL_INIT_ERR_CLR_SHIFT    15
#define ADC_CTRL_INIT_ERR_CLR_MASK     BIT(ADC_CTRL_INIT_ERR_CLR_SHIFT)

/* =========================================================
 * Function Prototypes
 * ========================================================= */
void bsp_adc_init(const adc_ctrl_cfg_t *cfg);
void bsp_adc_set_mode(adc_mode_t mode);
bool bsp_adc_get_mode(void);
void bsp_adc_clear_init_err(void);
void bsp_adc_set_3channel(u32 channel);
void bsp_adc_set_control_reg(u16 reg_val);
u32 bsp_adc_get_control_reg(void);

#endif
