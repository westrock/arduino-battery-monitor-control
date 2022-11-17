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
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license GPL-3.0+ <http://spdx.org/licenses/GPL-3.0+>
 */

#include <config.h>
#include <ds3231.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include "Arduino.h"
#include <avr/sleep.h>

#include "DataAcquisition.h"
#include "LCDHelper.h"
#include "HourlyDataTypes.h"
#include "DS3231Helper.h"
#include "DataAcquisition.h"

/*========================+
| #defines                |
+========================*/
#define VDIV_SCALE 4.555                          //
#define VREG 5.0                                  //
#define VREF_RESISTOR 7.7                         //
#define VREF VREG * 32 / (32 + VREF_RESISTOR)     //
#define VREFSCALE VREF / 1024.0 * VDIV_SCALE      //
#define TEMP_SENSOR A0                            // An LM34 thermometer
#define V5_SENSOR A1                              // This pin measures the VREF-limited external (battery) voltage
#define VBATT_RELAY 4                             // This relay allows current to flow in to the pin at V5_SENSOR

#define WAKE_SLEEP_PIN 2                                // When low, makes 328P go to sleep
#define RTC_WAKE_PIN 3                            // when low, makes 328P wake up, must be an interrupt pin (2 or 3 on ATMEGA328P)
#define LED_PIN 9                                 // output pin for the LED (to show it is awake)

#define DISABLE_VOLTAGE		8.9
#define ENABLE_VOLTAGE		9.0
#define ENABLE_WAIT_MINUTES	15
#define DATA_HOURS			24
#define BUFF_MAX			256

#ifdef DATA_HOURS
#if !(DATA_HOURS==1 || DATA_HOURS==2 || DATA_HOURS==3 || DATA_HOURS==4 || DATA_HOURS==6 || DATA_HOURS==12 || DATA_HOURS==24)
#error "DATA_HOURS must be either 1, 2, 3, 4, 6, 12, or 24."
#endif
#else
#error "DATA_HOURS is not defined."
#endif

/*========================+
  | Local Types            |
  +========================*/


/*========================+
| Local Variables         |
+========================*/
volatile bool	sleepRequested = true;
volatile bool	wakeSleepISRSet = false;

bool			isPowerOutDisabled = false;
uint16_t		currentDDD = -1;					// The current day of year.  Used to help caluclate time intervals and evaluate power events
uint8_t			currentDay = -1;					// The current day.  Used to help caluclate time intervals and evaluate power events
uint8_t			currentHour = -1;					// The current hour.  Used to store/update samples
CurrentHourData	currentHourData;					// Total, min, and max values for the current hour
HourlyData		hourlyData[DATA_HOURS];				//
DS3231Time		timeDisabled;


const uint8_t	rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal	lcd(rs, en, d4, d5, d6, d7);

// DS3231 alarm time

/*========================+
| Function Definitions    |
+========================*/
void doBlink(uint8_t ledPin);
void sleepISR();
void preSleep();
void DoWakingTasks();


void printCharInHexadecimal(char* str, int len);


void setup() {
	// put your setup code here, to run once:
	Serial.begin(9600);

	// Set the voltage-monitoring pins
	pinMode(TEMP_SENSOR, INPUT);
	pinMode(V5_SENSOR, INPUT);
	pinMode(VBATT_RELAY, OUTPUT);
	pinMode(WAKE_SLEEP_PIN, INPUT_PULLUP);
	pinMode(RTC_WAKE_PIN, INPUT_PULLUP);

	// Set the VREF basis to the external value
	// and allow voltage to come in.
	digitalWrite(VBATT_RELAY, HIGH);
	delay(100);

	analogReference(EXTERNAL);

	// Flashing LED just to show the �Controller is running
	digitalWrite(LED_PIN, LOW);
	pinMode(LED_PIN, OUTPUT);

	lcd.begin(20, 4);
	CreateArrows(lcd);

	// Clear the current alarm (puts DS3231 INT high)
	Wire.begin();
	DS3231_init(DS3231_CONTROL_INTCN);

	DS3231_clear_a1f();

	PrepHourlyData(hourlyData, DATA_HOURS);

	Serial.println("Setup completed.");

	DS3231Time t;
	getDS3231Time(&t);

	uint16_t dayOfYear = MMDDtoDDD(IS_LEAP_YEAR(t.year), t.mon, t.mday);

	Serial.println(t.mday);
	Serial.println(dayOfYear);
	Serial.println(t.wday);

	DoWakingTasks();
}


void printCharInHexadecimal(char* str, int len) {
	for (int i = 0; i < len; ++i) {
		unsigned char val = str[i];
		char tbl[] = "0123456789ABCDEF";
		Serial.print("0x");
		Serial.print(tbl[val / 16]);
		Serial.print(tbl[val % 16]);
		Serial.print(" ");
	}
	Serial.println();
}

