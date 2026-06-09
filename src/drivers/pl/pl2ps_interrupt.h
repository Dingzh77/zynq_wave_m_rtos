#ifndef _PL2PS_INTERRUPT_H
#define _PL2PS_INTERRUPT_H

#include "xparameters.h"
#include "xstatus.h"
#include "xscugic.h"

//b01 Active HIGH level sensitive
//b11 Rising edge sensitive
#define TRIGGER_HIGH_LEVEL 	    0x1
#define TRIGGER_RISE 		    0x3

#define PL_INT_ID0   61
#define PL_INT_ID1   62

/* =========================================================
 * Function Prototypes
 * ========================================================= */

void pl2ps_int_init(XScuGic *gic_ins_ptr, u16 pl_intID, u8 type);
void pl_int_handler(void *CallbackRef);

//extern unsigned char PlInterruptTirg;

#endif
