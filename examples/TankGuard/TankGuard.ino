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
  // only one temp/humid sensor can be used
#if defined (sgDHT)
  dht.begin();
#elif defined (sgHDC)
  hdc.begin(0x40);
#elif defined (sgBME)
  if (!bme.begin()) sg.pubDebug(time(nullptr), "BME280 not found");
#endif
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
#ifdef sgRANGE
  if (!sg.getDist(TRIG, ECHO)) return;
#endif
  Serial.printf("%s, %d°F, %d%%RH, %d.%02d inHg, %4d mm, %s uptime, %d heap \r", \
                t.c_str(), round(sg.temp), round(sg.humid), sg.range, u.c_str(), sg.heap);
  sg.pubJSON(now);
  sg.ledOff();
} // loop

