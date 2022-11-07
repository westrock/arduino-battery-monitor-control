/**
 *  My Extension to some open source application
 *
 *  Copyright 2022 by Edward Figarsky <efigarsky@gmail.com>
 *
 * This file is part of an open source application.
 *
 * Some open source application is free software: you can redistribute
 * it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */
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
