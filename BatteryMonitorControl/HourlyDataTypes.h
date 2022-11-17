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
#ifndef _HourlyDataTypes_h_
#define _HourlyDataTypes_h_

#include "Arduino.h"
#include <ds3231.h>

struct currentHourDataStruct {
  uint8_t   hour;			// The hour this represents
  uint8_t   minMinute;		// The minute the vMin was recorded
  uint8_t   maxMinute;		// The minute the vMax was recorded
  uint8_t   samples;		// The number of samples
  uint16_t  vTotal;			// The total of raw voltage readings
  uint16_t  vMin;			// The minimum raw voltage this hour
  uint16_t  vMax;			// The maximum raw voltage this hour
  float		tTotal;			// The total of raw temperature readings
  float		tMin;			// The minimum raw temperature this hour
  float		tMax;			// The maximum raw temperature this hour
};
typedef struct currentHourDataStruct CurrentHourData;

struct hourlyDataStruct {
	uint8_t		hour;			// The hour this represents
	uint8_t		minMinute;		// The minute the vMin was recorded
	uint8_t		maxMinute;		// The minute the vMax was recorded
	uint16_t	vMin;			// The minimum raw voltage this hour
	uint16_t	vMax;			// The maximum raw voltage this hour
	uint16_t	vAvg;			// The average raw voltage this hour
	float  		tMin;			// The minimum raw temperature this hour
	float  		tMax;			// The maximum raw temperature this hour
	float		tAvg;			// The average raw temperature this hour
};
typedef struct hourlyDataStruct HourlyData;

typedef struct ts DS3231Time;

void PrepCurrentHour(CurrentHourData* hourData, DS3231Time* t, uint16_t* rawVoltage, float* temp);
void AddSampleToCurrentHour(CurrentHourData* hourData, DS3231Time* t, uint16_t* rawVoltage, float* temp);
void prepHourlyDataSlot(HourlyData *hourlyDataSlot);
void PrepHourlyData(HourlyData *firstHourlyDataSlot, uint8_t count);
void CloseCurrentHour(HourlyData* hourlyData, CurrentHourData* currentHourData, uint8_t hourIndex);
HourlyData* FindMinVoltage(HourlyData* hourSlots, uint8_t count);
HourlyData* FindMinTemp(HourlyData* hourSlots, uint8_t count);
HourlyData* FindMaxVoltage(HourlyData* hourSlots, uint8_t count);
HourlyData* FindMaxTemp(HourlyData* hourSlots, uint8_t count);

#endif
