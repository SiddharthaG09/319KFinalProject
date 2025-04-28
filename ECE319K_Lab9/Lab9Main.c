// Lab9Main.c
// Runs on MSPM0G3507
// Lab 9 ECE319K
// Your name
// Last Modified: 12/26/2024

#include <stdio.h>
#include <stdint.h>
#include <ti/devices/msp/msp.h>
#include "../inc/ST7735.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/TExaS.h"
#include "../inc/Timer.h"
#include "../inc/ADC1.h"
#include "../inc/DAC5.h"
#include "../inc/Arabic.h"
#include "SmallFont.h"
#include "LED.h"
#include "Switch.h"
#include "Sound.h"
#include "images/images.h"
#include <stdlib.h>
#include <time.h>

int getRandomval(void) {
  return (rand() % (40 - 5 + 1)) + 5;
}

// PLL / clock setup
void PLL_Init(void){
  Clock_Init80MHz(0);
}

// Arabic demo data (unchanged)
Arabic_t ArabicAlphabet[] = {
  alif, ayh, baa, daad, daal, dhaa, dhaal,
  faa, ghayh, haa, ha, jeem, kaaf, khaa,
  laam, meem, noon, qaaf, raa, saad, seen,
  sheen, ta, thaa, twe, waaw, yaa, zaa,
  space, dot, null
};
Arabic_t Hello[] = { alif, baa, ha, raa, meem, null };
Arabic_t WeAreHonoredByYourPresence[] = { alif, noon, waaw, ta, faa, raa, sheen, null };

// simple 32-bit LCG for wider random range
static uint32_t M = 1;
uint32_t Random32(void){
  M = 1664525*M + 1013904223;
  return M;
}
uint32_t Random(uint32_t n){
  return (Random32() >> 16) % n;
}

// TExaS helper for PB27/PB26
uint8_t TExaS_LaunchPadLogicPB27PB26(void){
  return (0x80 | ((GPIOB->DOUT31_0 >> 26) & 0x03));
}

// language/phrase tables (unchanged)
typedef enum { English, Spanish, Portuguese, French } Language_t;
typedef enum { HELLO, GOODBYE, LANGUAGE } phrase_t;
const char Hello_English[]    = "Hello";
const char Hello_Spanish[]    = "\xADHola!";
const char Hello_Portuguese[] = "Ol\xA0";
const char Hello_French[]     = "All\x83";
const char Goodbye_English[]    = "Goodbye";
const char Goodbye_Spanish[]    = "Adi\xA2s";
const char Goodbye_Portuguese[] = "Tchau";
const char Goodbye_French[]     = "Au revoir";
const char Language_English[]    = "English";
const char Language_Spanish[]    = "Espa\xA4ol";
const char Language_Portuguese[] = "Portugu\x88s";
const char Language_French[]     = "Fran\x87" "ais";
const char *Phrases[3][4] = {
  { Hello_English, Hello_Spanish, Hello_Portuguese, Hello_French },
  { Goodbye_English, Goodbye_Spanish, Goodbye_Portuguese, Goodbye_French },
  { Language_English, Language_Spanish, Language_Portuguese, Language_French }
};

// game state
uint32_t laseractive = 0;
uint32_t score       = 0;
volatile uint32_t slideplot = 0;
uint32_t flag        = 1;
uint32_t end         = 0;
uint32_t respawn     = 0;

typedef struct {
  uint16_t *pic;
  int32_t posx, posy;
  int32_t w, l;
  int32_t health;
  int32_t vx, vy;
  int32_t prevx, prevy;
} ship_t;

ship_t bguys, me, laser;

// initialize enemy: full-width spawn, random speeds & drift
void bguys_init(void) {
  bguys.pic    = SmallEnemy30pointA;
  bguys.health = 1;
  bguys.w      = 16;
  bguys.l      = 10;

  // spawn X anywhere in [0, 128 - w]
  bguys.posx = Random(128 - bguys.w);
  bguys.posy = 5;

  // vertical speed ∈ {1,2,3}
  bguys.vy = Random(3) + 1;

  // horizontal speed ∈ { -2,-1,0,+1,+2 }
  {
    int dir = (int)Random(3) - 1;   // -1,0,+1
    int mag = (int)Random(2) + 1;   //  1,2
    bguys.vx = dir * mag;
  }

  bguys.prevx = bguys.posx;
  bguys.prevy = bguys.posy;
}

void me_init(void) {
  me.pic    = PlayerShip0;
  me.health = 2;
  me.w      = 18;
  me.l      =  8;
  me.posx   =  0;
  me.posy   = 155;
  me.vx     =  0;
  me.vy     =  0;
  me.prevx  = me.posx;
  me.prevy  = me.posy;
}

void laser_init(void) {
  laser.pic    = ycoin;
  laser.w      = 16;
  laser.l      = 16;
  laser.health =  1;
  laser.posx   =  0;
  laser.posy   =  0;
  laser.vx     =  0;
  laser.vy     =  0;
  laser.prevx  =  0;
  laser.prevy  =  0;
}

// only fire when PA17 (bit 1) is pressed
void shoot_laser(void) {
  Clock_Delay1ms(15);
  uint32_t sw = Switch_In();
  if ((sw & 0x02) == 0) {
    return;
  }
  if (laseractive == 0) {
    laseractive = 1;
    laser.posx  = me.posx + (me.w/2 - laser.w/2);
    laser.posy  = me.posy;
    laser.vy    = -4;
    Sound_Shoot();
  }
}

