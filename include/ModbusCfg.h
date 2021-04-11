/*
 Name:			ModbusCfg.h
 Created:		07/04/2021
 Author:		Andrea Santinelli
 Description:	Header file for modbus configuration
*/

#ifndef _ModbusCfg_h
#define _ModbusCfg_h

/* dependencies */
#include <Modbus.h>
#include <ModbusSerial.h>

/* class definitions */
ModbusSerial ModbusRTU;

/* modbus configuration */
#define MODBUS_SPEED	9600
#define RX_TX_PIN		9					// Arduino pin connected to MAX485 (pin RE/DE)
#define SLAVE_ADDRESS	1

/* modbus holding registers - read */
#define ACTUAL_TEMPERATURE_HREG		100		// register 100, actual dry bulb temperature read by sensor BME280
#define ACTUAL_PRESSURE_HREG		101		// register 101, actual barometric pressure read by sensor BME280
#define ACTUAL_HUMIDITY_HREG		102		// register 102, actual humidity read by sensor BME280
#define WET_BULB_TEMPERATURE_HREG	103		// register 103, wet bulb temperature - calculated
#define DEW_POINT_HREG				104		// register 104, dew point temperature - calculated
#define HEAT_INDEX					105		// register 105, heat index temperature - calculated
#define ABS_HUMIDITY_HREG			106		// register 106, absolute humidity - calculated

/* modbus holding registers - read */
#define WEATER_DATA_REFRESH_TIME	200		// register 200
#define ACTUAL_WIND_VELOCITY		201		// register 201
#define WIND_VELOCITY_THRESHOLD		202		// register 202
#define TEMPERATURE_THRESHOLD		203		// register 203
#define FAN_ON_TIME_HREG			204		// register 204
#define FAN_OFF_TIME_HREG			205		// register 205

#endif