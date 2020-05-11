//******************************************************************************
//  Implementation of a first order IIR-filter (moving average)
//
//  Details described in header file
//
//  U.Creutzburg
//  FH-Stralsund
//  Mai 2012
//  based on TI-examples
//  Built with IAR Embedded Workbench Version: 5.4
//******************************************************************************

//-----------------------------------------------------------------------
// Identifier    $Id: LPF_IIR_1st_order.c 93 2020-01-01 17:45:53Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/IRSensors/LPF_IIR_1st_order.c $ */
//-----------------------------------------------------------------------*/

#include "LPF_IIR_1st_order.h"

//############################### lpf_iir_1st_order
uint16_t lpf_iir_1st_order( uint32_t * ulState, uint16_t uiCurrentVal )
{
  *ulState = (uint32_t)
               (
                   (uint32_t) (*ulState << LSHFT) - *ulState // decay
                 + ((uint32_t) uiCurrentVal << (2*LSHFT))    // shift input value twice
                 + (1 << (2*LSHFT-1))                        // add 1/2 LSB
               ) >> LSHFT;                                   // adjust shifts
               
  return (uint16_t) (*ulState >> (2*LSHFT));            // adjust internal scaling
}

//############################### lpf_iir_1st_order_set_state
uint16_t lpf_iir_1st_order_set_state( uint32_t * ulState, uint16_t uiCurrentVal )
{
  *ulState = ((uint32_t) uiCurrentVal << (2*LSHFT));    // shift input value twice
  return (uint16_t) (*ulState >> (2*LSHFT));            // adjust internal scaling
}
