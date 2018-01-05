/*
	SolarGuardn library
	copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#ifndef SolarGuardn_h
#define SolarGuardn_h

#define VERSION "0.8.1"

#include "Arduino.h"
#include "Stream.h"
#include <ESP8266WiFi.h>          // Arduino IDE ESP8266 from https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>    // included
#include <ESP8266mDNS.h>          // included
#include <ArduinoOTA.h>           // included
#include <time.h>                 // included
#include <Wire.h>                 // included
#include <ArduinoJson.h>          // install ArduinoJson using library manager, https://github.com/bblanchon/ArduinoJson/
#include <PubSubClient.h>         // install PubSubClient using library manager, https://github.com/knolleary/pubsubclient
#include <BME280I2C.h>            // install BME280 using library manager, https://github.com/finitespace/BME280

#include <EspSaveCrash.h>         // install EspSaveCrash using library manager, https://github.com/krzychb/EspSaveCrash
#undef SAVE_CRASH_SPACE_SIZE
#define SAVE_CRASH_SPACE_SIZE 0x1000  // FLASH space reserved to store crash data

#define UserAgent	"SolarGuardn/1.0 (Arduino ESP8266)"
#define SCL D6		// I2C clock
#define SDA D7		// I2C data

// openssl s_client -connect maps.googleapis.com:443 | openssl x509 -fingerprint -noout
#define gMapsCrt	"‎‎67:7B:99:A4:E5:A7:AE:E4:F0:92:01:EF:F5:58:B8:0B:49:CF:53:D4"

class SolarGuardn {
public:
	SolarGuardn(const char* hostname, const char* wifi_ssid, const char* wifi_pass, const char* server, unsigned int port, const char* topic, const char* user, const char* pass, const char* key, Stream * out, PubSubClient & mqtt);

	void setup();
	bool loop();
	void ledOn(int pin);
	void ledOff(int pin);
	void setNTP();
	String upTime(const time_t now);
	String getIPlocation();
	String getLocation(const String address, const char* key);
	int getTimeZone(time_t now, String loc, const char* key);
	void mqttConnect();
	void mqttPublish(String topic, String data);

	String location;
	time_t twoAM;
	time_t UPTIME;

private:
	void flushIn();
	String UrlEncode(const String);

	Stream * _out;
	IPAddress _pubip;
	String _hostname;
	long _timer;
	const char* _wifi_ssid;
	const char* _wifi_pass;
	const char* _mqttServer;
	unsigned int _mqttPort;
	const char* _mqttTopic;
	const char* _mqttUser;
	const char* _mqttPass;
	const char* _gMapsKey;
	PubSubClient _mqtt;
};
#endif

