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
#pragma ONCE

#ifndef _LCDHelper_h
#define _LCDHelper_h

#include "arduino.h"

#include <LiquidCrystal.h>

#define LCD_DOWN_ARROW	0
#define LCD_UP_ARROW	1

uint8_t significantDigits(float floatNum);

void CreateArrows(LiquidCrystal lcd);

void FormatFloat(float voltage, char* buffer, uint8_t width, uint8_t precis);

void LcdPrint(LiquidCrystal lcd, char* text, int padLength);

#endif


