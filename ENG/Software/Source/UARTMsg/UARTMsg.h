//! @file UARTMsg.h
//! @brief TI-RSLK MSP432 - UART Backchannel Module (Header)
//!
//!  UART Module (back channel) \n
//!  P1.2        BCLUART_RXD    \n
//!  P1.3        BCLUART_TXD    \n
//!
//!  Internal resources         \n
//!  UART back channel (EUSCI_A0, 57k6, 8, n, 1) \n
//!  Internal state machines plus ISR support for non-blocking receiving/transmitting of strings \n
//!  Separate internal ring buffer for both receive & transmit \n
//!
//! Description: .
//!
//!                 MSP432P401
//!              ------------------
//!          /|\|                  |
//!           | |                  |
//!           --|RST          P1.2 |<-- RX
//!             |                  |
//!             |             P1.3 |--> TX
//!             |                  |
//!             |                  |
//!
//!

//-----------------------------------------------------------------------
// Identifier    $Id: UARTMsg.h 94 2020-01-01 19:55:47Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/UARTMsg/UARTMsg.h $
//-----------------------------------------------------------------------*/

#ifndef UARTMSG_H
#define UARTMSG_H

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* DriverLib Includes */
#include "driverlib.h"

//! length of circular buffer --> must be 2^(positive integer)
#define UARTMSG_CIRCBUFFER_MAX (128)

//! conditional compilation: use LED to show activity
#define UARTMSG_USE_DEBUG_LED
#define UARTMSG_DEBUG_LED_ON  P2OUT |= BIT0
#define UARTMSG_DEBUG_LED_OFF P2OUT &= ~BIT0

//! Structure defines circular buffer (one instance each for send & receive)
typedef struct _TUARTMsgCircBuffer
{
    uint8_t content[UARTMSG_CIRCBUFFER_MAX];    //!< the buffer itself
    uint_fast16_t posRead;                      //!< current position for read access
    uint_fast16_t posWrite;                     //!< current position for write access
} TUARTMsgCircBuffer;

//! Interrupt Handler for both send & receive (via circular buffer)
extern void EUSCIA0_IRQHandler(void);

//! Interface Function: Init UART
extern void UARTMsg_Init(void);

//! Interface Function: Send Message (via circular buffer)
extern void UARTMsg_Send(char* msg);

//! Interface Function: Get Received Message (via circular buffer)
extern bool UARTMsg_GetReceivedData(char* szMsg, uint8_t uMaxLen);

#endif
