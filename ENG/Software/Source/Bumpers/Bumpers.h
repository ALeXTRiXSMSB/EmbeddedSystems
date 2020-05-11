
//-----------------------------------------------------------------------
// Identifier    $Id: Bumpers.h 94 2020-01-01 19:55:47Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/Bumpers/Bumpers.h $ */
//-----------------------------------------------------------------------*/

#ifndef BUMPERS_H
#define BUMPERS_H

/* DriverLib Includes */
#include "driverlib.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

#define BUMPERS_PORT_BITS (BIT0+BIT2+BIT3+BIT5+BIT6+BIT7)


typedef enum _TBumpers_Position
{
    eBumper_R3 = BIT0,
    eBumper_R2 = BIT2,
    eBumper_R1 = BIT3,
    eBumper_L1 = BIT5,
    eBumper_L2 = BIT6,
    eBumper_L3 = BIT7,
} TBumpers_Position_t;


typedef struct _TBumpersBuffer
{
    uint8_t samples[3];  // debouncing buffer
} TBumpersBuffer;



// Procedures
extern void Bumpers_Init(void);
extern void Bumpers_Read(void);
TBumpersBuffer Bumpers_GetBuffer(void);


#endif
