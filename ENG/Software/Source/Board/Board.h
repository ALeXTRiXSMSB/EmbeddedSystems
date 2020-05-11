
//-----------------------------------------------------------------------
// Identifier    $Id: Board.h 159 2020-04-09 19:43:58Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/Board/Board.h $ */
//-----------------------------------------------------------------------*/

#ifndef BOARD_H
#define BOARD_H

#define TIRSLKMAX 1 // Board is TI-RSLK max

/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

// external interface of module
extern void Board_Init(void);

#endif
