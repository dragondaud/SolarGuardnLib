/*
   SolarGuardn - LightGuard v0.8.00 PRE-RELEASE 06-Dec-2017
   by David Denney <dragondaud@gmail.com>

   LightGuard monitors light intensity and color temperature and reports data using MQTT.

   Connect TCS34725 to 3.3V, GND, SDA to D7, SCL D6

   Configures NTP and sets timezone automatically from geoIP location
   GeoIP location can be overridden by setting location string to postal code.

   This code is offered "as is" with no warranty, expressed or implied, for any purpose,
   and is released to the public domain, while all libraries retain their respective licenses.

   Master repository: https://github.com/dragondaud/SolarGuardn

   Designed to run on an NodeMCU board with a BME280 and TCS34725 RGB light sensor
   Board: NodeMCU 1.0, Freq: 80MHz, Flash: 4M (1M SPIFFS), Speed: 115200, Port: serial or OTA IP

*/

