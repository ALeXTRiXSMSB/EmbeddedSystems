//! @file
//! @brief TI-RSLK MSP432 Line Sensor
//
//!  Line Sensor Module \n
//!  P7.0 ... P7.7   Left ... Right \n
//!  P5.3            IR-LED on/off \n
//!  P9.2            IR-LED on/off (TI_RSLKmax only)\n
//!
//!
//! Description: \n
//! The line sensor consists of eight reflex light barrier mounted in a straight line. \n
//! Each of the eight sensors discharges a capacitor that is pre-charged before the measurement starts. \n
//! The discharge current is a function of illumination, therefore, if the intensity of the reflected light from the sensors IR-LED
//! is high (white background) the capacitor discharges fast. For a non-reflecting background (black), the capacitor will stay longer charged.\n
//! The output voltage of the sensors (the voltage across the capacitor) goes to a digital input. Based on the threshold of the input the reading
//! of the port will result in decays (and zeros or ones) based on the illumination. \n
//! The inputs from the sensors are sampled several times with a rate of ~50µs resulting in a zero/one-pattern depending on the IR-illuminated background.
//!
//!
//!                 MSP432P401
//!              ------------------
//!          /|\|                  |
//!           | |                  |
//!           --|RST          P7   |<--8--> IR-Sensor (O: charge capacitor / I: read digital value)
//!             |                  |
//!             |             P5.3 |------> IR-LED even
//!             |             P9.2 |------> IR-LED odd (TI_RSLKmax only)
//!             |                  |
//!             |                  |
//!
//!

//-----------------------------------------------------------------------
//! $Id: LineSensor.h 159 2020-04-09 19:43:58Z UweCreutzburg $
//! $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/LineSensor/LineSensor.h $ */
//-----------------------------------------------------------------------*/

#ifndef LINESENSOR_H
#define LINESENSOR_H

/* DriverLib Includes */
#include "driverlib.h"

#include "bsp_int.h"
#include "Board.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

//! Debugging purposes
#define LINESENSOR_USE_DEBUG_LED
#define LINESENSOR_USE_DEBUG_LED_PORT (GPIO_PORT_P2)
#define LINESENSOR_USE_DEBUG_LED_PIN (GPIO_PIN0)

#define LINESENSOR_DEBUG_LED_ON  P2OUT |= BIT0
#define LINESENSOR_DEBUG_LED_OFF P2OUT &= ~BIT0

//! extreme values detected
#define LINESENSOR_ALL_1  (127)         //!< all black detected
#define LINESENSOR_ALL_0  (-128)        //!< all white detected

//! Timer period (µs)
#if TIRSLKMAX == 1
  #define TIMER_PERIOD    (50)
#else
  #define TIMER_PERIOD    (50)
#endif

//! Number of line-sensor readings to store in buffer (total timespan: @TIMER_PERIOD x @LINESENSOR_BUFFER_SIZE)
#define LINESENSOR_BUFFER_SIZE (30)

//! Helpers for accessing the Sensors Direction
typedef enum _TLineSensor_DirEnum
{
  eLineSensor_DirAhead       = 0,
  eLineSensor_DirRightMinor  = 1,
  eLineSensor_DirRight       = 2,
  eLineSensor_DirLeftMinor   = 3,
  eLineSensor_DirLeft        = 4
} TLineSensor_DirEnum;

//! Helpers for accessing the Sensors Quality Information
typedef enum _TLineSensor_QualEnum
{
  eLineSensor_QualAllWhite    = 0,
  eLineSensor_QualPoor        = 1,
  eLineSensor_QualGood        = 2,
  eLineSensor_QualAllBlack    = 3
} TLineSensor_QualEnum;

//! Helpers for accessing the Sensors Quality Information
typedef enum _TLineSensor_StateEnum
{
  eLineSensor_StateBad        = 0,
  eLineSensor_StateOK         = 1,
} TLineSensor_StateEnum;

//! Helpers for accessing the Sensors Quality Information
typedef enum _TLineSensor_ErrorEnum
{
  eLineSensor_ErrorNone       = 0,
  eLineSensor_ErrorDefault    = 1,
} TLineSensor_ErrorEnum;

//! Buffer for external usage via *_GetBuffer()
typedef struct _TLineSensorBuffer
{
    uint8_t samples[LINESENSOR_BUFFER_SIZE];
} TLineSensorBuffer;

//! Evaluated direction information buffer
typedef struct _TLineSensorBufferDirection
{
    int8_t samples[LINESENSOR_BUFFER_SIZE];
} TLineSensorBufferDirection;

//! LUT-method based on pre-calculated direction and quality information
typedef struct _TLineSensorPatternEvaluated
{
  uint8_t aDir   : 3;
  uint8_t aQual  : 2;
  uint8_t aState : 1;
  uint8_t aError : 2;
} TLineSensorPatternEvaluated;

//! LUT-method based on pre-calculated direction and quality information (I/O union)
typedef union _UTLineSensorPatternEvaluated
{
  uint8_t ucDataRaw;
  TLineSensorPatternEvaluated aPatternEvaluated;
} UTLineSensorPatternEvaluated;

//! IRQ-Handlers (used in "startup_msp432p401_ccs.c")
extern void TA1_0_IRQHandler(void);

//! Function Prototypes

//! Init the sensor
extern void LineSensor_Init(void);

//! Get evaluated direction information
extern TLineSensorPatternEvaluated LineSensor_GetBufferDirection(void);

//! Get curent buffer of the sensor
extern TLineSensorBuffer LineSensor_GetBuffer(void);

//! Start new measurement campaign
void LineSensor_StartTimer(void);

#endif
