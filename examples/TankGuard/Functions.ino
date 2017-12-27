void flushIn(Stream &s) { // flush serial input buffer (not what Serial.flush() does)
  while (s.available()) {
    s.read();
    yield();
  }
} // flushIn

void checkSer(void) { // check Serial input for commands
  if (Serial.available() > 0) {
    char inChar = Serial.read();
    flushIn(Serial);  // flush input after each command to prevent garbage DoS
    switch (inChar) {
      case '0':
        Serial.println(F("\nAttempting to divide by zero ..."));
        int result, zero;
        zero = 0;
        result = 1 / zero;
        Serial.print("Result = ");
        Serial.println(result);
        break;
      case 'e':
        Serial.println(F("\nAttempting to read through a pointer to no object ..."));
        int* nullPointer;
        nullPointer = NULL;
        Serial.print(*nullPointer);
        break;
      case 'c':
        SaveCrash.clear();
        Serial.println(F("\nCrash information cleared"));
        break;
      case 'r':
        Serial.println(F("\nRebooting..."));
        delay(1000);
        ESP.restart();
        break;
      default:
        Serial.println(F("\nc : clear crash information"));
        Serial.println(F("e : attempt to read through a pointer to no object"));
        Serial.println(F("0 : attempt to divide by zero"));
        Serial.println(F("r : restart esp"));
        break;
    }
  }
} // checkSer

String UrlEncode(const String url) {  // escape non-alphanumerics in URL
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

String getIPlocation() {  // Using freegeoip.net to map public IP's location
  HTTPClient http;
  String URL = "http://freegeoip.net/json/";
  String payload;
  String loc;
  http.setUserAgent(UserAgent);
  if (!http.begin(URL)) {
    Serial.println(F("getIPlocation: [HTTP] connect failed!"));
  } else {
    int stat = http.GET();
    if (stat > 0) {
      if (stat == HTTP_CODE_OK) {
        payload = http.getString();
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (root.success()) {
          String region = root["region_name"];
          String country = root["country_code"];
          String lat = root["latitude"];
          String lng = root["longitude"];
          loc = lat + "," + lng;
          String ip = root["ip"];
          PUB_IP = ip;
          Serial.println("getIPlocation: " + region + ", " + country);
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

String getLocation(const String address, const char* key) { // using google maps API, return location for provided Postal Code
  HTTPClient http;
  String URL = "https://maps.googleapis.com/maps/api/geocode/json?address="
               + UrlEncode(address) + "&key=" + String(key);
  String payload;
  String loc;
  http.setIgnoreTLSVerifyFailure(true);   // https://github.com/esp8266/Arduino/pull/2821
  http.setUserAgent(UserAgent);
  if (!http.begin(URL, gMapsCrt)) {
    Serial.println(F("getLocation: [HTTP] connect failed!"));
  } else {
    int stat = http.GET();
    if (stat > 0) {
      if (stat == HTTP_CODE_OK) {
        payload = http.getString();
        DynamicJsonBuffer jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject(payload);
        if (root.success()) {
          JsonObject& results = root["results"][0];           // http://arduinojson.org/assistant/
          JsonObject& results_geometry = results["geometry"];
          String address = results["formatted_address"];
          String lat = results_geometry["location"]["lat"];
          String lng = results_geometry["location"]["lng"];
          loc = lat + "," + lng;
          Serial.print(F("getLocation: "));
          Serial.println(address);
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

int getTimeZone(time_t now, String loc, const char* key) { // using google maps API, return TimeZone for provided timestamp
  HTTPClient http;
  int tz = false;
  String URL = "https://maps.googleapis.com/maps/api/timezone/json?location="
               + UrlEncode(loc) + "&timestamp=" + String(now) + "&key=" + String(key);
  String payload;
  http.setIgnoreTLSVerifyFailure(true);   // https://github.com/esp8266/Arduino/pull/2821
  http.setUserAgent(UserAgent);
  if (!http.begin(URL, gMapsCrt)) {
    Serial.println(F("getTimeZone: [HTTP] connect failed!"));
  } else {
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

void setNTP() { // using location configure NTP with local timezone
  int TZ = getTimeZone(time(nullptr), location, gMapsKey);
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

String upTime(const time_t now) { // output UPTIME as HH:MM:SS
  long t = now - UPTIME;
  long s = t % 60;
  long m = (t / 60) % 60;
  long h = (t / (60 * 60)) % 24;
  char ut[10];
  snprintf(ut, sizeof(ut), "%d:%02d:%02d", h, m, s);
  return String(ut);
} // upTime()

bool mqttConnect() {  // connect MQTT and emit ESP info to debug channel
  if (MQTTclient.connect(HOST.c_str(), MQTT_USER, MQTT_PASS)) {
    time_t now = time(nullptr);
    Serial.print("MQTT connected to ");
    Serial.println(String(MQTT_SERV) + ":" + String(MQTT_PORT));
    String t = "{\"hostname\": \"" + HOST + "\", \"wifi_ip\": \"" + WiFi.localIP().toString() + \
               "\", \"public_ip\": \"" + PUB_IP + "\", \"reset_reason\": \"" + ESP.getResetReason() + \
               "\", \"location\": \"" + location + "\", \"timestamp\": " + String(now) + \
               ", \"freeheap\": " + String(ESP.getFreeHeap()) + "}";
    mqttPublish("debug", t);
  } else {
    Serial.print(MQTTclient.state());
    Serial.println(" MQTT not connected.");
    delay(5000);
    ESP.restart();
  }
} // mqttConnect

void mqttPublish(String topic, String data) { // publish data to topic on mqtt, reconnect as needed
  if (!MQTTclient.connected()) {
    Serial.println();
    mqttConnect();
  }
  int r = MQTTclient.publish((String(MQTT_TOPIC) + "/" + HOST + "/" + topic).c_str(), data.c_str());
  if (!r) Serial.println("MQTT error: " + String(r));
} // mqttPublish

int getDist() { // get distance to water from HC-SR04 sensor
  int r = 0, c = 0;
  for (int i = 0; i < 10; i++) {
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);
    unsigned long p = pulseIn(ECHO, HIGH, 12000) * 0.34 / 2;
    if (!p == 0 || p > 400) {
      r += p;
      c++;
    }
    delay(20);
  }
  if (!c == 0) return round(r / c);
  else return -1;
} // getDist

