/**
 *  Name:       BatteryMonitorControl.ino
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

#include <Wire.h>
#include <LiquidCrystal.h>
#include "Arduino.h"
#include <avr/sleep.h>
#include "ds3231.h"
#include "config.h"

#include "DataAcquisitionAndReporting.h"
#include "LCDHelper.h"
#include "HourlyDataTypes.h"
#include "DS3231Helpers.h"
#include "DateTimeHelpers.h"


/*==========================+
|	#defines				|
+==========================*/
#define VDIV_SCALE 4.588694								//
#define VREG 5.0										//

#define DEBUGSERIAL

#ifdef DEBUGSERIAL
	#define DebugPrint(x) Serial.print(x)
	#define DebugPrintln(x) Serial.println(x)
	#define DebugFlush() Serial.flush()
#else
	#define DebugPrint(x) /*Serial.print(x);*/
	#define DebugPrintln(x) /*Serial.println(x);*/
	#define DebugFlush()	/*Serial.flush();*/
#endif

#ifdef USE_EXTERNALVREF
	#define VREF_RESISTOR 7.7							//
	#define VREF VREG * 32 / (32 + VREF_RESISTOR)		//
	#define VREFSCALE VREF / 1024.0 * VDIV_SCALE		//
#else
	#define VREFSCALE VREG / 1024.0 * VDIV_SCALE		//
#endif

#define TEMP_SENSOR				A3					// An LM34 thermometer
#define MODE_SWITCH				A2					// Used as a digital switch in a companion app
#define V5_SENSOR				A1					// This pin measures the VREF-limited external (battery) voltage
#define VBATT_RELAY				9					// This relay allows current to flow in to the pin at V5_SENSOR

#define RTC_WAKE_ALARM			2					// when low, makes 328P wake up, must be an interrupt pin (2 or 3 on ATMEGA328P)
#define WAKE_SLEEP_BUTTON		3					// When low, makes 328P go to sleep
#define LED_PIN					4					// output pin for the LED (to show it is awake)

#define DISABLE_VOLTAGE			11.25
#define ENABLE_VOLTAGE			11.30
#define ENABLE_WAIT_MINUTES		2					// <<---- 
#define BUFF_MAX				256
#define REPORTING_DELAY_SECONDS	6
#define BINARY_RELAY			1
#define PWM_RELAY				2
#define RELAY_TYPE				PWM_RELAY

#ifdef DATA_HOURS
	#if !(DATA_HOURS==1 || DATA_HOURS==2 || DATA_HOURS==3 || DATA_HOURS==4 || DATA_HOURS==6 || DATA_HOURS==12 || DATA_HOURS==24)
		#error "DATA_HOURS must be either 1, 2, 3, 4, 6, 12, or 24."
	#endif
	#else
		#error "DATA_HOURS is not defined."
#endif

#ifdef RELAY_TYPE
	#if !(RELAY_TYPE==BINARY_RELAY || RELAY_TYPE==PWM_RELAY)
		#error "RELAY_TYPE must be either BINARY_RELAY or PWM_RELAY."
	#endif
	#else
		#error "RELAY_TYPE is not defined."
#endif

/*==========================+
| Local structs				|
+==========================*/
struct currentSampleStruct
{
	DateTimeDS3231	timeNow;
	float			tempSample;
	float			scaledVoltage;
	uint16_t		minutesDisabled = 0;
};
typedef struct currentSampleStruct CurrentSample;


/*========================+
| Local Variables         |
+========================*/
volatile bool	sleepRequested = true;
volatile bool	wakeSleepISRSet = false;

static ReportControl	reportControl;
static SamplingData		samplingData;
static bool				isOutputRelayClosed = false;		// If not Closed then no power goes through.  If Closed power flows.

const uint8_t	rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal	lcd(rs, en, d4, d5, d6, d7);

/*========================+
| Function Definitions    |
+========================*/
void doBlink(uint8_t ledPin);
void DisablePower(SamplingData* samplingData, DateTimeDS3231* now, bool* isRelayClosed, uint8_t powerRelay);
void EnablePower(SamplingData* samplingData, DateTimeDS3231* now, bool* isRelayClosed, uint8_t powerRelay);
void SetupRecovery(SamplingData* samplingData, DateTimeDS3231* now, uint8_t recoveryDurationMinutes);
void RecordTimeDisabled(SamplingData* samplingData, CurrentSample* currentSample);
void DoWakingTasks(SamplingData * samplingData);
void DisplayCurrentStatus(SamplingData* samplingData, CurrentSample* currentSample);
void CloseCurrentAndPrepNewHourWithSample(SamplingData* samplingData, CurrentSample* currentSample, uint16_t* rawVoltage);
void preSleep();
void wakeSleepControlISR();
void realTimeClockWakeISR();
bool openRelay(uint8_t pin);
bool openRelay(uint8_t pin, bool immediate);
bool closeRelay(uint8_t pin);
bool closeRelay(uint8_t pin, bool immediate);
void printCharInHexadecimal(char* str, int len);


