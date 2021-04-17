/*
 Name:			BME280_Modbus.ino
 Created:		07/04/2021 20:19:39
 Author:		Andrea Santinelli
   
 A simple Arduino Nano based board used to read data from a BME280 sensor.
 It also has an automatic ventilation control to avoid BME280 sensor reading errors caused by stagnant air inside the sun shield.
 All the data collected and calculated are transmitted to the weather station by means of the Modbus RTU protocol (RS485) operating at the speed of 9600 bit/s.
 
 Copyright: 2021 Andrea Santinelli
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 
 Libraries used in the project:
 - https://github.com/adafruit/Adafruit_BME280_Library
 - https://github.com/andresarmento/modbus-arduino
*/

/* dependencies */
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <ModbusSerial.h>
#include <Modbus.h>
#include "include/IOMap.h"
#include "include/ModbusCfg.h"
#include "include/Functions.h"

#pragma region DECLARATIONS
	/* local defines */
	#define REFRESH_TIME_DEFAULT_VALUE	15	// set 15 second of default refresh time
	#define FAN_ON_TIME_DEFAULT_VALUE	240	// set to 4 min.
	#define FAN_OFF_TIME_DEFAULT_VALUE	30	// set to 30 sec.
	#define SERIAL_PRINT					// comment this define to deactivate print on serial monitor

	/* step of fan controller */
	#define VENTILATION_IDLE			0
	#define VENTILATION_ACTIVE			1
	#define VENTILATION_INACTIVE		2

	/* global var declaration*/
	char softwareVersion[]	= "2104.07";		// software version
	ulong millisAtLoopBegin	= 0;				// millis value at the begin of loop
	word fanControlStep		= VENTILATION_IDLE;
	bool setupIsDone		= false;			// is TRUE when setup is completed with no error
	word actualWindVelocity	= 0; 

	/* structured global var for timing */
	ST_Timer stRefreshTimer;
	ST_Timer stFanOnTimer;
	ST_Timer stFanOffTimer;
	ST_Comparator stTemperature;
	ST_Comparator stWindVelocity;

	/* output connected var */
	int doRunState		= LOW;	// rapresents the state of GREEN led on sensor board
	int doErrorState	= LOW;	// rapresents the state of RED led on sensor board
	int doFanState		= LOW;	// represents the state of sensor fan

	/* status var */
	word boardStatus = NO_ERROR;

	/* data read from BME280 */
	word actualTemperature	= 0;
	word actualPressure		= 0;
	word actualHumidity		= 0;

	/* data calculated */
	word wetBulbTemperature	= 0;
	word dewPoint			= 0;
	word heatIndex			= 0;
	word absHumidity		= 0;
#pragma endregion

