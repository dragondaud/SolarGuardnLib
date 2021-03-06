/*
   SolarGuardn - TankGuard v0.9.0 PRE-RELEASE
   copyright 2019 by David Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  sg.begin(SDA, SCL);
  pinMode(LED_BUILTIN, OUTPUT);
  // only one temp/humid sensor can be used
#ifdef sgBME
  if (!bme.begin()) sg.pubDebug("BME280 not found");
#endif
#ifdef sgDHT
  dht.begin();
#endif
#ifdef sgASA
  asa.begin();
#endif
#ifdef sgTCS
  if (!tcs.begin()) sg.pubDebug("TCS34725 not found");
#endif
#ifdef sgRANGE
  pinMode(TRIG, OUTPUT);
  digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT);
#endif
  delay(1000);  // wait for sensors to stabalize
} // setup

void loop() {
  if (!sg.handle()) return;
  sg.ledOn();
  String t = sg.localTime();
  String u = sg.upTime();
#if defined (sgDHT)
  if (!sg.readTemp(dht)) return;
#elif defined (sgASA)
  if (!sg.readTemp(asa)) return;
#elif defined (sgBME)
  if (!sg.readTemp(bme)) return;
#endif
#ifdef sgTCS
  if (!sg.readTCS(tcs)) return;
#endif
#ifdef sgRANGE
  if (!sg.getDist(TRIG, ECHO)) return;
#endif
  Serial.print(t);
  Serial.print(", ");
  Serial.print(round(sg.temp));
  Serial.print("°F, ");
  Serial.print(round(sg.humid));
  Serial.print("%RH, ");
  Serial.print(sg.pressure, 2);
  Serial.print(" inHg, ");
  Serial.print(sg.range);
  Serial.print(" mm, ");
  Serial.print(u);
  Serial.print(" uptime, ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" heap");
  sg.pubJSON();
  sg.ledOff();
} // loop
