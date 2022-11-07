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
#ifndef _HourlyDataTypes_h_
#define _HourlyDataTypes_h_

#include "Arduino.h"

struct currentHourDataStruct {
  uint8_t   hour;
  uint8_t   minMinute;
  uint8_t   maxMinute;
  uint8_t   samples;
  uint16_t  vTotal;
  uint16_t  vMin;
  uint16_t  vMax;
  uint16_t  tTotal;
  uint16_t  tMin;
  uint16_t  tMax;
};
typedef struct currentHourDataStruct CurrentHourData;

struct hourlyDataStruct {
  uint8_t   minMinute;
  uint8_t   maxMinute;
  uint16_t  vMin;
  uint16_t  vMax;
  uint16_t  tMin;
  uint16_t  tMax;
};
typedef struct hourlyDataStruct HourlyData;

void prepCurrentHour(CurrentHourData *hourData, uint8_t hour);
void prepHourlyDataSlot(HourlyData *hourlyDataSlot);
void prepHourlyData(HourlyData *firstHourlyDataSlot, uint8_t count);
void storeCurrentHour(CurrentHourData *hourData, HourlyData *hourlyDataSlot);

#endif
