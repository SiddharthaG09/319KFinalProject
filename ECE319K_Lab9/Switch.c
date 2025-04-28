/*
 * Switch.c
 *
 *  Created on: Nov 5, 2023
 *      Author:
 */
#include <ti/devices/msp/msp.h>
#include "../inc/LaunchPad.h"
#include "ti/devices/msp/m0p/mspm0g350x.h"
// LaunchPad.h defines all the indices into the PINCM table
void Switch_Init(void){
    IOMUX->SECCFG.PINCM[PA15INDEX] = (uint32_t) 0x00040081;
    IOMUX->SECCFG.PINCM[PA17INDEX] = (uint32_t) 0x00040081;
 
}
// return current state of switches
uint32_t Switch_In(void){
    uint32_t inputBits = 0x0;
    uint32_t input = GPIOA->DIN31_0;

    if ((input & (1 << 15)) != 0) {             // check PA15
        inputBits |= 1;                         // set bit 0 (e.g. shoot)
    }
    if ((input & (1 << 17)) != 0) {             // check PA17
        inputBits |= (1 << 1);                  // set bit 1 (e.g. pause/play)
    }
 
    return inputBits;
}
 
