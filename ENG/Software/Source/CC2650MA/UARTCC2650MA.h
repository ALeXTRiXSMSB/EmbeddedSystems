//! @file UARTCC2650MA.h
//! @brief TI-RSLK MSP432 - UART Backchannel Module (Header)
//!
//!  UART Module (CC2650MA) \n
//!  P3.2        UART_RXD (CC2650MA TX)   \n
//!  P3.3        UART_TXD (CC2650MA RX)   \n
//!
//!  Internal resources         \n
//!  UART/CC2650MA channel (EUSCI_A0, 115200, 8, n, 1) \n
//!  Internal state machines plus ISR support for non-blocking receiving/transmitting of strings \n
//!  Separate internal ring buffer for both receive & transmit \n
//!
//! Description: .
//!
//!                 MSP432P401
//!              ------------------
//!          /|\|                  |               --------------
//!           | |                  |               |
//!           --|RST          P3.2 |<-- RX - TX <--|
//!             |                  |               |  CC2650MA
//!             |             P3.3 |--> TX - RX -->|
//!             |                  |               |
//!             |                  |               --------------
//!
//!

//-----------------------------------------------------------------------
// Identifier    $Id: UARTCC2650MA.h 94 2020-01-01 19:55:47Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/CC2650MA/UARTCC2650MA.h $
//-----------------------------------------------------------------------*/

#ifndef UARTCC2650MA_H
#define UARTCC2650MA_H

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

/* DriverLib Includes */
#include "driverlib.h"
#include "CC2650_Module.h"

//! length of circular buffer --> must be 2^(positive integer)
#define UARTCC2650MA_CIRCBUFFER_MAX (128)
//! Start Of Frame
#define UARTCC2650MA_SOF (0xFE)

//! conditional compilation: use LED to show activity
#define UARTCC2650MA_USE_DEBUG_LED
#define UARTCC2650MA_DEBUG_LED_ON  P2OUT |= BIT0
#define UARTCC2650MA_DEBUG_LED_OFF P2OUT &= ~BIT0

//! Structure defines circular buffer (one instance each for send & receive)
typedef struct _TUARTCC2650MACircBuffer
{
  uint_fast16_t numMessages;   			//!< number of received SNP messages in buffer
  uint_fast16_t posRead;                      	//!< current position for read access
  uint_fast16_t posWrite;                     	//!< current position for write access
  uint8_t content[UARTCC2650MA_CIRCBUFFER_MAX]; //!< the buffer itself
} TUARTCC2650MACircBuffer;

//! Interrupt Handler for both send & receive (via circular buffer)
extern void EUSCIA2_IRQHandler(void);

//! Interface Function: Init UART
extern void UARTCC2650MA_Init(void);

//! Interface Function: Send Message (via circular buffer)
extern void UARTCC2650MA_Send(char* msg);

//! Interface Function: Put message into buffer (via circular buffer)
int32_t UARTCC2650MA_QueueMessage( uint8_t* msg );

//! Interface Function: Trigger send message prior filled into buffer (via circular buffer)
void UARTCC2650MA_TriggerSendMessage(void);

//! Interface Function: Get Received Message (via circular buffer)
extern uint16_t UARTCC2650MA_GetReceivedMessage(char* szMsg, uint16_t uMaxLen);

//! Interface Function: Is message processing in progeress?
extern bool UARTCC2650MA_GetReceivingMessage( void );

//! Interface Function: Get number of received messages in buffer
extern uint_fast16_t UARTCC2650MA_GetNumberOfReceivedMessages( void );

#endif
