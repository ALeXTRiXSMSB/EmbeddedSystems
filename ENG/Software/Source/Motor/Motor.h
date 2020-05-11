//! @file
//! @brief TI-RSLK MSP432 Motor Control
//!
//!  Motor Module \n
//!  P1.6            Right Motor Direction \n
//!  P3.6            Right Motor Sleep \n
//!  P2.6/PM_TA0.3   Right Motor PWM \n
//! \n
//!  P1.7            Left Motor Direction \n
//!  P3.7            Left Motor Sleep \n
//!  P2.7/PM_TA0.4   Left Motor PWM \n
//! \n
//!  Sensor Module \n
//!  P8.2/TA3CCP2    ELA OUT A Left Encoder A \n
//!  P9.2/GPIO       ELB OUT B Left Encoder B (unused) \n
//!  P10.4/TA3CCP0   ERA OUT A Right Encoder A \n
//!  P10.5/GPIO      ERB OUT B Right Encoder B (unused) \n
//! \n
//!  Internal resources \n
//!  TimerA0         PWM \n
//!  TimerA3         Encoder (Capture & IRQ) \n
//! \n
//! Description: .
//!
//!                 MSP432P401
//!              ------------------
//!          /|\|                  |
//!           | |                  |
//!           --|RST               |<--2- Encoder
//!             |                  |
//!             |                  |
//!             |                  |--2-> PWM
//!             |                  |--2-> Direction
//!             |                  |--2-> Sleep
//!             |                  |
//!             |                  |
//!
//!

//-----------------------------------------------------------------------

//! Identifier    $Id: Motor.h 159 2020-04-09 19:43:58Z UweCreutzburg $
//! Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/Motor/Motor.h $ */
//-----------------------------------------------------------------------*/

#ifndef MOTOR_H
#define MOTOR_H


//! conditional compilation: use LED to show activity
#define MOTOR_USE_DEBUG_LED                                                                 // for debugging purposes
#define MOTOR_DEBUG_LED_ON  P2OUT |= BIT0
#define MOTOR_DEBUG_LED_OFF P2OUT &= ~BIT0


//! PWM Timer parameters
#define TIMER_PWM_CCR0 (3000)                                                               //!< 1ms period of PWM (x 1/3MHz)
#define TIMER_PWM_CCRX_MAX (TIMER_PWM_CCR0 - TIMER_PWM_CCR0/4)                              //!< max. limit for desired duty cycle (75%)
#define TIMER_PWM_CCRX_MIN (100)                                                            //!< min. limit for desired duty cycle (
#define TIMER_PWM_BASE (TIMER_A0_BASE)

//! Capture (Speed) parameters
#define TIMER_CAP_BASE (TIMER_A3_BASE)

//! Fixed-point math: A real world value of 1.0 is represented by 1 * 2^MOTOR_CONTROLLER_PARAMETER_SCALING_SHIFT (e.g. 32768)
#define MOTOR_PGAIN_FLOAT (0.005)                                                           //!< P-Gain: RWW (real-world-value)
#define MOTOR_IGAIN_FLOAT (0.004)                                                           //!< I-Gain: RWW
#define MOTOR_CONTROLLER_PARAMETER_SCALING_SHIFT (15)                                       //!< ENC_M15: internal resolution for fixed point processing
#define MOTOR_CONTROLLER_PARAMETER_SCALING (2 << MOTOR_CONTROLLER_PARAMETER_SCALING_SHIFT)  //!< ENC_M15: pre-calculated helper
#define MOTOR_PGAIN (MOTOR_PGAIN_FLOAT*MOTOR_CONTROLLER_PARAMETER_SCALING)                  //!< P-Gain: stored integer
#define MOTOR_IGAIN (MOTOR_IGAIN_FLOAT*MOTOR_CONTROLLER_PARAMETER_SCALING)                  //!< I-Gain: stored integer

//! Standstill detection parameters
#define MOTOR_STANDSTILL_LIMIT (50)                                                         //!< ~100ms gate for no motor movement detected
#define MOTOR_STANDSTILL_PWM_DELTA (4)                                                      //!< increment for increasing duty cycle
#define MOTOR_STANDSTILL_PWM_MAX   (TIMER_PWM_CCR0/8)                                       //!< limited torque for blocked wheel


