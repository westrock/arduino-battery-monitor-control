// 
// 
// 

#include "config.h"
#include "ds3231.h"
#include "DataAcquisition.h"



float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis) {
	int32_t rawTempSum = 0;
	uint8_t i;
	for (i = 0; i < samples; i++) {
		delay(delayMillis);
		rawTempSum += analogRead(tempPin);
	}
	return rawTempSum / static_cast<float>(samples) * tempScale * 100.0 / 1024.0;
}


float GetAverageDS3231Temp(uint8_t samples, uint16_t delayMillis) {
	float rawTempSum = 0;

	for (uint8_t i = 0; i < samples; i++) {
		delay(delayMillis);
		rawTempSum += DS3231_get_treg();
	}
	return rawTempSum / static_cast<float>(samples) * 1.8 + 32.0;
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


