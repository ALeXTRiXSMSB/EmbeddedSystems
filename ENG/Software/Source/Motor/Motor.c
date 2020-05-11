//-----------------------------------------------------------------------
//! Identifier    $Id: Motor.c 162 2020-04-10 18:48:59Z UweCreutzburg $
//! Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/Motor/Motor.c $ */
//-----------------------------------------------------------------------*/

/* self */
#include "Motor.h"

//! Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig pwmConfig =
  {
  TIMER_A_CLOCKSOURCE_SMCLK,              //!< SMCLK Clock Source (3MHz)
  TIMER_A_CLOCKSOURCE_DIVIDER_1,          //!< ~3us/tick
  TIMER_PWM_CCR0,                         //!< tick period
  TIMER_A_TAIE_INTERRUPT_DISABLE,         //!< Disable Timer interrupt
  TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,     //!< Enable CCR0 interrupt
  TIMER_A_DO_CLEAR                        //!< Clear value
  };

static Timer_A_CompareModeConfig pwmConfigs[2] =
  {
    {
    //!< TA0.4 (P2.7, PWML)
    TIMER_A_CAPTURECOMPARE_REGISTER_4,
      TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
      TIMER_A_OUTPUTMODE_RESET_SET,
      MOTOR_STANDSTILL_LIMIT   //!< initial value ensures a spinning motor
    },
    {
    //!< TA0.3 (P2.6, PWMR)
    TIMER_A_CAPTURECOMPARE_REGISTER_3,
    TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE,
    TIMER_A_OUTPUTMODE_RESET_SET,
    MOTOR_STANDSTILL_LIMIT   //!< initial value ensures a spinning motor
    }
  };

//! Capture TA3
static const Timer_A_CaptureModeConfig captureModeConfigs[2] =
  {
    {
#if TIRSLKMAX == 1
     //!< Timer_A Capture Mode Configuration Parameter P10.5 (TA3.1-TA3 CCR2 capture: CCI1A input)
     TIMER_A_CAPTURECOMPARE_REGISTER_1,        //!< CC Register
#else
     //!< Timer_A Capture Mode Configuration Parameter P8.2 (TA3.2-TA3 CCR2 capture: CCI2A input, compare: Out2)
    TIMER_A_CAPTURECOMPARE_REGISTER_2,        //!< CC Register
#endif
    TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE,          //!< Rising & Falling Edge
    TIMER_A_CAPTURE_INPUTSELECT_CCIxA,        //!< CCIxA Input Select
    TIMER_A_CAPTURE_SYNCHRONOUS,              //!< Synchronized Capture
    TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,  //!< Enable interrupt
    TIMER_A_OUTPUTMODE_OUTBITVALUE            //!< Output bit value
    },
    {
    //!< Timer_A Capture Mode Configuration Parameter P10.4 (PIN24, TA3.0-TA3 CCR0 capture: CCI0A input, compare: Out0)
    TIMER_A_CAPTURECOMPARE_REGISTER_0,        //!< CC Register
    TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE,          //!< Rising & Falling Edge
    TIMER_A_CAPTURE_INPUTSELECT_CCIxA,        //!< CCIxA Input Select
    TIMER_A_CAPTURE_SYNCHRONOUS,              //!< Synchronized Capture
    TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE,  //!< Enable interrupt
    TIMER_A_OUTPUTMODE_OUTBITVALUE            //!< Output bit value
    }
  };

//! Interface
static TMotorBuffer aMotorBuffers[2];

//! internal motor controller data
static TMotorController aMotorControllers[2] =
  {
    {  //!< for eMotor_PositionLeft
    (int32_t) MOTOR_PGAIN,                    //!< PGain
    (int32_t) MOTOR_IGAIN,                    //!< IGain
    0,                                        //!< Initial State
    0                                         //!< PWM Capture Compare Value (CCRx)
    },
    {  //!< for eMotor_PositionRight
    (int32_t) MOTOR_PGAIN,                    //!< PGain
    (int32_t) MOTOR_IGAIN,                    //!< IGain
    0,                                        //!< Initial State
    0                                         //!< PWM Capture Compare Value (CCRx)
    }
  };

