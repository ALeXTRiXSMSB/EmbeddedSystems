//! @file UARTMsg.c
//! @brief TI-RSLK MSP432 UART Back Channel Module (Implementation)

//-----------------------------------------------------------------------
//! Identifier    $Id: UARTMsg.c 94 2020-01-01 19:55:47Z UweCreutzburg $ \n
//! Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/UARTMsg/UARTMsg.c $ */ \n
//-----------------------------------------------------------------------*/

/* self */
#include "UARTMsg.h"

/*!
  UART Back Channel Configuration Parameter. These are the configuration parameters to
  make the eUSCI A UART module to operate with a 57600 baud rate. These
  values were calculated using the online calculator that TI provides
  at:
  http://software-dl.ti.com/msp430/msp430_public_sw/mcu/msp430/MSP430BaudRateConverter/index.html
 */

//! Configuration of UART via structure from driverlib.h
static const eUSCI_UART_Config uartConfig =
{
 EUSCI_A_UART_CLOCKSOURCE_SMCLK,          //!< SMCLK Clock Source = 3MHz
 3,                                       //!< BRDIV
 4,                                       //!< UCxBRF
 0,                                       //!< UCxBRS
 EUSCI_A_UART_NO_PARITY,                  //!< No Parity
 EUSCI_A_UART_LSB_FIRST,                  //!< LSB First
 EUSCI_A_UART_ONE_STOP_BIT,               //!< One stop bit
 EUSCI_A_UART_MODE,                       //!< UART mode
 EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION  //!< Oversampling = 1
};

static TUARTMsgCircBuffer aUARTMsgBufferReceive;
static TUARTMsgCircBuffer aUARTMsgBufferTransmit;

// ################################
void UARTMsg_Init(void)
{
    //! Activate corresponding route for I/O-pins
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1, GPIO_PIN2 | GPIO_PIN3, GPIO_PRIMARY_MODULE_FUNCTION);

    //! Configure UART Module based on pre-filled structure
    MAP_UART_initModule(EUSCI_A0_BASE, &uartConfig);

    //! clear "old" interrupt flag for transmit channel if set
    MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG);

    //! Enable the UART module
    MAP_UART_enableModule(EUSCI_A0_BASE);

    //! Enable receive interrupt
    MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT );

    //! Enable interrupt for the module
    MAP_Interrupt_enableInterrupt(INT_EUSCIA0);
}

// ################################
bool UARTMsg_GetReceivedData(char* szMsg, uint8_t uMaxLen)
{

    //! Prepare processing of received data
    uint_fast16_t uCount = 0;
    bool bSuccess = false;
    uint_fast16_t uOldPosRead = aUARTMsgBufferReceive.posRead; // save for later repair if incomplete data

    //! Check for some not processed data in buffer
    bool bContinue = ( aUARTMsgBufferReceive.posRead != aUARTMsgBufferReceive.posWrite );

    //! While valid data do ...
    while (bContinue)
    {
        //! Get and split content received
        szMsg[uCount++] = aUARTMsgBufferReceive.content[aUARTMsgBufferReceive.posRead++];
        aUARTMsgBufferReceive.posRead &= (UARTMSG_CIRCBUFFER_MAX-1);
        bContinue = ( aUARTMsgBufferReceive.posRead != aUARTMsgBufferReceive.posWrite ) && (uCount < uMaxLen);

        //! check for termination of string (0x0d)
        if (uCount > 0)
        {
            if ( (szMsg[uCount-1] == 0x0d) )
            {
                bContinue   = false;
                bSuccess    = true;
                szMsg[uCount] = 0; //!< append ASCII zero in case of success
            }
        }
    }

    //! Something went wrong (e.g. message not terminated correctly), restore old state.
    if (!bSuccess)
    {
        aUARTMsgBufferReceive.posRead = uOldPosRead;
        szMsg[0] = 0; //!< first element of returned string is ASCII zero in case of no success.
    }

//! Return true if message successfully received, else return false.
return bSuccess;
}

// ################################
void UARTMsg_Send(char* msg)
{

    //! fill circular buffer with message to send
    uint_fast16_t uCount = 0;
    while (*msg)
    {
        aUARTMsgBufferTransmit.content[aUARTMsgBufferTransmit.posWrite++] = *msg;
        aUARTMsgBufferTransmit.posWrite &= (UARTMSG_CIRCBUFFER_MAX-1);
        msg++;
        uCount++;
    }

    //! new message to send?
    //! start transmitting message
    if (uCount > 0)
    {
        //! fill transmit buffer
        MAP_UART_transmitData(EUSCI_A0_BASE, (uint_fast8_t) aUARTMsgBufferTransmit.content[aUARTMsgBufferTransmit.posRead++] );
        //! advance pointer of circular buffer
        aUARTMsgBufferTransmit.posRead &= (UARTMSG_CIRCBUFFER_MAX-1);
        //! enable interrupt EUSCI_A_UART_TRANSMIT_INTERRUPT for interrupt driven processing of transmit
        MAP_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT);
    }
}

// ++++++++++++++++++++++++++++++++
void EUSCIA0_IRQHandler(void)
{

#ifdef UARTMSG_USE_DEBUG_LED
    UARTMSG_DEBUG_LED_ON;
#endif

    //! get source of interrupt
    uint32_t status = MAP_UART_getEnabledInterruptStatus(EUSCI_A0_BASE);

    //! source == Receive
    if (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG)
    {
        //! Insert received char into circular buffer and advance pointer
        aUARTMsgBufferReceive.content[aUARTMsgBufferReceive.posWrite++] = MAP_UART_receiveData(EUSCI_A0_BASE);

        //! INFO
        //! Not required: MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, (status & EUSCI_A_UART_RECEIVE_INTERRUPT_FLAG) );
        //! The UCRXIFG interrupt flag is set each time a character is received and loaded into UCAxRXBUF. An
        //! interrupt request is generated if UCRXIE is set. UCRXIFG and UCRXIE are reset by a Hard Reset signal
        //! or when UCSWRST = 1. UCRXIFG is automatically reset when UCAxRXBUF is read

        //! check buffer counter for overflow and wrap around
        aUARTMsgBufferReceive.posWrite &= (UARTMSG_CIRCBUFFER_MAX-1);
    }

    //! source == Transmit
    if ((status & EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG) )
    {
        //! INFO
        //! Not required: MAP_UART_clearInterruptFlag(EUSCI_A0_BASE, (status & EUSCI_A_UART_TRANSMIT_INTERRUPT_FLAG));
        //! The UCTXIFG interrupt flag is set by the transmitter to indicate that UCAxTXBUF is ready to accept
        //! another character. An interrupt request is generated if UCTXIE is set. UCTXIFG is automatically reset if a
        //! character is written to UCAxTXBUF.

        //! check for buffer empty (all data sent)
        if (aUARTMsgBufferTransmit.posRead != aUARTMsgBufferTransmit.posWrite)
        {
            //! buffer not empty: transmit next char
            MAP_UART_transmitData(EUSCI_A0_BASE, (uint_fast8_t) aUARTMsgBufferTransmit.content[aUARTMsgBufferTransmit.posRead++] );
            aUARTMsgBufferTransmit.posRead &= (UARTMSG_CIRCBUFFER_MAX-1);
        }
        else
        {
            //! buffer empty means end of transmission: disable interrupt EUSCI_A_UART_TRANSMIT_INTERRUPT
            MAP_UART_disableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_TRANSMIT_INTERRUPT);
        }
    }

#ifdef UARTMSG_USE_DEBUG_LED
    UARTMSG_DEBUG_LED_OFF;
#endif

}

