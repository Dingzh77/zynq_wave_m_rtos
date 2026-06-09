#ifndef _PL_RST_N_H_
#define _PL_RST_N_H_

#include "xil_misc_psreset_api.h"

#define XSLCR_LOCK_ADDR				    (XSLCR_BASEADDR + 0x00000004U)
#define XSLCR_UNLOCK_ADDR				(XSLCR_BASEADDR + 0x00000008U)
#define XSLCR_FPGA_RST_CTRL_ADDR        (XSLCR_BASEADDR + 0x00000240U)
#define XSLCR_LOCK_CODE		    		0x0000767BU
#define XSLCR_UNLOCK_CODE				0x0000DF0DU

/* =========================================================
 * Function Prototypes
 * ========================================================= */

void pl_reset_n(void);

#endif
