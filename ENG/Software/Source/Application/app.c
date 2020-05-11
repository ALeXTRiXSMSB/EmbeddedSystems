/*
 *********************************************************************************************************
 *                                              EXAMPLE CODE
 *
 *                          (c) Copyright 2003-2017 Micrium, Inc.; Weston, FL
 *
 *               All rights reserved.  Protected by international copyright laws.
 *               Knowledge of the source code may NOT be used to develop a similar product.
 *               Please help us continue to provide the Embedded community with the finest
 *               software available.  Your honesty is greatly appreciated.
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *
 *                                            EXAMPLE CODE
 *
 *                                             TI MSP432
 *                                              on the
 *                                         TI MSP-EXP432P401R
 *                                      LaunchPad Development Kit
 *
 * Filename      : app.c
 * Version       : V1.00
 * Programmer(s) : YS
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                             INCLUDE FILES
 *********************************************************************************************************
 */

#include  <cpu_core.h>
#include  <os.h>
#include  <os_cfg.h>
#include  <bsp.h>
#include  <bsp_int.h>
#include  <bsp_sys.h>
#include  <app_cfg.h>
#include  <cpu_cfg.h>
#include  <lib_mem.h>
#include  <lib_math.h>
#include  <stdio.h>
#include  <msp432.h>
#include "Board.h"
#include "LineSensor.h"
#include "Motor.h"
#include "IRSensors.h"
#include "Bumpers.h"
#include "simplelink.h"


/*
 *********************************************************************************************************
 *                                            LOCAL DEFINES
 *********************************************************************************************************
 */

/*
 *********************************************************************************************************
 *                                          GLOBAL VARIABLES
 *********************************************************************************************************
 */

CPU_INT16U App_LED1_Dly_ms; /* Startup Task Delay. Global variable.                 */

/*
 *********************************************************************************************************
 *                                       LOCAL GLOBAL VARIABLES
 *********************************************************************************************************
 */

static OS_TCB App_TaskStartTCB;
static OS_TCB App_TaskSensorsTCB;

static CPU_STK_SIZE App_TaskStartStk[APP_CFG_TASK_START_STK_SIZE];
static CPU_STK_SIZE App_TaskSensorsStk[APP_CFG_TASK_START_STK_SIZE];

static CPU_INT16U RGB_Ctr;

// debug
TLineSensorPatternEvaluated myCurVal;

/*
 *********************************************************************************************************
 *                                      LOCAL FUNCTION PROTOTYPES
 *********************************************************************************************************
 */

static void App_ObjCreate(void);
static void App_TaskCreate(void);
static void App_TaskStart(void *p_arg);
static void App_TaskSensors(void *p_arg);

/*
 *********************************************************************************************************
 *                                                main()
 *
 * Description : This is the standard entry point for C code.  It is assumed that your code will call
 *               main() once you have performed all necessary initialization.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This the main standard entry point.
 *
 * Note(s)     : none.
 *********************************************************************************************************
 */

int main(void)
{
  OS_ERR err;

  Mem_Init(); /* Initialize Memory Management Module                  */
  CPU_IntDis(); /* Disable all Interrupts.                              */

  OSInit(&err); /* Init uC/OS-III.                                      */

  OSTaskCreate((OS_TCB *) &App_TaskStartTCB, /* Create the start task                                */
               (CPU_CHAR *) "App Task Start",
               (OS_TASK_PTR) App_TaskStart,
               (void *) 0,
               (OS_PRIO) APP_CFG_TASK_START_PRIO,
               (CPU_STK *) &App_TaskStartStk[0],
               (CPU_STK) (APP_CFG_TASK_START_STK_SIZE / 10u),
               (CPU_STK_SIZE) APP_CFG_TASK_START_STK_SIZE,
               (OS_MSG_QTY) 0,
               (OS_TICK) 0,
               (void *) 0,
               (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR ),
               (OS_ERR *) &err);

  OSStart(&err); /* Start multitasking (i.e. give control to uC/OS-III). */
}

/*
 *********************************************************************************************************
 *                                          App_TaskStart()
 *
 * Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
 *               initialize the ticker only once multitasking has started.
 *
 * Argument(s) : p_arg   is the argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is a task.
 *
 * Notes       : (1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
 *                   used.  The compiler should not generate any code for this statement.
 *********************************************************************************************************
 */

