/*
 Name:		VrefScaleSetup.ino
 Created:	12/18/2022 16:10:19
 Author:	efigarsky
*/

#include <EEPROM.h>
#include <LiquidCrystal.h>
#include "LCDHelper.h"



/*==========================+
|	#defines				|
+==========================*/
#define USE_EXTERNALVREF
#define VREF_RESISTOR	14.92									//
#define VREG			4.982									//


#ifdef USE_EXTERNALVREF
#define VDIV_SCALE		4.477983								//
#define VREF			VREG * 32 / (32 + VREF_RESISTOR)		//
#define VREFSCALE(x)	VREF / 1024.0 * (x)						//
#else
#define VDIV_SCALE		4.718196								//
#define VREFSCALE(x)	VREG / 1024.0 * (x)						//
#endif


#define DEBUGSERIALx

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

#define WAKE_SLEEP_BUTTON		2					// When low, makes 328P go to sleep
#define LED_PIN					4					// output pin for the LED (to show it is awake)


/*========================+
| enums			          |
+========================*/
enum BUTTON_STATE_ENUM	{ STATE_NOT_SET = 0, STATE_LOW_ENABLED, STATE_WENT_LOW, STATE_HIGH_ENABLED, STATE_WENT_HIGH, STATE_WAS_PRESSED, STATE_ILLEGAL_STATE = 255 };
enum PRESS_TYPE_ENUM	{ PRESS_NOT_SET = 0, PRESS_LONG = 1, PRESS_SHORT = 2 };

void LcdPrintState(LiquidCrystal lcd, uint8_t state);
void SerialPrintState(uint8_t state);

/*========================+
| Local Variables         |
+========================*/
volatile uint8_t	errorStateOld = STATE_NOT_SET;
volatile uint8_t	errorStateNew = STATE_NOT_SET;
volatile uint32_t	buttonDownMillis = 0;
volatile uint8_t	buttonState = STATE_NOT_SET;
volatile uint8_t	pressType = PRESS_NOT_SET;
volatile bool		isLongPress = false;
volatile bool		buttonPressed = false;
volatile bool		wakeSleepISRSet = false;
volatile float		vDivScale;

const uint8_t	rs = 11, en = 10, d4 = 5, d5 = 6, d6 = 7, d7 = 8;
LiquidCrystal	lcd(rs, en, d4, d5, d6, d7);
uint32_t		lastDisplayMillis = 0;

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

	pinMode(LED_PIN, OUTPUT);
	pinMode(V5_SENSOR, INPUT);
	pinMode(WAKE_SLEEP_BUTTON, INPUT_PULLUP);
	pinMode(MODE_SWITCH, INPUT_PULLUP);

	EEPROM.get(0, vDivScale);

	DebugPrint("eeprom value: ");
	DebugPrintln(vDivScale);
	if (vDivScale != vDivScale)
	{
		vDivScale = VDIV_SCALE;
	}
	DebugPrint("vDivScale: ");
	DebugPrintln(vDivScale);

	CreateArrows(lcd);
}

