#include <SolarGuardn.h>
#include "userconfig.h"
#ifndef USERCONFIG
#define HOST "TankGuard"
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"
#define MQTT_SERV "mqtt.local"
#define MQTT_PORT 1883
#define MQTT_TOPIC "SolarGuardn"
#define MQTT_USER ""
#define MQTT_PASS ""
#define gMapsKey "APIKEY"       // https://developers.google.com/maps/documentation/timezone/intro
#endif

WiFiClient    wificlient;
PubSubClient  mqtt(wificlient);

SolarGuardn sg(HOST, WIFI_SSID, WIFI_PASS, MQTT_SERV, MQTT_PORT, MQTT_TOPIC, MQTT_USER, MQTT_PASS, gMapsKey, &Serial, mqtt);

#define POW D4    // BME280 power
#define GND D5    // BME280 ground

#define TRIG D2   // HC-SR04 trig
#define ECHO D1   // HC-SR04 echo

BME280I2C bme(BME280I2C::Settings(BME280::OSR_X4, BME280::OSR_X4, BME280::OSR_X1,
                                  BME280::Mode_Forced, BME280::StandbyTime_1000ms, BME280::Filter_Off,
                                  BME280::SpiEnable_False, 0x76));

