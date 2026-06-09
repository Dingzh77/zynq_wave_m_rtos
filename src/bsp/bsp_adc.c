#include "bsp_adc.h"

/**
 * @brief ADC 控制器初始化
 * @param cfg ADC 配置结构体指针，包含模式等参数
 * @note 如果 cfg 为空指针，函数直接返回不执行任何操作
 * @note 本函数将所有配置一次性写入寄存器，适合初始化时统一配置
 */
void bsp_adc_init(const adc_ctrl_cfg_t *cfg)
{
    u32 reg_val = 0;

    if (cfg == NULL) {
        return;
    }

    // 配置 ADC 工作模式
    reg_val |= ((cfg->mode & 0x1U) << ADC_CTRL_MODE_SHIFT) & ADC_CTRL_MODE_MASK;
    // 配置 ADC 通道
    reg_val |= ((cfg->three_channel & 0x1U) << ADC_CTRL_CH_SHIFT) & ADC_CTRL_CH_MASK;
    // 配置 ADC 初始化错误清除位
    reg_val |= ((cfg->init_err_clr ? 1U : 0U) << ADC_CTRL_INIT_ERR_CLR_SHIFT) & ADC_CTRL_INIT_ERR_CLR_MASK;

    // 将组合好的值写入控制寄存器
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_ADC_CONTROL_REG, reg_val);
}

/**
 * @brief 设置 ADC 工作模式
 * @param mode ADC 模式（枚举类型 adc_mode_t）
 * @note 使用字段写入函数，确保只修改模式位，不影响其他位
 */
void bsp_adc_set_mode(adc_mode_t mode)
{
    pl_reg_write_field(PL_REGISTER_ENGINE_BASE_ADDR,
                       PL_ADC_CONTROL_REG,
                       ADC_CTRL_MODE_MASK,
                       ADC_CTRL_MODE_SHIFT,
                       (u32)mode);
}

/**
 * @brief 获取 ADC 当前工作模式
 * @return true 表示当前SHA模式，false 表示CDS模式
 * @note 实际返回值的含义取决于硬件定义，通常模式 0 表示某种默认或停止状态
 * @note 如果模式位域超过 1 位，该函数仅判断是否非零，不返回具体模式值
 */
bool bsp_adc_get_mode(void)
{
    return (pl_reg_read_field(PL_REGISTER_ENGINE_BASE_ADDR,
                              PL_ADC_CONTROL_REG,
                              ADC_CTRL_MODE_MASK,
                              ADC_CTRL_MODE_SHIFT) != 0U);
}

void bsp_adc_clear_init_err(void)
{
    pl_reg_set_bits(PL_REGISTER_ENGINE_BASE_ADDR, PL_ADC_CONTROL_REG, ADC_CTRL_INIT_ERR_CLR_MASK);
}

void bsp_adc_set_3channel(u32 channel)
{
    pl_reg_write_field(PL_REGISTER_ENGINE_BASE_ADDR,
                       PL_ADC_CONTROL_REG,
                       ADC_CTRL_CH_MASK,
                       ADC_CTRL_CH_SHIFT,
                       channel);
}

u32 bsp_adc_get_control_reg(void)
{
    return pl_reg_read(PL_REGISTER_ENGINE_BASE_ADDR, PL_ADC_CONTROL_REG);
}

void bsp_adc_set_control_reg(u16 reg_val)
{
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_ADC_CONTROL_REG, (u32)reg_val);
}
