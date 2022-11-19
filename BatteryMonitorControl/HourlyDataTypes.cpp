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
#include "HourlyDataTypes.h"


void PrepCurrentHour(CurrentHourData* hourData, DateTimeDS3231* t, uint16_t* rawVoltage, float* temp) {
	hourData->samples = 1;
	hourData->hour = t->hour;
	hourData->minMinute = t->min;
	hourData->maxMinute = t->min;
	hourData->vTotal = *rawVoltage;
	hourData->vMin = *rawVoltage;
	hourData->vMax = *rawVoltage;
	hourData->tTotal = *temp;
	hourData->tMin = *temp;
	hourData->tMax = *temp;
}


void AddSampleToCurrentHour(CurrentHourData* hourData, DateTimeDS3231* t, uint16_t* rawVoltage, float* temp) {

	hourData->samples++;
	hourData->vTotal += *rawVoltage;

	if (*rawVoltage < hourData->vMin) {
		hourData->vMin = *rawVoltage;
		hourData->minMinute = t->min;
	}
	else {
		if (*rawVoltage > hourData->vMax) {
			hourData->vMax = *rawVoltage;
			hourData->maxMinute = t->min;
		}
	}

	hourData->tTotal += *temp;

	if (*temp < hourData->tMin) {
		hourData->tMin = *temp;
	}
	else {
		if (*temp > hourData->tMax) {
			hourData->tMax = *temp;
		}
	}
}


void prepHourlyDataSlot(HourlyData* hourlyDataSlot) {
	hourlyDataSlot->hour = 0xFF;
	hourlyDataSlot->minMinute = 0;
	hourlyDataSlot->maxMinute = 0;
	hourlyDataSlot->vMin = 0xFFFF;
	hourlyDataSlot->vMax = 0;
	hourlyDataSlot->tMin = 3E+38;
	hourlyDataSlot->tMax = -3E+38;
}


void PrepHourlyData(HourlyData* firstHourlyDataSlot, uint8_t count) {
	for (int i = 0; i < count; i++) {
		prepHourlyDataSlot(firstHourlyDataSlot + i);
	}
}




void CloseCurrentHour(HourlyData* hourlyData, CurrentHourData* currentHourData, uint8_t hourIndex) {
	// Finalize Current Hour, save in Hourly Data
	hourlyData[hourIndex].hour = currentHourData->hour;
	hourlyData[hourIndex].minMinute = currentHourData->minMinute;
	hourlyData[hourIndex].maxMinute = currentHourData->maxMinute;
	hourlyData[hourIndex].vMin = currentHourData->vMin;
	hourlyData[hourIndex].vMax = currentHourData->vMax;
	hourlyData[hourIndex].vAvg = (currentHourData->samples == 0) ? 0.0 : round(static_cast<double>(currentHourData->vTotal) / currentHourData->samples);
	hourlyData[hourIndex].tMin = currentHourData->tMin;
	hourlyData[hourIndex].tMax = currentHourData->tMax;
	hourlyData[hourIndex].tAvg = (currentHourData->samples == 0) ? 0.0 : currentHourData->tTotal / currentHourData->samples;
}


HourlyData* FindMinVoltage(HourlyData* hourSlots, uint8_t count) {
	HourlyData* minVoltageData = NULL;
	uint16_t	minVoltage = 0xFFFF;

	for (int i = 0; i < count; i++) {
		if (hourSlots[i].hour != 0xFF && hourSlots[i].vMin < minVoltage) {
			minVoltage = hourSlots[i].vMin;
			minVoltageData = &hourSlots[i];
		}
	}
	return minVoltageData;
}


HourlyData* FindMinTemp(HourlyData* hourSlots, uint8_t count) {
	HourlyData* minTempData = NULL;
	float	minTemp = 3E+38;

	for (int i = 0; i < count; i++) {
		if (hourSlots[i].hour != 0xFF && hourSlots[i].tMin < minTemp) {
			minTemp = hourSlots[i].tMin;
			minTempData = &hourSlots[i];
		}
	}
	return minTempData;
}


HourlyData* FindMaxVoltage(HourlyData* hourSlots, uint8_t count) {
	HourlyData* maxVoltageData = NULL;
	uint16_t	maxVoltage = 0xFFFF;

	for (int i = 0; i < count; i++) {
		if (hourSlots[i].hour != 0xFF && hourSlots[i].vMax > maxVoltage) {
			maxVoltage = hourSlots[i].vMax;
			maxVoltageData = &hourSlots[i];
		}
	}
	return maxVoltageData;
}


HourlyData* FindMaxTemp(HourlyData* hourSlots, uint8_t count) {
	HourlyData* maxTempData = NULL;
	float	maxTemp = -3E+38;

	for (int i = 0; i < count; i++) {
		if (hourSlots[i].hour != 0xFF && hourSlots[i].tMax > maxTemp) {
			maxTemp = hourSlots[i].tMax;
			maxTempData = &hourSlots[i];
		}
	}
	return maxTempData;
}