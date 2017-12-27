/*
   SolarGuardn - TankGuard v0.8.00 PRE-RELEASE 06-Dec-2017
   by David Denney <dragondaud@gmail.com>

   TankGuard monitors water level in tank and reports data using MQTT.

   Connect HC-SR04 VCC to 5V, GND to GND, ECHO to D1 (in), TRIG to D2 (out)

   Accepts the following one character commands from serial:
     c : clear crash information
     e : attempt to read through a pointer to no object
     0 : attempt to divide by zero
     r : restart esp

   Configures NTP and sets timezone automatically from geoIP location
   GeoIP location can be overridden by setting location string to postal code.

   This code is offered "as is" with no warranty, expressed or implied, for any purpose,
   and is released to the public domain, while all libraries retain their respective licenses.

   Master repository: https://github.com/dragondaud/SolarGuardn

   Designed to run on an NodeMCU board with a BME280 and ultrasonic range finder
   Board: NodeMCU 1.0, Freq: 80MHz, Flash: 4M (1M SPIFFS), Speed: 115200, Port: serial or OTA IP

*/

