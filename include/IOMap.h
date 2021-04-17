/*
 Name:		IOMap.h
 Created:	07/04/2021
 Author:	Andrea Santinelli
*/

#ifndef _IOMap_h
#define _IOMap_h

/* dependencies */
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/* class definitions */
Adafruit_BME280 BME280;

/* BME280 reserved pins (only for SPI mode) */
#define BME_CS		10
#define BME_MOSI	11
#define BME_MISO	12
#define BME_SCK		13

/* application pins */
#define FAN			8
#define RUN_LED		7
#define ERROR_LED	5

/* enumerated that defines the status of BME280 */
#define NO_ERROR			0
#define BME280_INIT_ERROR	1

/* miscellanea */
#define TASK_TIME	10	// Cycle time (10 ms)
#define ulong		unsigned long int	
#define uint		unsigned short int

/* structure definitions */
struct ST_Timer
{	/* structure used to define a timer function */
	uint PT = 0;
	uint ET = 0;
};
struct ST_Comparator
{	/* structure used to define a comparator function */
	word threshold;
	word HYSTERESYS;	// uppercase means that this element is used as a constant value
};

#endif