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
