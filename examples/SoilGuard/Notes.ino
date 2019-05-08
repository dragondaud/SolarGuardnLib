/*
   SolarGuardn - SoilGuard v0.9.0 PRE-RELEASE
   copyright 2019 by David Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html

   SoilGuard monitors soil moisture using capacative moisture sensor

   Configures NTP and sets timezone automatically from geoIP location
   GeoIP location can be overridden by setting location string to postal code.

   Master repository: https://github.com/dragondaud/SolarGuardn

   Designed to run on an NodeMCU board with a BME280, DHT22, or HDC1080 temperature/humidity sensor
   Board: NodeMCU 1.0, Freq: 80MHz, Flash: 4M (1M SPIFFS), Speed: 115200, Port: serial or OTA IP

  DHT-Sensor-library modified as per issue 94
  https://github.com/adafruit/DHT-sensor-library/issues/94
  In version 1.3.0 simply comment lines 155/156 out
  // End the start signal by setting data line high for 40 microseconds.
  // digitalWrite(_pin, HIGH);
  // delayMicroseconds(40);
*/
