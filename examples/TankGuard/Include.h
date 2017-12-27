#include <ESP8266WiFi.h>          // Arduino IDE ESP8266 from https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>    // included
#include <ESP8266mDNS.h>          // included
#include <ArduinoOTA.h>           // included
#include <time.h>                 // included

#include <ArduinoJson.h>          // install ArduinoJson using library manager, https://github.com/bblanchon/ArduinoJson/
#include <EspSaveCrash.h>         // install EspSaveCrash using library manager, https://github.com/krzychb/EspSaveCrash
#include <PubSubClient.h>         // install PubSubClient using library manager, https://github.com/knolleary/pubsubclient

#include <Wire.h>                 // included
#include <BME280I2C.h>            // install BME280 using library manager, https://github.com/finitespace/BME280

