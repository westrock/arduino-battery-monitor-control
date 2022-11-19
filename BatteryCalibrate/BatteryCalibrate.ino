/*
 Name:		BatteryCalibrate.ino
 Created:	11/18/2022 23:05:50
 Author:	efigarsky
*/

// the setup function runs once when you press reset or power the board
#include <LiquidCrystal.h>
#include "Arduino.h"
#include "LCDHelper.h"

/*========================+
| #defines                |
+========================*/
#define VDIV_SCALE 4.50045                          //
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
#define ENABLE_WAIT_MINUTES	5
#define DATA_HOURS			24
#define BUFF_MAX			256


/*========================+
  | Local Types            |
  +========================*/


  /*========================+
  | Local Variables         |
  +========================*/


const uint8_t	rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal	lcd(rs, en, d4, d5, d6, d7);

/*========================+
| Function Definitions    |
+========================*/


float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis) {
	int32_t rawTempSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawTempSum += analogRead(tempPin);
	}
	return rawTempSum / static_cast<float>(samples) * tempScale * 100.0 / 1024.0;
}


float GetAverageRawVoltage(uint8_t voltagePin, uint8_t samples, uint16_t delayMillis) {
	int32_t rawVoltageSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawVoltageSum += analogRead(voltagePin);
	}
	return rawVoltageSum / static_cast<float>(samples);
}


float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis) {
	return GetAverageRawVoltage(voltagePin, samples, delayMillis) * voltageScale;
}




void setup() {
	// put your setup code here, to run once:
	Serial.begin(9600);

	// Set the voltage-monitoring pins
	pinMode(TEMP_SENSOR, INPUT);
	pinMode(V5_SENSOR, INPUT);
	pinMode(VBATT_RELAY, OUTPUT);

	// Set the VREF basis to the external value
	// and allow voltage to come in.
	digitalWrite(VBATT_RELAY, HIGH);
	delay(100);

	analogReference(EXTERNAL);

	Serial.println("Setup completed.");

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
	char buff[BUFF_MAX];
	char voltStr[6];
	char tempStr[6];
	char buffer[20];
	float tempSample;
	float scaledVoltage;
	uint16_t rawVoltageSample;
	uint16_t minutesDisabled;
	static bool tempSource = true;


	tempSample = GetAverageTemp(TEMP_SENSOR, VREFSCALE, 3, 5);

	rawVoltageSample = round(GetAverageRawVoltage(V5_SENSOR, 3, 5));
	scaledVoltage = rawVoltageSample * VREFSCALE;

	formatFloat(scaledVoltage, voltStr, 5, 2);
	formatFloat(tempSample, tempStr, 5, 1);

	//sprintf(buffer, "V: %s T%d: %s", voltStr, tempSource + 1, tempStr);
	sprintf(buffer, "%sv %s%c", voltStr, tempStr);
	lcd.setCursor(0, 0);
	lcdPrint(lcd, buffer, 20);

	delay(250);
}


