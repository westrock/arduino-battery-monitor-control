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

/*==========================+
|	enums					|
+==========================*/

struct dataRecordingControlStruct
{
};
typedef struct dataRecordingControlStruct DataRecordingControl;



/*========================+
| Function Definitions    |
+========================*/


#endif