/* the setup function runs once when you press reset or power the board */
void setup()
{
	/* define i/o mode */
	pinMode(RUN_LED, OUTPUT);
	pinMode(ERROR_LED, OUTPUT);
	pinMode(FAN, OUTPUT);

	/* init output */
	digitalWrite(RUN_LED, doRunState);
	digitalWrite(ERROR_LED, doErrorState);
	digitalWrite(FAN, doFanState);

	/* init structured var used for timers */
	stRefreshTimer.PT	= REFRESH_TIME_DEFAULT_VALUE;
	stRefreshTimer.ET	= stRefreshTimer.PT * 1000;
	stFanOnTimer.PT		= FAN_ON_TIME_DEFAULT_VALUE;
	stFanOnTimer.ET		= 0;
	stFanOffTimer.PT	= FAN_OFF_TIME_DEFAULT_VALUE;
	stFanOffTimer.ET	= 0;

	/* init structured var used for comparators */
	/* temperature comparator */
	stTemperature.threshold		= 250;	// 250 means 25.0°C
	stTemperature.HYSTERESYS	= 25;	// 25 means 2.5°C
	/* wind velocity comparator */
	stWindVelocity.threshold	= 30;	// 30 means 3.0 m/s
	stWindVelocity.HYSTERESYS	= 5;	// 5 means 0.5 m/s

	/* init serial at modbus speed */
	Serial.begin(MODBUS_SPEED);
	Serial.print(F("Start modbus/serial at "));
	Serial.print(MODBUS_SPEED);
	Serial.println(F(" bit/s"));
	Serial.println();

	/* software info */
	#ifdef SERIAL_PRINT
	Serial.println(F("BME280 WEATHER SENSOR BOARD WITH MODBUS RTU"));
	Serial.print(F("Software version: "));
	Serial.println(softwareVersion);
	Serial.println();
	#endif

#pragma region SENSOR BME280
	/* init BME280 sensor */
	if (!BME280.begin(0x77, &Wire))
	{
		/* init of BME280 is in error */
		setupIsDone = false;
		/* write on serial monitor info about error */
		Serial.println(F("Could not find a valid BME280 sensor"));
		Serial.println(F("Check wiring, address, sensor ID!"));
		Serial.print(F("SensorID was:0x"));
		Serial.println(BME280.sensorID(), 16);
		Serial.println(F("ID of 0xFF probably means a bad address"));
	}
	else
	{
		/* init done! */
		setupIsDone = true;
		/* write on serial monitor that sensor init is ok */
		Serial.println(F("Communication established!"));
		Serial.println();
		/* weather monitoring */
		Serial.println(F("Weather Station Scenario"));
		Serial.println(F("FORCED MODE"));
		Serial.println(F("1x temperature / 1x humidity / 1x pressure oversampling"));
		Serial.println(F("FILTER OFF"));
		Serial.println();
		/* sampling mode configuration */
		BME280.setSampling(
			Adafruit_BME280::MODE_FORCED,
			Adafruit_BME280::SAMPLING_X1, // temperature
			Adafruit_BME280::SAMPLING_X1, // pressure
			Adafruit_BME280::SAMPLING_X1, // humidity
			Adafruit_BME280::FILTER_OFF);
	}
#pragma endregion

#pragma region MODBUS RTU
	/* wait before configure modbus (just to have all previous info printed on serial monitor) */
	Serial.print(F("Wait for ModbusRTU configuration..."));
	delay(1500);
	/* init Modbus RTU */
	ModbusRTU.config(&Serial, MODBUS_SPEED, SERIAL_8N1, RX_TX_PIN);
	ModbusRTU.setSlaveId(SLAVE_ADDRESS);
	/* add registers to modbus configuration: read holding registers*/
	ModbusRTU.addHreg(ACTUAL_TEMPERATURE_HREG, actualTemperature);
	ModbusRTU.addHreg(ACTUAL_PRESSURE_HREG, actualPressure);
	ModbusRTU.addHreg(ACTUAL_HUMIDITY_HREG, actualHumidity);
	ModbusRTU.addHreg(WET_BULB_TEMPERATURE_HREG, wetBulbTemperature);
	ModbusRTU.addHreg(DEW_POINT_HREG, dewPoint);
	ModbusRTU.addHreg(HEAT_INDEX, heatIndex);
	ModbusRTU.addHreg(ABS_HUMIDITY_HREG, absHumidity);
	ModbusRTU.addHreg(BOARD_STATUS_HREG, boardStatus);
	/* add registers to modbus configuration: write holding registers*/
	ModbusRTU.addHreg(WEATER_DATA_REFRESH_TIME, stRefreshTimer.PT);
	ModbusRTU.addHreg(ACTUAL_WIND_VELOCITY, actualWindVelocity);
	ModbusRTU.addHreg(TEMPERATURE_THRESHOLD, stTemperature.threshold);
	ModbusRTU.addHreg(FAN_ON_TIME_HREG, stFanOnTimer.PT);
	ModbusRTU.addHreg(FAN_OFF_TIME_HREG, stFanOffTimer.PT);
	ModbusRTU.addHreg(WIND_VELOCITY_THRESHOLD, stWindVelocity.threshold);
	/* modbus configuration completed */
	Serial.println(F("done!"));
	Serial.println();
#pragma endregion
}

