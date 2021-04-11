/*
 Name:		BME280_Modbus.ino
 Created:	07/04/2021 20:19:39
 Author:	Andrea Santinelli

 Libraries from github:
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
#define REFRESH_TIME_DEFAULT_VALUE		15				// set 15 second of default refresh time
#define FAN_OFF_HYSTERESIS				15				// set 1.5°C of hysteresis
#define FAN_START_TEMP_DEFAULT_VALUE	250				// set to 250°C
#define FAN_ON_TIME_DEFAULT_VALUE		240				// set to 4 min.
#define FAN_OFF_TIME_DEFAULT_VALUE		30				// set to 30 sec.
#define SERIAL_PRINT
/* step of fan control */
#define IDLE							0
#define TIMER_RESET						1
#define VENTILATION_ACTIVE				2
#define VENTILATION_INACTIVE			3
#define CHECK_TEMPERATURE				4

/* global var declaration*/
char softwareVersion[]		= "2104.07";					// software version
ulong millisAtLoopBegin		= 0;							// millis value at the begin of loop
word fanStartTemperature	= FAN_START_TEMP_DEFAULT_VALUE;
int fanControlStep			= IDLE;
bool setupIsDone			= false;						// is TRUE when setup is completed with no error
int oldFanState				= LOW;

/* structured global var for timing */
ST_Timer stRefreshTimer;
ST_Timer stFanOnTimer;
ST_Timer stFanOffTimer;

/* output connected var */
int doRunState				= LOW;							// rapresents the state of GREEN led on sensor board
int doErrorState			= LOW;							// rapresents the state of RED led on sensor board
int doFanState				= LOW;							// represents the state of sensor fan

/* data read from BME280 */
word actualTemperature		= 0;
word actualPressure			= 0;
word actualHumidity			= 0;

/* data calculated */
word wetBulbTemperature		= 0;
word dewPoint				= 0;
word heatIndex				= 0;
word absHumidity			= 0;
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

	/* init strucured var */
	stRefreshTimer.PT	= REFRESH_TIME_DEFAULT_VALUE;
	stRefreshTimer.ET	= stRefreshTimer.PT * 1000;
	stFanOnTimer.PT		= FAN_ON_TIME_DEFAULT_VALUE;
	stFanOnTimer.ET		= 0;
	stFanOffTimer.PT	= FAN_OFF_TIME_DEFAULT_VALUE;
	stFanOffTimer.ET	= 0;

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
	/* add registers to modbus configuration: write holding registers*/
	ModbusRTU.addHreg(WEATER_DATA_REFRESH_TIME, stRefreshTimer.PT);
	ModbusRTU.addHreg(FAN_START_TEMPERATURE, fanStartTemperature);
	ModbusRTU.addHreg(FAN_ON_TIME_HREG, stFanOnTimer.PT);
	ModbusRTU.addHreg(FAN_OFF_TIME_HREG, stFanOffTimer.PT);
	/* modbus configuration completed */
	Serial.println(F("done!"));
	Serial.println();
#pragma endregion
}

