#ifndef _PS_GIC_H_
#define _PS_GIC_H_

#include "xparameters.h"
#include "xstatus.h"
#include "xscugic.h"

#define INTC_DEVICE_ID      XPAR_SCUGIC_SINGLE_DEVICE_ID  //通用中断控制器ID

/* =========================================================
 * Function Prototypes
 * ========================================================= */

int ps_gic_init(XScuGic *ps_gic_inst_ptr);

#endif
