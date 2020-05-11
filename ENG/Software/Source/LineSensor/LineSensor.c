//! @file
//! @brief TI-RSLK MSP432 Line Sensor


//-----------------------------------------------------------------------
//! $Id: LineSensor.c 159 2020-04-09 19:43:58Z UweCreutzburg $
//! $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/LineSensor/LineSensor.c $ */
//-----------------------------------------------------------------------*/


/* self */
#include "LineSensor.h"
/* Timer_A PWM Configuration Parameters */

//! Buffer
static TLineSensorBuffer aLineSensorBuffer;

//! Lookup Table based on EXCEL-Table
static const UTLineSensorPatternEvaluated aLineSensorPatternLUT[256] =
  { 0, 18, 18, 18, 17, 9, 17, 18, 16, 8, 9, 9, 17, 9, 17, 18, 16, 8, 8, 8, 8, 8, 9, 9, 16, 8, 8, 9, 17, 8, 17, 18, 19, 11, 8, 8, 8, 8, 8, 9, 8, 8, 8, 8, 8, 8, 9, 9, 19, 11, 8, 8, 8, 8, 8, 9, 19, 11, 8, 9, 16, 8, 17, 17, 12, 11, 8, 8, 8, 8, 9, 9, 11,
    8, 9, 9, 9, 9, 10, 10, 11, 8, 8, 8, 8, 8, 8, 9, 11, 8, 8, 8, 8, 8, 8, 9, 19, 11, 8, 8, 8, 8, 8, 8, 9, 8, 8, 8, 8, 8, 8, 8, 19, 11, 11, 8, 8, 8, 8, 8, 19, 11, 11, 8, 16, 8, 16, 17, 20, 8, 9, 9, 9, 9, 9, 9, 11, 9, 9, 9, 9, 9, 9, 10, 11, 11, 8, 9,
    11, 9, 9, 9, 11, 8, 11, 9, 9, 9, 9, 10, 12, 11, 11, 9, 11, 8, 8, 9, 11, 8, 8, 9, 11, 8, 9, 9, 11, 11, 11, 8, 11, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 20, 12, 11, 8, 11, 8, 8, 8, 11, 11, 8, 8, 11, 8, 9, 9, 12, 11, 11, 8, 11, 11, 11, 9, 11, 11, 11, 8,
    11, 9, 11, 9, 20, 12, 12, 11, 11, 11, 11, 8, 11, 11, 11, 11, 11, 11, 11, 8, 20, 12, 12, 11, 11, 11, 11, 11, 20, 11, 11, 11, 20, 11, 20, 24 };

//! Timer_A UpMode Configuration Parameter
const Timer_A_UpModeConfig upConfig =
  {
  TIMER_A_CLOCKSOURCE_SMCLK,              //!< SMCLK Clock Source (3MHz)
  TIMER_A_CLOCKSOURCE_DIVIDER_3,          //!< ~1us/tick
  TIMER_PERIOD,                           //!< tick period @TIMER_PERIOD [µs]
  TIMER_A_TAIE_INTERRUPT_DISABLE,         //!< Disable Timer interrupt
  TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE,     //!< Enable CCR0 interrupt
  TIMER_A_DO_CLEAR                        //!< Clear value
  };

// ################################
void LineSensor_Init(void)
{
  //! Initialize the port for all eight light reflection barriers detectors as input
  MAP_GPIO_setAsInputPin( GPIO_PORT_P7, 0xFF);           // inputs

  //! Configure the driver for the IR-LED even and switch off LED
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN3);       // IR-LED
  MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);    // IR-LED: off

#if TIRSLKMAX == 1
  //! Configure the driver for the IR-LED odd and switch off LED
  MAP_GPIO_setAsOutputPin(GPIO_PORT_P9, GPIO_PIN2);       // IR-LED
  MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2);    // IR-LED: off
#endif

  //! Configure Timer_A1 for Up Mode (periodic measurements with fixed rate)
  MAP_Timer_A_configureUpMode(TIMER_A1_BASE, &upConfig);

  //! Set Interrupt Vector and enable Interrupt for the TimerA1_0
  BSP_IntVectSet(BSP_INT_ID_TA1_0, // Set the ISR for the TimerA1_0                */
      TA1_0_IRQHandler);
  BSP_IntEn(BSP_INT_ID_TA1_0);
}

// #################################
void LineSensor_StartTimer(void)
{
  MAP_Timer_A_startCounter(TIMER_A1_BASE, TIMER_A_UP_MODE);
  // clear aLineSensorBuffer !!!
}

// #################################
TLineSensorBuffer LineSensor_GetBuffer(void)
{
  return aLineSensorBuffer;
}

