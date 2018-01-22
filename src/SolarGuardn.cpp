/*
	SolarGuardn Arduino Library
	copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "SolarGuardn.h"

SolarGuardn::SolarGuardn(const char* hostname, const char* wifi_ssid, const char* wifi_pass, const char* server, uint16_t port, const char* topic, const char* user, const char* pass, const char* gMapsKey, Stream * out) : _out(out), _mqtt(_mqttwifi) {
	_appName = hostname;
	_wifi_ssid = wifi_ssid;
	_wifi_pass = wifi_pass;
	_mqttServer = server;
	_mqttPort = port;
	_mqttTopic = topic;
	_mqttUser = user;
	_mqttPass = pass;
	_gMapsKey = gMapsKey;
	_out = out;
	_ledPin = BUILTIN_LED;
	_tUnit = BME280::TempUnit_Fahrenheit;
	_pUnit = BME280::PresUnit_inHg;
}

void SolarGuardn::begin(uint16_t data, uint16_t clock) {
	while (!_out);		// wait for stream to open
	delay(100);			// ... and settle
	flushIn();			// purge input buffer
	String t = WiFi.macAddress();
	_hostname = String(_appName) + "-" + t.substring(9, 11) + t.substring(12, 14) + t.substring(15, 17);
	_out->println();
	_out->print("setup: starting ");
	_out->println(_hostname);
	_out->print("setup: WiFi connecting to ");
	_out->print(_wifi_ssid);
	_out->print("...");
	WiFi.persistent(false);	// do not re-save WiFi params every boot
	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(true);
	WiFi.hostname(_hostname);
	WiFi.begin(_wifi_ssid, _wifi_pass);
	while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
		_out->print(".");
		delay(500);
	}
	_out->println(" OK");
	_wifip = WiFi.localIP();
	if (location == "") {
		location = getIPlocation();
	} else {
		getIPlocation();
		location = getLocation(location);
	}
	setNTP();
	_upTime = time(nullptr);
	startOTA();
	Wire.begin(data, clock);
	IPAddress mqttIP;
	if (!WiFi.hostByName(_mqttServer, mqttIP)) {
		_out->print("setup: mqtt server invalid ");
		_out->println(_mqttServer);
		delay(5000);
		ESP.restart();
	}
	_mqtt.setServer(_mqttServer, _mqttPort);
	_mqtt.setCallback([this](char* topic, byte* payload, unsigned int length) {
		// display incoming MQTT messages
		payload[length] = NULL;
		String cmd = String((char*)payload);
		cmd.trim();
		cmd.toLowerCase();
		_out->print("mqtt: [");
		_out->print(topic);
		_out->println("] " + cmd);
		doCmd(cmd);
	} );
	outDiag();
	mqttConnect();
	_out->print(WiFi.hostname());
	_out->println(" ready");
} // begin

bool SolarGuardn::handle() {
	while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
		_out->println("loop: WiFi invalid");
		//WiFi.reconnect();
		delay(1000);
		ESP.restart();
	}
	checkIn();
	ArduinoOTA.handle();
	_mqtt.loop();
	_now = time(nullptr);
	if ((_now + _TZ) > _twoAM) {
		pubDebug("TZ update");
		setNTP();
	}
	if (millis() > _timer) {
		_timer = millis() + BETWEEN;
		return true;
	} else {
		delay(5000);
		return false;
	}
} // handle

void SolarGuardn::outDiag() {
	_out->print("Last reset reason: ");
	_out->println(ESP.getResetReason());
	_out->print("IP addr: ");
	_out->println(WiFi.localIP());
	_out->print("GW addr: ");
	_out->println(WiFi.gatewayIP());
	_out->print("MAC addr: ");
	_out->println(WiFi.macAddress());
	_out->print("Sketch size: ");
	_out->println(ESP.getSketchSize());
	_out->print("Flash free: ");
	_out->println(ESP.getFreeSketchSpace());
	_out->print("Flash Size: ");
	_out->println(ESP.getFlashChipRealSize());
	int c = SaveCrash.count();
	if (c) {
		_out->print("SAVED CRASH DUMPS: ");
		_out->println(c);
	}
} // outDiag

void SolarGuardn::checkIn() {
	// check _out Stream input for commands
	if (_out->available() > 0) {
		char inChar = _out->read();
		flushIn();  // flush input after each command to prevent garbage DoS
		switch (inChar) {
		case '0':
			doCmd("zero");
			break;
		case 'e':
			doCmd("null");
			break;
		case 'p':
			doCmd("print");
			break;
		case 'c':
			doCmd("clear");
			break;
		case 'r':
			doCmd("reboot");
			break;
		default:
			_out->println("p : print crash information");
			_out->println("c : clear crash information");
			_out->println("e : attempt to read through a null pointer");
			_out->println("0 : attempt to divide by zero");
			_out->println("r : restart esp");
			_out->println(WiFi.hostname());
			break;
		}
	}
} // checkIn

void SolarGuardn::doCmd(String cmd) {
	if (cmd == "reboot") {
		_out->println("cmd: Rebooting...");
		pubDebug("reboot");
		delay(1000);
		ESP.restart();
	} else if (cmd == "clear") {
		SaveCrash.clear();
		_out->println("cmd: Crash information cleared");
		pubDebug("SaveCrash cleared");
	} else if (cmd == "print") {
		SaveCrash.print();
	} else if (cmd == "zero") {
		_out->println("cmd: Attempting to divide by zero ...");
		pubDebug("divide by zero");
		int result, zero;
		zero = 0;
		result = 1 / zero;
		_out->print("Result = ");
		_out->println(result);
	} else if (cmd == "null") {
		_out->println("cmd: Attempting to read through a pointer to no object ...");
		pubDebug("read null pointer");
		int* nullPointer;
		nullPointer = NULL;
		_out->print(*nullPointer);
	} else {
		_out->println("cmd: [bad] " + cmd);
		pubDebug("[bad]" + cmd);
	}
} // doCmd

void SolarGuardn::startOTA() {
	ArduinoOTA.onStart([this]() {
		_out->println("OTA: Start");
		_mqtt.disconnect();
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
		delay(15000);
		ESP.restart();
	});
	ArduinoOTA.setHostname(_hostname.c_str());
	ArduinoOTA.begin();
} // startOTA

void SolarGuardn::ledOn() {
	analogWrite(_ledPin, 1);
} // ledOn

void SolarGuardn::ledOff() {
	// PWM dim LED to off
	for (int i = 23; i < 1023; i++) {
		analogWrite(_ledPin, i);
		delay(2);
	}
	analogWrite(_ledPin, 0);
	digitalWrite(_ledPin, HIGH);
} // ledOff

void SolarGuardn::flushIn() {
	// flush Stream input buffer (not what Serial.flush() does)
	while (_out->available()) {
		_out->read();
	}
	yield();
} // flushIn

String SolarGuardn::UrlEncode(const String url) {
	// escape non-alphanumerics in URL
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

String SolarGuardn::getIPlocation() {
	// Using freegeoip.net to map public IP's location
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

String SolarGuardn::getLocation(const String address) {
	// using google maps API, return location for provided Postal Code
	HTTPClient http;
	String URL = "https://maps.googleapis.com/maps/api/geocode/json?address="
					+ UrlEncode(address) + "&key=" + String(_gMapsKey);
	String loc;
	http.setIgnoreTLSVerifyFailure(true);   // https://github.com/esp8266/Arduino/pull/2821
	http.setUserAgent(UserAgent);
	if (http.begin(URL, gMapsCrt)) {
		int stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				String payload = http.getString();	// http://arduinojson.org/assistant/
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					JsonObject& results = root["results"][0];
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

long SolarGuardn::getTimeZone(String loc) {
	// using google maps API, return TimeZone for provided timestamp
	HTTPClient http;
	long tz = false;
	String URL = "https://maps.googleapis.com/maps/api/timezone/json?location="
		+ UrlEncode(loc) + "&timestamp=" + String(_now) + "&key=" + String(_gMapsKey);
	String payload;
	http.setIgnoreTLSVerifyFailure(true);	// https://github.com/esp8266/Arduino/pull/2821
	http.setUserAgent(UserAgent);
	if (http.begin(URL, gMapsCrt)) {
		int stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				payload = http.getString();
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					tz = (int (root["rawOffset"]) + int (root["dstOffset"]));  // combine Offset and dstOffset
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

void SolarGuardn::setNTP() {
	// using location configure NTP with local timezone
	long tz = getTimeZone(location);
	if (!tz) return;
	_TZ = tz;
	_out->print("setNTP: configure NTP ...");
	configTime(0, 0, "pool.ntp.org", "time.nist.gov");
	while (time(nullptr) < (30 * 365 * 24 * 60 * 60)) {
		// wait for time to advance to this century
		delay(1000);
		_out->print(".");
	}
	_out->println(localTime());
	struct tm * calendar;
	time_t now = _now + _TZ;
	calendar = localtime(&now);
	calendar->tm_mday++;
	calendar->tm_hour = 2;
	calendar->tm_min = 0;
	calendar->tm_sec = 0;
	_twoAM = mktime(calendar);
	String t = ctime(&_twoAM);
	t.trim();
	_out->print("setNTP: next timezone check @ ");
	_out->println(t);
} // setNTP

String SolarGuardn::upTime() {
	// output _upTime as d:h:MM:SS
	time_t t = _now - _upTime;
	long s = t % 60;
	long m = (t / 60) % 60;
	long h = (t / (60 * 60)) % 24;
	long d = (t / (60 * 60 * 24));
	char buf[12];
	snprintf(buf, sizeof(buf), "%d:%d:%02d:%02d", d, h, m, s);
	return String(buf);
} // upTime()

String SolarGuardn::localTime() {
	// output local time as MM-DD-YYYY HH:MM:SS
	_now = time(nullptr);
	time_t t = _now + _TZ;
	struct tm * calendar = localtime(&t);
	char buf[20];
	snprintf(buf, sizeof(buf), "%02d-%02d-%04d %02d:%02d:%02d", 
			calendar->tm_mon + 1, calendar->tm_mday, calendar->tm_year + 1900, 
			calendar->tm_hour, calendar->tm_min, calendar->tm_sec);
	return String(buf);
} // localTime

void SolarGuardn::mqttConnect() {
	// connect MQTT and emit ESP info to debug channel
	if (!_mqtt.connect(_hostname.c_str(), _mqttUser, _mqttPass)) {
		_out->print("mqtt: not connected ");
		_out->println(_mqtt.state());
		delay(15000);
		ESP.restart();
	}
	String t = String(_mqttTopic) + "/" + _hostname + "/cmd";
	_mqtt.subscribe(t.c_str());
	pubDebug("MQTT connect");
} // mqttConnect

void SolarGuardn::mqttPublish(String topic, String data) {
	// publish data to topic on mqtt, reconnect as needed
	if (!_mqtt.connected()) {
		mqttConnect();
	}
	int r = _mqtt.publish((String(_mqttTopic) + "/" + _hostname + "/" + topic).c_str(), data.c_str());
	if (!r) _out->println("mqtt: error: " + String(r));
} // mqttPublish

bool SolarGuardn::reLoop() {
	// force re-reading sensors
	_timer = _now;
	return false;
}

bool SolarGuardn::readDHT(DHT & dht) {
	// read temp and humidity from DHT22
	temp = dht.readTemperature(true);
	humid = dht.readHumidity();
	if (isnan(temp) || isnan(humid)) {
		pubDebug("dht reading invalid");
		_out->println("dht: bad reading");
		return reLoop();
	} else {
		return true;
	}
} // readDHT

bool SolarGuardn::readHDC(ClosedCube_HDC1080 & hdc) {
	// read temp and humidity from HDC1080
	temp = hdc.readTemperature();
	humid = hdc.readHumidity();
	if (isnan(temp) || isnan(humid)) {
		pubDebug("hdc reading invalid");
		_out->println("hdc: bad reading");
		return reLoop();
	} else {
		temp = temp * 1.8F + 32.0F;
		return true;
	}
} // readHDC

bool SolarGuardn::readBME(BME280I2C & bme) {
	// read temp, humidity and pressure from BME280
	bme.read(pressure, temp, humid, _tUnit, _pUnit);
	if (isnan(temp) || isnan(humid) || isnan(pressure)) {
		pubDebug("bme reading invalid");
		_out->println("bme: bad reading");
		return reLoop();
	} else {
		return true;
	}
} // readBME

bool SolarGuardn::readTCS(Adafruit_TCS34725 & tcs) {
	// read color from TCS light sensor
	uint16_t r, g, b, c;
	tcs.getRawData(&r, &g, &b, &c);
	colorTemp = tcs.calculateColorTemperature(r, g, b);
	lux = tcs.calculateLux(r, g, b);
	if (isnan(colorTemp) || isnan(lux)) {
		pubDebug("light invalid");
		_out->println("bad light");
		return reLoop();
	} else {
		return true;
	}
} // readTCS

bool SolarGuardn::readMoisture(uint16_t pin, uint16_t pow, uint16_t num, uint16_t tim) {
	// read soil moisture using DFRobot SEN0193
	int s = 0;
	for (int i = 0; i < num; i++) {
		int r = 0, x = 0;
		do {
			digitalWrite(pow, HIGH);       // power to moisture sensor
			delay(tim);
			r = 1023 - analogRead(pin);   // read analog value from moisture sensor (invert for capacitance sensor)
			digitalWrite(pow, LOW);        // turn off moisture sensor
			delay(tim * 1.2);
			x++;
			} while (((r < 200) || (r > 800)) && x < num);  // skip invalid values
		s += r;
	}
	moist = round((float)s / (float)num);
	if (!moist) {
		pubDebug("moisture invalid");
		_out->println("moist: bad reading");
		return reLoop();
	} else {
		return true;
	}
} // readMoisture

bool SolarGuardn::getDist(uint16_t trig, uint16_t echo) {
	// get distance to water from HC-SR04 sensor
	int r = 0, c = 0;
	for (int i = 0; i < 10; i++) {
		digitalWrite(trig, HIGH);
		delayMicroseconds(10);
		digitalWrite(trig, LOW);
		unsigned long p = pulseIn(echo, HIGH, 12000) * 0.34 / 2;
		if (!p == 0 || p > 400) {
			r += p;
			c++;
		}
		delay(20);
	}
	if (!c || !r) {
		pubDebug("range invalid");
		_out->println("range: invalid");
		range = 0;
		return reLoop();
	} else {
		range = round((float)r / (float)c);
		return true;
	}
} // getDist

void SolarGuardn::pubJSON() {
	// create and publish JSON buffer
	String t = localTime();
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["app"] = _appName;
	if (temp > 0) root["temp"] = round(temp);
	if (humid > 0) root["humid"] = round(humid);
	//if (pressure > 0) root["pressure"] = (float)int(pressure / 100) + (float)(int(pressure) % 100) / 100;
	if (pressure > 0) root["pressure"] = (float)round(pressure * 100) / 100;
	if (moist > 0) root["moist"] = moist;
	if (range > 0) root["range"] = range;
	if (lux > 0) {
		root["colorTemp"] = colorTemp;
		root["lux"] = lux;
	}
	root["ip"] = _wifip.toString();
	root["time"] = t;
	root["timestamp"] = int(_now);
	root["uptime"] = int(_now - _upTime);
	root["freeheap"] = ESP.getFreeHeap();
	root.printTo(t = "");
	mqttPublish("data", t);
} // pubJSON

void SolarGuardn::pubDebug(String cmd) {
	// create and publish JSON buffer of debug info
	String t = localTime();
	float tz = _TZ / 3600;
	int c = SaveCrash.count();
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["app"] = _appName;
	root["cmd"] = cmd;
	root["lastreset"] = ESP.getResetReason();
	if (c) root["savecrash"] = c;
	root["location"] = location;
	root["macaddr"] = WiFi.macAddress();
	root["ip"] = _wifip.toString();
	root["pubip"] = _pubip.toString();
	root["time"] = t;
	root["tz"] = tz;
	root["timestamp"] = int(_now);
	root["uptime"] = int(_now - _upTime);
	root["freeheap"] = int(ESP.getFreeHeap());
	root.printTo(t = "");
	mqttPublish("debug", t);
} // pubDebug

