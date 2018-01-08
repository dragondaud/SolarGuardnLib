/*
   SolarGuardn - TankGuard v0.8.1 PRE-RELEASE 27-Dec-2017
   by David Denney <dragondaud@gmail.com>
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  sg.setup();
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(TRIG, OUTPUT);
  digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT);
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);
  pinMode(POW, OUTPUT);
  digitalWrite(POW, HIGH);
  delay(100);
  if (!bme.begin()) {
    Serial.println(F("cannot find BME280 sensor"));
    delay(5000);
    ESP.restart();
  }
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

  float temp, humid, pressure;
  bme.read(pressure, temp, humid);
  temp = temp * 1.8F + 32.0F;
  pressure = pressure * 0.02953;

  int range = getDist();

  Serial.printf("%s: %s, %d°F, %d%%RH, %d.%02d inHg, %4d mm, %s uptime, %d heap \r", \
                ip.c_str(), t.c_str(), round(temp), round(humid), int(pressure / 100), \
                int(pressure) % 100, range, u.c_str(), heap);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["app"] = HOST;
  root["temp"] = round(temp);
  root["humid"] = round(humid);
  root["pressure"] = (float)int(pressure / 100) + (float)(int(pressure) % 100) / 100;
  root["range"] = range;
  root["ip"] = ip;
  root["time"] = t;
  root["timestamp"] = now;
  root["uptime"] = int(now - sg.UPTIME);
  root["freeheap"] = heap;
  root.printTo(t = "");
  sg.mqttPublish("data", t);
  sg.ledOff(BUILTIN_LED);
} // loop

