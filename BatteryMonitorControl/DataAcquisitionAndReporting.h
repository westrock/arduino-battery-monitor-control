// ReportingHelper.h

#ifndef _REPORTINGHELPER_h
#define _REPORTINGHELPER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

#include <LiquidCrystal.h>
#include "LCDHelper.h"
#include "ds3231.h"
#include "DS3231Helpers.h"
#include "DateTimeHelpers.h"
#include "HourlyDataTypes.h"

/*==========================+
|	#defines				|
+==========================*/
#define DATA_HOURS				24

/*==========================+
|	enums					|
+==========================*/

enum reportType { FirstReport = 0, Report0 = 0, Report1, Report2, Report3, EndOfReports };

/*==========================+
|	typedefs				|
+==========================*/

struct reportControlStruct
{
	DateTimeDS3231	previousTime;
	int				reportingCycle = FirstReport;
};
typedef struct reportControlStruct ReportControl;


struct samplingDataStruct
{
	DateTimeDS3231	timeEnabled;						// Time the voltage initially (re)enabled 
	DateTimeDS3231	timeDisabled;						// Time the voltage initially dropped below the low threshold 
	DateTimeDS3231	timeRecoveryStarted;				// Time the voltage started to rebound
	DateTimeDS3231	recoveryTime;						// Time recovery will be completed
	CurrentHourData	currentHourData;					// Total, min, and max values for the current hour
	HourlyData		hourlyData[DATA_HOURS];				//
	int8_t			currentHour = -1;					// The current hour.  Used to store/update samples
	bool			isPowerOutDisabled = false;			// Indicates the power out has been disabled (the relay is open)
	bool			isPowerOutRecovering = false;		// Indicates the power is recovering (the relay is still open)
	bool			isIntialized = false;				// Indicates whether we're using startup logic
	float			disableVoltage = 0;					// Voltage at which output power is disabled
	float			enableVoltage = 0;					// Voltage at which output power is (re-)enabled
};
typedef struct samplingDataStruct SamplingData;


struct currentSampleStruct
{
	DateTimeDS3231	timeNow;
	float			tempSample;
	float			scaledVoltage;
	uint16_t		minutesDisabled = 0;
};
typedef struct currentSampleStruct CurrentSample;


/*========================+
| Function Definitions    |
+========================*/
void DoReportingTasks(SamplingData* samplingData, CurrentSample* currentSample, ReportControl* reportControl, LiquidCrystal lcd, int8_t reportingDelaySeconds);
float GetAverageRawVoltage(uint8_t voltagePin, uint8_t samples, uint16_t delayMillis);
float GetAverageVoltage(uint8_t voltagePin, float voltageScale, uint8_t samples, uint16_t delayMillis);
float GetAverageTemp(uint8_t tempPin, float tempScale, uint8_t samples, uint16_t delayMillis);
float GetAverageDS3231Temp(uint8_t samples, uint16_t delayMillis);


#endif

