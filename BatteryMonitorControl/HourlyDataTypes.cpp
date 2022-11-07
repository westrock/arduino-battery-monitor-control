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


void prepCurrentHour(CurrentHourData *hourData, uint8_t hour) {
  hourData->hour = hour;
  hourData->minMinute = 0;
  hourData->maxMinute = 0;
  hourData->vTotal = 0;
  hourData->vMin = 0xFFFF;
  hourData->vMax = 0;
  hourData->tTotal = 0;
  hourData->tMin = 0xFFFF;
  hourData->tMax = 0;
}


void prepHourlyDataSlot(HourlyData *hourlyDataSlot) {
  hourlyDataSlot->minMinute = 0;
  hourlyDataSlot->maxMinute = 0;
  hourlyDataSlot->vMin = 0xFFFF;
  hourlyDataSlot->vMax = 0;
  hourlyDataSlot->tMin = 0xFFFF;
  hourlyDataSlot->tMax = 0;
}


void prepHourlyData(HourlyData *firstHourlyDataSlot, uint8_t count) {
  for (int i = 0; i < count; i++) {
    prepHourlyDataSlot(firstHourlyDataSlot + i);
  }
}



void storeCurrentHour(CurrentHourData *hourData, HourlyData *hourlyDataSlot) {
  hourlyDataSlot->vMin = hourData->vMin;
  hourlyDataSlot->vMax = hourData->vMax;
  hourlyDataSlot->tMin = hourData->tMin;
  hourlyDataSlot->tMax = hourData->tMax;
}
