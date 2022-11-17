// DataAcquisition.h

#ifndef _DATAACQUISITION_h
#define _DATAACQUISITION_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

float GetAverageRawVoltage(uint8_t voltagePin, uint8_t samples, uint16_t delayMillis);
float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis);
float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis);
float GetAverageDS3231Temp(uint8_t samples, uint16_t delayMillis);

#endif

