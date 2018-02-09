/*
   SolarGuardn - TankGuard v0.8.3 PRE-RELEASE
   copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  //sg.location = "81007"; // over-ride geoIP
  sg.begin(SDA, SCL);
  pinMode(LED_BUILTIN, OUTPUT);
  // only one temp/humid sensor can be used
#ifdef sgBME
  pinMode(GND, OUTPUT);
  digitalWrite(GND, LOW);
  pinMode(POW, OUTPUT);
  digitalWrite(POW, HIGH);
  delay(100);
  if (!bme.begin()) sg.pubDebug("BME280 not found");
#endif
#ifdef sgDHT
  dht.begin();
#endif
#ifdef sgHDC
  hdc.begin(0x40);
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
#elif defined (sgHDC)
  if (!sg.readTemp(hdc)) return;
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
  Serial.print(sg.temp, 2);
  Serial.print("°F, ");
  Serial.print(sg.humid, 2);
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

