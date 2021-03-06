/*
	SolarGuardn Arduino Library
	copyright 2019 by David Denney <dragondaud@gmail.com>
	distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "SolarGuardn.h"

SolarGuardn::SolarGuardn(Stream * out,
		const char * hostname, const char * wifi_ssid, const char * wifi_pass,
		const char * mqtt_server, uint16_t mqtt_port,
		const char * mqtt_topic, const char * mqtt_user, const char * mqtt_pass,
		const char * tzKey, uint16_t sensors, uint16_t ledPin
	) : _out(out), _mqtt(_mqttwifi) {
	_app_name = hostname;
	_wifi_ssid = wifi_ssid;
	_wifi_pass = wifi_pass;
	_mqtt_server = mqtt_server;
	_mqtt_port = mqtt_port;
	_mqtt_topic = mqtt_topic;
	_mqtt_user = mqtt_user;
	_mqtt_pass = mqtt_pass;
	_tzKey = tzKey;
	_out = out;
	_ledPin = ledPin;
	_tUnit = BME280::TempUnit_Fahrenheit;
	_pUnit = BME280::PresUnit_inHg;
	_sensors = sensors;
}

void SolarGuardn::begin(uint8_t data, uint8_t clock) {
	while (!_out);		// wait for stream to open
	delay(100);			// ... and settle
	flushIn();			// purge input buffer
	if (_ledPin) pinMode(_ledPin, OUTPUT);
	_out->println();
	_out->print("setup: starting ");
	_out->println(_app_name);
	_out->print("setup: WiFi connecting to ");
	_out->print(_wifi_ssid);
	_out->print("...");
	String t = WiFi.macAddress();
	_hostname = String(_app_name) + "-" + t.substring(9, 11) + t.substring(12, 14) + t.substring(15, 17);
	WiFi.persistent(false);	// do not re-save WiFi params every boot
	WiFi.mode(WIFI_STA);
	WiFi.setAutoReconnect(true);
	WiFi.hostname(_hostname);
	WiFi.begin(_wifi_ssid, _wifi_pass);
	WiFi.waitForConnectResult();
	while (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
		_out->print(".");
		delay(500);
	}
	_out->println(" OK");
	_wifip = WiFi.localIP();
	if (timezone == "") timezone = getIPlocation();
	setNTP(timezone);
	_upTime = time(nullptr);
	startOTA();
	if (data) Wire.begin(data, clock);
	IPAddress mqttIP;
	if (!WiFi.hostByName(_mqtt_server, mqttIP)) {
		_out->print("setup: mqtt server invalid ");
		_out->println(_mqtt_server);
		delay(5000);
		ESP.restart();
	}
	_mqtt.setServer(_mqtt_server, _mqtt_port);
	_mqtt.setCallback([this](char* topic, byte* payload, unsigned int length) {
		// handle incoming MQTT messages
		payload[length] = '\0';
		String cmd = String((char*)payload);
		cmd.trim();
		cmd.toLowerCase();
		_out->print(F("mqtt: ["));
		_out->print(topic);
		_out->println("] " + cmd);
		doCmd(cmd);
	} );
	outDiag();
	mqttConnect();
	voltage = getVoltage(5);
	_out->print(WiFi.hostname());
	_out->println(" ready");
} // begin

bool SolarGuardn::handle() {
	// handle background loop() stuff
	int c = 0;
	while (WiFi.status() != WL_CONNECTED && c < 10) {
		WiFi.reconnect();
		delay(1000);
		c++;
	}
	if (WiFi.status() != WL_CONNECTED || WiFi.localIP() == IPAddress(0,0,0,0)) {
		_out->println("loop: WiFi invalid");
		delay(5000);
		ESP.restart();
	}
	checkIn();
	ArduinoOTA.handle();
	if (!_mqtt.connected()) mqttConnect();
	_mqtt.loop();
	_now = time(nullptr);
	voltage = getVoltage(5);
	yield();
	if ((_now + _TZ) > _twoAM) {
		pubDebug("TZ update");
		setNTP(timezone);
	}
	if (millis() > _timer) {
		_timer = millis() + SG_BETWEEN;
		return true;
	} else {
		delay(5000);
		return false;
	}
} // handle

void SolarGuardn::outDiag() {
	// output useful debug info
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
	_out->print("Free Heap: ");
	_out->println(ESP.getFreeHeap());
	int c = _SaveCrash.count();
	if (c) {
		_out->print("SAVED CRASH DUMPS: ");
		_out->println(c);
	}
} // outDiag

void SolarGuardn::checkIn() {
	// check _out Stream input for commands
	if (_out->available() > 0) {
		char inChar = _out->read();
		// flush input after each command to prevent garbage DoS
		flushIn();
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
			break;
		}
	}
} // checkIn

void SolarGuardn::doCmd(String cmd) {
	// handle command from serial or mqtt
	if (cmd == "reboot") {
		_out->println("cmd: Rebooting...");
		pubDebug("reboot");
		delay(1000);
		ESP.restart();
	} else if (cmd == "clear") {
		_SaveCrash.clear();
		_out->println("cmd: Crash information cleared");
		pubDebug("SaveCrash cleared");
	} else if (cmd == "print") {
		_SaveCrash.print();
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
	// setup ArduinoOTA for over the air updates
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
	if (_ledPin) digitalWrite(_ledPin, LOW);
} // ledOn

void SolarGuardn::ledOff() {
	if (_ledPin) digitalWrite(_ledPin, HIGH);
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
	WiFiClient client;
	String URL = "http://ip-api.com/json";
	String loc;
	http.setUserAgent(UserAgent);
	if (http.begin(client, URL)) {
		int stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				String payload = http.getString();
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					String isp = root["isp"];
					String region = root["regionName"];
					String country = root["countryCode"];
					String tz = root["timezone"];
					_out->println("getIPlocation: " + isp + ", " + region + ", " + country + ", " + tz);
					String ip = root["query"];
					_pubip.fromString(ip);
					http.end();
					return tz;
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
} // getIPlocation

long SolarGuardn::getOffset(const String timezone) {
	// using google maps API, return TimeZone for provided timestamp
	HTTPClient http;
	WiFiClient client;
	String URL = "http://api.timezonedb.com/v2/list-time-zone?key=" + String(_tzKey)
				+ "&format=json&zone=" + timezone;
	String payload;
	int stat;
	http.setUserAgent(UserAgent);
	if (http.begin(client, URL)) {
		stat = http.GET();
		if (stat > 0) {
			if (stat == HTTP_CODE_OK) {
				payload = http.getString();
				DynamicJsonBuffer jsonBuffer;
				JsonObject& root = jsonBuffer.parseObject(payload);
				if (root.success()) {
					JsonObject& zones = root["zones"][0];
					_TZ = zones["gmtOffset"];
					_out->println("getOffset: " + timezone + "(" + String(_TZ) + ")");
				} else {
					_out->println("getOffset: JSON parse failed!");
					_out->println(payload);
				}
			} else {
				_out->printf("getOffset: [HTTP] GET reply %d\r\n", stat);
			}
		} else {
			_out->printf("getOffset: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
		}
	}
	http.end();
	return stat;
} // getOffset

void SolarGuardn::setNTP(const String timezone) {
	// using timezone configure NTP with local timezone
	while (getOffset(timezone) != HTTP_CODE_OK) {
		delay(1000);
	}
	_out->print("setNTP: configure NTP ...");
	configTime(0, 0, "pool.ntp.org", "time.nist.gov");
	while (time(nullptr) < (30 * 365 * 24 * 60 * 60)) {
		// wait for time to advance to this century
		delay(1000);
		_out->print(".");
	}
	delay(1100);
	_out->println(localTime());
	struct tm * calendar;
	time_t now = _now + _TZ;
	calendar = localtime(&now);
	calendar->tm_mday++;
	calendar->tm_hour = 2;
	calendar->tm_min = 0;
	calendar->tm_sec = 0;
	_twoAM = mktime(calendar);	// next day @ 2AM update NTP
	String t = ctime(&_twoAM);
	t.trim();
	_out->print("setNTP: next timezone check @ ");
	_out->println(t);
} // setNTP

String SolarGuardn::upTime() {
	// output _upTime as [d:]h:MM:SS
	time_t t = _now - _upTime;
	long s = t % 60;
	long m = (t / 60) % 60;
	long h = (t / (60 * 60)) % 24;
	long d = (t / (60 * 60 * 24));
	char buf[12];
	if (d) snprintf(buf, sizeof(buf), "%d:%d:%02d:%02d", d, h, m, s);	// skip 0 days in output
	else snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
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
	if (!_mqtt.connect(_hostname.c_str(), _mqtt_user, _mqtt_pass)) {
		_out->println(PSTR("mqtt: not connected ") + String(_mqtt.state()));
		delay(15000);
		ESP.restart();
	}
	String t = String(_mqtt_topic) + "/" + _hostname + "/cmd";
	_mqtt.subscribe(t.c_str());	//subscribe cmd topic
	pubDebug("MQTT connect");
} // mqttConnect

void SolarGuardn::mqttPublish(String topic, String data) {
	// publish data to topic on mqtt, reconnect as needed
	if (!_mqtt.connected()) mqttConnect();
	if (!_mqtt.publish((String(_mqtt_topic) + "/" + _hostname + "/" + topic).c_str(), data.c_str())) {
		_out->println(PSTR("mqtt pub error ") + String(_mqtt.state()));
	}
} // mqttPublish

bool SolarGuardn::readTemp(DHT & dht) {
	int c = 0;
	float t, h;
	while (c < SG_RETRIES) {
		h = dht.readHumidity();
		t = dht.readTemperature(true);
		if (isnan(t) || isnan(h)) {
			c++;
			delay(500);
		} else {
			temp = t;
			humid = h;
			return true;
		}
	}
	temp = 0;
	humid = 0;
	pubDebug("dht reading invalid");
	_out->print("dht: bad reading, ");
	_out->print(t);
	_out->print(", ");
	_out->print(h);
	_out->println();
	_timer = millis();
	return false;
}

bool SolarGuardn::readTemp(Adafruit_AM2320 & asa) {
	int c = 0;
	float t, h;
	while (c < SG_RETRIES) {
		t = asa.readTemperature();
		h = asa.readHumidity();
		if (isnan(t) || isnan(h)) {
			c++;
			delay(200);
		} else {
			temp = t * 1.8F + 32.0F;
			humid = h;
			return true;
		}
	}
	temp = 0;
	humid = 0;
	pubDebug("asa reading invalid");
	_out->print("asa: bad reading, ");
	_out->print(t);
	_out->print(", ");
	_out->print(h);
	_out->println();
	_timer = millis();
	return false;
}

bool SolarGuardn::readTemp(BME280I2C & bme) {
	int c = 0;
	float p, t, h;
	while (c < SG_RETRIES) {
		bme.read(p, t, h, _tUnit, _pUnit);
		if (isnan(t) || isnan(h) || isnan(p)) {
			c++;
			delay(200);
		} else {
			temp = t;
			humid = h;
			pressure = p;
			return true;
		}
	}
	temp = 0;
	humid = 0;
	pressure = 0;
	pubDebug("bme reading invalid");
	_out->print("bme: bad reading, ");
	_out->print(t);
	_out->print(", ");
	_out->print(h);
	_out->print(", ");
	_out->print(p);
	_out->println();
	_timer = millis();
	return false;
}

bool SolarGuardn::readTCS(Adafruit_TCS34725 & tcs) {
	// read color from TCS light sensor
	uint16_t r, g, b, c;
	tcs.getRawData(&r, &g, &b, &c);
	colorTemp = tcs.calculateColorTemperature(r, g, b);
	lux = tcs.calculateLux(r, g, b);
	if (isnan(colorTemp) || isnan(lux)) {	// zero both if either invalid
		colorTemp = 0;
		lux = 0;
		pubDebug("tcs invalid reading");
		_out->println("tcs: bad reading");
		_timer = millis();
		return false;
	} else {
		if (lux == 0) colorTemp = 0;
		return true;
	}
} // readTCS

bool SolarGuardn::readCCS(Adafruit_CCS811 & ccs) {
	int c = 0;
	if (temp != 0) ccs.setEnvironmentalData(humid, temp);
	//ccs.setDriveMode(CCS811_DRIVE_MODE_250MS);
	delay(300);
	while (c < SG_RETRIES) {
		if (!ccs.available()) {
			c++;
			delay(250);
		} else {
			if (!ccs.readData()) {
				eCO2 = ccs.geteCO2();
				TVOC = ccs.getTVOC();
				//ccs.setDriveMode(CCS811_DRIVE_MODE_IDLE);
				return true;
			}
		}
	}
	pubDebug("ccs invalid reading");
	_out->println("ccs: bad reading");
	//_timer = millis();
	return false;
} // readCCS

bool SolarGuardn::readMoisture(Adafruit_seesaw & ss, uint16_t num) {
	// read soil moisture using Adafruit STEMMA Soil Sensor
	// average 'num' samples with current value
	uint16_t average, capread, count = 1;
	if (moist) average = moist;		// average starts with previous value
	else average = ss.touchRead(0);	// or extra sample if none
	for (int i = 0; i < num; i++) {
		capread = ss.touchRead(0);
		if ((capread > 100) && (capread < 1000)) {	// sum of valid values
			average += capread;
			count++;
		}
		delay(1);
	}
	if (count > 1) {
		moist = round((float)average / (float)count);	// store running average
		return true;
	} else {
		pubDebug("moisture invalid");
		return false;
	}
} // readMoisture

bool SolarGuardn::readMoisture(uint16_t pin, uint16_t pow, uint16_t num, uint16_t tim) {
	// read soil moisture using DFRobot SEN0193
	int s = 0;
	for (int i = 0; i < num; i++) {
		int r = 0, x = 0;
		do {
			digitalWrite(pow, HIGH);	// power to moisture sensor
			delay(tim);
			r = 1023 - analogRead(pin);	// read analog value from moisture sensor (invert for capacitance sensor)
			digitalWrite(pow, LOW);		// turn off moisture sensor
			delay(tim * 1.2);
			x++;
			} while (((r < 200) || (r > 800)) && x < num);	// skip invalid values
		s += r;
	}
	moist = round((float)s / (float)num);
	if (!moist) {
		pubDebug("moisture invalid");
		_out->println("moist: bad reading");
		_timer = millis();
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
		pubDebug("range invalid");
		_out->println("range: invalid");
		range = 0;
		_timer = millis();
		return false;
	} else {
		range = round((float)r / (float)c);
		return true;
	}
} // getDist

float SolarGuardn::getVoltage(uint16_t num) {
	// multisample Vcc 'num' times
	uint32_t average;
	for (int i = 0; i < num; i++) {
		average += ESP.getVcc();
		delay(10);
	}
	return (float)round((float)average / (float)num) / 1000.0;
} // getVoltage

void SolarGuardn::pubJSON() {
	// create and publish JSON buffer
	String t = localTime();
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["app"] = _app_name;
	if (temp > 0) root["temp"] = round(temp);
	if (humid > 0) root["humid"] = round(humid);
	//if (pressure > 0) root["pressure"] = (float)int(pressure / 100) + (float)(int(pressure) % 100) / 100;
	if (pressure > 0) root["pressure"] = (float)round(pressure * 100) / 100;
	if (moist > 0) root["moist"] = moist;
	if (range > 0) root["range"] = range;
	root["colorTemp"] = colorTemp;
	root["lux"] = lux;
	if (eCO2 > 0) root["eCO2"] = eCO2;
	if (TVOC > 0) root["TVOC"] = TVOC;
	if (voltage > 0) root["volts"] = voltage;
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
	int c = _SaveCrash.count();
	DynamicJsonBuffer jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();
	root["app"] = _app_name;
	root["libver"] = VERSION;
	root["sensor"] = _sensors;
	root["cmd"] = cmd;
	root["lastreset"] = ESP.getResetReason();
	if (c) root["savecrash"] = c;
	root["location"] = timezone;
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
	delay(1000);
} // pubDebug

void SolarGuardn::deepSleep(uint16_t time) {
	// put ESP into deepSleep for [time] seconds
	// requires D0 connected to RST
	ESP.deepSleep(time * 1000000);
} // deepSleep