/* the loop function runs over and over again until power down or reset */
void loop()
{
	/* START LOOP: get millis value at loop begin */
	millisAtLoopBegin = millis();

	/* init output state */
	if (setupIsDone)
	{
		/* only needed in forced mode! In normal mode, you can remove the next line. */
		BME280.takeForcedMeasurement();
		/* notify correct execution of setup */
		doRunState   = HIGH;
		doErrorState = LOW;
		boardStatus  = NO_ERROR;
	}
	else
	{
		/* notify error during setup */
		doRunState   = LOW;
		doErrorState = HIGH;
		boardStatus  = BME280_INIT_ERROR;
	}

	/* call once inside loop() */
	ModbusRTU.task();

	/* read data from Modbus Holding registers (master node write this vaules) */
	stRefreshTimer.PT			= constrain(ModbusRTU.Hreg(WEATER_DATA_REFRESH_TIME), 1, 240);	// min: 1 sec.  max: 240 sec. (4 min.)
	stFanOffTimer.PT			= constrain(ModbusRTU.Hreg(FAN_OFF_TIME_HREG), 30, 600);		// min: 30 sec. max: 600 sec. (10 min.)
	stFanOnTimer .PT			= constrain(ModbusRTU.Hreg(FAN_ON_TIME_HREG), 30, 600);			// min: 30 sec. max: 600 sec. (10 min.)
	stWindVelocity.threshold	= constrain(ModbusRTU.Hreg(WIND_VELOCITY_THRESHOLD), 10, 100);	// min: 1.0 m/s max: 10.0 m/s
	stTemperature.threshold		= constrain(ModbusRTU.Hreg(TEMPERATURE_THRESHOLD), 100, 350);	// min: 10°C.   max: 35°C
	actualWindVelocity			= ModbusRTU.Hreg(ACTUAL_WIND_VELOCITY, actualWindVelocity);

	/* get values only if setup is done (otherwise it means that the BME280 sensor is in error) */
	if (setupIsDone)
	{
		/* wait a refresh time before every call of F_GetValues function */
		if (stRefreshTimer.ET < (stRefreshTimer.PT * 1000))
		{
			/* elapsed time is updated at every loop */
			stRefreshTimer.ET += TASK_TIME;
		}
		else
		{
			/* read all weater data from BME280 */
			F_GetValues();
		}

		/* check if temperature or wind speed allows to switch off the ventilation */
		if ((actualTemperature <= (stTemperature.threshold - stTemperature.HYSTERESYS)) || (actualWindVelocity >= (stWindVelocity.threshold + stWindVelocity.HYSTERESYS)))
		{
			/* init control variables */
			fanControlStep = VENTILATION_IDLE;
			/* if fan is active.. */
			if (doFanState == HIGH)
			{	
				/* ..deactivate it and.. */
				doFanState = LOW;
				/* ..print info about ventilation */
				#ifdef SERIAL_PRINT
				Serial.print(F("Sensor ventilation is: INACTIVE"));
				Serial.println(doFanState);
				Serial.println();
				#endif
			}
		}
		/* check if temperature and wind speed require to use ventilation */
		else if (((actualTemperature > stTemperature.threshold) && (actualWindVelocity < stWindVelocity.threshold)) || (fanControlStep != VENTILATION_IDLE))
		{
			/* sensor ventilation control */
			if (fanControlStep == VENTILATION_IDLE)
			{
				/* init timers */
				stFanOffTimer.ET = 0;
				stFanOnTimer.ET  = 0;
				fanControlStep 	 = VENTILATION_ACTIVE;
			}
			else if (fanControlStep == VENTILATION_ACTIVE)
			{
				/* if fan is inactive.. */
				if (doFanState == LOW)
				{
					/* ..activate it and.. */
					doFanState = HIGH;
					/* ..print info about ventilation */
					#ifdef SERIAL_PRINT
					Serial.print(F("Sensor ventilation is: ACTIVE"));
					Serial.println(doFanState);
					Serial.println();
					#endif
				}	
				/* count elapsed time at HIGH level by using fixed task time */
				if (stFanOnTimer.ET < (stFanOnTimer.PT * 1000))
				{
					/* elapsed time is updated at every loop */
					stFanOnTimer.ET += TASK_TIME;
				}
				else
				{
					/* prepare for fan deactivation */
					stFanOffTimer.ET = 0;					
					fanControlStep = VENTILATION_INACTIVE;
				}
			}
			else if (fanControlStep == VENTILATION_INACTIVE)
			{
				/* if fan is active.. */
				if (doFanState == HIGH)
				{	
					/* ..deactivate it and.. */
					doFanState = LOW;
					/* ..print info about ventilation */
					#ifdef SERIAL_PRINT
					Serial.print(F("Sensor ventilation is: INACTIVE"));
					Serial.println(doFanState);
					Serial.println();
					#endif
				}	
				/* count elapsed time at LOW level by using fixed task time */
				if (stFanOffTimer.ET < (stFanOffTimer.PT *1000))
				{
					/* elapsed time is updated at every loop */
					stFanOffTimer.ET += TASK_TIME;
				}
				else
				{
					/* prepare for fan activation */
					stFanOnTimer.ET = 0;					
					fanControlStep = VENTILATION_ACTIVE;
				}
			}
		}
	}

	/* write data on Modbus Holding registers (master node read this vaules) */
	ModbusRTU.Hreg(ACTUAL_TEMPERATURE_HREG, actualTemperature);
	ModbusRTU.Hreg(ACTUAL_PRESSURE_HREG, actualPressure);
	ModbusRTU.Hreg(ACTUAL_HUMIDITY_HREG, actualHumidity);
	ModbusRTU.Hreg(WET_BULB_TEMPERATURE_HREG, wetBulbTemperature);
	ModbusRTU.Hreg(DEW_POINT_HREG, dewPoint);
	ModbusRTU.Hreg(HEAT_INDEX, heatIndex);
	ModbusRTU.Hreg(ABS_HUMIDITY_HREG, absHumidity);
	ModbusRTU.Hreg(BOARD_STATUS_HREG, boardStatus);

	/* write digital output */
	digitalWrite(RUN_LED, doRunState);
	digitalWrite(ERROR_LED, doErrorState);
	digitalWrite(FAN, doFanState);
	
	/* END LOOP: wait task time (every loop has a fixed duration) */
	while (abs(millis() - millisAtLoopBegin) <= TASK_TIME);
}

