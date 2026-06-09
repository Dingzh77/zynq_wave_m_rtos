#include "ps_private_timer.h"

volatile u32 g_timer_counter = 0;
volatile u8 g_250ms_flag = 0;
volatile u8 g_500ms_flag = 0;
volatile u8 g_1000ms_flag = 0;
volatile u8 g_2000ms_flag = 0;

//定时器初始化程序
int ps_private_timer_init(XScuTimer *timer_ptr, u8 TimerPrescaler, u32 TimerValue_ms)
{
    int status = 0 ;
    XScuTimer_Config *timer_cfg_ptr;

    // 添加参数检查
    if (TimerPrescaler == 0) {
        return XST_FAILURE;
    }

    // 优化计算顺序，避免中间结果溢出
    u64 TimerCnt = (u64)TimerValue_ms * SYSTEM_FREQ_HZ;
    TimerCnt /= (1000 * TimerPrescaler);

    //私有定时器初始化
    timer_cfg_ptr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
    if (NULL == timer_cfg_ptr)
        return XST_FAILURE;
    status = XScuTimer_CfgInitialize(timer_ptr, timer_cfg_ptr,timer_cfg_ptr->BaseAddr);
    if (status != XST_SUCCESS)
        return XST_FAILURE;

    XScuTimer_SetPrescaler(timer_ptr, TimerPrescaler);
    XScuTimer_LoadTimer(timer_ptr, (u32)TimerCnt);           // 加载计数周期
    XScuTimer_EnableAutoReload(timer_ptr);              // 设置自动装载模式

    return XST_SUCCESS;
}

//定时器中断初始化
void ps_private_timer_int_init(XScuGic *intc_ptr,XScuTimer *timer_ptr)
{
    //设置定时器中断
    XScuGic_Connect(intc_ptr, TIMER_IRPT_INTR,
          (Xil_ExceptionHandler)ps_private_timer_int_handler, (void *)timer_ptr);

    XScuGic_Enable(intc_ptr, TIMER_IRPT_INTR); //使能GIC中的定时器中断
    XScuTimer_EnableInterrupt(timer_ptr);      //使能定时器中断
}

void ps_private_timer_start(XScuTimer *timer_ptr)
{
    XScuTimer_Start(timer_ptr);
}

//定时器中断处理程序
void ps_private_timer_int_handler(void *CallBackRef)
{
    XScuTimer *timer_ptr = (XScuTimer *)CallBackRef;
    // 1. 检查定时器是否到期
    if (XScuTimer_IsExpired(timer_ptr)) {

        // 2. 清除中断状态，防止持续中断
        XScuTimer_ClearInterruptStatus(timer_ptr);

        // 使用重命名的全局变量
        if(++g_timer_counter == 1001)
        	g_timer_counter = 0;

        // 设置标志位
        if(g_timer_counter % 250 == 0)
        	g_250ms_flag = 1;
        if(g_timer_counter % 500 == 0)
        	g_500ms_flag = 1;
        if(g_timer_counter % 1000 == 0)
        	g_1000ms_flag = 1;
        if(g_timer_counter % 2000 == 0)
        	g_2000ms_flag = 1;
    }
}
