# SolarGuardnLib
## v0.8.4 PRE-RELEASE 31-Jul-2018
### Copyright 2018 by David Denney <dragondaud@gmail.com>

Master repository: https://github.com/dragondaud/SolarGuardnLib

### Features
- ESP8266/Arduino platform
- Over-the-Air (OTA) updates from Arduino IDE
- Temperature, Humidity monitoring
- Soil moisture monitoring
- MQTT data logging
- WiFi client
- Pump control with SONOFF/Espurna
- SPIFFS config file

This code is offered "as is" with no warranty, expressed or implied, for any purpose,
and is released to the public domain, while all libraries retain their respective licenses.

See config.h for configurable settings and all includes.

Designed to run on an ESP-12E NodeMCU board with additional hardware,
the sketch examples will monitor soil conditions, ambient temperature, humidity
and atmospheric pressure, then report changes to any MQTT broker.

**Board: NodeMCU 1.0, Freq: 80MHz, Flash: 4M (1M SPIFFS), Speed: 115200, Port: serial or OTA IP**

Some code is based on examples from the ESP8266, ArduinoOTA and other libraries.

Sketch requires ESP8266 library v2.4.1, docs at: https://arduino-esp8266.readthedocs.io/en/2.4.1/

Add this to File->Preferences->Additional Board Manager URLs:
http://arduino.esp8266.com/stable/package_esp8266com_index.json
