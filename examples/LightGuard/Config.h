#include <SolarGuardn.h>
#include "userconfig.h"
#ifndef USERCONFIG
#define USERCONFIG
#define HOST "LightGuard"
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"
#define MQTT_SERV "mqtt.local"
#define MQTT_PORT 1883
#define MQTT_TOPIC "SolarGuardn"
#define MQTT_USER ""
#define MQTT_PASS ""
#define gMapsKey "APIKEY"   // https://developers.google.com/maps/documentation/timezone/intro
//#define sgBME    // BME280 temperature/humidity/pressure sensor
//#define sgDHT    // DHT22 temperature/humidity sensor
#define sgHDC    // HDC1080 temperature and humidity sensor
#define sgTCS    // TCS34725 RGB light sensor
//#define sgRANGE  // HC-SR04 ultrasonic range finder
#define SCL D6    // I2C clock
#define SDA D7    // I2C data
#endif

WiFiClient wificlient;
PubSubClient mqtt(wificlient);
SolarGuardn sg(HOST, WIFI_SSID, WIFI_PASS, MQTT_SERV, MQTT_PORT, MQTT_TOPIC, MQTT_USER, MQTT_PASS, gMapsKey, &Serial, mqtt);

#ifdef sgDHT
#define DHTPIN  D4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#endif

#ifdef sgHDC  // I2C
ClosedCube_HDC1080 hdc;
#endif

#ifdef sgTCS  // I2C
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
#endif

#ifdef sgBME  // I2C
BME280I2C bme(BME280I2C::Settings(BME280::OSR_X4, BME280::OSR_X4, BME280::OSR_X1,
                                  BME280::Mode_Forced, BME280::StandbyTime_1000ms, BME280::Filter_Off,
                                  BME280::SpiEnable_False, 0x76));
#endif