void setup() {
#ifdef USE_EXTERNALVREF
	analogReference(EXTERNAL);
#else
	analogReference(DEFAULT);
#endif

	// Start LCD and Serial
	lcd.begin(20, 4);

#ifdef DEBUGSERIAL
	Serial.begin(9600);
#endif

	// Set the voltage-monitoring, temperature, button, and RTC alarm pins
	pinMode(TEMP_SENSOR, INPUT);
	pinMode(V5_SENSOR, INPUT);
	pinMode(VBATT_RELAY, OUTPUT);
	pinMode(WAKE_SLEEP_BUTTON, INPUT_PULLUP);
	pinMode(RTC_WAKE_ALARM, INPUT_PULLUP);
	pinMode(LED_PIN, OUTPUT);

	// Open the relay to prevent power out until we establish what's what
	isOutputRelayClosed = openRelay(VBATT_RELAY, true);
	samplingData.isPowerOutDisabled = true;
	samplingData.isIntialized = false;

	// Clear the current alarm (puts DS3231 INT high)
	Wire.begin();
	DS3231_init(DS3231_CONTROL_INTCN);
	DS3231_clear_a1f();

	PrepHourlyData(&samplingData.hourlyData[0], DATA_HOURS);
	reportControl.previousTime = GetTime();

	CreateArrows(lcd);

	DebugPrintln(F("Setup completed."));
}


void loop()
{
	static bool firstTime = true;
	static byte prevADCSRA;
	char buff[BUFF_MAX];

	if (!wakeSleepISRSet)
	{
		// Wake/Sleep Interrupts are not set up or were disabled. Set up the button
		noInterrupts();
		attachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON), wakeSleepControlISR, LOW);
		wakeSleepISRSet = true;
		interrupts();
		DebugPrintln(F("wakeSleepISRSet ACTIVATED"));
	}

	// Just blink LED twice to show we're running
	doBlink(LED_PIN);

	// Has the button on the "go to sleep" pin been toggled?
	if (sleepRequested)
	{
		DoWakingTasks(&samplingData);
		DebugPrintln(F("sleep REQUESTED"));

		setAlarmAndSleep(RTC_WAKE_ALARM, realTimeClockWakeISR, preSleep, &prevADCSRA, 0, 0, 10);	// 0, 1, 0
		postWakeISRCleanup(&prevADCSRA);
	}
	else
	{
		DoReportingTasks(&samplingData, &reportControl, lcd, REPORTING_DELAY_SECONDS);
	}
}



// Double blink just to show we are running. Note that we do NOT
// use the delay for the final delay here, this is done by checking
// millis instead (non-blocking)
void doBlink(uint8_t ledPin) {
	static unsigned long lastMillis = 0;

	if (millis() > lastMillis + 1000)
	{
		digitalWrite(ledPin, HIGH);
		delay(10);
		digitalWrite(ledPin, LOW);
		delay(200);
		digitalWrite(ledPin, HIGH);
		delay(10);
		digitalWrite(ledPin, LOW);
		lastMillis = millis();
	}
}

void DisablePower(SamplingData* samplingData, DateTimeDS3231 *now, bool *isRelayClosed, uint8_t powerRelay)
{
	DebugPrintln(F("in DisablePower()"));

	samplingData->isPowerOutDisabled = true;
	samplingData->timeDisabled = *now;
	if (*isRelayClosed)
	{
		*isRelayClosed = openRelay(powerRelay);
	}
}

void EnablePower(SamplingData* samplingData, DateTimeDS3231 *now, bool* isRelayClosed, uint8_t powerRelay)
{
	DebugPrintln(F("in EnablePower()"));

	samplingData->isPowerOutDisabled = false;
	samplingData->isPowerOutRecovering = false;
	samplingData->timeEnabled = *now;
	if (!*isRelayClosed)
	{
		*isRelayClosed = closeRelay(powerRelay);
	}
	
}

void SetupRecovery(SamplingData* samplingData, DateTimeDS3231 *now, uint8_t recoveryDurationMinutes)
{
	DebugPrintln(F("in SetupRecovery()"));

	samplingData->isPowerOutRecovering = true;
	samplingData->timeRecoveryStarted = *now;
	samplingData->recoveryTime = *now;
	addMinutes(&samplingData->recoveryTime, recoveryDurationMinutes);
	DebugPrint(F("Will recover in "));
	DebugPrint(recoveryDurationMinutes);
	DebugPrintln(F(" minutes"));
	DebugPrintln(samplingData->isPowerOutRecovering);
}

