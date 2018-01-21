/*
   SolarGuardn - LightGuard v0.8.2 PRE-RELEASE 21-Jan-2018
   copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  //sg.location = "81007"; // over-ride geoIP
  sg.setup(SDA, SCL);
#ifdef sgBME
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);
  pinMode(POW, OUTPUT);
  digitalWrite(POW, HIGH);
#endif
  delay(100);
  // only one temp/humid sensor can be used
#if defined (sgDHT)
  dht.begin();
#elif defined (sgHDC)
  hdc.begin(0x40);
#elif defined (sgBME)
  if (!bme.begin()) sg.pubDebug("BME280 not found");
#endif
#ifdef sgTCS
  if (!tcs.begin()) sg.pubDebug("TCS34725 not found");
#endif
  pinMode(BUILTIN_LED, OUTPUT);
  delay(1000);  // wait for sensors to stabalize
} // setup

void loop() {
  if (!sg.handle()) return;
  sg.ledOn();
  String t = sg.localTime();
  String u = sg.upTime();
#if defined (sgDHT)
  if (!sg.readDHT(dht)) return;
#elif defined (sgHDC)
  if (!sg.readHDC(hdc)) return;
#elif defined (sgBME)
  if (!sg.readBME(bme)) return;
#endif
#ifdef sgTCS
  if (!sg.readTCS(tcs)) return;
#endif
  Serial.print("\033[A\r\033[K");
  Serial.print(WiFi.hostname());
  Serial.print(": ");
  Serial.print(t);
  Serial.print(", ");
  Serial.print(round(sg.temp));
  Serial.print("°F, ");
  Serial.print(round(sg.humid));
  Serial.print("%RH, ");
  Serial.print(sg.colorTemp);
  Serial.print(" colorTemp, ");
  Serial.print(sg.lux);
  Serial.print(" Lux, ");
  Serial.print(u);
  Serial.print(", ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" heap");
  sg.pubJSON();
  sg.ledOff();
} // loop