// ####################################################
void Motor_Init(void)
{
  //! TA0: Continuous Mode Configuration Parameter (PWM)
  const Timer_A_ContinuousModeConfig continuousModeConfig =
    {
    TIMER_A_CLOCKSOURCE_SMCLK,                 //!< SMCLK Clock Source
    TIMER_A_CLOCKSOURCE_DIVIDER_8,             //!< SMCLK = 3MHz; SMCLK/8 = 375kHz --> 2.666µs
    TIMER_A_TAIE_INTERRUPT_DISABLE,            //!< Disable Timer ISR
    TIMER_A_SKIP_CLEAR                         //!< Skip Clear Counter
    };

  //! Initialize Control Outputs
  Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
  Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
  Motor_SetState(eMotor_PositionLeft, eMotor_StateSleep);
  Motor_SetState(eMotor_PositionRight, eMotor_StateSleep);

  //! Motor Direction
#if TIRSLKMAX == 1
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN4); // DIRLmax
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN5); // DIRRmax
#else
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN6); // DIRL ??? check
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN7); // DIRR ??? check
#endif

  //! Motor Sleep mode
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN6); // SLPR
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN7); // SLPL

  //! TA0: Configuring GPIO2.6/7 as peripheral output for PWM
  MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN6, GPIO_PRIMARY_MODULE_FUNCTION); // PWMR
  MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION); // PWML

  //! TA0: Configuring to have a period of approximately 1ms
  MAP_Timer_A_configureUpMode(TIMER_PWM_BASE, &pwmConfig);

  //! TA0: Initialize compare registers to generate PWM
  MAP_Timer_A_initCompare(TIMER_PWM_BASE, &pwmConfigs[eMotor_PositionLeft]);
  MAP_Timer_A_initCompare(TIMER_PWM_BASE, &pwmConfigs[eMotor_PositionRight]);

  //! TA0: Set Interrupt Vector and Enable interrupt for CCR0 (Set the CCRx for the motors)
  BSP_IntVectSet(BSP_INT_ID_TA0_0, TA0_0_IRQHandler);
  BSP_IntEn(BSP_INT_ID_TA0_0);
  // obsolete: MAP_Interrupt_enableInterrupt(INT_TA0_0);

  //! TA0: Start PWM-Timer
  MAP_Timer_A_startCounter(TIMER_PWM_BASE, TIMER_A_UP_MODE);

  //! TA3: Configure Capture Input
#if TIRSLKMAX == 1
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10, GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);
#else
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P8, GPIO_PIN2, GPIO_PRIMARY_MODULE_FUNCTION);
#endif
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P10, GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);

  //! TA3: Configuring Capture Mode
  MAP_Timer_A_initCapture(TIMER_CAP_BASE, &captureModeConfigs[eMotor_PositionLeft]);
  MAP_Timer_A_initCapture(TIMER_CAP_BASE, &captureModeConfigs[eMotor_PositionRight]);

  //! TA3: Configuring Continuous Mode & Interrupts for motor sensors
  MAP_Timer_A_configureContinuousMode(TIMER_CAP_BASE, &continuousModeConfig);

  //! TA3_N: Set Interrupt Vector and enable interrupt
  BSP_IntVectSet(BSP_INT_ID_TA3_N, TA3_N_IRQHandler);
  BSP_IntEn(BSP_INT_ID_TA3_N);
  // obsolete: MAP_Interrupt_enableInterrupt(INT_TA3_N);

  //! TA3_0: Set Interrupt Vector and enable interrupt
  BSP_IntVectSet(BSP_INT_ID_TA3_0, TA3_0_IRQHandler);
  BSP_IntEn(BSP_INT_ID_TA3_0);
  // obsolete: MAP_Interrupt_enableInterrupt(INT_TA3_0);

  //! TA3: Start Continuous Mode
  MAP_Timer_A_startCounter(TIMER_CAP_BASE, TIMER_A_CONTINUOUS_MODE);
}

// ####################################################
void Motor_SetDesiredPeriod(TMotor_PositionEnum_t myMotorPosition, uint16_t aDesiredPeriod)
{
  aMotorBuffers[myMotorPosition].aPeriodSensorDesired = aDesiredPeriod;   //!< to reflect in buffer
}

