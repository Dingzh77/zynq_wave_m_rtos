#ifndef _PS_PRIVATE_TIMER_H
#define _PS_PRIVATE_TIMER_H

#include "xscutimer.h"
#include "xparameters.h"
#include "xstatus.h"
#include "xscugic.h"

#define TIMER_DEVICE_ID     XPAR_XSCUTIMER_0_DEVICE_ID   //定时器ID
#define TIMER_IRPT_INTR     XPAR_SCUTIMER_INTR           //定时器中断ID

#define SYSTEM_FREQ_HZ      666666666


extern volatile u8 g_250ms_flag;
extern volatile u8 g_500ms_flag;
extern volatile u8 g_1000ms_flag;
extern volatile u8 g_2000ms_flag;

/* =========================================================
 * Function Prototypes
 * ========================================================= */

int ps_private_timer_init(XScuTimer *timer_ptr, u8 TimerPrescaler, u32 TimerValue_ms);
void ps_private_timer_int_init(XScuGic *intc_ptr,XScuTimer *timer_ptr);
void ps_private_timer_start(XScuTimer *timer_ptr);
void ps_private_timer_int_handler(void *CallBackRef);

#endif //INC_6__TIMER_H
