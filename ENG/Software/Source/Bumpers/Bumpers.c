 
//-----------------------------------------------------------------------
// Identifier    $Id: Bumpers.c 94 2020-01-01 19:55:47Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/Bumpers/Bumpers.c $ */
//-----------------------------------------------------------------------*/

/* self */
#include "Bumpers.h"

// Buffer
static TBumpersBuffer aBumpersBuffer;

// ################################
void Bumpers_Init(void)
{
    MAP_GPIO_setAsInputPinWithPullUpResistor( GPIO_PORT_P4, BUMPERS_PORT_BITS );   // inputs
}

// ################################
void Bumpers_Read(void)
{
    aBumpersBuffer.samples[2] = aBumpersBuffer.samples[1];
    aBumpersBuffer.samples[1] = aBumpersBuffer.samples[0];
    aBumpersBuffer.samples[0] = P4IN & BUMPERS_PORT_BITS;
}

// #################################
TBumpersBuffer Bumpers_GetBuffer( void )
{
    return aBumpersBuffer;
}