static void App_TaskStart(void *p_arg)
{
  OS_ERR os_err;

  (void) p_arg; /* See Note #1                                          */

  BSP_Init(); /* Start BSP and tick initialization                    */
  BSP_Tick_Init();

  Board_Init();

  Math_Init();

  // uc/probe requirements (s. p. 123)
  OSStatTaskCPUUsageInit(&os_err);
  OSStatReset(&os_err);

#ifdef CPU_CFG_INT_DIS_MEAS_EN
  CPU_IntDisMeasMaxCurReset();
#endif

  APP_TRACE_INFO(("Creating Application Events...\n\r"));
  App_ObjCreate(); /* Create Application kernel objects                    */

  APP_TRACE_INFO(("Creating Application Tasks...\n\r"));
  App_TaskCreate(); /* Create Application tasks                             */

  BSP_LED_Off(BSP_GPIO_LED_ALL);

  App_LED1_Dly_ms = 200u;

  while (DEF_TRUE)
  { /* Task body, always written as an infinite loop.       */
    BSP_LED_Toggle(BSP_GPIO_LED1);
    OSTimeDlyHMSM(0u, 0u, 1, 0,
    OS_OPT_TIME_HMSM_STRICT,
                  &os_err);
  }
}

/*
 *********************************************************************************************************
 *                                        App_Port1_ISR()
 *
 * Description : ISR for GPIO Port 1. The delay for App_TaskStart() will increase by 200 ms if S1 is
 *               pressed. LED2 will cycle through its RGB components every time S2 is pressed.
 *
 * Argument(s) : none.
 *
 * Return(s)   : none.
 *
 * Caller(s)   : This is an ISR.
 *
 * Note(s)     : This ISR only services on-board button presses (P1.1 & P1.4). Buttons are not de-bounced.
 *********************************************************************************************************
 */

void App_Port1_ISR(void)
{
  CPU_INT08U startup_led;
  CPU_BOOLEAN sw1;
  CPU_BOOLEAN sw2;

  sw1 = DEF_BIT_IS_SET(BSP_GPIO_REG_IFG(BSP_GPIO_P1_BASE_ADDR),
                       BSP_GPIO_SW1_PIN);
  sw2 = DEF_BIT_IS_SET(BSP_GPIO_REG_IFG(BSP_GPIO_P1_BASE_ADDR),
                       BSP_GPIO_SW2_PIN);

  startup_led = BSP_GPIO_LED2_RED; /* Select red component of LED2 as first LED to be lit. */

  if (sw2 == DEF_ON)
  {
    if (RGB_Ctr < startup_led)
    {
      RGB_Ctr = startup_led;
    }
    BSP_LED_Toggle(RGB_Ctr);

    RGB_Ctr++;
    RGB_Ctr %= 5u;
  }
  if (sw1 == DEF_ON)
  {
    App_LED1_Dly_ms += 200u; /* Increment AppTaskStart's millisecond delay by 200.   */
    if (App_LED1_Dly_ms > 1000u)
    { /* Reset delay if it reaches 1000.                      */
      App_LED1_Dly_ms = 100u;
    }
  }

  BSP_GPIO_REG_IV(BSP_GPIO_P1_BASE_ADDR); /* Clear interrupt flag. Interrupt already serviced.    */
}

/*
 *********************************************************************************************************
 *                                      App_ObjCreate()
 *
 * Description:  Creates the application kernel objects.
 *
 * Argument(s) :  none.
 *
 * Return(s)   :  none.
 *
 * Caller(s)   :  App_TaskStart().
 *
 * Note(s)     :  none.
 *********************************************************************************************************
 */

static void App_ObjCreate(void)
{
}

/*
 *********************************************************************************************************
 *                                      App_TaskCreate()
 *
 * Description :  This function creates the application tasks.
 *
 * Argument(s) :  none.
 *
 * Return(s)   :  none.
 *
 * Caller(s)   :  App_TaskStart().
 *
 * Note(s)     :  none.
 *********************************************************************************************************
 */

static void App_TaskCreate(void)
{

  OS_ERR err;

  OSTaskCreate((OS_TCB *) &App_TaskSensorsTCB,
               (CPU_CHAR *) "Task Sensors",
               (OS_TASK_PTR) App_TaskSensors,
               (void *) 0,
               (OS_PRIO) APP_CFG_TASK_SENSORS_PRIO,
               (CPU_STK *) &App_TaskSensorsStk[0],
               (CPU_STK_SIZE) APP_CFG_TASK_SENSORS_STK_SIZE / 10u,
               (CPU_STK_SIZE) APP_CFG_TASK_SENSORS_STK_SIZE,
               (OS_MSG_QTY) 0u,
               (OS_TICK) 0u,
               (void *) 0,
               (OS_OPT) (OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR ),
               (OS_ERR *) &err);

}

