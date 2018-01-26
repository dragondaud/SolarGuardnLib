/*
   SolarGuardn - TankGuard v0.8.2 PRE-RELEASE
   copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  //sg.location = "81007"; // over-ride geoIP
  sg.begin(SDA, SCL);
  pinMode(BUILTIN_LED, OUTPUT);
#ifdef sgRANGE
  pinMode(TRIG, OUTPUT);
  digitalWrite(TRIG, LOW);
  pinMode(ECHO, INPUT);
#endif
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
  delay(1000);  // wait for sensors to stabalize
} // setup

void loop() {
  if (!sg.handle()) return;
  sg.ledOn();
  String t = sg.localTime();
  String u = sg.upTime();
#ifdef sgRANGE
  if (!sg.getDist(TRIG, ECHO)) return;
#endif
#if defined (sgDHT)
  if (!sg.readTemp(dht)) return;
#elif defined (sgHDC)
  if (!sg.readTemp(hdc)) return;
#elif defined (sgBME)
  if (!sg.readTemp(bme)) return;
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

