/*
   SolarGuardn - SoilGuard v0.8.1 PRE-RELEASE 27-Dec-2017
   by David Denney <dragondaud@gmail.com>
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  sg.setup();
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(MGND, OUTPUT);          // moisture sensor ground
  digitalWrite(MGND, LOW);
  pinMode(MPOW, OUTPUT);          // moisture sensor power
  digitalWrite(MPOW, LOW);
  delay(100);
  dht.begin();
  delay(1000);  // wait for sensors to stabalize
} // setup

void loop() {
  if (!sg.loop()) return;
  sg.ledOn(BUILTIN_LED);
  String ip = WiFi.localIP().toString();
  int heap = ESP.getFreeHeap();
  time_t now = time(nullptr);
  String t = ctime(&now);
  t.trim(); // ctime returns extra whitespace
  String u = sg.upTime(now);

  float temp, humid;
  if (!readDHT(&humid, &temp)) return;

  int soil = readMoisture(MOIST, MPOW, SNUM, STIM);

  Serial.printf("%s: %s, %d°F, %d%%RH, %d moisture, %s uptime, %d heap \r", \
                ip.c_str(), t.c_str(), round(temp), round(humid), soil, u.c_str(), heap);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["app"] = HOST;
  root["temp"] = round(temp);
  root["humid"] = round(humid);
  root["soil"] = soil;
  root["ip"] = ip;
  root["time"] = t;
  root["timestamp"] = now;
  root["uptime"] = int(now - sg.UPTIME);
  root["freeheap"] = heap;
  root.printTo(t = "");
  sg.mqttPublish("data", t);
  sg.ledOff(BUILTIN_LED);
} // loop

