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

#include "LCDHelper.h"
#include "HourlyDataTypes.h"
#include "DS3231Helper.h"

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

#define DATA_HOURS      24
#define BUFF_MAX 256

/*========================+
  | Local Types            |
  +========================*/

struct vMinMaxTMinMax {
	uint16_t vMin;
	uint16_t vMax;
	uint16_t tMin;
	uint16_t tMax;
};

typedef struct vMinMaxTMinMax VMinVMaxTMinTMax;

/*========================+
| Local Variables         |
+========================*/
volatile bool sleepRequested = true;
volatile bool wakeSleepISRSet = false;

uint8_t currentHour;					// The current hour.  Used to store/update samples
CurrentHourData currentHourData;		// Total, min, and max values for the current hour
HourlyData hourlyData[DATA_HOURS];		// 


const uint8_t rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
struct ts t;

VMinVMaxTMinTMax dataPoints[DATA_HOURS];

// DS3231 alarm time

/*========================+
| Function Definitions    |
+========================*/
float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis);
float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis);
void doBlink(uint8_t ledPin);
void sleepISR();
void preSleep();
void doWakingTasks();


void printCharInHexadecimal(char* str, int len);


void setup() {
	// put your setup code here, to run once:
	Serial.begin(9600);

	for (int hour = 0; hour < DATA_HOURS; hour++) {
		dataPoints[hour].vMin = 0xFFFF;
		dataPoints[hour].vMax = 0x0000;
		dataPoints[hour].tMin = 0xFFFF;
		dataPoints[hour].tMax = 0x0000;
	}

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
	// Clear the current alarm (puts DS3231 INT high)
	Wire.begin();
	DS3231_init(DS3231_CONTROL_INTCN);

	if (!rtcIsRunning()) {
		Serial.println("DS3231 is NOT running.");
	}

	DS3231_clear_a1f();

	DS3231_get(&t);
	prepCurrentHour(&currentHourData, t.hour);
	prepHourlyData(hourlyData, DATA_HOURS);

	Serial.println("Setup completed.");
}



float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis) {
	int32_t rawTempSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawTempSum += analogRead(tempPin);
	}
	return rawTempSum / static_cast<float>(samples) * tempScale * 100.0 / 1024.0;
}


float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis) {
	int32_t rawVoltageSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawVoltageSum += analogRead(voltagePin);
	}
	return rawVoltageSum / static_cast<float>(samples) * voltageScale;
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

		doWakingTasks();
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

void doWakingTasks() {
	char voltStr[6];
	char tempStr[6];
	char buffer[20];
	float temp;
	static bool tempSource = true;

	Serial.println("Waking");

	DS3231_get(&t);

	float v5 = GetAverageVoltage(V5_SENSOR, VREFSCALE, 5, 10);

	//if (tempSource) {
	//	temp = GetAverageTemp(TEMP_SENSOR, VREF, 5, 10);
	//}
	//else {
		temp = DS3231_get_treg() * 1.8 + 32.0;
	//}
	//tempSource = !tempSource;

	FormatFloat(v5, voltStr, 5, 2);
	FormatFloat(temp, tempStr, 5, 1);

	//sprintf(buffer, "V: %s T%d: %s", voltStr, tempSource + 1, tempStr);
	sprintf(buffer, "%sv %s%c %02d:%02d", voltStr, tempStr, 0xDF, t.hour, t.min);
	lcd.setCursor(0, 0);
	LcdPrint(lcd, buffer, 20);

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
