#pragma ONCE
#ifndef LCDHelper_h
#define LCDHelper_h

#include <LiquidCrystal.h>
#include "Arduino.h"


uint8_t significantDigits(float floatNum);

void FormatVoltage(float voltage, char *buffer, uint8_t width, uint8_t precis);

void LcdPrint(LiquidCrystal lcd, char *text, int padLength);

#endif
