/*
   SolarGuardn - LightGuard v0.8.1 PRE-RELEASE 28-Dec-2017
   by David Denney <dragondaud@gmail.com>
*/

#include "Config.h"

void setup() {
  Serial.begin(BAUD);
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
  sg.loop();
  if (millis() > TIMER) {
    TIMER = millis() + BETWEEN;
  } else {
    delay(5000);
    return;
  }
  analogWrite(BUILTIN_LED, 1);
  String ip = WiFi.localIP().toString();
  int heap = ESP.getFreeHeap();
  time_t now = time(nullptr);
  if (now > sg.TWOAM) {
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

  Serial.printf("%s: %s, %d°F, %d%%RH, %d.%d inHg, %dK, %d Lux, %s uptime, %d heap \r", \
                ip.c_str(), t.c_str(), round(temp), round(humid), int(pressure / 100), \
                int(pressure) % 100, colorTemp, lux, u.c_str(), heap);

  t = "{\"temp\": " + String(round(temp)) + ", \"humid\": " + String(round(humid)) + \
      ", \"pressure\": " + String(int(pressure / 100)) + "." + String(int(pressure) % 100) + \
      ", \"colorTemp\": " + String(colorTemp) + ", \"Lux\": " + String(lux) + \
      ", \"RGBC\": [" + String(r) + "," + String(g) + "," + String(b) + "," + String(c) + \
      "], \"timestamp\": " + String(now) + ", \"freeheap\": " + String(heap) + "}";

  sg.mqttPublish("data", t);

  for (int i = 23; i < 1023; i++) {
    analogWrite(BUILTIN_LED, i);
    delay(2);
  }
  analogWrite(BUILTIN_LED, 0);
  digitalWrite(BUILTIN_LED, HIGH);
} // loop

