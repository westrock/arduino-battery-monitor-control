// 
// 
// 

#include <LiquidCrystal.h>
#include "ds3231.h"
#include "LCDHelper.h"
#include "DS3231Helpers.h"
#include "DataAcquisitionAndReporting.h"
#include "DateTimeHelpers.h"
#include "HourlyDataTypes.h"


void DoReportingTasks(SamplingData* samplingData, CurrentSample* currentSample, ReportControl* reportControl, LiquidCrystal lcd, int8_t reportingDelaySeconds)
{
	DateTimeDS3231	timeNow;
	char			buffer[20];
	char			voltStr1[6];
	char			voltStr2[6];
	char			tempStr1[6];
	char			tempStr2[6];

	DS3231_get(&timeNow);
	int32_t secondsElapsed = dateDiffSeconds(&timeNow, &(reportControl->previousTime));

	if (secondsElapsed >= reportingDelaySeconds)
	{
		reportControl->previousTime = timeNow;
		lcd.clear();
		switch (reportControl->reportingCycle)
		{
		case Report0:
			lcd.setCursor(0, 0);
			if (samplingData->isPowerOutDisabled)
			{
				lcdPrint(lcd, "Power Off", 20);
			}
			else {
				lcdPrint(lcd, "Power On", 20);
			}


			lcd.setCursor(0, 1);
			formatFloat(samplingData->disableVoltage, voltStr1, 5, 2);
			sprintf(buffer, "Disable at %sv", voltStr1);
			lcdPrint(lcd, buffer, 20);

			lcd.setCursor(0, 2);
			formatFloat(samplingData->enableVoltage, voltStr2, 5, 2);
			sprintf(buffer, "Enable at  %sv", voltStr2);
			lcdPrint(lcd, buffer, 20);
			break;
		case Report1:

			lcd.setCursor(0, 1);
			lcdPrint(lcd, "reportingCycle 1", 20);
			break;
		case Report2:
			lcd.setCursor(0, 1);
			lcdPrint(lcd, "reportingCycle 2", 20);
			break;
		case Report3:
			lcd.setCursor(0, 1);
			lcdPrint(lcd, "reportingCycle 3", 20);
			break;
		default:
			break;
		}
		reportControl->reportingCycle++;
		if (reportControl->reportingCycle >= EndOfReports)
		{
			reportControl->reportingCycle = FirstReport;
		}
	}
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


