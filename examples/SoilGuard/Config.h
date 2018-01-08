#include <SolarGuardn.h>
#include "userconfig.h"
#ifndef USERCONFIG
#define HOST "SoilGuard"
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

#define DHTPIN  D4    // DHT22 temp/humid sensor
#define DHTTYPE DHT22
#define MOIST A0      // moisture sensor analog input
#define MPOW  D1      // moisture sensor power
#define MGND  D2      // moisture sensor ground
#define STIM 180      // delay time between moisture samples
#define SNUM 3        // number of samples to average

#include <DHT.h>                  // install DHT using library manager

DHT dht(DHTPIN, DHTTYPE);

