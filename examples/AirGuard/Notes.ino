/*
   SolarGuardn - AirGuard v0.9.0 PRE-RELEASE
   copyright 2019 by David Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html

   LightGuard monitors light intensity and color temperature and reports data using MQTT.

   Connect TCS34725 to 3.3V, GND, SDA to D1, SCL D2

   Configures NTP and sets timezone automatically from geoIP location
   GeoIP location can be overridden by setting location string to postal code.

   Master repository: https://github.com/dragondaud/SolarGuardn

   Designed to run on an NodeMCU board with a BME280, DHT22 or HDC1080 and TCS34725 RGB light sensor
   Board: NodeMCU 1.0, Freq: 80MHz, Flash: 4M (1M SPIFFS), Speed: 115200, Port: serial or OTA IP

*/