// ####################################################
void Motor_SetCompareValue(TMotor_PositionEnum_t myMotorPosition, uint16_t aCompareValue)
{
  pwmConfigs[myMotorPosition].compareValue = aCompareValue;  //!< used to set duty cycle
  aMotorBuffers[myMotorPosition].aCompareValue = aCompareValue;    //!< to reflect in buffer
}

// ####################################################
extern uint16_t Motor_GetCompareValue(TMotor_PositionEnum_t myMotorPosition)
{
  return pwmConfigs[myMotorPosition].compareValue;
}

// ####################################################
void Motor_SetState(TMotor_PositionEnum_t myMotorPosition, TMotor_StateEnum_t myMotorState)
{
  aMotorBuffers[myMotorPosition].aState = myMotorState;

  switch (myMotorPosition)
    {
    case eMotor_PositionLeft:
      if (myMotorState == eMotor_StateActive)
        {
          MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN7);
        }
      else if (myMotorState == eMotor_StateSleep)
        {
          MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN7);
        }
      break;
    case eMotor_PositionRight:
      if (myMotorState == eMotor_StateActive)
        {
          MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P3, GPIO_PIN6);
        }
      else if (myMotorState == eMotor_StateSleep)
        {
          MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN6);
        }
      break;
    }
}

// ####################################################
extern TMotor_DirectionEnum_t Motor_GetDirection(TMotor_PositionEnum_t myMotorPosition)
{
  return aMotorBuffers[myMotorPosition].aDirection;
}

// ####################################################
void Motor_SetDirection(TMotor_PositionEnum_t myMotorPosition, TMotor_DirectionEnum_t myDirection)
{

  aMotorBuffers[myMotorPosition].aDirection = myDirection;

  switch (myMotorPosition)
    {
    case eMotor_PositionLeft:
      if (myDirection == eMotor_DirectionAhead)
        {
#if TIRSLKMAX == 1
          MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN4);
#else
          MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN7); // ??? check
#endif
        }
      else if (myDirection == eMotor_DirectionReverse)
        {
#if TIRSLKMAX == 1
          MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN4);
#else
          MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN7); // ??? check
#endif
        }
      break;
    case eMotor_PositionRight:
      if (myDirection == eMotor_DirectionAhead)
        {
#if TIRSLKMAX == 1
          MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN5);
#else
          MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN6); // ??? check
#endif
        }
      else if (myDirection == eMotor_DirectionReverse)
        {
#if TIRSLKMAX == 1
          MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN5);
#else
          MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN6); // ??? check
#endif
        }
      break;
    }
}

// ####################################################
//! Motor_Controller calculates the new values for PWM CCRx units.
//! These new values will be set active by CCR0 TA0_0_IRQHandler (TAR == small)
void Motor_ControllerStep(void)
{
  int32_t iError, iP, iI;
  TMotor_PositionEnum_t aCountMotors;

  for (aCountMotors = eMotor_PositionLeft; aCountMotors <= eMotor_PositionRight; aCountMotors++)
    {
      if (aMotorBuffers[aCountMotors].aSpinning && (aMotorBuffers[aCountMotors].aState == eMotor_StateActive))
        {   //!< the motor controller gets executed only if the motor is spinning
          iError = aMotorBuffers[aCountMotors].aPeriodSensorMeasRaw - aMotorBuffers[aCountMotors].aPeriodSensorDesired;   //!< positive value means speed is too low

          iP = iError * aMotorControllers[aCountMotors].aPGain;
          iI = iError * aMotorControllers[aCountMotors].aIGain + aMotorControllers[aCountMotors].aIState;

          //!< some heuristic limitations
          if (iI > (TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2))
            iI = TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2;
          if (iI < -(TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2))
            iI = -TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2;

          //! first guess of new CCR value
          aMotorControllers[aCountMotors].aCompareValue = (iP + iI) >> MOTOR_CONTROLLER_PARAMETER_SCALING_SHIFT; //!< adjust for RWV-scaling

          //!< over all saturation
          if (aMotorControllers[aCountMotors].aCompareValue > TIMER_PWM_CCRX_MAX)
            aMotorControllers[aCountMotors].aCompareValue = TIMER_PWM_CCRX_MAX;
          if (aMotorControllers[aCountMotors].aCompareValue < TIMER_PWM_CCRX_MIN)
            aMotorControllers[aCountMotors].aCompareValue = TIMER_PWM_CCRX_MIN;

          Motor_SetCompareValue(aCountMotors, aMotorControllers[aCountMotors].aCompareValue);
          aMotorControllers[aCountMotors].aIState = iI;                                    //!< save new state for the motor controller
        }
      else
        {
          aMotorControllers[aCountMotors].aIState = 0;                                     //!< reset internal state of motor controller
        }

    }

}

