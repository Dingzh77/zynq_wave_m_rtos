#include "pl_rst_n.h"

/**
 * @brief  通过 PS 对 PL 进行一次软复位
 *         实质是操作 Zynq 的 SLCR 寄存器：
 *         解锁 -> 置位 FPGA_RST_CTRL[3:0] -> 清零 -> 再上锁，
 *         从而产生一个 FCLK_RESETN[3:0] 的低脉冲（复位 PL 逻辑）。
 */
void pl_reset_n(void) {
	// 1. 解锁 SLCR（System Level Control Registers），
	//    只有写入特定的解锁码后，才允许修改 SLCR 里的控制寄存器。
	Xil_Out32(XSLCR_UNLOCK_ADDR, XSLCR_UNLOCK_CODE);

	// 2. 往 FPGA_RST_CTRL 寄存器写 0x0F：
	//    低 4 位 = 1 -> 对应的 FCLK_RESETN[3:0] 输出复位（拉低），
	//    等效于对 PL 里使用这些复位信号的逻辑进行复位。
	Xil_Out32(XSLCR_FPGA_RST_CTRL_ADDR, 0x0F);

	// 3. 再写 0x00：
	//    低 4 位清零 -> 释放 FCLK_RESETN[3:0]（拉高），
	//    PL 逻辑从复位状态出来，重新开始工作。
	Xil_Out32(XSLCR_FPGA_RST_CTRL_ADDR, 0x00);

	// 4. 重新上锁 SLCR，防止后续误写系统级控制寄存器。
	Xil_Out32(XSLCR_LOCK_ADDR, XSLCR_LOCK_CODE);
}
