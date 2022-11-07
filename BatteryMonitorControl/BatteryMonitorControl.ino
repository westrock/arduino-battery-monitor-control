// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
	Name:       BatteryMonitorControl.ino
	Created:	11/6/2022 19:21:14
	Author:     NUC8\efigarsky
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

const uint8_t rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
struct ts t;

VMinVMaxTMinTMax dataPoints[24];

// DS3231 alarm time

/*========================+
| Function Definitions    |
+========================*/
float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis);
float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis);
void doBlink(uint8_t ledPin);
void sleepISR();
void setNextAlarm(uint8_t wakeInHours, uint8_t wakeInMinutes, uint8_t wakeInSeconds);

void printCharInHexadecimal(char* str, int len);


void setup() {
	// put your setup code here, to run once:
	int hour;

	for (hour = 0; hour < 24; hour++) {
		dataPoints[hour].vMin = 0xFFFF;
		dataPoints[hour].vMax = 0x0000;
		dataPoints[hour].tMin = 0xFFFF;
		dataPoints[hour].tMax = 0x0000;
	}

	Serial.begin(115200);

	lcd.begin(20, 4);

	// Set the voltage-monitoring pins
	pinMode(TEMP_SENSOR, INPUT);
	pinMode(V5_SENSOR, INPUT);
	pinMode(VBATT_RELAY, OUTPUT);

	analogReference(EXTERNAL);

	digitalWrite(VBATT_RELAY, HIGH);
	delay(500);

	// Keep pins high until we ground them
	pinMode(WAKE_SLEEP_PIN, INPUT_PULLUP);
	pinMode(RTC_WAKE_PIN, INPUT_PULLUP);

	// Flashing LED just to show the �Controller is running
	digitalWrite(LED_PIN, LOW);
	pinMode(LED_PIN, OUTPUT);

	// Clear the current alarm (puts DS3231 INT high)
	Wire.begin();
	DS3231_init(DS3231_CONTROL_INTCN);

	if (!rtcIsRunning()) {
		Serial.println("DS3231 is NOT running.");
	}

	DS3231_clear_a1f();

	// Set the VREF basis to the external value
	// and allow voltage to come in.
	analogReference(EXTERNAL);
	digitalWrite(VBATT_RELAY, HIGH);
	delay(100);

	Serial.println("Setup completed.");
}

int tempInt = analogRead(TEMP_SENSOR) * VREF * 100 / 1024.0;


float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis) {
	int32_t rawTempSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawTempSum += analogRead(tempPin);
	}
	return rawTempSum / samples * tempScale * 100.0 / 1024.0;
}


float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis) {
	int32_t rawVoltageSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawVoltageSum += analogRead(voltagePin);
	}
	return rawVoltageSum / samples * voltageScale;
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
	char voltStr[6];
	char tempStr[6];
	char buffer[20];
	float temp;
	static bool tempSource = true;

	float v5 = GetAverageVoltage(V5_SENSOR, VREFSCALE, 5, 10);

	if (tempSource) {
		temp = GetAverageTemp(TEMP_SENSOR, VREF, 5, 10);
	}
	else {
		temp = DS3231_get_treg() * 1.8 + 32.0;
	}
	tempSource = !tempSource;

	FormatVoltage(v5, voltStr, 5, 2);
	FormatVoltage(temp, tempStr, 5, 1);

	sprintf(buffer, "V: %s T%d: %s", voltStr, tempSource + 1, tempStr);
	lcd.setCursor(0, 0);
	LcdPrint(lcd, buffer, 20);

	DS3231_get(&t);

	// display current time
	snprintf(buffer, 20, "%02d/%02d/%02d %02d:%02d:%02d", t.year_s, t.mon, t.mday, t.hour, t.min, t.sec);

	lcd.setCursor(0, 1);
	LcdPrint(lcd, buffer, 20);


	/*
	  float tReg = DS3231_get_treg() * 1.8 + 32.0;
	  Serial.print("Temperature is ");
	  Serial.println(tReg);
	*/

	//  Serial.print("Temperature is ");
	//  Serial.println(tempInt);
	delay(20000);
	return;
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
	sleep_disable()
		;

	// Detach the interrupt that brought us out of sleep
	detachInterrupt(digitalPinToInterrupt(RTC_WAKE_PIN));

	// Now we continue running the main Loop() just after we went to sleep
}