// the loop function runs over and over again until power down or reset
void loop() {
	uint16_t		rawVoltageSample;
	float			scaledVoltage;

	// Just blink LED twice to show we're running
	int modeSwitchValue = digitalRead(MODE_SWITCH);

	switch (buttonState)
	{
	case STATE_LOW_ENABLED:
		break;
	case STATE_WENT_LOW:
		buttonDownMillis = millis();
		delay1(1);
		noInterrupts();
		attachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON), wakeSleepHighISR, HIGH);
		buttonState = STATE_HIGH_ENABLED;
		interrupts();
		DebugPrintln(F("wakeSleepHighISR ACTIVATED"));
		break;
	case STATE_HIGH_ENABLED:
		break;
	case STATE_WENT_HIGH:
		pressType = (millis() - buttonDownMillis > 2000) ? PRESS_LONG : PRESS_SHORT;
		buttonDownMillis = 0;
		buttonState = STATE_WAS_PRESSED;
		break;
	case STATE_WAS_PRESSED:
		if (pressType == PRESS_SHORT)
		{
			vDivScale += (modeSwitchValue == 0) ? -0.002002 : 0.002002;
		}
		else
		{
			HandleLongPress(vDivScale);
		}
		buttonState = STATE_NOT_SET;
		break;
 	case STATE_NOT_SET:
		noInterrupts();
		attachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON), wakeSleepLowISR, LOW);
		buttonState = STATE_LOW_ENABLED;
		interrupts();
		DebugPrintln(F("wakeSleepLowISR ACTIVATED"));
		break;
	case STATE_ILLEGAL_STATE:
		lcd.clear();
		lcd.setCursor(0,0);
		lcd.print(F("ILLEGAL STATE"));
		lcd.setCursor(0, 1);
		lcd.print(F("OLD: "));
		LcdPrintState(lcd, errorStateOld);
			lcd.setCursor(0, 2);
		lcd.print(F("NEW: "));
		LcdPrintState(lcd, errorStateNew);
		break;
	default:
		break;
	};

	if (buttonState == STATE_LOW_ENABLED)
	{
		lcd.setCursor(0, 3);
		lcd.write((modeSwitchValue == 0) ? (byte)LCD_DOWN_ARROW : (byte)LCD_UP_ARROW);

		rawVoltageSample = round(GetAverageRawVoltage(V5_SENSOR, 3, 5));
		scaledVoltage = rawVoltageSample * VREFSCALE(vDivScale);

		DisplayCurrentStatus(vDivScale, rawVoltageSample, scaledVoltage);

		doBlink(LED_PIN);
		delay1(500);
	}
}




// When WAKE_SLEEP_BUTTON is brought LOW (ie pushed) this interrupt is triggered
void wakeSleepLowISR()
{
	// Detach the interrupt that brought us here
	detachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON));

	if (buttonState == STATE_LOW_ENABLED)
	{
		buttonState = STATE_WENT_LOW;
	}
	else
	{
		buttonState = STATE_ILLEGAL_STATE;
		errorStateOld = buttonState;
		errorStateNew = STATE_WENT_LOW;
	}
	delay1(5);
}




// When WAKE_SLEEP_BUTTON is brought HIGH this interrupt is triggered
void wakeSleepHighISR()
{
	// Detach the interrupt that brought us here
	detachInterrupt(digitalPinToInterrupt(WAKE_SLEEP_BUTTON));

	if (buttonState == STATE_HIGH_ENABLED)
	{
		buttonState = STATE_WENT_HIGH;
	}
	else
	{
		buttonState = STATE_ILLEGAL_STATE;
		errorStateOld = buttonState;
		errorStateNew = STATE_WENT_HIGH;
	}
	delay1(5);
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
	EEPROM.put(0, vDivScale);

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

void LcdPrintState(LiquidCrystal lcd, uint8_t state)
{
	switch (state)
	{
	case STATE_LOW_ENABLED:
		lcd.print(F("LOW_ENABLED"));
		break;
	case STATE_WENT_LOW:
		lcd.print(F("WENT_LOW"));
		break;
	case STATE_HIGH_ENABLED:
		lcd.print(F("HIGH_ENABLED"));
		break;
	case STATE_WENT_HIGH:
		lcd.print(F("WENT_HIGH"));
		break;
	case STATE_WAS_PRESSED:
		lcd.print(F("WAS_PRESSED"));
		break;
	case STATE_NOT_SET:
		lcd.print(F("NOT_SET"));
		break;
	case STATE_ILLEGAL_STATE:
		lcd.print(F("ILLEGAL_STATE"));
		break;
	default:
		lcd.print(F("UNKNOWN"));
		break;
	};
}

void SerialPrintState(uint8_t state)
{
	switch (state)
	{
	case STATE_LOW_ENABLED:
		Serial.print(F("LOW_ENABLED"));
		break;
	case STATE_WENT_LOW:
		Serial.print(F("WENT_LOW"));
		break;
	case STATE_HIGH_ENABLED:
		Serial.print(F("HIGH_ENABLED"));
		break;
	case STATE_WENT_HIGH:
		Serial.print(F("WENT_HIGH"));
		break;
	case STATE_WAS_PRESSED:
		Serial.print(F("WAS_PRESSED"));
		break;
	case STATE_NOT_SET:
		Serial.print(F("NOT_SET"));
		break;
	case STATE_ILLEGAL_STATE:
		Serial.print(F("ILLEGAL_STATE"));
		break;
	default:
		Serial.print(F("UNKNOWN"));
		break;
	};
}
