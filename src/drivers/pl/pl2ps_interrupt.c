#include "pl2ps_interrupt.h"
#include "FreeRTOS.h"
#include "portmacro.h"

extern XScuGic xInterruptController;

//unsigned char PlInterruptTirg = 0;

//初始化PL端向PS端传递的中断请求
//  @param    gic_ins_ptr是一个指向XScuGic驱动实例的指针
//  @param    pl_intID 是 Pl 中断 ID
//  @param    type 是 pl中断源触发类型
void pl2ps_int_init(XScuGic *gic_ins_ptr,u16 pl_intID,u8 type)
{
    (void)gic_ins_ptr;

    xPortInstallInterruptHandler((uint8_t)pl_intID,
                                 (XInterruptHandler)pl_int_handler,
                                 (void *)(uintptr_t)pl_intID);

    XScuGic_SetPriorityTriggerType(&xInterruptController,
                                   pl_intID,
                                   configMAX_API_CALL_INTERRUPT_PRIORITY << portPRIORITY_SHIFT,
                                   type);

    vPortEnableInterrupt((uint8_t)pl_intID);
}

//用户的中断回调函数
//void pl_int_handler(void *CallbackRef)
//{
//    u16 InterruptID = (u16)(uintptr_t)CallbackRef;  // 把 void* 转回中断号
//    // if(InterruptID == PL_INTERRUPT_INTR_ID0)
//    // {
//    //     printf("PL0 Interrupt\n");
//    //     PlInterruptTirg = 1;
//    // }
//    // else if(InterruptID == PL_INTERRUPT_INTR_ID1)
//    // {
//    //     printf("PL1 Interrupt\n");
//    //     PlInterruptTirg = 2;
//    // }
//    // else
//    // {
//    //     PlInterruptTirg = 0;
//    //     printf("PLX Interrupt\n");
//    // }
//
//}
