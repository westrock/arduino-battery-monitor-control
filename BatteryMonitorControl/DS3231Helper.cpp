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

#include "DS3231Helper.h"
#include <ds3231.h>
#include <avr/sleep.h>

static const uint16_t _CommonMonthDays[13] = { 0,31,59,90,120,151,181,212,243,273,304,334,365 };   // Common cumulative days per month.
static const uint16_t _LeapMonthDays[13] = { 0,31,60,91,121,152,182,213,244,274,305,335,366 };    // Leap cumulative days per month.


DS3231Time getDS3231Time(DS3231Time* time)
{
    DS3231_get(time);
    if (time->year < 2000) {
        time->year = 2000 + time->year_s;
    }
}



// Set the next alarm
void setNextAlarm(uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds)
{
  DS3231Time timeData;
  uint8_t wake_HOUR;
  uint8_t wake_MINUTE;
  uint8_t wake_SECOND;
  uint8_t flags[5] = { 0, 0, 0, 1, 1 };
  // flags define what calendar component to be checked against the current time in order
  // to trigger the alarm - see datasheet
  // A1M1 (seconds) (0 to enable, 1 to disable)
  // A1M2 (minutes) (0 to enable, 1 to disable)
  // A1M3 (hour)    (0 to enable, 1 to disable)
  // A1M4 (day)     (0 to enable, 1 to disable)
  // DY/DT          (dayofweek == 1/dayofmonth == 0)
  // get current time so we can calc the next alarm

  DS3231_get(&timeData);

  wake_SECOND = timeData.sec + wakeInSeconds;
  wake_MINUTE = timeData.min + wakeInMinutes;
  wake_HOUR = timeData.hour + wakeInHours;

  // Adjust times to wake
  if (wake_SECOND > 59)
  {
    wake_MINUTE++;
    wake_SECOND -= 60;
  }
  if (wake_MINUTE > 59)
  {
    wake_HOUR++;
    wake_MINUTE -= 60;
  }
  if (wake_HOUR > 23)
  {
    wake_HOUR -= 24;
  }

  // Set the alarm time (but not yet activated)
  DS3231_set_a1(wake_SECOND, wake_MINUTE, wake_HOUR, 0, flags);

  // Turn the alarm on
  DS3231_set_creg(DS3231_CONTROL_INTCN | DS3231_CONTROL_A1IE);
}



void setAlarmAndSleep(uint8_t wakePin, void (*wakeISR)(), void (*preSleepAction)(), byte *prevADCSRA, uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds) {
  // Set the DS3231 alarm to wake up in some number of hours, minutes, seconds
  setNextAlarm(wakeInHours, wakeInMinutes, wakeInSeconds);

  // Disable the ADC (Analog to digital converter, pins A0 [14] to A5 [19])
  *prevADCSRA = ADCSRA;
  ADCSRA = 0;

  /* Set the type of sleep mode we want. Can be one of (in order of power saving):

    SLEEP_MODE_IDLE (Timer 0 will wake up every millisecond to keep millis running)
    SLEEP_MODE_ADC
    SLEEP_MODE_PWR_SAVE (TIMER 2 keeps running)
    SLEEP_MODE_EXT_STANDBY
    SLEEP_MODE_STANDBY (Oscillator keeps running, makes for faster wake-up)
    SLEEP_MODE_PWR_DOWN (Deep sleep)
  */
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();

  // Turn of Brown Out Detection (low voltage)
  // Thanks to Nick Gammon for how to do this (temporarily) in software rather than
  // permanently using an avrdude command line.
  //
  // Note: Microchip state: BODS and BODSE only available for picoPower devices ATmega48PA/88PA/168PA/328P
  //
  // BODS must be set to one and BODSE must be set to zero within four clock cycles. This sets
  // the MCU Control Register (MCUCR)
  MCUCR = bit (BODS) | bit(BODSE);

  // The BODS bit is automatically cleared after three clock cycles so we better get on with it
  MCUCR = bit(BODS);

  // Ensure we can wake up again by first disabling interupts (temporarily) so
  // the wakeISR does not run before we are asleep and then prevent interrupts,
  // and then defining the ISR (Interrupt Service Routine) to run when poked awake
  noInterrupts();
  attachInterrupt(digitalPinToInterrupt(wakePin), wakeISR, LOW);

  if (preSleepAction != NULL) {
    (*preSleepAction)();
  }

  // Allow interrupts now
  interrupts();

  // And enter sleep mode as set above
  sleep_cpu();
}




void postWakeISRCleanup(byte *prevADCSRA) {
  // Clear existing alarm so int pin goes high again
  DS3231_clear_a1f();

  // Re-enable ADC if it was previously running
  ADCSRA = *prevADCSRA;
}



int32_t ElapsedMinutes(DS3231Time* pCurDayTime, uint16_t* pCurDayOfYear, DS3231Time* pTgtDayTime, uint16_t* pTgtDayOfYear) {
    DateDiff(pCurDayTime, pCurDayOfYear, pTgtDayTime, pTgtDayOfYear) / 60;
}


int32_t DateDiff(DS3231Time* pCurDayTime, uint16_t* pCurDayOfYear, DS3231Time* pTgtDayTime, uint16_t* pTgtDayOfYear)
{
    int32_t	seconds = 0;

    seconds = pCurDayTime->sec - pTgtDayTime->sec;
    seconds += (pCurDayTime->min - pTgtDayTime->min) * 60;

    // Shortcutting calculations below to skip unneccessary arithmetic
    if (pCurDayTime->hour != pTgtDayTime->hour) {
        seconds += (pCurDayTime->hour - pTgtDayTime->hour) * 60 * 60;
    }
    if (*pCurDayOfYear != *pTgtDayOfYear) {
        seconds += (*pCurDayOfYear - *pTgtDayOfYear) * 24 * 60 * 60;
    }
    if (pCurDayTime->year != pTgtDayTime->year) {
        // I am being sloppy here w.r.t. leap years.  So sue me
        seconds += (pCurDayTime->year - pTgtDayTime->year) * 365 * 24 * 60 * 60;
    }

    return seconds;
}



/*
    Params:
        dayOfYear       The one-based day of the year
        *mm             The zero-based corresponding month
        *dd             The corresponding day of the month
*/
void DDDtoMMDD(bool isLeapYear, int16_t dayOfYear, int8_t* mm, int8_t* dd)
{
    int8_t  i = 1;
    const uint16_t* monthDays = (isLeapYear ? _LeapMonthDays : _CommonMonthDays);

    while ((dayOfYear > monthDays[i]) && (i < 11))
    {
        i++;
    }
    *dd = dayOfYear - monthDays[i - 1];
    *mm = i;
}

/*
    Params:
        isLeapYear      If true it's a leap year
        mm              The 1-based  month
        dd              The corresponding day of the month
    Returns:
        dayOfYear       The one-based day of the year
*/
uint16_t MMDDtoDDD(bool isLeapYear, int8_t mm, int8_t dd)
{
    return (mm < 1 || mm > 12) ? -1 : (isLeapYear ? _LeapMonthDays[mm - 1] : _CommonMonthDays[mm - 1]) + dd;
}



uint16_t DS3231Time_MMDDtoDDD(DS3231Time* pCurDayTime)
{
    return MMDDtoDDD(IS_LEAP_YEAR(pCurDayTime->year), pCurDayTime->mon, pCurDayTime->mday);
}