// ####################################################
//! Motor_Controller calculates the new values for PWM CCRx units.
//! These new values will be set active by CCR0 TA0_0_IRQHandler (TAR == small)
void Motor_ControllerRaw(TMotor_PositionEnum_t myMotorPosition, uint16_t desiredPeriod)
{
  int32_t iError, iP, iI;

  if (aMotorBuffers[myMotorPosition].aSpinning)
    {   //!< the motor controller gets executed only if the motor is spinning
      iError = aMotorBuffers[myMotorPosition].aPeriodSensorMeasRaw - desiredPeriod;   //!< positive value means speed is too low

      iP = iError * aMotorControllers[myMotorPosition].aPGain;
      iI = iError * aMotorControllers[myMotorPosition].aIGain + aMotorControllers[myMotorPosition].aIState;

      //!< some heuristic limitations
      if (iI > (TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2))
        iI = TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2;
      if (iI < -(TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2))
        iI = -TIMER_PWM_CCR0 * MOTOR_CONTROLLER_PARAMETER_SCALING / 2;

      //! first guess of new CCR value
      aMotorControllers[myMotorPosition].aCompareValue = (iP + iI) >> MOTOR_CONTROLLER_PARAMETER_SCALING_SHIFT; //!< adjust for RWV-scaling

      //!< over all saturation
      if (aMotorControllers[myMotorPosition].aCompareValue > TIMER_PWM_CCRX_MAX)
        aMotorControllers[myMotorPosition].aCompareValue = TIMER_PWM_CCRX_MAX;
      if (aMotorControllers[myMotorPosition].aCompareValue < TIMER_PWM_CCRX_MIN)
        aMotorControllers[myMotorPosition].aCompareValue = TIMER_PWM_CCRX_MIN;

      Motor_SetCompareValue(myMotorPosition, aMotorControllers[myMotorPosition].aCompareValue);
      aMotorControllers[myMotorPosition].aIState = iI;                                    //!< save new state for the motor controller
    }
  else
    {
      aMotorControllers[myMotorPosition].aIState = 0;                                     //!< reset internal state of motor controller
    }
}

// ####################################################
TMotorBuffer Motor_GetBuffer(TMotor_PositionEnum_t myMotorPosition)
{
  return aMotorBuffers[myMotorPosition];
}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//! ISR TA3_N_IRQHandler: Sensor Left Motor
void TA3_N_IRQHandler(void)
{
  static uint_fast16_t uiTickOld = 0;
  uint_fast16_t uiTickNew;

#ifdef MOTOR_USE_DEBUG_LED
  MOTOR_DEBUG_LED_ON;
#endif

#if TIRSLKMAX == 1
  //! handle capture
  uiTickNew = MAP_Timer_A_getCaptureCompareCount(TIMER_CAP_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_1);
  MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_CAP_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_1);
#else
  //! handle capture
  uiTickNew = MAP_Timer_A_getCaptureCompareCount(TIMER_CAP_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_2);
  MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_CAP_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_2);
#endif

  //! update buffer
  aMotorBuffers[eMotor_PositionLeft].aPeriodSensorMeasRaw = uiTickNew - uiTickOld;
  aMotorBuffers[eMotor_PositionLeft].aPosition += aMotorBuffers[eMotor_PositionLeft].aDirection;

  //! save current value
  uiTickOld = uiTickNew;

