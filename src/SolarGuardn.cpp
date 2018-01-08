/*
	SolarGuardn library
	copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "SolarGuardn.h"

SolarGuardn::SolarGuardn(const char* hostname, const char* wifi_ssid, const char* wifi_pass, const char* server, unsigned int port, const char* topic, const char* user, const char* pass, const char* key, Stream * out, PubSubClient & mqtt) : _out(out) {
	_hostname = String(hostname);
	_wifi_ssid = wifi_ssid;
	_wifi_pass = wifi_pass;
	_mqttServer = server;
	_mqttPort = port;
	_mqttTopic = topic;
	_mqttUser = user;
	_mqttPass = pass;
	_gMapsKey = key;
	_out = out;
	_mqtt = mqtt;
}

void SolarGuardn::setup() {
	while (!_out);		// wait for stream to open
	delay(100);			// ... and settle
	flushIn();
	_out->println();
	_out->print("setup: WiFi connecting to ");
	_out->print(_wifi_ssid);
	_out->print("...");
	WiFi.persistent(false);
	WiFi.mode(WIFI_STA);
	String t = WiFi.macAddress();
	_hostname += "-" + t.substring(9, 11) + t.substring(12, 14) + t.substring(15, 17);
	WiFi.hostname(_hostname);
	WiFi.begin(_wifi_ssid, _wifi_pass);
	while (WiFi.status() != WL_CONNECTED) {
		_out->print(".");
		delay(500);
	}
	_out->println(" OK");

	if (location == "") {
		location = getIPlocation();
	} else {
		getIPlocation();
		location = getLocation(location, _gMapsKey);
	}
	setNTP();

	ArduinoOTA.setHostname(_hostname.c_str());
	ArduinoOTA.onStart([this]() {
		_out->println("\nOTA: Start");
	} );
	ArduinoOTA.onEnd([this]() {
		_out->println("\nOTA: End");
		delay(1000);
		ESP.restart();
	} );
	ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
		_out->print("OTA Progress: " + String((progress / (total / 100))) + " \r");
	});
	ArduinoOTA.onError([this](ota_error_t error) {
		_out->print("\nError[" + String(error) + "]: ");
		if (error == OTA_AUTH_ERROR) _out->println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) _out->println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) _out->println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) _out->println("Receive Failed");
		else if (error == OTA_END_ERROR) _out->println("End Failed");
		else _out->println("unknown error");
		delay(5000);
		ESP.restart();
	});
	ArduinoOTA.begin();
	Wire.begin(SDA, SCL);
	_mqtt.setServer(_mqttServer, _mqttPort);
	mqttConnect();
	UPTIME = time(nullptr);
	_out->print(F("Last reset reason: "));
	_out->println(ESP.getResetReason());
	_out->print(F("WiFi Hostname: "));
	_out->println(WiFi.hostname());
	_out->print(F("WiFi IP addr: "));
	_out->println(WiFi.localIP());
	_out->print(F("WiFi gw addr: "));
	_out->println(WiFi.gatewayIP());
	_out->print(F("WiFi MAC addr: "));
	_out->println(WiFi.macAddress());
	_out->print(F("ESP Sketch size: "));
	_out->println(ESP.getSketchSize());
	_out->print(F("ESP Flash free: "));
	_out->println(ESP.getFreeSketchSpace());
	_out->print(F("ESP Flash Size: "));
	_out->println(ESP.getFlashChipRealSize());
} // setup()

bool SolarGuardn::loop() {
	if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
		String t = "Invalid null address";
		mqttPublish("debug", t);
		_out->println(t);
		delay(500);
		ESP.restart();
	}
	if (_out->available() > 0) {	// check Stream input for commands
		char inChar = _out->read();
		flushIn();  // flush input after each command to prevent garbage DoS
		switch (inChar) {
		case '0':
			_out->println("\nAttempting to divide by zero ...");
			int result, zero;
			zero = 0;
			result = 1 / zero;
			_out->print("Result = ");
			_out->println(result);
			break;
		case 'e':
			_out->println("\nAttempting to read through a pointer to no object ...");
			int* nullPointer;
			nullPointer = NULL;
			_out->print(*nullPointer);
			break;
		case 'p':
			_out->println();
			SaveCrash.print();
			break;
		case 'c':
			SaveCrash.clear();
			_out->println("\nCrash information cleared");
			break;
		case 'r':
			_out->println("\nRebooting...");
			delay(1000);
			ESP.restart();
			break;
		default:
			_out->println("\np : print crash information");
			_out->println("c : clear crash information");
			_out->println("e : attempt to read through a pointer to no object");
			_out->println("0 : attempt to divide by zero");
			_out->println("r : restart esp");
			break;
		}
	}
	ArduinoOTA.handle();
	_mqtt.loop();
	if (millis() > _timer) {
		_timer = millis() + 60000;
		return true;
	} else {
		delay(5000);
		return false;
	}
} // loop

void SolarGuardn::ledOn(int pin) {	// LED on
	analogWrite(pin, 1);
} // ledOn

void SolarGuardn::ledOff(int pin) {	// PWM dim LED to off
  for (int i = 23; i < 1023; i++) {
    analogWrite(pin, i);
    delay(2);
  }
  analogWrite(BUILTIN_LED, 0);
  digitalWrite(BUILTIN_LED, HIGH);
} // ledOff

void SolarGuardn::flushIn() { // flush Stream input buffer (not what Serial.flush() does)
	while (_out->available()) {
		_out->read();
	}
	yield();
} // flushIn

String SolarGuardn::UrlEncode(const String url) {  // escape non-alphanumerics in URL
	String e;
	for (int i = 0; i < url.length(); i++) {
		char c = url.charAt(i);
		if (c == 0x20) {
			e += "%20";
		} else if (isalnum(c)) {
			e += c;
		} else {
			e += "%";
			if (c < 0x10) e += "0";
			e += String(c, HEX);
		}
	}
	return e;
} // UrlEncode

String SolarGuardn::getIPlocation() {  // Using freegeoip.net to map public IP's location
	HTTPClient http;
	String URL = "http://freegeoip.net/json/";
	String loc;
	http.setUserAgent(UserAgent);
	if (http.begin(URL)) {
		int stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				String payload = http.getString();
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					String region = root["region_name"];
					String country = root["country_code"];
					String lat = root["latitude"];
					String lng = root["longitude"];
					loc = lat + "," + lng;
					String ip = root["ip"];
					_pubip.fromString(ip);
				} else {
					_out->println("getIPlocation: JSON parse failed!");
					_out->println(payload);
				}
			} else {
				_out->printf("getIPlocation: [HTTP] GET reply %d\r\n", stat);
			}
		} else {
			_out->printf("getIPlocation: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
		}
	}
	http.end();
	return loc;
} // getIPlocation

String SolarGuardn::getLocation(const String address, const char* key) { // using google maps API, return location for provided Postal Code
	HTTPClient http;
	String URL = "https://maps.googleapis.com/maps/api/geocode/json?address="
	+ UrlEncode(address) + "&key=" + String(key);
	String loc;
	http.setIgnoreTLSVerifyFailure(true);   // https://github.com/esp8266/Arduino/pull/2821
	http.setUserAgent(UserAgent);
	if (http.begin(URL, gMapsCrt)) {
		int stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				String payload = http.getString();
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					JsonObject& results = root["results"][0];           // http://arduinojson.org/assistant/
					JsonObject& results_geometry = results["geometry"];
					String address = results["formatted_address"];
					String lat = results_geometry["location"]["lat"];
					String lng = results_geometry["location"]["lng"];
					loc = lat + "," + lng;
				} else {
					_out->println("getLocation: JSON parse failed!");
					_out->println(payload);
				}
			} else {
				_out->printf("getLocation: [HTTP] GET reply %d\r\n", stat);
			}
		} else {
			_out->printf("getLocation: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
		}
	}
	http.end();
	return loc;
} // getLocation

int SolarGuardn::getTimeZone(time_t now, String loc, const char* key) { // using google maps API, return TimeZone for provided timestamp
	HTTPClient http;
	int tz = false;
	String URL = "https://maps.googleapis.com/maps/api/timezone/json?location="
	+ UrlEncode(loc) + "&timestamp=" + String(now) + "&key=" + String(key);
	String payload;
	http.setIgnoreTLSVerifyFailure(true);   // https://github.com/esp8266/Arduino/pull/2821
	http.setUserAgent(UserAgent);
	if (http.begin(URL, gMapsCrt)) {
		int stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				payload = http.getString();
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					tz = (int (root["rawOffset"]) + int (root["dstOffset"])) / 3600;  // combine Offset and dstOffset
					const char* tzname = root["timeZoneName"];
					_out->printf("getTimeZone: %s (%d)\r\n", tzname, tz);
				} else {
					_out->println("getTimeZone: JSON parse failed!");
					_out->println(payload);
				}
			} else {
				_out->printf("getTimeZone: [HTTP] GET reply %d\r\n", stat);
			}
		} else {
			_out->printf("getTimeZone: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
		}
	}
	http.end();
	return tz;
} // getTimeZone

void SolarGuardn::setNTP() { // using location configure NTP with local timezone
	int TZ = getTimeZone(time(nullptr), location, _gMapsKey);
	_out->print("setNTP: configure NTP ...");
	configTime((TZ * 3600), 0, "pool.ntp.org", "time.nist.gov");
	while (time(nullptr) < (30 * 365 * 24 * 60 * 60)) {         // wait for time to advance to this century
		delay(1000);
		_out->print(".");
	}
	delay(5000);
	time_t now = time(nullptr);
	String t = ctime(&now);
	t.trim();
	_out->println(t.substring(3));
	struct tm * calendar;
	calendar = localtime(&now);
	calendar->tm_mday++;
	calendar->tm_hour = 2;
	calendar->tm_min = 0;
	calendar->tm_sec = 0;
	twoAM = mktime(calendar);
	t = ctime(&twoAM);
	t.trim();
	_out->print("setNTP: next timezone check @ ");
	_out->println(t);
} // setNTP

String SolarGuardn::upTime(const time_t now) { // output UPTIME as d:h:MM:SS
	long t = now - UPTIME;
	long s = t % 60;
	long m = (t / 60) % 60;
	long h = (t / (60 * 60)) % 24;
	long d = (t / (60 * 60 * 24));
	char ut[12];
	snprintf(ut, sizeof(ut), "%d:%d:%02d:%02d", d, h, m, s);
	if (now > twoAM) {
		_out->println();
		setNTP();
	}
	return String(ut);
} // upTime()

void SolarGuardn::mqttConnect() {  // connect MQTT and emit ESP info to debug channel
	if (_mqtt.connect(_hostname.c_str(), _mqttUser, _mqttPass)) {
		time_t now = time(nullptr);
		String t = "{\"mqtt\": \"connect\", \"hostname\": \"" + _hostname + \
			"\", \"wifi_ip\": \"" + WiFi.localIP().toString() + \
			"\", \"public_ip\": \"" + _pubip.toString() + \
			"\", \"reset_reason\": \"" + ESP.getResetReason() + \
			"\", \"location\": \"" + location + "\", \"timestamp\": " + String(now) + \
			", \"freeheap\": " + String(ESP.getFreeHeap()) + "}";
		mqttPublish("debug", t);
	} else {
		_out->print(_mqtt.state());
		_out->println(" MQTT not connected.");
		delay(5000);
		ESP.restart();
	}
} // mqttConnect

void SolarGuardn::mqttPublish(String topic, String data) { // publish data to topic on mqtt, reconnect as needed
	if (!_mqtt.connected()) {
		mqttConnect();
	}
	int r = _mqtt.publish((String(_mqttTopic) + "/" + _hostname + "/" + topic).c_str(), data.c_str());
	if (!r) _out->println("MQTT error: " + String(r));
} // mqttPublish

