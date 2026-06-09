#ifndef _BSP_CCD_H_
#define _BSP_CCD_H_

#include "xparameters.h"
#include "xstatus.h"
#include "../drivers/pl/axi_to_pl.h"

/* =========================================================
 * Mode Definition
 * ========================================================= */

/**
 * @brief CCD 工作模式枚举
 * @note 对应 CCD 控制寄存器中的模式位（bit4~bit3），共 2 位
 */
typedef enum {
    CCD_MODE_INNER_AUTO = 1,  /**< 模式 1：内部自动模式 */
    CCD_MODE_EXTER_ONCE = 2   /**< 模式 2：外部触发一次 */
} ccd_mode_t;


/* =========================================================
 * Config Struct
 * ========================================================= */

/**
 * @brief CCD 控制配置结构体
 * @note 用于一次性配置 CCD 控制寄存器的各个字段
 */
typedef struct {
    bool       enable;      /**< CCD 使能，true: 使能，false: 禁用 */
    u32        ccd1_gain;   /**< CCD1 增益，当前按 1bit 处理，有效值 0/1 */
    u32        ccd2_gain;   /**< CCD2 增益，当前按 1bit 处理，有效值 0/1 */
    ccd_mode_t mode;        /**< 工作模式，2bit，对应枚举值 */
    bool       sudo;        /**< sudo 模式，true: 使能，false: 禁用 */
} ccd_ctrl_cfg_t;

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

#define PL_CCD_CONTROL_REG   		    0x04   /**< CCD 控制寄存器偏移 */
#define PL_CCD_INTEGTIM_REG 		    0x08   /**< CCD 积分时间寄存器偏移 */
#define PL_CCD_WAITTIM_REG  		    0x0C   /**< CCD 等待时间寄存器偏移 */
#define PL_CCD_SPS_REG  			    0x10   /**< CCD SPS 寄存器偏移 */

/* =========================================================
 * CCD 控制寄存器位域定义
 * ========================================================= */

/* 使能位 */
#define CCD_CTRL_ENABLE_SHIFT           0                       /**< 使能位偏移 */
#define CCD_CTRL_ENABLE_MASK            BIT(CCD_CTRL_ENABLE_SHIFT) /**< 使能位掩码 */

/* CCD1 增益位 */
#define CCD_CTRL_CCD1_GAIN_SHIFT        1                       /**< CCD1 增益位偏移 */
#define CCD_CTRL_CCD1_GAIN_MASK         BIT(CCD_CTRL_CCD1_GAIN_SHIFT) /**< CCD1 增益位掩码 */

/* CCD2 增益位 */
#define CCD_CTRL_CCD2_GAIN_SHIFT        2                       /**< CCD2 增益位偏移 */
#define CCD_CTRL_CCD2_GAIN_MASK         BIT(CCD_CTRL_CCD2_GAIN_SHIFT) /**< CCD2 增益位掩码 */

/* 模式位（2 位，bit4~bit3） */
#define CCD_CTRL_MODE_SHIFT             3                       /**< 模式位起始偏移 */
#define CCD_CTRL_MODE_MASK              GENMASK(4, 3)           /**< 模式位掩码（bit4~bit3） */

/* sudo 模式位 */
#define CCD_CTRL_SUDO_SHIFT             15                      /**< sudo 模式位偏移 */
#define CCD_CTRL_SUDO_MASK              BIT(CCD_CTRL_SUDO_SHIFT) /**< sudo 模式位掩码 */


/* =========================================================
 * Function Prototypes
 * ========================================================= */
void bsp_ccd_init(const ccd_ctrl_cfg_t *cfg);
void bsp_ccd_deinit(void);
void bsp_ccd_set_enable(bool enable);
void bsp_ccd_set_ccd1_gain(u32 gain);
void bsp_ccd_set_ccd2_gain(u32 gain);
void bsp_ccd_set_mode(ccd_mode_t mode);
void bsp_ccd_set_sudo(bool sudo);
u32 bsp_ccd_get_control_reg(void);
bool bsp_ccd_get_sudo_status(void);
bool bsp_ccd_get_enable_status(void);
void bsp_ccd_set_integration_time_us(u16 time_us);
void bsp_ccd_set_conversion_rate_us(u16 rate_us);
void bsp_ccd_set_delay_time_us(u16 delay_us);

#endif
