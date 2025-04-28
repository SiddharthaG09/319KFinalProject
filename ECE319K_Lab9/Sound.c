// Sound.c
// Runs on MSPM0
// Sound assets in sounds/sounds.h
// your name
// your data 


#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "Sound.h"
#include "sounds/sounds.h"
#include "../inc/DAC5.h"
#include "../inc/Timer.h"
// #include "soundAstroparty.h"

void SysTick_IntArm(uint32_t period, uint32_t priority){
  // write this
  SysTick->CTRL = 0x00;      // disable SysTick during setup

  SysTick->LOAD = period-1;  // reload value
  SCB->SHP[1] = (SCB->SHP[1]&(~0xC0000000))|(priority<<30); // priority 2
  SysTick->VAL = 0;          // any write to VAL clears COUNT and sets VAL equal to LOAD
  SysTick->CTRL = 0x07;      // enable SysTick with 80 MHz bus clock and interrupts
}
// initialize a 11kHz SysTick, however no sound should be started
// initialize any global variables
// Initialize the 5-bit DAC
void Sound_Init(void){
// write this
  DAC5_Init();
  SysTick_IntArm(80000000/11000, 0);
  SysTick->CTRL = 0; 
}

uint32_t id = 0; 
static const uint8_t *arr; 
uint32_t len = 0; 
void SysTick_Handler(void){ // called at 11 kHz
  // output one value to DAC if a sound is active
    // output one value to DAC if a sound is active
  if( arr && id < len){
    DAC5_Out(arr[id]);
    id++;
  }
  else {
    SysTick->CTRL = 0; //turns the SysTick interrupt off
    id = 0; //sets id back to 0
  }
}
bool Sound_Busy(void){
  return (SysTick->CTRL & SysTick_CTRL_ENABLE_Msk); 
}

//******* Sound_Start ************
// This function does not output to the DAC. 
// Rather, it sets a pointer and counter, and then enables the SysTick interrupt.
// It starts the sound, and the SysTick ISR does the output
// feel free to change the parameters
// Sound should play once and stop
// Input: pt is a pointer to an array of DAC outputs
//        count is the length of the array
// Output: none
// special cases: as you wish to implement
void Sound_Start(const uint8_t *pt, uint32_t count){
// write this
arr = pt;
len = count; 
id = 0; 
SysTick->VAL = 0; 
SysTick->CTRL = 0x7; 

}
void Sound_Shoot(void){
// write this
  Sound_Start( shoot, 4080);
}
void Sound_Killed(void){
 Sound_Start( invaderkilled, 3377);

}
void Sound_Explosion(void){
 Sound_Start( explosion, 3377);
//  Sound_Start( invaderkilled, 3377);

}

void Sound_Fastinvader1(void){

}
void Sound_Fastinvader2(void){

}
void Sound_Fastinvader3(void){

}
void Sound_Fastinvader4(void){

}
void Sound_Highpitch(void){
  Sound_Start( highpitch, 1802);
}
