/*
 Name:		VrefScaleSetup.ino
 Created:	12/18/2022 16:10:19
 Author:	efigarsky
*/

#include <LiquidCrystal.h>
#include "LCDHelper.h"



/*==========================+
|	#defines				|
+==========================*/
#define VDIV_SCALE 4.588694								//
#define VREG 5.0										//
#define VREFSCALE(x) VREG / 1024.0 * (x)				//

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

#define MODE_SWITCH				A2					// Used as a digital switch in a companion app
#define V5_SENSOR				A1					// This pin measures the VREF-limited external (battery) voltage

#define WAKE_SLEEP_BUTTON		3					// When low, makes 328P go to sleep
#define LED_PIN					4					// output pin for the LED (to show it is awake)


/*========================+
| enums			          |
+========================*/
enum buttonState	{ STATE_NOT_SET = 0, STATE_WENT_LOW = 1, STATE_WENT_HIGH = 2, STATE_WAS_PRESSED = 3 };
enum pressType		{ PRESS_NOT_SET = 0, PRESS_LONG = 1, PRESS_SHORT = 2 };


/*========================+
| Local Variables         |
+========================*/
volatile bool	isLongPress = false;
volatile bool	buttonPressed = false;
volatile bool	wakeSleepISRSet = false;
volatile float	vDivScale = VDIV_SCALE;

const uint8_t	rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal	lcd(rs, en, d4, d5, d6, d7);

/*========================+
| Function Definitions    |
+========================*/

void doBlink(uint8_t ledPin);
void wakeSleepLowISR();
void wakeSleepHighISR();
float GetAverageRawVoltage(uint8_t voltagePin, uint8_t samples, uint16_t delayMillis);
float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis);
void DisplayCurrentStatus(float scale, float rawValue, float scaledValue);
void HandleLongPress(float vDivScale);
void delay1(unsigned long ms);



// the setup function runs once when you press reset or power the board
void setup() {
	analogReference(DEFAULT);

	// Start LCD and Serial
	lcd.begin(20, 4);

#ifdef DEBUGSERIAL
	Serial.begin(9600);
#endif

	pinMode(LED_PIN, OUTPUT);
	pinMode(V5_SENSOR, INPUT);
	pinMode(WAKE_SLEEP_BUTTON, INPUT_PULLUP);
	pinMode(MODE_SWITCH, INPUT_PULLUP);

	CreateArrows(lcd);
}

// the loop function runs over and over again until power down or reset
void loop() {
	uint16_t		rawVoltageSample;
	float			scaledVoltage;

	// Just blink LED twice to show we're running
	int modeSwitchValue = digitalRead(MODE_SWITCH);
	lcd.setCursor(0, 3);
	lcd.write((modeSwitchValue == 0) ? (byte) LCD_DOWN_ARROW : (byte)LCD_UP_ARROW);

	// Has the button on the "go to sleep" pin been toggled?
	if (buttonPressed)
	{
		buttonPressed = false;
		if (isLongPress)
		{
			isLongPress = false;
			HandleLongPress(vDivScale);
		}
		else
		{
			vDivScale += (modeSwitchValue == 0) ? -0.000002 : 0.000002;
		}
	}

	rawVoltageSample = round(GetAverageRawVoltage(V5_SENSOR, 3, 5));
	scaledVoltage = rawVoltageSample * VREFSCALE(vDivScale);

	DisplayCurrentStatus(vDivScale, rawVoltageSample, scaledVoltage);


	if (!wakeSleepISRSet)
	{
		// Wake/Sleep Interrupts are not set up or were disabled. Set up the button
		noInterrupts();
		attachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON), wakeSleepLowISR, LOW);
		wakeSleepISRSet = true;
		interrupts();
		DebugPrintln(F("wakeSleepISRSet ACTIVATED"));
	}



	doBlink(LED_PIN);
	delay1(500);
	}




