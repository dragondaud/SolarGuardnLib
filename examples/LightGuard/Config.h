#include <SolarGuardn.h>
#include "userconfig.h"
#ifndef USERCONFIG
#define WIFI_SSID "SSID"
#define WIFI_PASS "PASS"
#define MQTT_SERV "mqtt.local"
#define MQTT_PORT 1883
#define MQTT_TOPIC "SolarGuardn"
#define MQTT_USER ""
#define MQTT_PASS ""
#define BETWEEN 60000           // delay between readings in loop()
#define gMapsKey "APIKEY"       // https://developers.google.com/maps/documentation/timezone/intro
#endif

SolarGuardn sg(MQTT_SERV, MQTT_PORT, MQTT_TOPIC, MQTT_USER, MQTT_PASS, gMapsKey);

#define SCL D6    // I2C clock
#define SDA D7    // I2C data

#include <Wire.h>                 // included
#include "Adafruit_TCS34725.h"    // install Adafruit_TCS34725 using library manager, https://github.com/adafruit/Adafruit_TCS34725
#include <BME280I2C.h>            // install BME280 using library manager, https://github.com/finitespace/BME280

WiFiClient    wifiClient;
PubSubClient  MQTTclient(wifiClient);

long    TIMER;

Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);

BME280I2C::Settings settings(
  BME280::OSR_X4,  // tempOSR
  BME280::OSR_X4,  // humOSR
  BME280::OSR_X1,  // presOSR
  BME280::Mode_Forced,
  BME280::StandbyTime_1000ms,
  BME280::Filter_Off,
  BME280::SpiEnable_False,
  0x76 // I2C address
);
BME280I2C     bme(settings);

