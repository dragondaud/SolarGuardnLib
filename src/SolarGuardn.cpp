/*
	SolarGuardn Arduino Library
	copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "SolarGuardn.h"

SolarGuardn::SolarGuardn(const char* hostname, const char* wifi_ssid, const char* wifi_pass, const char* server, uint16_t port, const char* topic, const char* user, const char* pass, const char* gMapsKey, Stream * out, PubSubClient & mqtt) : _out(out) {
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
	_mqtt = mqtt;
	_ledPin = BUILTIN_LED;
	_tUnit = BME280::TempUnit_Fahrenheit;
	_pUnit = BME280::PresUnit_inHg;
}

void SolarGuardn::setup(uint16_t data, uint16_t clock) {
	while (!_out);		// wait for stream to open
	delay(100);			// ... and settle
	flushIn();			// purge input buffer
	_out->println();
	_out->print("setup: WiFi connecting to ");
	_out->print(_wifi_ssid);
	_out->print("...");
	WiFi.persistent(false);	// do not re-save WiFi params every boot
	WiFi.mode(WIFI_STA);
	String t = WiFi.macAddress();
	_hostname = String(_appName) + "-" + t.substring(9, 11) + t.substring(12, 14) + t.substring(15, 17);
	WiFi.hostname(_hostname);
	WiFi.begin(_wifi_ssid, _wifi_pass);
	while (WiFi.status() != WL_CONNECTED) {
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
	startOTA();
	Wire.begin(data, clock);
	_mqtt.setServer(_mqttServer, _mqttPort);
	mqttConnect();
	UPTIME = time(nullptr);
	outDiag();
} // setup()

bool SolarGuardn::handle() {
	while (WiFi.status() != WL_CONNECTED) {
		_out->println("loop: WiFi reconnect");
		WiFi.reconnect();
		delay(1000);
	}
	checkIn();
	ArduinoOTA.handle();
	_mqtt.loop();
	now = time(nullptr);
	heap = ESP.getFreeHeap();
	if (now > _twoAM) {
		_out->println();
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
}

void SolarGuardn::checkIn() {
	// check _out Stream input for commands
	if (_out->available() > 0) {
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
}

void SolarGuardn::startOTA() {
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
	ArduinoOTA.setHostname(_hostname.c_str());
	ArduinoOTA.begin();
	_out->println("OTA begin");
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

int SolarGuardn::getTimeZone(const time_t now, String loc) {
	// using google maps API, return TimeZone for provided timestamp
	HTTPClient http;
	int tz = false;
	String URL = "https://maps.googleapis.com/maps/api/timezone/json?location="
		+ UrlEncode(loc) + "&timestamp=" + String(now) + "&key=" + String(_gMapsKey);
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
	_TZ = getTimeZone(time(nullptr), location);
	_out->print("setNTP: configure NTP ...");
	configTime(_TZ, 0, "pool.ntp.org", "time.nist.gov");
	while (time(nullptr) < (30 * 365 * 24 * 60 * 60)) {
		// wait for time to advance to this century
		delay(1000);
		_out->print(".");
	}
	delay(5000);
	now = time(nullptr);
	String t = localTime();
	_out->println(t.substring(3));
	struct tm * calendar;
	calendar = localtime(&now);
	calendar->tm_mday++;
	calendar->tm_hour = 2;
	calendar->tm_min = 0;
	calendar->tm_sec = 0;
	_twoAM = mktime(calendar);
	t = ctime(&_twoAM);
	t.trim();
	_out->print("setNTP: next timezone check @ ");
	_out->println(t);
} // setNTP

String SolarGuardn::upTime() {
	// output UPTIME as d:h:MM:SS
	long t = now - UPTIME;
	long s = t % 60;
	long m = (t / 60) % 60;
	long h = (t / (60 * 60)) % 24;
	long d = (t / (60 * 60 * 24));
	char ut[12];
	snprintf(ut, sizeof(ut), "%d:%d:%02d:%02d", d, h, m, s);
	if (now > _twoAM) {
		_out->println();
		setNTP();
	}
	return String(ut);
} // upTime()

String SolarGuardn::localTime() {
	String t = ctime(&now);
	t.trim(); // ctime returns extra whitespace
	return t;
} // localTime

void SolarGuardn::mqttConnect() {
	// connect MQTT and emit ESP info to debug channel
	if (_mqtt.connect(_hostname.c_str(), _mqttUser, _mqttPass)) {
		_out->println("mqtt: reconnect");
		pubDebug("MQTT connect");
	} else {
		_out->print("mqtt: not connected ");
		_out->println(_mqtt.state());
		delay(5000);
		ESP.restart();
	}
} // mqttConnect

void SolarGuardn::mqttPublish(String topic, String data) {
	// publish data to topic on mqtt, reconnect as needed
	if (!_mqtt.connected()) {
		mqttConnect();
	}
	int r = _mqtt.publish((String(_mqttTopic) + "/" + _hostname + "/" + topic).c_str(), data.c_str());
	if (!r) _out->println("mqtt: error: " + String(r));
} // mqttPublish

bool SolarGuardn::readDHT(DHT & dht) {
	// read temp and humidity from DHT22
	temp = dht.readTemperature(true);
	humid = dht.readHumidity();
	if (isnan(temp) || isnan(humid)) {
		_out->println("dht: bad reading");
		return false;
	} else {
		return true;
	}
} // readDHT

bool SolarGuardn::readHDC(ClosedCube_HDC1080 & hdc) {
	// read temp and humidity from HDC1080
	temp = hdc.readTemperature();
	humid = hdc.readHumidity();
	if (isnan(temp) || isnan(humid)) {
		_out->println("hdc: bad reading");
		return false;
	} else {
		temp = temp * 1.8F + 32.0F;
		return true;
	}
} // readHDC

bool SolarGuardn::readBME(BME280I2C & bme) {
	// read temp, humidity and pressure from BME280
	bme.read(pressure, temp, humid, _tUnit, _pUnit);
	if (isnan(temp) || isnan(humid) || isnan(pressure)) {
		_out->println("bme: bad reading");
		return false;
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
		_out->println("bad light");
		return false;
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
		_out->println("moist: bad reading");
		return false;
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
		_out->println("range: invalid");
		range = 0;
		return false;
	} else {
		range = round((float)r / c);
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
	root["timestamp"] = now;
	root["uptime"] = int(now - UPTIME);
	root["freeheap"] = heap;
	root.printTo(t = "");
	mqttPublish("data", t);
} // pubJSON

void SolarGuardn::pubDebug(String cmd) {
	// create and publish JSON buffer of debug info
	String t = localTime();
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["app"] = _appName;
	root["cmd"] = cmd;
	root["reset"] = ESP.getResetReason();
	root["location"] = location;
	root["ip"] = _wifip.toString();
	root["pubip"] = _pubip.toString();
	root["time"] = t;
	root["timestamp"] = now;
	root["uptime"] = int(now - UPTIME);
	root["freeheap"] = heap;
	root.printTo(t = "");
	mqttPublish("debug", t);
} // pubDebug

