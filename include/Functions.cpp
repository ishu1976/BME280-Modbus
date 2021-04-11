/*
 Name:		Functions.cpp
 Created:	08/04/2021
 Author:	Andrea Santinelli
*/

/* Dependencies */
#include "Functions.h"

/* CONVERSION : Celsius ----> Fahrenheit */
float SetFahrenheit(float refTempC)
{
    /* init function output */
    float _tempF = NAN;

    /* check and calculate*/
    if (isnan(refTempC))
    {
        _tempF = NAN;
    }
    else
    {
        _tempF = refTempC * (9.0 / 5.0) + 32.0; // conversion to [°F]
    }
    return _tempF;
}

/* CONVERSION : Fahrenheit ----> Celsius */
float SetCelsius(float refTempF)
{
    /* init function output */
    float _tempC = NAN;

    /* check and calculate*/
    if (isnan(refTempF))
    {
        _tempC = NAN;
    }
    else
    {
        _tempC = (refTempF - 32.0) * (5.0 / 9.0); // conversion to [°C] 
    }

    /* Set function output*/
    return _tempC;
}

/* DEW POINT VALUE */
float GetDewPoint(float refTemp, float refHum, E_TempUnit eTempUnit)
{
    /* init function output */
    float _dewPoint = NAN;

    /* check and calculate */
    if (isnan(refTemp) || isnan(refHum))
    {
        return _dewPoint;
    }

    /* convert the temperature to celsius if I have a TempUnit expressed in fahrenheit */
    if (eTempUnit == isFahrenheit)
    {
        refTemp = SetCelsius(refTemp);
    }

    /* dewPoint calculation (always in celsius) */
    _dewPoint = 243.04 * (log(refHum / 100.0) + ((17.625 * refTemp) / (243.04 + refTemp))) / (17.625 - log(refHum / 100.0) - ((17.625 * refTemp) / (243.04 + refTemp)));

    /* convert the dew point to fahrenheit if I have a TempUnit expressed in fahrenheit */
    if (eTempUnit == isFahrenheit)
    {
        _dewPoint = SetFahrenheit(_dewPoint);
    }

    /* set function output*/
    return _dewPoint;
}

/* HEAT INDEX VALUE */
float GetHeatIndex(float refTemp, float refHum, E_TempUnit eTempUnit)
{
    /* init function output */
    float _refTempF = NAN;

    /* init function output */
    float _heatIndex = NAN;

    /* check and calculate */
    if (isnan(refTemp) || isnan(refHum))
    {
        return _heatIndex;
    }

    /* calculation always in fahrenheit */
    if (eTempUnit == isCelsius)
    {
        _refTempF = SetFahrenheit(refTemp); // conversion to [°F]
    }
    /* Using both Rothfusz and Steadman's equations */
    // http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml
    if (_refTempF <= 40.0)
    {
        _heatIndex = _refTempF; // first red block
    }
    else
    {
        /* Calculate A -- From the official site, not the flow graph */
        _heatIndex = 0.5 * (_refTempF + 61.0 + ((_refTempF - 68.0) * 1.2) + (refHum * 0.094));
        /* check calculated value */
        if (_heatIndex >= 79)
        {
            /* B calc */
            _heatIndex = hi_coeff1 + (hi_coeff2 * _refTempF) + (hi_coeff3 * refHum) + (hi_coeff4 * _refTempF * refHum)
                + (hi_coeff5 * pow(_refTempF, 2)) + (hi_coeff6 * pow(refHum, 2)) + (hi_coeff7 * pow(_refTempF, 2) * refHum)
                + (hi_coeff8 * _refTempF * pow(refHum, 2)) + (hi_coeff9 * pow(_refTempF, 2) * pow(refHum, 2));
            /* third red block */
            if ((refHum < 13) && (_refTempF >= 80.0) && (_refTempF <= 112.0))
            {
                _heatIndex -= ((13.0 - refHum) * 0.25) * sqrt((17.0 - abs(_refTempF - 95.0)) * 0.05882);
            }
            /* fourth red block */
            else if ((refHum > 85.0) && (_refTempF >= 80.0) && (_refTempF <= 87.0))
            {
                _heatIndex += (0.02 * (refHum - 85.0) * (87.0 - _refTempF));
            }
        }
    }

    /* calculation always in fahrenheit */
    if (eTempUnit == isCelsius)
    {
        _heatIndex = SetCelsius(_heatIndex); // conversion to [°C]
    }

    /* set function output*/
    return _heatIndex;
}

/* ABSOLUTE HUMIDITY VALUE */
float GetAbsHumidity(float refTemp, float refHum, E_TempUnit eTempUnit)
{
    /* init function output */
    float _tempVar = NAN;

    /* init function output */
    float _absHumidity = NAN;

    /* check and calculate */
    if (isnan(refTemp) || isnan(refHum))
    {
        return _absHumidity;
    }
    if (eTempUnit == isFahrenheit)
    {
        refTemp = SetCelsius(refTemp);  // conversion to [°C]
    }

    /* first calc on temp var */
    _tempVar = pow(2.718281828, ((17.67 * refTemp) / (refTemp + 243.5)));
    /* return (6.112 * temp * humidity * 2.1674) / (273.15 + temperature); ----> simplified version */
    _absHumidity = (6.112 * _tempVar * refHum * waterMolarMass) / ((273.15 + refTemp) * gasConstant); 	/* ----> long version */

    /* set function output*/
    return _absHumidity;
}

/* WET BULB TEMPERATURE VALUE */
float GetWetBulbTemp(float refTemp, float refHum, float refPress, E_TempUnit eTempUnit)
{
    /* init function output */
    float _wetBulbTemp = NAN;

    /* check and calculate */
    if (isnan(refTemp) || isnan(refHum) || isnan(refPress))
    {
        return _wetBulbTemp;
    }

    /* convert the temperature to celsius if I have a TempUnit expressed in fahrenheit */
    if (eTempUnit == isFahrenheit)
    {
        refTemp = SetCelsius(refTemp);
    }

    /* wetBulbTemp calculation */
    _wetBulbTemp = refTemp * (0.45 + (0.006 * refHum * sqrt(refPress / 1060.0)));

    /* convert the wet bulb temperature to fahrenheit if I have a TempUnit expressed in fahrenheit */
    if (eTempUnit == isFahrenheit)
    {
        _wetBulbTemp = SetFahrenheit(_wetBulbTemp);
    }

    /* set function output*/
    return _wetBulbTemp;
}