// #################################
TLineSensorPatternEvaluated LineSensor_GetBufferDirection(void)
{
  uint_fast16_t uCount;
  uint_fast16_t uCountMin;
  uint_fast8_t bContinue;
  TLineSensorPatternEvaluated myCurVal;
  TLineSensorPatternEvaluated myLineSensorPatternEvaluated;

  uCountMin = 4;
  uCount = LINESENSOR_BUFFER_SIZE - 1;
  myLineSensorPatternEvaluated.aState = eLineSensor_StateBad;
  bContinue = true;
  while (bContinue)
    {
      myCurVal = aLineSensorPatternLUT[aLineSensorBuffer.samples[uCount]].aPatternEvaluated;
      switch (myCurVal.aQual)
        {
        case eLineSensor_QualGood:
          myLineSensorPatternEvaluated = myCurVal;
          myLineSensorPatternEvaluated.aState = eLineSensor_StateOK;
          myLineSensorPatternEvaluated.aError = eLineSensor_ErrorNone;
          bContinue = false;
          break;
        case eLineSensor_QualPoor:
          myLineSensorPatternEvaluated = myCurVal;
          myLineSensorPatternEvaluated.aState = eLineSensor_StateOK;
          myLineSensorPatternEvaluated.aError = eLineSensor_ErrorNone;
          uCount--;
          bContinue = (uCount >= uCountMin);
          break;
        case eLineSensor_QualAllBlack:
          if (myLineSensorPatternEvaluated.aState == eLineSensor_StateBad)
            {
              myLineSensorPatternEvaluated = myCurVal;
              myLineSensorPatternEvaluated.aError = eLineSensor_ErrorNone;
            }
          uCount--;
          bContinue = (uCount >= uCountMin);
          break;
        case eLineSensor_QualAllWhite:
          if (myLineSensorPatternEvaluated.aState == eLineSensor_StateBad)
            {
              myLineSensorPatternEvaluated = myCurVal;
              myLineSensorPatternEvaluated.aError = eLineSensor_ErrorNone;
            }
          uCount--;
          bContinue = (uCount >= uCountMin);
          break;
        default:    // should never happen ....
          myLineSensorPatternEvaluated.aError = eLineSensor_ErrorDefault;
          bContinue = false;
          break;
        }

    }

  return myLineSensorPatternEvaluated;
}

// #################################
/*
 * According to the uC/OS III book, the active code for the ISR eventually needs enhancement as shown below...
 *
    #if (APP_TIMER_INT_PROCESSING == 1)
      CPU_SR_ALLOC();
      CPU_CRITICAL_ENTER();
      OSIntEnter();
      CPU_CRITICAL_EXIT();
    #endif

      ... do something

      // inform consumer task
      OSTaskSemPost( &AppTaskTimerSyncTCB,
                     OS_OPT_POST_NONE,
                     &err );

      ... do something

    #if (APP_TIMER_INT_PROCESSING == 1)
      OSIntExit();
    #endif

*/

void TA1_0_IRQHandler(void)
{
  static int16_t uTickCount = 0;

#ifdef LINESENSOR_USE_DEBUG_LED
  LINESENSOR_DEBUG_LED_ON;
#endif

  MAP_Timer_A_clearCaptureCompareInterrupt(TIMER_A1_BASE, TIMER_A_CAPTURECOMPARE_REGISTER_0);

  if (uTickCount == 0)
    {
      MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, 0xFF);              // (dis-) charge capacitor of sensor array
      MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P7, 0xFF);          // (dis-) charge capacitor of sensor array
      MAP_GPIO_setDriveStrengthHigh(GPIO_PORT_P7, 0xFF);
    }

  else if (uTickCount == 1)
    {
      MAP_GPIO_setAsInputPin(GPIO_PORT_P7, 0xFF);               // change P7 direction
      MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN3);     // IR-LED: on
#if TIRSLKMAX == 1
      MAP_GPIO_setOutputHighOnPin(GPIO_PORT_P9, GPIO_PIN2);     // IR-LED: on
#endif
      aLineSensorBuffer.samples[uTickCount - 1] = P7IN;       // read first sample and save value
    }

  else if ((uTickCount > 0) && (uTickCount < LINESENSOR_BUFFER_SIZE))
    {
      aLineSensorBuffer.samples[uTickCount - 1] = P7IN;         // read sample(s) and save value(s)
    }

  else if (uTickCount == LINESENSOR_BUFFER_SIZE)
    {
      aLineSensorBuffer.samples[uTickCount - 1] = P7IN;         // read last sample and save value
      MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN3);      // IR-LED: off
#if TIRSLKMAX == 1
      MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, GPIO_PIN2);      // IR-LED: off
#endif
      MAP_Timer_A_stopTimer(TIMER_A1_BASE);
      uTickCount = 0xFFFF;
    }

  uTickCount++;

#ifdef LINESENSOR_USE_DEBUG_LED
  LINESENSOR_DEBUG_LED_OFF;
#endif

}