/* cyclic function used to read data from BME280 sensor */
void F_GetValues()
{
	/* init loop timer for next call */
	stRefreshTimer.ET = 0;

	/* direct data: All data have been read by BME280 sensor */
	float _actualTemperature	= BME280.readTemperature();
	float _actualPressure		= BME280.readPressure() / 100.0F;
	float _actualHumidity		= BME280.readHumidity();

	/* indirect data: All data have been calculated */
	float _wetBulbTemperature	= GetWetBulbTemp(_actualTemperature, _actualHumidity, _actualPressure, isCelsius);
	float _dewPoint				= GetDewPoint	(_actualTemperature, _actualHumidity, isCelsius);
	float _heatIndex			= GetHeatIndex	(_actualTemperature, _actualHumidity, isCelsius);
	float _absHumidity			= GetAbsHumidity(_actualTemperature, _actualHumidity, isCelsius);

	/* conversion in word format just to write values in Modbus registers */
	actualTemperature	= F_FloatToWord(_actualTemperature, 1);
	actualPressure		= F_FloatToWord(_actualPressure, 1);
	actualHumidity		= F_FloatToWord(_actualHumidity, 1);
	wetBulbTemperature	= F_FloatToWord(_wetBulbTemperature, 1);
	dewPoint			= F_FloatToWord(_dewPoint, 1);
	heatIndex			= F_FloatToWord(_heatIndex, 1);
	absHumidity			= F_FloatToWord(_absHumidity, 1);

	/* print data on serial monitor  */
	#ifdef SERIAL_PRINT
	Serial.print(F("Refresh time....... "));
	Serial.print(stRefreshTimer.PT);
	Serial.println(F(" sec."));

	Serial.print(F("Dry temperature.... "));
	Serial.print(_actualTemperature);
	Serial.println(F(" °C"));

	Serial.print(F("Pressure........... "));
	Serial.print(_actualPressure);
	Serial.println(F(" hPa"));

	Serial.print(F("Humidity........... "));
	Serial.print(_actualHumidity);
	Serial.println(F(" %"));

	Serial.print(F("Wet temperature.... "));
	Serial.print(_wetBulbTemperature);
	Serial.println(F(" °C"));

	Serial.print(F("DewPoint........... "));
	Serial.print(_dewPoint);
	Serial.println(F(" °C"));

	Serial.print(F("HeatIndex.......... "));
	Serial.print(_heatIndex);
	Serial.println(F(" °C"));

	Serial.print(F("AbsoluteHumidity... "));
	Serial.print(_absHumidity);
	Serial.println(F(" %"));

	Serial.println();
	#endif
}

/* function used to convert float value to word value with defined precision */
word F_FloatToWord(float value, double precision)
{
	/* rounds based on 'precision' and converts to integer */
	return (word)(roundf(value * pow(10, precision)));
}