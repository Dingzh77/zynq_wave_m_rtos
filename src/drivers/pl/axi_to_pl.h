#ifndef _AXI_TO_PL_H_
#define _AXI_TO_PL_H_

#include "xparameters.h"
#include "xstatus.h"
#include "xil_io.h"
#include "xil_types.h"
#include <stdbool.h>

#define BIT(n)                 (1U << (n))
#define GENMASK(h, l)          (((0xFFFFFFFFU) << (l)) & (0xFFFFFFFFU >> (31 - (h))))


static inline u32 pl_reg_read(u32 base, u32 offset)
{
    return Xil_In32(base + offset);
}


static inline void pl_reg_write(u32 base, u32 offset, u32 value)
{
    Xil_Out32(base + offset, value);
}

/**
 * 设置寄存器中指定的位（将mask中为1的位设为1）
 * @param base   寄存器的基地址
 * @param offset 相对于基地址的偏移量
 * @param mask   位掩码，需要置1的位
 */
static inline void pl_reg_set_bits(u32 base, u32 offset, u32 mask)
{
    u32 val = pl_reg_read(base, offset);
    val |= mask;
    pl_reg_write(base, offset, val);
}

/**
 * 清除寄存器中指定的位（将mask中为1的位设为0）
 * @param base   寄存器的基地址
 * @param offset 相对于基地址的偏移量
 * @param mask   位掩码，需要清零的位
 */
static inline void pl_reg_clear_bits(u32 base, u32 offset, u32 mask)
{
    u32 val = pl_reg_read(base, offset);
    val &= ~mask;
    pl_reg_write(base, offset, val);
}

/**
 * 更新寄存器中指定的位（原子读-改-写）
 * @param base   寄存器的基地址
 * @param offset 相对于基地址的偏移量
 * @param mask   位掩码，指定需要修改的位
 * @param value  新值，只有与mask对应的位生效
 */
static inline void pl_reg_update_bits(u32 base, u32 offset, u32 mask, u32 value)
{
    u32 val = pl_reg_read(base, offset);
    val = (val & ~mask) | (value & mask);
    pl_reg_write(base, offset, val);
}

/**
 * 写入寄存器中的一个位字段
 * @param base      寄存器的基地址
 * @param offset    相对于基地址的偏移量
 * @param mask      字段的位掩码（例如0x7<<shift）
 * @param shift     字段起始位的位置（从0开始计数）
 * @param field_val 要写入字段的值（已右对齐，不能超出字段宽度）
 * @note 内部调用pl_reg_update_bits完成实际更新
 */
static inline void pl_reg_write_field(u32 base, u32 offset, u32 mask, u32 shift, u32 field_val)
{
    pl_reg_update_bits(base, offset, mask, (field_val << shift) & mask);
}

/**
 * @brief 读取寄存器中指定字段的值
 * @param base   寄存器的基地址
 * @param offset 相对于基地址的偏移量
 * @param mask   字段的位掩码（例如 0x7<<shift）
 * @param shift  字段起始位的位置（从 0 开始计数）
 * @return 字段的值（已右对齐，即字段的低位对齐到返回值的最低有效位）
 * @note 该函数先读取整个寄存器，然后通过掩码提取指定字段，最后右移得到对齐后的值
 * @note 调用者需确保 mask 与 shift 对应正确，且字段宽度不超过 32 位
 */
static inline u32 pl_reg_read_field(u32 base, u32 offset, u32 mask, u32 shift)
{
    u32 val = pl_reg_read(base, offset);
    return (val & mask) >> shift;
}

#endif
