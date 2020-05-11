//-----------------------------------------------------------------------
// Identifier    $Id: IRSensors.c 100 2020-01-05 20:30:24Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/IRSensors/IRSensors.c $ */
//-----------------------------------------------------------------------*/

/* self */
#include "IRSensors.h"

// Buffer
static TIRSensorsBuffer aIRSensorsBuffer;
static TIRSensorsDistance aIRSensorsDistance;
static uint32_t aLPFilterState[3];

// ################################
void IRSensors_Init(void)
{
    // Enabling & Initializing ADC (MCLK; 1; 4)
    MAP_ADC14_enableModule();
    MAP_ADC14_initModule(ADC_CLOCKSOURCE_MCLK, ADC_PREDIVIDER_1, ADC_DIVIDER_4, 0);

    // Configuring GPIOs for Analog Input
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P9, GPIO_PIN1 | GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
    MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN1, GPIO_TERTIARY_MODULE_FUNCTION );

    // Configuring ADC Memory (ADC_MEM0 - ADC_MEM2 (A16, A12, A17)  with no repeat) with 0V ... 3.3Vss
    MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM2, false);
    MAP_ADC14_configureConversionMemory(ADC_MEM0,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A16, false);
    MAP_ADC14_configureConversionMemory(ADC_MEM1,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A12, false);
    MAP_ADC14_configureConversionMemory(ADC_MEM2,
            ADC_VREFPOS_AVCC_VREFNEG_VSS,
            ADC_INPUT_A17, false);

    //! Enabling the interrupt when a conversion on channel 2 (end of sequence) is complete
    MAP_ADC14_enableInterrupt(ADC_INT2);

    //! Enabling Interrupts for ADC14
    BSP_IntVectSet(BSP_INT_ID_ADC,                        // Set the ISR for ADC14
                   ADC14_IRQHandler);
    BSP_IntEn(BSP_INT_ID_ADC);
    // MAP_Interrupt_enableInterrupt(INT_ADC14);

    // Setting up the sample timer to automatically step through the sequence convert.
    MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

    // Triggering the start of the sequence
    MAP_ADC14_enableConversion();
}

// #################################
void IRSensors_StartConversion( void )
{
    MAP_ADC14_toggleConversionTrigger();
}

// #################################
TIRSensorsBuffer IRSensors_GetBuffer( void )
{
    return aIRSensorsBuffer;
}

// #################################
TIRSensorsDistance IRSensors_GetDistanceMilliMeters(void)
{
    aIRSensorsDistance.distance_pt1_mm[0] = (int16_t) ( (IRSENSORS_CONVERT_A / ( aIRSensorsBuffer.samples_pt1[0] + IRSENSORS_CONVERT_B)) + IRSENSORS_CONVERT_C );
    aIRSensorsDistance.distance_pt1_mm[1] = (int16_t) ( (IRSENSORS_CONVERT_A / ( aIRSensorsBuffer.samples_pt1[1] + IRSENSORS_CONVERT_B)) + IRSENSORS_CONVERT_C );
    aIRSensorsDistance.distance_pt1_mm[2] = (int16_t) ( (IRSENSORS_CONVERT_A / ( aIRSensorsBuffer.samples_pt1[2] + IRSENSORS_CONVERT_B)) + IRSENSORS_CONVERT_C );

    return aIRSensorsDistance;
}

// #################################
/* This interrupt is fired whenever a conversion is completed and placed in
 * ADC_MEM2. This signals the end of conversion and the results array is
 * grabbed and placed in the buffer */
void ADC14_IRQHandler(void)
{
    uint64_t status;

#ifdef IRSENSORS_USE_DEBUG_LED
    IRSENSORS_DEBUG_LED_ON;
#endif

    status = MAP_ADC14_getEnabledInterruptStatus();
    MAP_ADC14_clearInterruptFlag(status);

    if(status & ADC_INT2)
    {
        MAP_ADC14_getMultiSequenceResult(aIRSensorsBuffer.samples);
        aIRSensorsBuffer.samples_pt1[0] = lpf_iir_1st_order( &aLPFilterState[0], aIRSensorsBuffer.samples[0] );
        aIRSensorsBuffer.samples_pt1[1] = lpf_iir_1st_order( &aLPFilterState[1], aIRSensorsBuffer.samples[1] );
        aIRSensorsBuffer.samples_pt1[2] = lpf_iir_1st_order( &aLPFilterState[2], aIRSensorsBuffer.samples[2] );
    }

#ifdef IRSENSORS_USE_DEBUG_LED
    IRSENSORS_DEBUG_LED_OFF;
#endif

}


