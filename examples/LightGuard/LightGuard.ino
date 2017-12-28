/*
   SolarGuardn - LightGuard v0.8.1 PRE-RELEASE 28-Dec-2017
   by David Denney <dragondaud@gmail.com>
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  while (!Serial);
  sg.flushIn(Serial);
  delay(100);
  Serial.println();
  Serial.print(F("setup: WiFi connecting to "));
  Serial.print(WIFI_SSID);
  Serial.print(F("..."));
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  if (sg.host == "") {
    String t = WiFi.macAddress();
    sg.host = String(HOST) + "-" + t.substring(9, 11) + t.substring(12, 14) + t.substring(15, 17);
  }
  WiFi.hostname(sg.host);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(500);
  }
  MDNS.begin(sg.host.c_str());
  Serial.println(F(" OK"));

  if (sg.location == "") {
    sg.location = sg.getIPlocation();
  } else {
    sg.getIPlocation();
    sg.location = sg.getLocation(sg.location, gMapsKey);
  }
  sg.setNTP();

  ArduinoOTA.onStart([]() {
    Serial.println(F("\nOTA: Start"));
  } );
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nOTA: End"));
    delay(500);
  } );
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print("OTA Progress: " + String((progress / (total / 100))) + " \r");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print("\nError[" + String(error) + "]: ");
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    else Serial.println(F("unknown error"));
    delay(5000);
    ESP.restart();
  });
  ArduinoOTA.begin();

  MQTTclient.setServer(MQTT_SERV, MQTT_PORT);
  sg.mqttConnect(MQTTclient);

  pinMode(BUILTIN_LED, OUTPUT);

  Wire.begin(SDA, SCL);
  if (!tcs.begin()) {
    Serial.println(F("No TCS34725 found"));
    delay(5000);
    ESP.restart();
  }

  delay(1000);
  sg.UPTIME = time(nullptr);

  Serial.print(F("Last reset reason: "));
  Serial.println(ESP.getResetReason());
  Serial.print(F("WiFi Hostname: "));
  Serial.println(sg.host);
  Serial.print(F("WiFi IP addr: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("WiFi gw addr: "));
  Serial.println(WiFi.gatewayIP());
  Serial.print(F("WiFi MAC addr: "));
  Serial.println(WiFi.macAddress());
  Serial.print(F("ESP Sketch size: "));
  Serial.println(ESP.getSketchSize());
  Serial.print(F("ESP Flash free: "));
  Serial.println(ESP.getFreeSketchSpace());
  Serial.print(F("ESP Flash Size: "));
  Serial.println(ESP.getFlashChipRealSize());
  SaveCrash.print(Serial);
} // setup

void loop() {
  if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
    String t = "Invalid null address";
    sg.mqttPublish(MQTTclient, "debug", t);
    Serial.println(t);
    delay(500);
    ESP.restart();
  }
  ArduinoOTA.handle();
  MQTTclient.loop();
  if (millis() > TIMER) {
    TIMER = millis() + BETWEEN;
  } else {
    delay(5000);
    return;
  }
  analogWrite(BUILTIN_LED, 1);
  String ip = WiFi.localIP().toString();
  float temp, humid, pressure;
  int heap = ESP.getFreeHeap();
  time_t now = time(nullptr);
  if (now > sg.TWOAM) {
    Serial.println();
    sg.setNTP();
  }
  String t = ctime(&now);
  t.trim(); // ctime returns extra whitespace
  String u = sg.upTime(now);

  uint16_t r, g, b, c, colorTemp, lux;
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);

  Serial.printf("%s: %s, %dK, %d Lux, %s uptime, %d heap \r", ip.c_str(), t.c_str(), colorTemp, lux, u.c_str(), heap);

  t = "{\"colorTemp\": " + String(colorTemp) + ", \"Lux\": " + String(lux) + \
      ", \"RGBC\": [" + String(r) + "," + String(g) + "," + String(b) + "," + String(c) + \
      "], \"timestamp\": " + String(now) + ", \"freeheap\": " + String(heap) + "}";
  sg.mqttPublish(MQTTclient, "data", t);

  for (int i = 23; i < 1023; i++) {
    analogWrite(BUILTIN_LED, i);
    delay(2);
  }
  analogWrite(BUILTIN_LED, 0);
  digitalWrite(BUILTIN_LED, HIGH);
} // loop

