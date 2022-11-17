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
#pragma once
#ifndef _DS3231Helper_h_
#define _DS3231Helper_h_

#include "Arduino.h"
#include <config.h>
#include <ds3231.h>
#include <Wire.h>

typedef struct ts DS3231Time;

#define IS_LEAP_YEAR(y) ((y % 400 == 0) || (y % 100 != 0) && (y % 4 == 0))

DS3231Time getDS3231Time(DS3231Time* time);

void setAlarmAndSleep(uint8_t wakePin, void (*wakeISR)(), void (*preSleepAction)(), byte *prevADCSRA, uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void setNextAlarm(uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void postWakeISRCleanup(byte *prevADCSRA);

int32_t ElapsedMinutes(DS3231Time* pCurDayTime, uint16_t* pCurDayOfYear, DS3231Time* pTgtDayTime, uint16_t* pTgtDayOfYear);

int32_t DateDiff(DS3231Time* pCurDayTime, uint16_t* pCurDayOfYear, DS3231Time* pTgtDayTime, uint16_t* pTgtDayOfYear);

void DDDtoMMDD(bool isLeapYear, int16_t dayOfYear, int8_t* mm, int8_t* dd);

uint16_t MMDDtoDDD(bool isLeapYear, int8_t mm, int8_t dd);

#endif