#ifdef MOTOR_USE_DEBUG_LED
  MOTOR_DEBUG_LED_OFF;
#endif

}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//! ISR TA3_0_IRQHandler: Sensor Right Motor
void TA3_0_IRQHandler(void)
{
  static uint_fast16_t uiTickOld = 0;
  uint_fast16_t uiTickNew;

#ifdef MOTOR_USE_DEBUG_LED
  MOTOR_DEBUG_LED_ON;
#endif

  //! handle capture
  uiTickNew = MAP_Timer_A_getCaptureCompareCount(TIMER_CAP_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);
  MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_CAP_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

  //! update buffer
  aMotorBuffers[eMotor_PositionRight].aPeriodSensorMeasRaw = uiTickNew - uiTickOld;
  aMotorBuffers[eMotor_PositionRight].aPosition += aMotorBuffers[eMotor_PositionRight].aDirection;

  //! save current value
  uiTickOld = uiTickNew;

#ifdef MOTOR_USE_DEBUG_LED
  MOTOR_DEBUG_LED_OFF;
#endif

}

// XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
//! ISR TA0_0_IRQHandler: PWM CCR0 Interrupt (set new duty cycle, standstill detection)
void TA0_0_IRQHandler(void)
{
  static int32_t iMotorOldPosition[2];
  static int32_t iMotorOldPositionCount[2];
  TMotor_PositionEnum_t uiCountMotors;

#ifdef MOTOR_USE_DEBUG_LED
  MOTOR_DEBUG_LED_ON;
#endif

  MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_PWM_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);    //!< clear interrupt flag

  //! update TACCRx to last values calculated by "Motor_ControllerRaw"
  for (uiCountMotors = eMotor_PositionLeft; uiCountMotors <= eMotor_PositionRight; uiCountMotors++)
    {
      MAP_Timer_A_setCompareValue( TIMER_PWM_BASE, pwmConfigs[uiCountMotors].compareRegister, pwmConfigs[uiCountMotors].compareValue);
    }

  //! standstill detection/handling
  for (uiCountMotors = eMotor_PositionLeft; uiCountMotors <= eMotor_PositionRight; uiCountMotors++)
    {
      if ((aMotorBuffers[uiCountMotors].aPosition == iMotorOldPosition[uiCountMotors]) && (aMotorBuffers[uiCountMotors].aState == eMotor_StateActive))
        {   //!< motor eventually not spinning
          iMotorOldPositionCount[uiCountMotors]++;                                                //!< fault counter
          if (iMotorOldPositionCount[uiCountMotors] > MOTOR_STANDSTILL_LIMIT)
            {   //!< fault counter exceeds predefined limit
              aMotorBuffers[uiCountMotors].aSpinning = false;                                     //!< indicate motor is not spinning anymore
              aMotorControllers[uiCountMotors].aIState = 0;                                       //!< reset internal state of motor controller
              if (aMotorControllers[uiCountMotors].aCompareValue > (MOTOR_STANDSTILL_PWM_MAX + MOTOR_STANDSTILL_PWM_DELTA))
                {
                  aMotorControllers[uiCountMotors].aCompareValue = MOTOR_STANDSTILL_PWM_MAX;
                }
              if (aMotorControllers[uiCountMotors].aCompareValue <= MOTOR_STANDSTILL_PWM_MAX)
                {
                  aMotorControllers[uiCountMotors].aCompareValue += MOTOR_STANDSTILL_PWM_DELTA;        //!< try increased motor moment
                  Motor_SetCompareValue(uiCountMotors, aMotorControllers[uiCountMotors].aCompareValue); //!< set duty cycle indirectly to enable motor start in next cycle
                }
            }
        }
      else
        {   //!< motor is spinning correctly
          iMotorOldPositionCount[uiCountMotors] = 0;
          iMotorOldPosition[uiCountMotors] = aMotorBuffers[uiCountMotors].aPosition;
          aMotorBuffers[uiCountMotors].aSpinning = true;
        }
    }

#ifdef MOTOR_USE_DEBUG_LED
  MOTOR_DEBUG_LED_OFF;
#endif

}
