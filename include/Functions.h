/*
 Name:		    Functions.cpp
 Created:	    08/04/2021
 Author:        Andrea Santinelli
 Description:   Header file for Function.cpp
*/

#ifndef _Functions_h
#define _Functions_h

/* Dependencies */
#include <Arduino.h>
#include <math.h>

/* Constant for calc */
#define hi_coeff1       -42.379
#define hi_coeff2       2.04901523
#define hi_coeff3       10.14333127
#define hi_coeff4       -0.22475541
#define hi_coeff5       -0.00683783
#define hi_coeff6       -0.05481717
#define hi_coeff7       0.00122874
#define hi_coeff8       0.00085282
#define hi_coeff9       -0.00000199
#define waterMolarMass  18.01534    
#define gasConstant     8.31447215  

/* Enumerate */
enum E_TempUnit : int {
	isCelsius	 = 1,
	isFahrenheit = 2
};
enum PresUnit : int {
	PresUnit_Pa		= 1,
	PresUnit_hPa	= 2,
	PresUnit_inHg	= 3,
	PresUnit_atm	= 4,
	PresUnit_bar	= 5,
	PresUnit_torr	= 6,
	PresUnit_psi	= 7
};

/* Function */
float GetDewPoint(float refTemp, float refHum, E_TempUnit eTempUnit);
float GetHeatIndex(float refTemp, float refHum, E_TempUnit eTempUnit);
float GetAbsHumidity(float refTemp, float refHum, E_TempUnit eTempUnit);
float GetWetBulbTemp(float refTemp, float refHum, float refPress, E_TempUnit eTempUnit);
float SetFahrenheit(float refTempC);
float SetCelsius(float refTempF);
#endif