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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */
#pragma once
#ifndef _DS3231Helpers_h_
#define _DS3231Helpers_h_

#include "Arduino.h"
#include "config.h"
#include "ds3231.h"
#include <Wire.h>


void setAlarmAndSleep(uint8_t wakePin, void (*wakeISR)(), void (*preSleepAction)(), byte *prevADCSRA, uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void setNextAlarm(uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void postWakeISRCleanup(byte *prevADCSRA);

#endif
