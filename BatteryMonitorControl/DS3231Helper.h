#pragma ONCE
#ifndef _DS3231Helper_h_
#define _DS3231Helper_h_

#include "Arduino.h"
#include <config.h>
#include <ds3231.h>
#include <Wire.h>

bool rtcIsRunning();

void setAlarmAndSleep(uint8_t wakePin, void (*wakeISR)(), void (*preSleepAction)(), byte *prevADCSRA, uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void setNextAlarm(uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void postWakeISRCleanup(byte *prevADCSRA);

#endif
