# BME280-Modbus
A simple Arduino Nano based board used to read and transmit data from a BME280 sensor to the main weater station via RS485 serial communication and Modbus RTU protocol. The card returns the main temperature parameters. It also has an automatic ventilation control to avoid error in the temperature reading caused by stagnant air inside the sun shield (especially on hot and windless days). All data collected (and calculated) are transmitted to main control unit (weather station) by RS485 serial communication. Modbus RTU protocol is used to exchange data.

### Libraries used in the project
_________________________________________________________________
#### Modbus-Arduino
Author: André Sarmento Barbosa

Github: https://github.com/andresarmento/modbus-arduino

#### Adafruit BME280 Library
Author: Adafruit industries

Github: https://github.com/adafruit/Adafruit_BME280_Library

_________________________________________________________________
###### Note
_The code was written using Visual Studio 2019 with the Visual Micro extension installed._