/*! DriverLib Includes */
#include "driverlib.h"

#include "bsp_int.h"
#include "Board.h"

/*! Standard Includes */
#include <stdint.h>
#include <stdbool.h>

typedef enum _TMotor_DirectionEnum
{
    eMotor_DirectionAhead = 1,      //!< ahead
    eMotor_DirectionReverse = -1    //!< reverse
} TMotor_DirectionEnum_t;

typedef enum _TMotor_StateEnum
{
    eMotor_StateSleep = 0,          //!< motor sleeps
    eMotor_StateActive = 1          //!< motor active
} TMotor_StateEnum_t;

typedef enum _TMotor_PositionEnum
{
    eMotor_PositionLeft = 0,        //!< left motor
    eMotor_PositionRight = 1        //!< left motor
} TMotor_PositionEnum_t;

//! Buffer for external usage via *_GetBuffer()
typedef struct _TMotorBuffer
{
    int32_t  aPosition;                     //!< current position reached derived from motor sensor
    uint16_t aCompareValue;                 //!< current PWM-value derived by controller
    uint16_t aPeriodSensorMeasRaw;          //!< measured ticks of the motor sensor between two calls of the control loop
    uint16_t aPeriodSensorDesired;          //!< desired ticks of the motor sensor between two calls of the control loop
    TMotor_DirectionEnum_t  aDirection;     //!< direction of mototr movement
    TMotor_StateEnum_t      aState;         //!< current state of the motor
    TMotor_PositionEnum_t   aMotor;         //!< position of the motor (which motor?)
    uint8_t  aSpinning;                     //!< is motor currently spinning?
} TMotorBuffer;

//! parameters and states of the PI-controller
typedef struct _TMotorController
{
    int32_t  aPGain;                        //!< Proportional gain
    int32_t  aIGain;                        //!< Integrator gain
    int32_t  aIState;                       //!< Integrator state
    int32_t  aCompareValue;                 //!< calculated CCRx value for the controller
} TMotorController;

//! @brief IRQ-Handlers (used in "startup_msp432p401_ccs.c")

//! ISR TA3_N_IRQHandler: Sensor Left Motor
extern void TA0_0_IRQHandler(void);

//! ISR TA3_N_IRQHandler: Sensor Right Motor
extern void TA3_N_IRQHandler(void);

//! ISR TA3_N_IRQHandler: PWM Controller and Standstill Detection
extern void TA3_0_IRQHandler(void);

//! external interface of the module

//! Initialize
extern void Motor_Init(void);

//! Set PWMRaw for one specific motor
extern void Motor_SetCompareValue( TMotor_PositionEnum_t myMotorPosition, uint16_t aCompareValue );

//! Set State of motor (sleep/active)
extern void Motor_SetState( TMotor_PositionEnum_t myMotorPosition,  TMotor_StateEnum_t myMotorState );

//! Get current CCR value
extern uint16_t Motor_GetCompareValue( TMotor_PositionEnum_t myMotorPosition );

//! Get current Direction
extern TMotor_DirectionEnum_t Motor_GetDirection( TMotor_PositionEnum_t myMotorPosition );

//! Set direction for a specific motor
extern void Motor_SetDirection( TMotor_PositionEnum_t myMotorPosition, TMotor_DirectionEnum_t myDirection );

//! Set desired motor sensor period
extern void Motor_SetDesiredPeriod( TMotor_PositionEnum_t myMotorPosition, uint16_t aDesiredPeriod );

//! PI-Controller for the motor (works on data prior set by )
extern void Motor_ControllerStep(void);

//! PI-Controller for the motor
extern void Motor_ControllerRaw( TMotor_PositionEnum_t myMotorPosition, uint16_t desiredPeriod );

//! Get current state of the module (eventually used in main loop)
extern TMotorBuffer Motor_GetBuffer( TMotor_PositionEnum_t myMotorPosition );

#endif
