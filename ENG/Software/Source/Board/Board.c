
//-----------------------------------------------------------------------
// Identifier    $Id: Board.c 94 2020-01-01 19:55:47Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/Board/Board.c $ */
//-----------------------------------------------------------------------*/

/* self */
#include "Board.h"

// ################################
void Board_Init(void)
{
    MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_16);    // SMCLK    = DCO/16    = 3MHz (TimerA0, ...)
}

