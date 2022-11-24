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
#ifndef _DateTimeHelpers_h_
#define _DateTimeHelpers_h_

#include "Arduino.h"
#include "ds3231.h"


struct elapsedTimeStruct {
	int32_t	totalSecond;		// Total Elapsed seconds
	int32_t	totalMinute;		// Elapsed seconds
	int32_t	totalHour;			// Elapsed seconds
	int8_t	second;				// Elapsed seconds
	int8_t	minute;				// Elapsed minutes
	int8_t	hour;				// Elapsed hours
	int8_t	year;				// Elapsed years
	int16_t	day;				// Elapsed days
};
typedef struct elapsedTimeStruct ElapsedTime;

int32_t dateDiffMinutes(DateTimeDS3231* pCurDayTime, DateTimeDS3231* pTgtDayTime);
int32_t dateDiffSeconds(DateTimeDS3231* pCurDayTime, DateTimeDS3231* pTgtDayTime);
ElapsedTime dateDiff(DateTimeDS3231* pCurDayTime, DateTimeDS3231* pTgtDayTime);
void addSeconds(DateTimeDS3231* pCurDayTime, uint8_t seconds);
void addMinutes(DateTimeDS3231* pCurDayTime, uint8_t minutes);
void addHours(DateTimeDS3231* pCurDayTime, uint8_t hours);
void addDays(DateTimeDS3231* pCurDayTime, uint16_t days);
void addYears(DateTimeDS3231* pCurDayTime, uint8_t years);

#endif