/* the loop function runs over and over again until power down or reset */
void loop()
{
	/* get millis value at loop begin */
	millisAtLoopBegin = millis();

	/* init output state */
	if (setupIsDone)
	{
		/* only needed in forced mode! In normal mode, you can remove the next line. */
		BME280.takeForcedMeasurement();
		/* notify correct execution of setup */
		doRunState = HIGH;
		doErrorState = LOW;
	}
	else
	{
		/* notify error during setup */
		doRunState = LOW;
		doErrorState = HIGH;
	}

	/* call once inside loop() */
	ModbusRTU.task();

	/* update Modbus configuration registers */
	stRefreshTimer.PT	= constrain(ModbusRTU.Hreg(WEATER_DATA_REFRESH_TIME), 1, 240);	// min: 1 sec.  max: 4 min.
	stFanOffTimer.PT	= constrain(ModbusRTU.Hreg(FAN_OFF_TIME_HREG), 30, 600);		// min: 30 sec. max: 10 min.
	stFanOnTimer .PT	= constrain(ModbusRTU.Hreg(FAN_ON_TIME_HREG), 30, 600);			// min: 30 sec. max: 10 min.
	fanStartTemperature = constrain(ModbusRTU.Hreg(FAN_START_TEMPERATURE), 100, 350);	// min: 10°C.   max: 35°C

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
			/* Write weater data on Modbus registers */
			ModbusRTU.Hreg(ACTUAL_TEMPERATURE_HREG, actualTemperature);
			ModbusRTU.Hreg(ACTUAL_PRESSURE_HREG, actualPressure);
			ModbusRTU.Hreg(ACTUAL_HUMIDITY_HREG, actualHumidity);
			ModbusRTU.Hreg(WET_BULB_TEMPERATURE_HREG, wetBulbTemperature);
			ModbusRTU.Hreg(DEW_POINT_HREG, dewPoint);
			ModbusRTU.Hreg(HEAT_INDEX, heatIndex);
			ModbusRTU.Hreg(ABS_HUMIDITY_HREG, absHumidity);
		}

		/* sensor ventilation control */
		switch (fanControlStep)
		{	//--------------------------------------------------------------------------
			case IDLE:
				/* check if temperature requires to use a fan */
				if (actualTemperature > fanStartTemperature)
				{
					/* reset timer before cycle */
					fanControlStep = TIMER_RESET;
				}
				break;
			//--------------------------------------------------------------------------
			case TIMER_RESET:
				stFanOnTimer.ET  = 0;
				stFanOffTimer.ET = 0;
				fanControlStep	 = VENTILATION_ACTIVE;
				break;
			//--------------------------------------------------------------------------
			case VENTILATION_ACTIVE:
				/* activate fan */
				doFanState = HIGH;
				/* count elapsed time at HIGH level by using fixed task time */
				if (stFanOnTimer.ET < (stFanOnTimer.PT * 1000))
				{
					/* elapsed time is updated at every loop */
					stFanOnTimer.ET += TASK_TIME;
				}
				else
				{
					fanControlStep = VENTILATION_INACTIVE;
				}
				break;
			//--------------------------------------------------------------------------
			case VENTILATION_INACTIVE:
				/* deactivate fan */
				doFanState = LOW;
				/* count elapsed time at LOW level by using fixed task time */
				if (stFanOffTimer.ET < (stFanOffTimer.PT *1000))
				{
					/* elapsed time is updated at every loop */
					stFanOffTimer.ET += TASK_TIME;
				}
				else
				{
					fanControlStep = CHECK_TEMPERATURE;
				}
				break;
			//--------------------------------------------------------------------------
			case CHECK_TEMPERATURE:
				/* define if temperature requires to switch off fan */
				if (actualTemperature <= (fanStartTemperature - FAN_OFF_HYSTERESIS))
				{
					fanControlStep = IDLE;
				}
				else
				{
					fanControlStep = TIMER_RESET;
				}
				break;
		}
	}

	/* print info about ventilation */
	#ifdef SERIAL_PRINT
	if (doFanState != oldFanState)
	{
		oldFanState = doFanState;
		Serial.print(F("Sensor ventilation is: "));
		Serial.println(doFanState);
		Serial.println();
	}
	#endif

	/* write digital output */
	digitalWrite(RUN_LED, doRunState);
	digitalWrite(ERROR_LED, doErrorState);
	digitalWrite(FAN, doFanState);

	/* wait task time (every loop has a fixed duration) */
	while ((millis() - millisAtLoopBegin) <= TASK_TIME);
}

/* cyclic function used to read data from BME280 sensor
   all data will be stored in modbus registers */
void F_GetValues()
{
	/* init loop timer for next call */
	stRefreshTimer.ET = 0;

	/* direct data: All data have been read by BME280 sensor */
	float _actualTemperature	= BME280.readTemperature();
	float _actualPressure		= BME280.readPressure() / 100.0F;
	float _actualHumidity		= BME280.readHumidity();

	/* indirec data: All data have been calculated */
	float _wetBulbTemperature	= GetWetBulbTemp(_actualTemperature, _actualHumidity, _actualPressure, isCelsius);
	float _dewPoint				= GetDewPoint	(_actualTemperature, _actualHumidity, isCelsius);
	float _heatIndex			= GetHeatIndex	(_actualTemperature, _actualHumidity, isCelsius);
	float _absHumidity			= GetAbsHumidity(_actualTemperature, _actualHumidity, isCelsius);

	/* conversion */
	actualTemperature			= F_FloatToWord(_actualTemperature, 1);
	actualPressure				= F_FloatToWord(_actualPressure, 1);
	actualHumidity				= F_FloatToWord(_actualHumidity, 1);
	wetBulbTemperature			= F_FloatToWord(_wetBulbTemperature, 1);
	dewPoint					= F_FloatToWord(_dewPoint, 1);
	heatIndex					= F_FloatToWord(_heatIndex, 1);
	absHumidity					= F_FloatToWord(_absHumidity, 1);

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