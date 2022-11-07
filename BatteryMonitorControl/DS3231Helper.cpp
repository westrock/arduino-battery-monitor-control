#include "DS3231Helper.h"
#include <ds3231.h>
#include <avr/sleep.h>

bool rtcIsRunning() {
  return ((DS3231_get_sreg() & DS3231_STATUS_OSF) != 0);
}




// Set the next alarm
void setNextAlarm(uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds)
{
  struct ts timeData;
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