void RecordTimeDisabled(SamplingData* samplingData, CurrentSample *currentSample)
{
	DebugPrintln(F("in RecordTimeDisabled()"));

	if (samplingData->timeDisabled.hour == currentSample->timeNow.hour)
	{
		samplingData->currentHourData.downMinutes += (currentSample->minutesDisabled > 60) ? 60 : currentSample->minutesDisabled; // Account for 24 hours+ downtime
	}
	else
	{
		// The power was disabled in an earlier hour - it crossed hour boundaries.
		if (currentSample->timeNow.hour == samplingData->currentHour)
		{
			samplingData->currentHourData.downMinutes += currentSample->timeNow.min;
		}
	}
}



void DoWakingTasks(SamplingData *samplingData)
{
	CurrentSample	currentSample;
	//DateTimeDS3231	timeNow;
	char			voltStr[6];
	char			tempStr[6];
	char			buffer[20];
	//float			tempSample;
	//float			scaledVoltage;
	uint16_t		rawVoltageSample;
	//uint16_t		minutesDisabled = 0;
	static bool		tempSource = true;

	DebugPrintln(F("Waking"));

	DS3231_get(&currentSample.timeNow);

	currentSample.tempSample = GetAverageDS3231Temp(3, 5);
	rawVoltageSample = round(GetAverageRawVoltage(V5_SENSOR, 3, 5));
	currentSample.scaledVoltage = rawVoltageSample * VREFSCALE;

	if (currentSample.scaledVoltage < DISABLE_VOLTAGE)
	{
		DebugPrintln(F("scaledVoltage < DISABLE_VOLTAGE"));

		if (!samplingData->isIntialized)
		{
			DebugPrintln(F("!samplingData->isIntialized"));

			samplingData->isPowerOutRecovering = false;
			samplingData->isPowerOutDisabled = false;
			samplingData->isIntialized = true;
		}
		if (!samplingData->isPowerOutDisabled || samplingData->isPowerOutRecovering)
		{
			if (!samplingData->isPowerOutDisabled)
			{
				DisablePower(samplingData, &currentSample.timeNow, &isOutputRelayClosed, VBATT_RELAY);
			}

			if (samplingData->isPowerOutRecovering)
			{
				samplingData->isPowerOutRecovering = false;
				lcdClearLine(lcd, 2);
			}
		}
	}

	if (samplingData->isPowerOutDisabled)
	{
		currentSample.minutesDisabled = dateDiffMinutes(&currentSample.timeNow, &samplingData->timeDisabled);
	}

	if (currentSample.scaledVoltage >= ENABLE_VOLTAGE)
	{
		DebugPrintln(F("scaledVoltage >= ENABLE_VOLTAGE"));

		if (!samplingData->isIntialized)
		{
			DebugPrintln(F("!samplingData->isIntialized"));

			EnablePower(samplingData, &currentSample.timeNow, &isOutputRelayClosed, VBATT_RELAY);
			samplingData->isIntialized = true;
		}
		else
		{
			if (samplingData->isPowerOutDisabled)
			{
				if (!samplingData->isPowerOutRecovering)
				{
					SetupRecovery(samplingData, &currentSample.timeNow, ENABLE_WAIT_MINUTES);
				}
				else
				{
					int32_t secondsSpentRecovering = dateDiffSeconds(&currentSample.timeNow, &samplingData->timeRecoveryStarted);

					DebugPrint(F("Seconds Spent Recovering: "));
					DebugPrintln(secondsSpentRecovering);

					if (secondsSpentRecovering >= ENABLE_WAIT_MINUTES * 60)
					{
						RecordTimeDisabled(samplingData, &currentSample);
						EnablePower(samplingData, &currentSample.timeNow, &isOutputRelayClosed, VBATT_RELAY);
						lcd.clear();
					}
				}
			}
		}
	}

	//if (samplingData->isPowerOutRecovering && (currentSample.scaledVoltage >= DISABLE_VOLTAGE || currentSample.scaledVoltage < ENABLE_VOLTAGE))
	//{
	//	samplingData->isPowerOutRecovering = false;
	//	lcdClearLine(lcd, 2);
	//}

	DebugFlush();

	if (currentSample.timeNow.hour != samplingData->currentHour)
	{
		CloseCurrentAndPrepNewHourWithSample(samplingData, &currentSample, &rawVoltageSample);
	}
	else
	{
		AddSampleToCurrentHour(&samplingData->currentHourData, &currentSample.timeNow, &rawVoltageSample, &currentSample.tempSample);
	}

	DisplayCurrentStatus(samplingData, &currentSample);

}


