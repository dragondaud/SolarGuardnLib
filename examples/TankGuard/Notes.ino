/*
   SolarGuardn - TankGuard v0.8.4 PRE-RELEASE
   copyright 2018 by David M Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html

   TankGuard monitors water level in tank and reports data using MQTT.

   Connect HC-SR04 VCC to 5V, GND to GND, ECHO to D1 (in), TRIG to D2 (out)

   Configures NTP and sets timezone automatically from geoIP location
   GeoIP location can be overridden by setting location string to postal code.

   Master repository: https://github.com/dragondaud/SolarGuardn

   Designed to run on an NodeMCU board with a BME280 and ultrasonic range finder
   Board: NodeMCU 1.0, Freq: 80MHz, Flash: 4M (1M SPIFFS), Speed: 115200, Port: serial or OTA IP

*/

