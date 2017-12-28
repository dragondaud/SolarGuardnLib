/*
	SolarGuardn library
*/

#include "SolarGuardn.h"

SolarGuardn::SolarGuardn(const char* server, unsigned int port, const char* topic, const char* user, const char* pass, const char* key) {
	_mqttServer = server;
	_mqttPort = port;
	_mqttTopic = topic;
	_mqttUser = user;
	_mqttPass = pass;
	_gMapsKey = key;
}

void SolarGuardn::flushIn(Stream &s) { // flush serial input buffer (not what Serial.flush() does)
  while (s.available()) {
    s.read();
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
          Serial.println(F("getIPlocation: JSON parse failed!"));
          Serial.println(payload);
        }
      } else {
        Serial.printf("getIPlocation: [HTTP] GET reply %d\r\n", stat);
      }
    } else {
      Serial.printf("getIPlocation: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
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
          Serial.println(F("getLocation: JSON parse failed!"));
          Serial.println(payload);
        }
      } else {
        Serial.printf("getLocation: [HTTP] GET reply %d\r\n", stat);
      }
    } else {
      Serial.printf("getLocation: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
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
          Serial.printf("getTimeZone: %s (%d)\r\n", tzname, tz);
        } else {
          Serial.println(F("getTimeZone: JSON parse failed!"));
          Serial.println(payload);
        }
      } else {
        Serial.printf("getTimeZone: [HTTP] GET reply %d\r\n", stat);
      }
    } else {
      Serial.printf("getTimeZone: [HTTP] GET failed: %s\r\n", http.errorToString(stat).c_str());
    }
  }
  http.end();
  return tz;
} // getTimeZone

void SolarGuardn::setNTP() { // using location configure NTP with local timezone
  int TZ = getTimeZone(time(nullptr), location, _gMapsKey);
  Serial.print(F("setNTP: configure NTP ..."));
  configTime((TZ * 3600), 0, "pool.ntp.org", "time.nist.gov");
  while (time(nullptr) < (30 * 365 * 24 * 60 * 60)) {         // wait for time to advance to this century
    delay(1000);
    Serial.print(F("."));
  }
  delay(5000);
  time_t now = time(nullptr);
  String t = ctime(&now);
  t.trim();
  Serial.println(t.substring(3));
  struct tm * calendar;
  calendar = localtime(&now);
  calendar->tm_mday++;
  calendar->tm_hour = 2;
  calendar->tm_min = 0;
  calendar->tm_sec = 0;
  TWOAM = mktime(calendar);
  t = ctime(&TWOAM);
  t.trim();
  Serial.print(F("setNTP: next timezone check @ "));
  Serial.println(t);
} // setNTP

String SolarGuardn::upTime(const time_t now) { // output UPTIME as HH:MM:SS
  long t = now - UPTIME;
  long s = t % 60;
  long m = (t / 60) % 60;
  long h = (t / (60 * 60)) % 24;
  char ut[10];
  snprintf(ut, sizeof(ut), "%d:%02d:%02d", h, m, s);
  return String(ut);
} // upTime()

bool SolarGuardn::mqttConnect(PubSubClient mqtt) {  // connect MQTT and emit ESP info to debug channel
  if (mqtt.connect(host.c_str(), _mqttUser, _mqttPass)) {
    time_t now = time(nullptr);
    Serial.print("MQTT connected to ");
    Serial.println(String(_mqttServer) + ":" + String(_mqttPort));
    String t = "{\"hostname\": \"" + host + "\", \"wifi_ip\": \"" + WiFi.localIP().toString() + \
               "\", \"public_ip\": \"" + _pubip.toString() + "\", \"reset_reason\": \"" + ESP.getResetReason() + \
               "\", \"location\": \"" + location + "\", \"timestamp\": " + String(now) + \
               ", \"freeheap\": " + String(ESP.getFreeHeap()) + "}";
    mqttPublish(mqtt, "debug", t);
  } else {
    Serial.print(mqtt.state());
    Serial.println(" MQTT not connected.");
    delay(5000);
    ESP.restart();
  }
} // mqttConnect

void SolarGuardn::mqttPublish(PubSubClient mqtt, String topic, String data) { // publish data to topic on mqtt, reconnect as needed
  if (!mqtt.connected()) {
    Serial.println();
    mqttConnect(mqtt);
  }
  int r = mqtt.publish((String(_mqttTopic) + "/" + host + "/" + topic).c_str(), data.c_str());
  if (!r) Serial.println("MQTT error: " + String(r));
} // mqttPublish

