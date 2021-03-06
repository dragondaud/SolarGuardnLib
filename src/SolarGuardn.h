/*
	SolarGuardn Arduino Library
	copyright 2019 by David Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#ifndef SolarGuardn_h
#define SolarGuardn_h

#define VERSION "0.9.0"

#include "Arduino.h"
#include "Stream.h"
#include <ESP8266WiFi.h>			// Arduino IDE ESP8266 2.4.2 from https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>		// included
#include <ESP8266mDNS.h>			// included
#include <WiFiUdp.h> 				// included
#include <ArduinoOTA.h>				// included
#include <time.h>					// included
#include <Wire.h>					// included
#include <ArduinoJson.h>			// https://github.com/bblanchon/ArduinoJson/
#include <BME280I2C.h>				// https://github.com/finitespace/BME280
#include <DHT.h>					// https://github.com/adafruit/DHT-sensor-library
#include "Adafruit_Sensor.h"		// https://github.com/adafruit/Adafruit_Sensor
#include "Adafruit_TCS34725.h"		// https://github.com/adafruit/Adafruit_TCS34725
#include "Adafruit_CCS811.h"		// https://github.com/adafruit/Adafruit_CCS811
#include "Adafruit_AM2320.h"		// https://github.com/adafruit/Adafruit_AM2320
#include "Adafruit_seesaw.h"		// https://github.com/adafruit/Adafruit_Seesaw

#include <EspSaveCrash.h>			// https://github.com/krzychb/EspSaveCrash
#undef SAVE_CRASH_SPACE_SIZE
#define SAVE_CRASH_SPACE_SIZE 0x1000	// flash reserved to store crash data

// must change MQTT_MAX_PACKET_SIZE to 512
#include <PubSubClient.h>			// https://github.com/knolleary/pubsubclient

#define UserAgent	"SolarGuardn/1.0 (Arduino ESP8266)"

#define SG_BETWEEN 60000			// time between sensor sampling
#define SG_RETRIES 3				// number of retries on sampling

enum sg_sensors {
	SG_INVALID = 0,
	SG_BME = 1 << 0,	// 1
	SG_DHT = 1 << 1,	// 2
	SG_ASA = 1 << 2,	// 4
	SG_LIGHT = 1 << 4,	// 16
	SG_RANGE = 1 << 5,	// 32
	SG_MOIST = 1 << 6,	// 64
	SG_AIR = 1 << 7		// 128
};

class SolarGuardn {
public:
	SolarGuardn(Stream * out,
		const char * hostname, const char * wifi_ssid, const char * wifi_pass,
		const char * mqtt_server, uint16_t mqtt_port,
		const char * mqtt_topic, const char * mqtt_user, const char * mqtt_pass,
		const char * tzKey, uint16_t sensors, uint16_t ledpin
	);

	void begin(uint8_t data, uint8_t clock);
	bool handle();
	void ledOn();
	void ledOff();
	String upTime();
	String localTime();
	void mqttCallback(char* topic, byte* payload, unsigned int length);
	bool readTemp(DHT & dht);
	bool readTemp(Adafruit_AM2320 & asa);
	bool readTemp(BME280I2C & bme);
	bool readTCS(Adafruit_TCS34725 & tcs);
	bool readMoisture(Adafruit_seesaw & ss, uint16_t num);
	bool readMoisture(uint16_t pin, uint16_t pow, uint16_t num, uint16_t tim);
	bool readCCS(Adafruit_CCS811 & ccs);
	bool getDist(uint16_t trig, uint16_t echo);
	void pubJSON();
	void pubDebug(String cmd);
	void deepSleep(uint16_t time);
	float getVoltage(uint16_t num);

	String timezone;
	float temp, humid, pressure, voltage;
	uint16_t colorTemp, lux, moist, range, eCO2, TVOC;

private:
	void flushIn();
	void mqttConnect();
	void mqttPublish(String topic, String data);
	String UrlEncode(const String);
	void startOTA();
	String getIPlocation();
	long getOffset(const String);
	void setNTP(const String);
	void checkIn();
	void outDiag();
	void doCmd(String);
	bool reLoop();

	WiFiClient _mqttwifi;
	PubSubClient _mqtt;
	Stream * _out;
	IPAddress _pubip, _wifip;
	String _hostname;
	time_t _now, _upTime, _twoAM;
	long _TZ;
	uint32_t _timer;
	uint16_t _ledPin;
	const char* _app_name;
	const char* _wifi_ssid;
	const char* _wifi_pass;
	const char* _mqtt_server;
	uint16_t _mqtt_port;
	const char* _mqtt_topic;
	const char* _mqtt_user;
	const char* _mqtt_pass;
	const char* _tzKey;
	BME280::TempUnit _tUnit;
	BME280::PresUnit _pUnit;
	uint16_t _sensors;
	EspSaveCrash _SaveCrash;
};
#endif

