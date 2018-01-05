/*
   SolarGuardn - LightGuard v0.8.1 PRE-RELEASE 28-Dec-2017
   by David Denney <dragondaud@gmail.com>
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  sg.setup();
  if (!bme.begin()) {
    Serial.println(F("cannot find BME280 sensor"));
    delay(5000);
    ESP.restart();
  }
  if (!tcs.begin()) {
    Serial.println(F("No TCS34725 found"));
    delay(5000);
    ESP.restart();
  }
  pinMode(BUILTIN_LED, OUTPUT);
  delay(1000);  // wait for sensors to stabalize
} // setup

void loop() {
  if (!sg.loop()) return;
  sg.ledOn(BUILTIN_LED);
  String ip = WiFi.localIP().toString();
  int heap = ESP.getFreeHeap();
  time_t now = time(nullptr);
  if (now > sg.twoAM) {
    Serial.println();
    sg.setNTP();
  }
  String t = ctime(&now);
  t.trim(); // ctime returns extra whitespace
  String u = sg.upTime(now);

  float temp, humid, pressure;
  bme.read(pressure, temp, humid);
  temp = temp * 1.8F + 32.0F;
  pressure = pressure * 0.02953;

  uint16_t r, g, b, c, colorTemp, lux;
  tcs.getRawData(&r, &g, &b, &c);
  colorTemp = tcs.calculateColorTemperature(r, g, b);
  lux = tcs.calculateLux(r, g, b);

  Serial.printf("%s: %s, %d°F, %d%%RH, %d.%02d inHg, %dK, %d Lux, %s uptime, %d heap \r", \
                ip.c_str(), t.c_str(), round(temp), round(humid), int(pressure / 100), \
                int(pressure) % 100, colorTemp, lux, u.c_str(), heap);

  DynamicJsonBuffer jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["app"] = HOST;
  root["temp"] = round(temp);
  root["humid"] = round(humid);
  root["pressure"] = (float)int(pressure / 100) + (float)(int(pressure) % 100) / 100;
  root["colorTemp"] = colorTemp;
  root["Lux"] = lux;
  root["ip"] = ip;
  root["time"] = t;
  root["timestamp"] = now;
  root["uptime"] = int(now - sg.UPTIME);
  root["freeheap"] = heap;
  root.printTo(t = "");
  sg.mqttPublish("data", t);
  sg.ledOff(BUILTIN_LED);
} // loop

