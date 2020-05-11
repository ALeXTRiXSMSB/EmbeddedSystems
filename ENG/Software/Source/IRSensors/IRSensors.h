// *******************************************************************************
// TI-RSLK MSP432 IR-Sensor Module
//
//  Sensor Module
//  P9.1            Left    (A16)
//  P4.1            Center  (A12)
//  P9.0            Right   (A17)
//
//  Internal resources
//  ADC14
//
// Description: .
//
//                 MSP432P401
//              ------------------
//          /|\|                  |
//           | |                  |
//           --|RST               |<--3- IR-Sensor
//             |                  |
//             |                  |
//             |                  |
//             |                  |
//
//

//-----------------------------------------------------------------------
// Identifier    $Id: IRSensors.h 100 2020-01-05 20:30:24Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/IRSensors/IRSensors.h $ */
//-----------------------------------------------------------------------*/

#ifndef IRSENSORS_H
#define IRSENSORS_H

/* DriverLib Includes */
#include "driverlib.h"
#include "LPF_IIR_1st_order.h"

/* board support package (Interrupts) */
#include "bsp_int.h"

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>

#define IRSENSORS_USE_DEBUG_LED
//#define IRSENSORS_DEBUG_LED_ON MAP_GPIO_setOutputHighOnPin(MOTOR_USE_DEBUG_LED_PORT, MOTOR_USE_DEBUG_LED_PIN)
//#define IRSENSORS_DEBUG_LED_OFF MAP_GPIO_setOutputLowOnPin(MOTOR_USE_DEBUG_LED_PORT, MOTOR_USE_DEBUG_LED_PIN)
#define IRSENSORS_DEBUG_LED_ON  P2OUT |= BIT0
#define IRSENSORS_DEBUG_LED_OFF P2OUT &= ~BIT0


// linearization from excel solver for dist = a/(ADC + b) + c
#define IRSENSORS_CONVERT_A (1243936l)
#define IRSENSORS_CONVERT_B (-695l)
#define IRSENSORS_CONVERT_C (-16l)

// Helpers for accessing the Sensors
typedef enum _TIRSensors_PositionsEnum
{
    eIRSensors_PositionLeft = 0,
    eIRSensors_PositionCenter = 1,
    eIRSensors_PositionRight = 2
} TIRSensors_PositionEnum_t;


// Buffer for external usage via *_GetBuffer()
typedef struct _TIRSensorsBuffer
{
    uint16_t samples[3];        // ADC buffer
    uint16_t samples_pt1[3];    // LP-filtered samples
} TIRSensorsBuffer;

typedef struct _TIRSensorsDistance
{
    uint16_t distance_pt1_mm[3]; // LP-filtered distances [mm]
} TIRSensorsDistance;


// Procedures
extern void IRSensors_Init(void);
extern TIRSensorsBuffer IRSensors_GetBuffer(void);
extern TIRSensorsDistance IRSensors_GetDistanceMilliMeters(void);
extern void IRSensors_StartConversion(void);

extern void ADC14_IRQHandler(void);

#endif
