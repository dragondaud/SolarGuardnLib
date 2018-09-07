/*
   SolarGuardn - AirGuard v0.8.4 PRE-RELEASE
   copyright 2018 by David M Denney <dragondaud@gmail.com>
   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
*/

#include "Config.h"

void setup() {
  Serial.begin(115200);
  //Serial.setDebugOutput(true);
  sg.begin(SDA, SCL);
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
#ifdef sgAIR
  if (!ccs.begin()) sg.pubDebug("CCS811 not found");
  else {
    delay(1000);
    ccs.calculateTemperature();
    while (!ccs.available());
    delay(1000);
    ccs.readData();
    ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);
    delay(10000);
  }
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
#ifdef sgAIR
  if (!sg.readCCS(ccs)) return;
#endif
  Serial.print(t);
  Serial.print(", ");
  Serial.print(round(sg.temp));
  Serial.print("°F, ");
  Serial.print(round(sg.humid));
  Serial.print("%RH, ");
  Serial.print(sg.eCO2);
  Serial.print(" eCO2, ");
  Serial.print(sg.TVOC);
  Serial.print(" ppm, ");
  Serial.print(u);
  Serial.print(" uptime, ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" heap");
  sg.pubJSON();
  sg.ledOff();
} // loop
