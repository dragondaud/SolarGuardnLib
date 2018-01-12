/*
   SolarGuardn - SoilGuard v0.8.2 PRE-RELEASE 12-Jan-2018
   copyright 2017, 2018 by David M Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  sg.setup();
  // only one temp/humid sensor can be used
#if defined (sgDHT)
  dht.begin();
#elif defined (sgHDC)
  hdc.begin(0x40);
#elif defined (sgBME)
  if (!bme.begin()) sg.pubDebug(time(nullptr), "BME280 not found");
#endif
#ifdef sgTCS
  if (!tcs.begin()) sg.pubDebug(time(nullptr), "TCS34725 not found");
#endif
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(MGND, OUTPUT);          // moisture sensor ground
  digitalWrite(MGND, LOW);
  pinMode(MPOW, OUTPUT);          // moisture sensor power
  digitalWrite(MPOW, LOW);
  delay(1000);  // wait for sensors to stabalize
} // setup

void loop() {
  time_t now = sg.loop();
  if (!now) return;
  sg.ledOn();
  String t = sg.localTime(now);
  String u = sg.upTime(now);
#if defined (sgDHT)
  if (!sg.readDHT(&dht)) return;
#elif defined (sgHDC)
  if (!sg.readHDC(&hdc)) return;
#elif defined (sgBME)
  if (!sg.readBME(&bme)) return;
#endif
#ifdef sgTCS
  if (!sg.readTCS(&tcs)) return;
#endif
  sg.moist = readMoisture(MOIST, MPOW, SNUM, STIM);
  Serial.printf("%s, %d°F, %d%%RH, %d moisture, %s uptime, %d heap \r", \
                t.c_str(), round(sg.temp), round(sg.humid), sg.moist, u.c_str(), sg.heap);
  sg.pubJSON(now);
  sg.ledOff();
} // loop

