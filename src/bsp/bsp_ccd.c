#include "bsp_cfg_state.h"
#include "bsp_ccd.h"

/**
 * @brief CCD 控制器初始化
 * @param cfg CCD 配置结构体指针，包含使能、增益、模式、sudo等参数
 */
void bsp_ccd_init(const ccd_ctrl_cfg_t *cfg)
{
    u32 reg_val = 0;

	if (cfg == 0) {
		return;
	}

    // 配置 CCD 使能位
    reg_val |= ((cfg->enable ? 1U : 0U) << CCD_CTRL_ENABLE_SHIFT) & CCD_CTRL_ENABLE_MASK;
    // 配置 CCD1 增益（仅使用最低1位有效）
    reg_val |= ((cfg->ccd1_gain & 0x1U) << CCD_CTRL_CCD1_GAIN_SHIFT) & CCD_CTRL_CCD1_GAIN_MASK;
    // 配置 CCD2 增益（仅使用最低1位有效）
    reg_val |= ((cfg->ccd2_gain & 0x1U) << CCD_CTRL_CCD2_GAIN_SHIFT) & CCD_CTRL_CCD2_GAIN_MASK;
    // 配置工作模式（使用低2位）
    reg_val |= ((cfg->mode & 0x3U) << CCD_CTRL_MODE_SHIFT) & CCD_CTRL_MODE_MASK;
    // 配置 sudo 模式
    reg_val |= ((cfg->sudo ? 1U : 0U) << CCD_CTRL_SUDO_SHIFT) & CCD_CTRL_SUDO_MASK;

    // 将组合好的值写入控制寄存器
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG, reg_val);

    bsp_ccd_set_integration_time_us(g_ccd_integtim_us);    // 默认积分时间 100 μs
    bsp_ccd_set_conversion_rate_us(g_ccd_sps_us);    // 默认转换时间 1000 μs
}

/**
 * @brief CCD 控制器反初始化（关闭所有功能）
 * @note 直接将控制寄存器写 0，禁用所有功能
 */
void bsp_ccd_deinit(void)
{
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG, 0U);
}

/**
 * @brief 设置 CCD 使能状态
 * @param enable true 表示使能，false 表示禁用
 * @note 使用读-改-写方式仅修改使能位，不影响其他配置
 */
void bsp_ccd_set_enable(bool enable)
{
    if (enable) {
        pl_reg_set_bits(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG, CCD_CTRL_ENABLE_MASK);
    } else {
        pl_reg_clear_bits(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG, CCD_CTRL_ENABLE_MASK);
    }
}

/**
 * @brief 设置 CCD1 增益
 * @param gain 增益值（仅最低位有效，0 或 1）
 * @note 使用字段写入函数，确保只修改增益位
 */
void bsp_ccd_set_ccd1_gain(u32 gain)
{
    pl_reg_write_field(PL_REGISTER_ENGINE_BASE_ADDR,
                       PL_CCD_CONTROL_REG,
                       CCD_CTRL_CCD1_GAIN_MASK,
                       CCD_CTRL_CCD1_GAIN_SHIFT,
                       gain);
}

/**
 * @brief 设置 CCD2 增益
 * @param gain 增益值（仅最低位有效，0 或 1）
 * @note 使用字段写入函数，确保只修改增益位
 */
void bsp_ccd_set_ccd2_gain(u32 gain)
{
    pl_reg_write_field(PL_REGISTER_ENGINE_BASE_ADDR,
                       PL_CCD_CONTROL_REG,
                       CCD_CTRL_CCD2_GAIN_MASK,
                       CCD_CTRL_CCD2_GAIN_SHIFT,
                       gain);
}

/**
 * @brief 设置 CCD 工作模式
 * @param mode 工作模式，类型为 ccd_mode_t（枚举值，应使用低2位）
 * @note 使用字段写入函数，确保只修改模式位
 */
void bsp_ccd_set_mode(ccd_mode_t mode)
{
    pl_reg_write_field(PL_REGISTER_ENGINE_BASE_ADDR,
                       PL_CCD_CONTROL_REG,
                       CCD_CTRL_MODE_MASK,
                       CCD_CTRL_MODE_SHIFT,
                       (u32)mode);
}

/**
 * @brief 设置 CCD sudo 模式
 * @param sudo true 表示启用 sudo，false 表示禁用
 * @note 使用读-改-写方式仅修改 sudo 位，不影响其他配置
 */
void bsp_ccd_set_sudo(bool sudo)
{
    if (sudo) {
        pl_reg_set_bits(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG, CCD_CTRL_SUDO_MASK);
    } else {
        pl_reg_clear_bits(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG, CCD_CTRL_SUDO_MASK);
    }
}

/**
 * @brief 读取 CCD 控制寄存器的当前值
 * @return 控制寄存器的 32 位值
 */
u32 bsp_ccd_get_control_reg(void)
{
    return pl_reg_read(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG);
}

/**
 * @brief 读取 CCD sudo 位的当前值
 * @return true 表示 sudo 模式已启用，false 表示 sudo 模式未启用
 * @note 通过读取寄存器并提取 sudo 位来判断当前状态
 */
bool bsp_ccd_get_sudo_status(void)
{
    return (pl_reg_read_field(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG,
                               CCD_CTRL_SUDO_MASK, CCD_CTRL_SUDO_SHIFT) != 0U);
}

/**
 * @brief 读取 CCD enable 位的当前值
 * @return true 表示 enable ，false 表示 disable
 * @note 通过读取寄存器并提取 enable 位来判断当前状态
 */
bool bsp_ccd_get_enable_status(void)
{
    return (pl_reg_read_field(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_CONTROL_REG,
                               CCD_CTRL_ENABLE_MASK, CCD_CTRL_ENABLE_SHIFT) != 0U);
}

/**
 * @brief 设置 CCD 积分时间
 * @param time_us 积分时间，单位：微秒
 * @note 寄存器为 16 位有效，高 16 位清零
 * @note 硬件设计为每个 LSB 代表 1 μs，即写入值直接等于积分时间（μs）
 *       实际对应关系需根据硬件确认。
 */
void bsp_ccd_set_integration_time_us(u16 time_us)
{
    /* 只写低16位，高16位清零 */
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_INTEGTIM_REG, (u32)time_us);
}

/**
 * @brief 设置 CCD 转换速率（SPS）
 * @param rate_us 转换周期，单位：微秒（即每帧间隔）
 * @note 寄存器为 16 位有效，高 16 位清零
 * @note 硬件设计为每个 LSB 代表 1 μs，即写入值直接等于转换周期（μs）
 *       实际对应关系需根据硬件确认。
 */
void bsp_ccd_set_conversion_rate_us(u16 rate_us)
{
    /* 只写低16位，高16位清零 */
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_SPS_REG, (u32)rate_us);
}

/**
 * @brief 设置 CCD 延时时间
 * @param delay_us 延时时间，单位：微秒（即每帧间隔）
 * @note 寄存器为 16 位有效，高 16 位清零
 * @note 硬件设计为每个 LSB 代表 1 μs，即写入值直接等于延时时间（μs）
 *       实际对应关系需根据硬件确认。
 */
void bsp_ccd_set_delay_time_us(u16 delay_us)
{
    /* 只写低16位，高16位清零 */
    pl_reg_write(PL_REGISTER_ENGINE_BASE_ADDR, PL_CCD_WAITTIM_REG, (u32)delay_us);
}
