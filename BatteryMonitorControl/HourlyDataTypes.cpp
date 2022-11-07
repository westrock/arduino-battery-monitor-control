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
