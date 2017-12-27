#define USERCONFIG

#ifdef USERCONFIG
#include "userconfig.h"
#else
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"
#define MQTT_SERV "mqtt.local"
#define MQTT_PORT 1883
#define MQTT_TOPIC "SolarGuardn"
#define MQTT_USER ""
#define MQTT_PASS ""
#define BETWEEN 60000             // delay between readings in loop()
String location;                 // set to postal code or region name to bypass geoIP lookup
String HOST = "";
#endif

#define POW D4    // BME280 power
#define GND D5    // BME280 ground
#define SCL D6    // BME280 I2C clock
#define SDA D7    // BME280 I2C data

#define TRIG D2   // HC-SR04 trig
#define ECHO D1   // HC-SR04 echo

#include "Include.h"
#include "Global.h"

