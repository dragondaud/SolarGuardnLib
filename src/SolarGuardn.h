/*
	SolarGuardn Arduino Library
	copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#ifndef SolarGuardn_h
#define SolarGuardn_h

#define VERSION "0.8.2"

#include "Arduino.h"
#include "Stream.h"
#include <ESP8266WiFi.h>			// Arduino IDE ESP8266 2.4.0 from https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>		// included
#include <ESP8266mDNS.h>			// included
#include <ArduinoOTA.h>				// included
#include <time.h>					// included
#include <Wire.h>					// included
#include <ArduinoJson.h>			// https://github.com/bblanchon/ArduinoJson/
#include <BME280I2C.h>				// https://github.com/finitespace/BME280
#include <DHT.h>					// https://github.com/adafruit/DHT-sensor-library
#include "ClosedCube_HDC1080.h"		// https://github.com/closedcube/ClosedCube_HDC1080_Arduino
#include "Adafruit_TCS34725.h"		// https://github.com/adafruit/Adafruit_TCS34725

#include <EspSaveCrash.h>			// https://github.com/krzychb/EspSaveCrash
#undef SAVE_CRASH_SPACE_SIZE
#define SAVE_CRASH_SPACE_SIZE 0x1000	// flash reserved to store crash data

#define MQTT_MAX_PACKET_SIZE 512
#include <PubSubClient.h>			// https://github.com/knolleary/pubsubclient

#define UserAgent	"SolarGuardn/1.0 (Arduino ESP8266)"

// openssl s_client -connect maps.googleapis.com:443 | openssl x509 -fingerprint -noout
#define gMapsCrt	"‎‎67:7B:99:A4:E5:A7:AE:E4:F0:92:01:EF:F5:58:B8:0B:49:CF:53:D4"

#define BETWEEN 60000

class SolarGuardn {
public:
	SolarGuardn(const char* hostname, const char* wifi_ssid, const char* wifi_pass, const char* server, uint16_t port, const char* topic, const char* user, const char* pass, const char* gMapsKey, Stream * out, PubSubClient & mqtt);

	void setup(uint16_t data, uint16_t clock);
	bool handle();
	void ledOn();
	void ledOff();
	void setNTP();
	String upTime();
	String localTime();
	String getIPlocation();
	String getLocation(const String address);
	uint32_t getTimeZone(String loc);
	void mqttConnect();
	void mqttPublish(String topic, String data);
	bool readDHT(DHT & dht);
	bool readHDC(ClosedCube_HDC1080 & hdc);
	bool readBME(BME280I2C & bme);
	bool readTCS(Adafruit_TCS34725 & tcs);
	bool readMoisture(uint16_t pin, uint16_t pow, uint16_t num, uint16_t tim);
	bool getDist(uint16_t trig, uint16_t echo);
	void pubJSON();
	void pubDebug(String cmd);

	String location;
	float temp, humid, pressure;
	uint16_t colorTemp, lux, moist, range;
	uint32_t heap;

private:
	void flushIn();
	String UrlEncode(const String);
	void startOTA();
	void checkIn();
	void outDiag();

	Stream * _out;
	IPAddress _pubip, _wifip;
	String _hostname;
	time_t _now, _upTime, _twoAM;
	uint32_t _TZ, _timer;
	uint16_t _ledPin;
	const char* _appName;
	const char* _wifi_ssid;
	const char* _wifi_pass;
	const char* _mqttServer;
	uint16_t _mqttPort;
	const char* _mqttTopic;
	const char* _mqttUser;
	const char* _mqttPass;
	const char* _gMapsKey;
	PubSubClient _mqtt;
	BME280::TempUnit _tUnit;
	BME280::PresUnit _pUnit;
};
#endif

