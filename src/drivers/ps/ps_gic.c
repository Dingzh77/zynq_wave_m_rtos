#include "ps_gic.h"


/* 初始化中断控制器
 * @param   gic_ins_ptr是一个指向XScuGic驱动实例的指针
 * @return  如果成功返回XST_SUCCESS, 否则返回XST_FAILURE
 */

int ps_gic_init(XScuGic *ps_gic_inst_ptr)
{
    int status;
    XScuGic_Config *gic_cfg_ptr;

    if (ps_gic_inst_ptr == NULL) {
        return XST_FAILURE;
    }

    gic_cfg_ptr = XScuGic_LookupConfig(INTC_DEVICE_ID);
    if (gic_cfg_ptr == NULL) {
        return XST_FAILURE;
    }

    status = XScuGic_CfgInitialize(ps_gic_inst_ptr,
                                   gic_cfg_ptr,
                                   gic_cfg_ptr->CpuBaseAddress);
    if (status != XST_SUCCESS) {
        return XST_FAILURE;
    }

    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
                                 (Xil_ExceptionHandler)XScuGic_InterruptHandler,
                                 ps_gic_inst_ptr);

    Xil_ExceptionEnable();

    return XST_SUCCESS;
}