void loop() {
	DS3231Time t;
	static byte prevADCSRA;
	static uint8_t oldSec = 99;
	char buff[BUFF_MAX];

	if (!wakeSleepISRSet) {
		noInterrupts();
		attachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_PIN), wakeSleepControlISR, LOW);
		wakeSleepISRSet = true;
		interrupts();
		Serial.println("wakeSleepISRSet ACTIVATED");
	}

	// Just blink LED twice to show we're running
	doBlink(LED_PIN);

	//  if (digitalRead(WAKE_SLEEP_PIN) == LOW) {
	//    sleepRequested = !sleepRequested;
	//  }

	// Is the "go to sleep" pin now LOW?
	if (sleepRequested)
	{
		Serial.println("sleep REQUESTED");

		setAlarmAndSleep(RTC_WAKE_PIN, sleepISR, preSleep, &prevADCSRA, 0, 1, 0);
		postWakeISRCleanup(&prevADCSRA);

		DoWakingTasks();
	}
	else
	{
		// Get the time
		DS3231_get(&t);

		// If the seconds has changed, display the (new) time
		if (t.sec != oldSec)
		{
			// display current time
			snprintf(buff, BUFF_MAX, "%d.%02d.%02d %02d:%02d:%02d\n", t.year,
				t.mon, t.mday, t.hour, t.min, t.sec);
			Serial.print(buff);
			Serial.println(t.year);
			oldSec = t.sec;
		}
	}
}


void DoWakingTasks() {
	DS3231Time t;
	char voltStr[6];
	char tempStr[6];
	char buffer[20];
	float tempSample;
	float scaledVoltage;
	uint16_t rawVoltageSample;
	static bool tempSource = true;

	Serial.println("Waking");

	getDS3231Time(&t);

	tempSample = GetAverageDS3231Temp(3, 5);
	rawVoltageSample = round(GetAverageRawVoltage(V5_SENSOR, 3, 5));
	scaledVoltage = rawVoltageSample * VREFSCALE;

	if (t.hour != currentHour) {
		if (currentHour != -1) {
			CloseCurrentHour(hourlyData, &currentHourData, DATA_HOURS % currentHour);
		}
		PrepCurrentHour(&currentHourData, &t, &rawVoltageSample, &tempSample);
		currentHour = t.hour;
	}
	else {
		AddSampleToCurrentHour(&currentHourData, &t, &rawVoltageSample, &tempSample);
	}

	if (scaledVoltage < DISABLE_VOLTAGE && !isPowerOutDisabled) {
		if (!isPowerOutDisabled) {
			isPowerOutDisabled = true;
			// Open relay
		}
		timeDisabled = t;		// Set/Update the most recent time the voltage is below the threshold
	}
	else {
		if (scaledVoltage >= ENABLE_VOLTAGE && isPowerOutDisabled) {
			isPowerOutDisabled = false;
			// Close relay
		}
	}

	FormatFloat(scaledVoltage, voltStr, 5, 2);
	FormatFloat(tempSample, tempStr, 5, 1);

	//sprintf(buffer, "V: %s T%d: %s", voltStr, tempSource + 1, tempStr);
	sprintf(buffer, "%sv %s%c %02d:%02d", voltStr, tempStr, 0xDF, t.hour, t.min);
	lcd.setCursor(0, 0);
	LcdPrint(lcd, buffer, 20);

	lcd.setCursor(0, 1);
	lcd.write((byte)LCD_DOWN_ARROW);
	lcd.write((byte)LCD_UP_ARROW);
	
	sprintf(buffer, "        ");
	buffer[0] = (byte)LCD_DOWN_ARROW;
	buffer[2] = (byte)LCD_UP_ARROW;
	buffer[4] = (byte)LCD_DOWN_ARROW;
	buffer[6] = (byte)LCD_UP_ARROW;
	lcd.setCursor(0, 2);
	lcd.write(buffer, strlen(buffer));
	//LcdPrint(lcd, buffer, 20);

	//sprintf(buffer, "V: %s T%d: %s", voltStr, tempSource + 1, tempStr);
	//sprintf(buffer, "%s%c", tempStr, 0xDF);
	//lcd.setCursor(0, 1);
	//LcdPrint(lcd, buffer, 20);

	// display current time
	//snprintf(buffer, 20, "%02d/%02d/%02d %02d:%02d:%02d", t.year_s, t.mon, t.mday, t.hour, t.min, t.sec);

	//lcd.setCursor(0, 1);
	//LcdPrint(lcd, buffer, 20);
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


void preSleep() {
	// Send a message just to show we are about to sleep
	Serial.println("Going to sleep now.");
	Serial.flush();
}



// When WAKE_SLEEP_PIN is brought LOW this interrupt is triggered
void wakeSleepControlISR() {
	// Detach the interrupt that brought us here
	detachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_PIN));
	sleepRequested = !sleepRequested;
	wakeSleepISRSet = false;

	delay(5);
	while (digitalRead(WAKE_SLEEP_PIN) == LOW) {
		delay(5);
	}
}


// When RTC_WAKE_PIN is brought LOW this interrupt is triggered FIRST (even in PWR_DOWN sleep)
void sleepISR() {
	// Prevent sleep mode, so we don't enter it again, except deliberately, by code
	sleep_disable();

	// Detach the interrupt that brought us out of sleep
	detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));

	// Now we continue running the main Loop() just after we went to sleep
}
