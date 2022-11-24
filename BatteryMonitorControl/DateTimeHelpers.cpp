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
#include "DateTimeHelpers.h"


DateTimeDS3231 GetTime()
{
	DateTimeDS3231 timeNow;

	DS3231_get(&timeNow);
	return timeNow;
}


int32_t dateDiffSecondsSinceNow(DateTimeDS3231* pTgtDayTime)
{
	DateTimeDS3231 timeNow;

	DS3231_get(&timeNow);
	return dateDiffSeconds(&timeNow, pTgtDayTime);
}


int32_t dateDiffMinutes(DateTimeDS3231* pCurDayTime, DateTimeDS3231* pTgtDayTime)
{
	return dateDiffSeconds(pCurDayTime, pTgtDayTime) / 60;
}


int32_t dateDiffSeconds(DateTimeDS3231* pCurDayTime, DateTimeDS3231* pTgtDayTime)
{
	return ((((pCurDayTime->year - pTgtDayTime->year) * 365
		+ (pCurDayTime->yday - pTgtDayTime->yday)) * 24
		+ (pCurDayTime->hour - pTgtDayTime->hour)) * 60
		+ (pCurDayTime->min - pTgtDayTime->min)) * 60
		+ pCurDayTime->sec - pTgtDayTime->sec;
}


#define sign(x) ((x > 0) ? 1 : ((x < 0) ? -1 : 0))

ElapsedTime dateDiff(DateTimeDS3231* pCurDayTime, DateTimeDS3231* pTgtDayTime)
{
	ElapsedTime elapsedTime;
	int32_t elapsedSeconds = dateDiffSeconds(pCurDayTime, pTgtDayTime);
	int8_t	dateSign = sign(elapsedSeconds);
	int dividend = abs(elapsedSeconds);

	elapsedTime.totalSecond = elapsedSeconds;

	int32_t quotient = dividend / 60;
	elapsedTime.second = (dividend - (quotient * 60)) * dateSign;
	dividend = quotient;
	elapsedTime.totalMinute = quotient * dateSign;

	quotient = dividend / 60;
	elapsedTime.minute = (dividend - (quotient * 60)) * dateSign;
	dividend = quotient;
	elapsedTime.totalHour = quotient * dateSign;

	quotient = dividend / 24;
	elapsedTime.hour = (dividend - (quotient * 24)) * dateSign;
	dividend = quotient;

	quotient = dividend / 365;
	elapsedTime.day = (dividend - (quotient * 365)) * dateSign;

	elapsedTime.year = quotient * dateSign;

	return elapsedTime;
}





void addSeconds(DateTimeDS3231* pCurDayTime, uint8_t seconds)
{
	pCurDayTime->sec += seconds;

	if (pCurDayTime->sec >= 60)
	{
		pCurDayTime->sec -= 60;
		addMinutes(pCurDayTime, 1);
	}
}



void addMinutes(DateTimeDS3231* pCurDayTime, uint8_t minutes)
{
	pCurDayTime->min += minutes;

	if (pCurDayTime->min >= 60)
	{
		pCurDayTime->min -= 60;
		addHours(pCurDayTime, 1);
	}
}



void addHours(DateTimeDS3231* pCurDayTime, uint8_t hours)
{
	pCurDayTime->hour += hours;

	if (pCurDayTime->hour >= 24)
	{
		pCurDayTime->hour -= 24;
		addDays(pCurDayTime, 1);
	}
}



void addDays(DateTimeDS3231* pCurDayTime, uint16_t days)
{
	pCurDayTime->yday += days;

	if (pCurDayTime->yday > (pCurDayTime->leapYear ? 365 : 364))
	{
		pCurDayTime->yday -= (pCurDayTime->leapYear ? 365 : 364);
		addYears(pCurDayTime, 1);
	}
	DDDtoMMDD(pCurDayTime, &pCurDayTime->mon, &pCurDayTime->mday);
}



void addYears(DateTimeDS3231* pCurDayTime, uint8_t years)
{
	pCurDayTime->year += years;
	pCurDayTime->year_s += years;

	pCurDayTime->leapYear = isLeapYear(pCurDayTime->year);

	if (!pCurDayTime->leapYear&& pCurDayTime->mon == 2 && pCurDayTime->mday == 29)
	{
		pCurDayTime->mon = 3;
		pCurDayTime->mday = 1;
	}
}