/*
 *********************************************************************************************************
 *                                          SENSORS TASK
 *
 * Description : This is a task to t.b.d
 *
 * Arguments   : p_arg   is the argument passed to 'AppTaskDisplay()' by 'OSTaskCreate()'.
 *
 * Returns     : none.
 *
 * Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
 *                  used.  The compiler should not generate any code for this statement.
 *********************************************************************************************************
 */

static void App_TaskSensors(void *p_arg)
{

  OS_ERR err;

  uint16_t uiSensDesPerL = 2000;
  uint16_t uiSensDesPerR = 2000;
  uint16_t uiSensDesPerDelta = 1200;

  (void) &p_arg;

  Bumpers_Init();
  LineSensor_Init();
  Motor_Init();
  IRSensors_Init();

  Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
  Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);

  Motor_SetDesiredPeriod(eMotor_PositionLeft, uiSensDesPerL);
  Motor_SetDesiredPeriod(eMotor_PositionRight, uiSensDesPerR);

  Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
  Motor_SetState(eMotor_PositionRight, eMotor_StateActive);

  while (DEF_ON)
  {
    OSTimeDlyHMSM(0u, 0, 0, 20,
    OS_OPT_TIME_HMSM_STRICT,
                  &err);

    Motor_ControllerStep();
    myCurVal = LineSensor_GetBufferDirection();
    IRSensors_GetDistanceMilliMeters();

    LineSensor_StartTimer();
    IRSensors_StartConversion();
    Bumpers_Read();

    switch (myCurVal.aQual)
    {
    case eLineSensor_QualGood:
      switch (myCurVal.aDir)
      {
      case eLineSensor_DirRight:
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, uiSensDesPerL - uiSensDesPerDelta);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionReverse);
        Motor_SetDesiredPeriod(eMotor_PositionRight, 3000);
        break;
      case eLineSensor_DirRightMinor:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, uiSensDesPerL - uiSensDesPerDelta);
        Motor_SetDesiredPeriod(eMotor_PositionRight, uiSensDesPerR + uiSensDesPerDelta);
        break;
      case eLineSensor_DirAhead:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, uiSensDesPerL);
        Motor_SetDesiredPeriod(eMotor_PositionRight, uiSensDesPerR);
        break;
      case eLineSensor_DirLeftMinor:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, uiSensDesPerL + uiSensDesPerDelta);
        Motor_SetDesiredPeriod(eMotor_PositionRight, uiSensDesPerR - uiSensDesPerDelta);
        break;
      case eLineSensor_DirLeft:
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionReverse);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, 3000);
        Motor_SetDesiredPeriod(eMotor_PositionRight, uiSensDesPerR - uiSensDesPerDelta);
        break;
      }
      break;
    case eLineSensor_QualPoor:
      switch (myCurVal.aDir)
      {
      case eLineSensor_DirRight:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateSleep);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, (uiSensDesPerL << 1) - (uiSensDesPerDelta >> 1));
        Motor_SetDesiredPeriod(eMotor_PositionRight, 5000 + uiSensDesPerDelta);
        break;
      case eLineSensor_DirRightMinor:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, (uiSensDesPerL << 1) - (uiSensDesPerDelta >> 1));
        Motor_SetDesiredPeriod(eMotor_PositionRight, (uiSensDesPerR << 1) + (uiSensDesPerDelta >> 1));
        break;
      case eLineSensor_DirAhead:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, uiSensDesPerL << 1);
        Motor_SetDesiredPeriod(eMotor_PositionRight, uiSensDesPerR << 1);
        break;
      case eLineSensor_DirLeftMinor:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateActive);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, (uiSensDesPerL << 1) + (uiSensDesPerDelta >> 1));
        Motor_SetDesiredPeriod(eMotor_PositionRight, (uiSensDesPerR << 1) - (uiSensDesPerDelta >> 1));
        break;
      case eLineSensor_DirLeft:
        Motor_SetDirection(eMotor_PositionLeft, eMotor_DirectionAhead);
        Motor_SetDirection(eMotor_PositionRight, eMotor_DirectionAhead);
        Motor_SetState(eMotor_PositionLeft, eMotor_StateSleep);
        Motor_SetState(eMotor_PositionRight, eMotor_StateActive);
        Motor_SetDesiredPeriod(eMotor_PositionLeft, 5000 + uiSensDesPerDelta);
        Motor_SetDesiredPeriod(eMotor_PositionRight, (uiSensDesPerR << 1) - (uiSensDesPerDelta >> 1));
        break;
      }
      break;
    default:
      Motor_SetState(eMotor_PositionLeft, eMotor_StateSleep);
      Motor_SetState(eMotor_PositionRight, eMotor_StateSleep);
      break;
    }
  }
}