void laserhit(void) {
  if (!laseractive) return;
  laser.posy += laser.vy;
  if (laser.posy <= 30) {
    laseractive = 0;
    return;
  }
  // collision test
  if (bguys.health > 0 &&
      laser.posx <  bguys.posx + bguys.w &&
      laser.posx + laser.w > bguys.posx &&
      laser.posy <  bguys.posy + bguys.l &&
      laser.posy + laser.l > bguys.posy) {
    bguys.health = 0;
    Sound_Explosion();
    Clock_Delay1ms(1);
    Sound_Explosion();
    laseractive = 0;
    score++;
    respawn = 1;
  }
}

// game tick: move enemy, player, laser, handle bounce & death
void cont(void) {
  bguys.prevx = bguys.posx;
  bguys.prevy = bguys.posy;

  if (respawn) {
    bguys_init();
    respawn = 0;
  }

  // update player from slide pot
  slideplot = ADCin();
  Clock_Delay1ms(10);
  me.prevx = me.posx;
  me.posx  = slideplot >> 5;

  // horizontal drift + edge bounce
  bguys.posx += bguys.vx;
  if (bguys.posx <= 0 || bguys.posx >= (128 - bguys.w)) {
    bguys.vx = -bguys.vx;
    if (bguys.posx < 0) bguys.posx = 0;
    else if (bguys.posx > (128 - bguys.w)) bguys.posx = 128 - bguys.w;
  }

  // vertical motion & end‐game
  if (bguys.health) {
    if (bguys.posy > 150) {
      bguys.health = 0;
      Sound_Killed();
      end = 1;
    } else {
      bguys.posy += bguys.vy;
    }
  }

  // laser movement & collision
  if (laseractive) {
    laser.prevx = laser.posx;
    laser.prevy = laser.posy;
    laser.posy += laser.vy;
  }
  if (laser.posy < -1) {
    laseractive = 0;
  }
  laserhit();
}

void draw(void) {
  // erase old
  ST7735_DrawBitmap(me.prevx,    me.prevy,    PlayerShipBlank, me.w,    me.l);
  ST7735_DrawBitmap(bguys.prevx, bguys.prevy, SmallEnemyBlank, bguys.w, bguys.l);
  ST7735_DrawBitmap(laser.prevx, laser.prevy, LaserBlank,      laser.w, laser.l);
  // draw new
  ST7735_DrawBitmap(me.posx,    me.posy,    me.pic,    me.w,    me.l);
  if (bguys.health) {
    ST7735_DrawBitmap(bguys.posx, bguys.posy, bguys.pic, bguys.w, bguys.l);
  }
  if (laseractive) {
    ST7735_DrawBitmap(laser.posx, laser.posy, laser.pic, laser.w, laser.l);
  }
}

// main game entry
int main(void) {
  __disable_irq();
  PLL_Init();
  LaunchPad_Init();
  SysTick_IntArm(400000,2);
  ST7735_InitR(INITR_GREENTAB);
  ST7735_FillScreen(ST7735_BLACK);

  ADCinit();
  Switch_Init();
  LED_Init();
  Sound_Init();

  me_init();
  bguys_init();
  laser_init();

  TExaS_Init(0, 0, &TExaS_LaunchPadLogicPB27PB26);

  // choose language (just burns cycles here)
  ST7735_OutString("*Espanol ");
  ST7735_OutString("or ");
  ST7735_OutString("*English ");
  uint32_t but = 0;
  while ((but = Switch_In()) == 0) { /* wait */ }

  // start the 30Hz tick
  ST7735_FillScreen(ST7735_BLACK);
  TimerG12_IntArm(80000000/30, 3);
  __enable_irq();
  ST7735_FillScreen(ST7735_BLACK);

  // game loop
  while (1) {
    if (flag) {
      draw();
      flag = 0;
    }
    if (end) {
      // show final
      ST7735_FillScreen(ST7735_BLACK);
      if (but == 2) {
        ST7735_SetCursor(1,3);
        ST7735_OutString("juego TERMINANDO!");
        ST7735_SetCursor(1,6);
        ST7735_OutString("Puntaje ");
        ST7735_OutUDec(score);
        ST7735_SetCursor(1,8);
        ST7735_OutString("Presione cualquier botón");
        ST7735_SetCursor(1,10);
        ST7735_OutString("para reiniciar");
        Sound_Explosion();
      } else {
        ST7735_SetCursor(1,3);
        ST7735_OutString("Game OVER!");
        ST7735_SetCursor(1,6);
        ST7735_OutString("Score ");
        ST7735_OutUDec(score);
        ST7735_SetCursor(1,8);
        ST7735_OutString("Press any button");
        ST7735_SetCursor(1,10);
        ST7735_OutString("to restart");
        Sound_Explosion();
      }
      // two-line restart prompt

      for (int i = 0; i < 5; i++) {
        Sound_Highpitch();
      }

      // wait for press
      while (Switch_In() == 0) { }
      Clock_Delay1ms(200);

      // reset game
      score   = 0;
      end     = 0;
      respawn = 1;
      me_init();
      bguys_init();
      laser_init();
      ST7735_FillScreen(ST7735_BLACK);
    }
  }
}

// TimerG12 ISR drives our 30Hz tick
void TIMG12_IRQHandler(void) {
  if (TIMG12->CPU_INT.IIDX == 1) {
    if (!laseractive) {
      shoot_laser();
    }
    cont();
    flag = 1;
  }
}