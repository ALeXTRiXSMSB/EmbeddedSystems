// *******************************************************************************
// TI-RSLK MSP432 + CC2650MA Module
//
//  MSP432 LaunchPad    Booster Pack Module
//  P6.0 GPIO-out       MRDY 	(2)
//  P3.2 RXD            TXD  	(3)
//  P3.3 TXD            RXD  	(4)
//  P2.5 GPIO-in        SRDY 	(19)
//  P6.7 GPIO-out       NRESET  (35)
//
//  Internal resources
//
// Description: .
//
//                 MSP432P401
//              ------------------
//          /|\|                  |
//           | |                  |
//           --|RST      MRDY P6.0|-->
//             |          RXD P3.2|<--
//             |          TXD P3.3|-->
//             |         SRDY P2.5|<--
//             |       NRESET P6.7|-->
//
//

//-----------------------------------------------------------------------
// Identifier    $Id: CC2650_Module.h 94 2020-01-01 19:55:47Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/CC2650MA/CC2650_Module.h $ */
//-----------------------------------------------------------------------*/

#ifndef CC2650_MODULE_H
#define CC2650_MODULE_H

/* DriverLib Includes */
#include "driverlib.h"
#include "UARTCC2650MA.h"


/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

#define CC2650_MODULE_SOF (0xFE)
#define CC2650_MODULE_USE_DEBUG_LED
#define CC2650_MODULE_DEBUG_LED_ON  P2OUT |= BIT0
#define CC2650_MODULE_DEBUG_LED_OFF P2OUT &= ~BIT0

#define CC2650_MODULE_SRDY GPIO_PORT_P2,GPIO_PIN5
#define CC2650_MODULE_MRDY GPIO_PORT_P6,GPIO_PIN0
#define CC2650_MODULE_NRESET GPIO_PORT_P6,GPIO_PIN7

// Procedures
//! Interface Function: Init
extern bool CC2650_Module_Init( void );

extern bool CC2650_Module_InitDone( void );

extern void CC2650_Module_GetMessage( void );

extern int CC2650_AP_AddService(uint16_t uuid);

extern int CC2650_AP_AddCharacteristic( uint16_t uuid, uint16_t uSize, void *pData, uint8_t uPermission,
					 uint8_t uProperties, char name[], void(*ReadFunc)(void), void(*WriteFunc)(void) );


extern bool CC2650_Wait_SRDY_and_MRDY( void );



//! Interface Function: Init

//! Interrupt Handler for SRDY
void PORT2_IRQHandler(void);

#endif