// When WAKE_SLEEP_BUTTON is brought LOW this interrupt is triggered
void wakeSleepLowISR()
{
	// Detach the interrupt that brought us here
	detachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON));
	buttonPressed = true;
	wakeSleepISRSet = false;

	delay1(5);
	unsigned long time = 5;
	while (digitalRead(WAKE_SLEEP_BUTTON) == LOW) {
		delay1(5);
		time += 1;
	}

	Serial.print("time: ");
	Serial.print(time);
	Serial.println();

	isLongPress = (time / 1000 >= 2);

	Serial.print("time / 1000: ");
	Serial.print(time / 1000);
	Serial.println();
	Serial.print("isLongPress: ");
	Serial.print(isLongPress);
	Serial.println();
}




// When WAKE_SLEEP_BUTTON is brought HIGH this interrupt is triggered
void wakeSleepHighISR()
{
	// Detach the interrupt that brought us here
	detachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON));
	buttonPressed = true;
	wakeSleepISRSet = false;

	delay1(5);
	unsigned long time = 5;
	while (digitalRead(WAKE_SLEEP_BUTTON) == LOW) {
		delay1(5);
		time += 1;
	}

	Serial.print("time: ");
	Serial.print(time);
	Serial.println();

	isLongPress = (time / 1000 >= 2);

	Serial.print("time / 1000: ");
	Serial.print(time / 1000);
	Serial.println();
	Serial.print("isLongPress: ");
	Serial.print(isLongPress);
	Serial.println();
}


// Double blink just to show we are running. Note that we do NOT
// use the delay for the final delay here, this is done by checking
// millis instead (non-blocking)
void doBlink(uint8_t ledPin) {
	static unsigned long lastMillis = 0;

	if (millis() > lastMillis + 1000)
	{
		digitalWrite(ledPin, HIGH);
		delay1(10);
		digitalWrite(ledPin, LOW);
		delay1(200);
		digitalWrite(ledPin, HIGH);
		delay1(10);
		digitalWrite(ledPin, LOW);
		lastMillis = millis();
	}
}


float GetAverageRawVoltage(uint8_t voltagePin, uint8_t samples, uint16_t delayMillis) {
	int32_t rawVoltageSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay1(delayMillis);
		rawVoltageSum += analogRead(voltagePin);
	}
	return rawVoltageSum / static_cast<float>(samples);
}


float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis) {
	return GetAverageRawVoltage(voltagePin, samples, delayMillis) * voltageScale;
}


void DisplayCurrentStatus(float scale, float rawValue, float scaledValue)
{
	char			buffer[20];
	char			dataStr[10];
	char			tempStr[6];

	formatFloat(scale, dataStr, 9, 6);
	sprintf(buffer, "Scale: %s", dataStr);
	lcd.setCursor(0, 0);
	lcdPrint(lcd, buffer, 20);

	formatFloat(rawValue, dataStr, 7, 2);
	sprintf(buffer, "Raw:   %s", dataStr);
	lcd.setCursor(0, 1);
	lcdPrint(lcd, buffer, 20);

	formatFloat(scaledValue, dataStr, 7, 3);
	sprintf(buffer, "Volts: %s", dataStr);
	lcd.setCursor(0, 2);
	lcdPrint(lcd, buffer, 20);
}

void HandleLongPress(float vDivScale)
{
	lcd.clear();
	lcd.setCursor(0, 1);
	lcd.print("        LONG        ");
	lcd.setCursor(0, 2);
	lcd.print("       PRESS!       ");
	for (int i = 0; i < 4; i++)
	{
		delay1(500);
		lcd.noDisplay();
		delay1(500);
		lcd.display();
	}
}

void delay1(unsigned long ms)
{
	uint32_t start = micros() & 0xFFFFFF00;

	while (ms > 0) {
		// yield();
		while (ms > 0 && ((micros() & 0xFFFFFF00) - start) >= 1000) {
			ms--;
			start = micros() & 0xFFFFFF00;
		}
	}
}
