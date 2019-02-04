// matrix display related definitions

#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

//#define double_buffer
//#define DISPLAY_32x64
#define DISPLAY_64x64

#include <PxMatrix.h> // https://github.com/2dom/PxMatrix
#include <gfxfont.h>
#include "fonts/TomThumb.h"

#include <Ticker.h>
Ticker display_ticker;

#if defined(ESP8266)
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_D 12
#define P_E 0   // not needed on 64x32
#define P_OE 2
#else
#define P_LAT 26
#define P_A 22
#define P_B 21
#define P_C 5
#define P_D 19
#define P_E 17  // not needed on 64x32
#define P_OE 16

#endif

#ifdef DISPLAY_64x64
PxMATRIX display(64, 64, P_LAT, P_OE, P_A, P_B, P_C, P_D, P_E);
#else
PxMATRIX display(64, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);
#endif

uint16_t htmlColor565(const String htmlColor) {
  long c = strtol(htmlColor.substring(1).c_str(), NULL, 16);
  uint8_t r = (c >> 19);
  uint8_t g = (c >> 10) & 0x3F;
  uint8_t b = (c >> 3) & 0x1F;
  return ((r << 11) | (g << 5) | b);
}

uint32_t color565to888(uint16_t c) {
  uint16_t r = (c >> 11) & 0x01f;
  uint16_t g = (c >> 5) & 0x03f;
  uint16_t b = (c) & 0x01f;
  r <<= 3;
  g <<= 2;
  b <<= 3;
  return ((r << 16) | (g << 8) | b);
}

#if defined (ESP8266)
// ISR for display refresh
void display_updater()
{
  display.display(70);
}

#elif defined (ESP32)

portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR display_updater() 
{
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(15);
  portEXIT_CRITICAL_ISR(&timerMux);
}
#endif


#include "Digit.h"
#ifdef DISPLAY_64x64
#ifdef ESP8266
const byte row1 = 6;
const byte row2 = 14;
const byte row3 = 54;
const byte row4 = 62;
#else
const byte row1 = 6;
const byte row2 = 14;
const byte row3 = 22;
const byte row4 = 30;
const byte row5 = 38;
const byte row6 = 46;
const byte row7 = 54;
const byte row8 = 62;
#endif
#else
const byte row1 = 6;
const byte row2 = 14;
const byte row3 = 22;
const byte row4 = 31;
#endif

const uint16_t myRED = display.color565(255, 0, 0);
const uint16_t myGREEN = display.color565(0, 255, 0);
const uint16_t myBLUE = display.color565(0, 0, 255);
const uint16_t myLTBLUE = display.color565(32, 64, 160);
const uint16_t myWHITE = display.color565(255, 255, 255);
const uint16_t myYELLOW = display.color565(255, 255, 0);
const uint16_t myORANGE = display.color565(255, 165, 0);
const uint16_t myCYAN = display.color565(0, 255, 255);
const uint16_t myMAGENTA = display.color565(255, 0, 255);
const uint16_t myGRAY = display.color565(102, 102, 102);
const uint16_t myBLACK = display.color565(0, 0, 0);
uint16_t myColor = myGREEN;

Digit digit0(&display, 0, 63 - 1 - 9 * 1, 9, myColor);
Digit digit1(&display, 0, 63 - 1 - 9 * 2, 9, myColor);
Digit digit2(&display, 0, 63 - 4 - 9 * 3, 9, myColor);
Digit digit3(&display, 0, 63 - 4 - 9 * 4, 9, myColor);
Digit digit4(&display, 0, 63 - 7 - 9 * 5, 9, myColor);
Digit digit5(&display, 0, 63 - 7 - 9 * 6, 9, myColor);

#endif
