//******************************************************************************
//  First order IIR-filter
//
//
//  U.Creutzburg
//  FH-Stralsund
//  Mai 2012
//  based on TI-examples
//  Built with IAR Embedded Workbench Version: 5.4
//******************************************************************************

//-----------------------------------------------------------------------
// Identifier    $Id: LPF_IIR_1st_order.h 93 2020-01-01 17:45:53Z UweCreutzburg $
// Location $HeadURL: svn://172.23.3.25/MSP432/BRANCHES/UCOSIII_MSP432_SDK_TIRSLK_max/ENG/Software/Source/IRSensors/LPF_IIR_1st_order.h $ */
//-----------------------------------------------------------------------*/

#ifndef LPF_IIR_1ST_ORDER_H
#define LPF_IIR_1ST_ORDER_H

//
//  Implementation: y(n) = y(n-1) + {x(n) - y(n-1)} * P      
//
//        -----       -----     -----
//  x(n)->|+  |------>| p |---->| + |----------------------o---------- y(n) 
//        | - |       -----     -----                      |            
//        -----                   ^                        | 
//          ^                     |           --------     |
//          |                     |           |  -1  |     |
//          ----------------------o-----------| z    |<----- 
//          y(n-1)                            |      |
//                                            --------       
//
//  Implementation: y(n) = p * x(n) + {1-p} * y(n-1) * P      
//
//        -----       -----
//  x(n)->| p |------>| + |---------------------------------o---------- y(n) 
//        -----       -----                                 |            
//                      ^               y(n-1)              | 
//                      |                     --------      |
//                      |         -------     |  -1  |      |
//                      ----------| 1-p |<----| z    |<------ 
//                                -------     |      |
//                                            --------       
//
// Adjust filters time constant to meet your requirements.
// The time constant of the filter is controlled by the value "LSHFT"
// A value of 4 means that the current input is weighted by p=1/(2^4).
// The current state of the filter is then weigthed by (1-p) = 1-1/(2^4).
// Assuming a constant sample time for the filter this means that the time
// constant for the filter can be estimated by:
//
//
//        -1            n
// y = x e    ~= x (1-p)       ; e.g. assume: input=0.0, state = 1.0
//
//
//  -1           n
// e     ~= (1-p)
//
//
// -1     = n ln(1-p)
//
//              -1          -1
// n      = ---------- = --------- = 15.5
//            ln(1-p)     -0.0645
//
//
// Assuming a sample period of 1s, the time constant of that filter
// is appr. 15.5s
//
//
// OR:
//        -1            n                                                                       -1
// y = x e    ~= x (1-p)       ; input=0.0, state = 1.0, n=sample for which output falls below e 
//
//
//  -1           n
// e     ~= (1-p)
//
//
// -1     = n ln(1-p)
//
//  -1/n           
// e      = (1-p)
//
//                1/n    
// P      = 1 - e
//
// 
// Notes for the current implementation:
// The internal scaling of the filters state varies with "p". This leads to
// good noise suppression and better averaging behaviour.
// The output-scaling matches the input scaling.
// The internal algorithm is based on shifts. That means only a limited 
// set of values for p is possible.
//
//

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>


#define LSHFT (4)

extern uint16_t lpf_iir_1st_order( uint32_t * ulState, uint16_t uiCurrentVal );
extern uint16_t lpf_iir_1st_order_set_state( uint32_t * ulState, uint16_t uiCurrentVal );

#endif
