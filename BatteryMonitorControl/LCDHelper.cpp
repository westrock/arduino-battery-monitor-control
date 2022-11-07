#include "LCDHelper.h"


uint8_t significantDigits(float floatNum) {
  int     intNum = fabs(floatNum);
  uint8_t count = (floatNum < 0); /* bound to be at least one digit! */

  while (intNum != 0) {
    intNum /= 10;
    ++count;
  }
  return count;
}

void FormatVoltage(float voltage, char *buffer, uint8_t width, uint8_t precis) {
  uint8_t digits = significantDigits(voltage);
  double  logVoltage;

  if (digits > width) {
    memset(buffer, '*', width);
    buffer[width] = 0;
  }
  else {
    dtostrf(voltage, width, precis, buffer);
  }
}

void LcdPrint(LiquidCrystal lcd, char *text, int padLength) {
  int textLength = strlen(text);
  int neededPadding;

  if (textLength > padLength || textLength > 20) {
    neededPadding = 0;
  } else {
    neededPadding = max(padLength, 20) - strlen(text);
  }

  lcd.print(text);

  if (neededPadding > 0) {
    int i;
    char padding[21];

    for (i = 0; i < neededPadding; i++) {
      padding[i] = ' ';
    }
    padding[i] = 0;

    lcd.print(padding);
  }
}
