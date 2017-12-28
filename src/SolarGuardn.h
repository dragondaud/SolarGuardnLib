#ifndef SolarGuardn_h
#define SolarGuardn_h

#include "Arduino.h"

#include <ESP8266WiFi.h>          // Arduino IDE ESP8266 from https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>    // included
#include <ESP8266mDNS.h>          // included
#include <ArduinoOTA.h>           // included
#include <time.h>                 // included

#include <ArduinoJson.h>          // install ArduinoJson using library manager, https://github.com/bblanchon/ArduinoJson/
#include <EspSaveCrash.h>         // install EspSaveCrash using library manager, https://github.com/krzychb/EspSaveCrash
#include <PubSubClient.h>         // install PubSubClient using library manager, https://github.com/knolleary/pubsubclient

#define UserAgent	"SolarGuardn/1.0 (Arduino ESP8266)"

// openssl s_client -connect maps.googleapis.com:443 | openssl x509 -fingerprint -noout
#define gMapsCrt	"‎‎67:7B:99:A4:E5:A7:AE:E4:F0:92:01:EF:F5:58:B8:0B:49:CF:53:D4"

class SolarGuardn {
	public:
		SolarGuardn(const char* server, unsigned int port, const char* topic, const char* user, const char* pass, const char* key);
		void flushIn(Stream &s);
		String UrlEncode(const String);
		String getIPlocation();
		String getLocation(const String address, const char* key);
		int getTimeZone(time_t now, String loc, const char* key);
		void setNTP();
		String upTime(const time_t now);
		bool mqttConnect(PubSubClient mqtt);
		void mqttPublish(PubSubClient mqtt, String topic, String data);
		String host, location;
		time_t TWOAM;
		time_t UPTIME;

	protected:
		IPAddress _pubip;
		const char * _mqttServer;
		unsigned int _mqttPort;
		const char * _mqttTopic;
		const char * _mqttUser;
		const char * _mqttPass;
		const char * _gMapsKey;
};
#endif