void DisplayCurrentStatus(SamplingData *samplingData, CurrentSample *currentSample)
{
	char			buffer[20];
	char			voltStr[6];
	char			tempStr[6];

	formatFloat(currentSample->scaledVoltage, voltStr, 5, 2);
	formatFloat(currentSample->tempSample, tempStr, 5, 1);

	//sprintf(buffer, "V: %s T%d: %s", voltStr, tempSource + 1, tempStr);
	sprintf(buffer, "%sv %s%c %02d:%02d", voltStr, tempStr, 0xDF, currentSample->timeNow.hour, currentSample->timeNow.min);
	lcd.setCursor(0, 0);
	lcdPrint(lcd, buffer, 20);

	lcd.setCursor(0, 1);
	if (samplingData->isPowerOutDisabled)
	{
		sprintf(buffer, "Power off %d mins", currentSample->minutesDisabled);
	}
	else
	{
		strcpy(buffer, "Power ON");
	}
	lcdPrint(lcd, buffer, 20);


	if (samplingData->isPowerOutRecovering)
	{
		ElapsedTime timeToRecover = dateDiff(&currentSample->timeNow, &samplingData->recoveryTime);
		lcd.setCursor(0, 2);
		sprintf(buffer, "Recovery in %2d:%02d", abs(timeToRecover.minute), abs(timeToRecover.second));
		lcdPrint(lcd, buffer, 20);
	}
}

void CloseCurrentAndPrepNewHourWithSample(SamplingData *samplingData, CurrentSample *currentSample, uint16_t *rawVoltage)
{
	if (samplingData->currentHour != -1)
	{
		DebugPrint(F("Closing current hour "));
		DebugPrint(samplingData->currentHour);
		DebugPrint(F(", minutesDisabled "));
		DebugPrintln(currentSample->minutesDisabled);
		DebugFlush();

		if (samplingData->isPowerOutDisabled)
		{
			samplingData->currentHourData.downMinutes += (currentSample->minutesDisabled > 60) ? 60 : currentSample->minutesDisabled;
		}
		CloseCurrentHour(samplingData->hourlyData, &samplingData->currentHourData, samplingData->currentHour % DATA_HOURS);
	}
	PrepCurrentHour(&samplingData->currentHourData, &currentSample->timeNow, rawVoltage, &currentSample->tempSample);
	samplingData->currentHour = currentSample->timeNow.hour;
}


void preSleep()
{
	// Send a message just to show we are about to sleep
	DebugPrintln(F("Going to sleep now."));
	DebugFlush();
}



// When WAKE_SLEEP_BUTTON is brought LOW this interrupt is triggered
void wakeSleepControlISR()
{
	// Detach the interrupt that brought us here
	detachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON));
	sleepRequested = !sleepRequested;
	wakeSleepISRSet = false;

	delay(5);
	while (digitalRead(WAKE_SLEEP_BUTTON) == LOW) {
		delay(5);
	}
}


// When RTC_WAKE_ALARM is brought LOW this interrupt is triggered FIRST (even in PWR_DOWN sleep)
void realTimeClockWakeISR() {
	// Prevent sleep mode, so we don'timeNow enter it again, except deliberately, by code
	sleep_disable();

	// Detach the interrupt that brought us out of sleep
	detachInterrupt(digitalPinToInterrupt(RTC_WAKE_ALARM));

	// Now we continue running the main Loop() just after we went to sleep
}


bool openRelay(uint8_t pin)
{
	return openRelay(pin, false);
}


bool openRelay(uint8_t pin, bool immediate)
{
	DebugPrintln(F("opening relay"));

#if (RELAY_TYPE==PWM_RELAY)
	if (!immediate)
	{
		for (int i = 254; i > 0; i--)
		{
			analogWrite(pin, i);
			delay(4);
		}
	}
#endif
	digitalWrite(pin, LOW);
	return false;
}


bool closeRelay(uint8_t pin)
{
	return closeRelay(pin, false);
}


bool closeRelay(uint8_t pin, bool immediate)
{
	DebugPrintln(F("closing relay"));

#if (RELAY_TYPE==PWM_RELAY)
	if (!immediate)
	{
		for (int i = 0; i < 255; i++)
		{
			analogWrite(pin, i);
			delay(4);
		}
	}
#endif
	digitalWrite(pin, HIGH);
	return true;
}


void printCharInHexadecimal(char* str, int len) {
	for (int i = 0; i < len; ++i) {
		unsigned char val = str[i];
		char tbl[] = "0123456789ABCDEF";
		DebugPrint(F("0x"));
		DebugPrint(tbl[val / 16]);
		DebugPrint(tbl[val % 16]);
		DebugPrint(F(" "));
	}
	DebugPrintln();
}



