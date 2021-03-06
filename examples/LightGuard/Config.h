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
#define MQTT_USER NULL
#define MQTT_PASS NULL
#define tzKey "APIKEY"    // https://timezonedb.com/api
//#define sgBME    // BME280 temperature/humidity/pressure sensor
//#define sgDHT    // DHT22 temperature/humidity sensor
//#define sgASA    // AM2320 temperature and humidity sensor
//#define sgTCS    // TCS34725 RGB light sensor
//#define sgRANGE  // HC-SR04 ultrasonic range finder
#endif

SolarGuardn sg(
  &Serial, HOST, WIFI_SSID, WIFI_PASS,
  MQTT_SERV, MQTT_PORT, MQTT_TOPIC, MQTT_USER, MQTT_PASS,
  tzKey, (SG_ASA | SG_LIGHT)
);

#define SCL D1    // I2C clock
#define SDA D2    // I2C data

#ifdef sgBME
BME280I2C bme(
  BME280I2C::Settings(
    BME280::OSR_X4, BME280::OSR_X4, BME280::OSR_X1,
    BME280::Mode_Forced, BME280::StandbyTime_1000ms,
    BME280::Filter_Off, BME280::SpiEnable_False, 0x76
  )
);
#endif

#ifdef sgDHT
#define DHTPIN  D4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#endif

#ifdef sgASA  // I2C
Adafruit_AM2320 asa = Adafruit_AM2320();
#endif

#ifdef sgRANGE
#define TRIG D2   // HC-SR04 trig
#define ECHO D1   // HC-SR04 echo
#endif

#ifdef sgTCS  // I2C
Adafruit_TCS34725 tcs(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_1X);
#endif
