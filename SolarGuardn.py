#!/usr/bin/env python

#   SolarGuardn AIO uploader V0.9.0 development
#   copyright 2019 by David Denney <dragondaud@gmail.com>
#   distributed under the terms of LGPL https://www.gnu.org/licenses/lgpl.html
#
#   Monitor 'localhost' mqtt server and publish data to Adafruit IO

import time
import sys
import signal
import json
import paho.mqtt.client as mqtt

username = ""               # AIO username
password = ""               # AIO key
myTopic  = "SolarGuardn/#"  # Or set to specific unit "SolarGuardn/SolarGuardn-XXXXXX/#"
sghost   = ""               # unit to publish cmd channel to, automatically set to first unit heard or set here

stripped = lambda s: "".join(i for i in s if 31 < ord(i) < 127)

def myStop(error=None):
    adafruit.loop_stop()
    adafruit.disconnect()
    myClient.loop_stop()
    myClient.disconnect()
    sys.exit(error)

def handler(signum, frame):
    myStop(signum)

def on_connect(client, userdata, flags, rc):
    if rc==0:
        print("Connected to %s:%s" % (client._host, client._port))
        if client._host=="localhost":
            client.subscribe([(myTopic, 0)])
        else:
            client.subscribe([(username + "/f/cmd", 0)])
    else:
        print("Connection refused", rc)
        myStop()

def on_message(client, userdata, msg):
    topic = stripped(msg.topic.decode("ascii", errors="ignore"))
    chan = topic.split("/")[2]
    if chan == "data":
        upload(topic, json.loads(msg.payload))
    elif chan == "cmd" and client._host != "localhost":
        cmd = stripped(msg.payload.decode("ascii", errors="ignore"))
        print topic, cmd, sghost
        if sghost != "":
            myClient.publish(sghost, cmd)

def upload(topic, msg):
    global sghost
    try:
        print topic.split("/")[1],
        if msg["app"] == "SolarGuardn":
            if sghost == "":
                sghost = topic.split("/")[0] + "/" + topic.split("/")[1] + "/cmd"
            print msg["time"] + ": ", msg["temp"], "*F,", msg["humid"], "%RH,", msg["lux"], "lux,", msg["colorTemp"], "*K,", msg["moist"], "moist"
            adafruit.publish(username + "/f/temp", msg["temp"])
            adafruit.publish(username + "/f/humid", msg["humid"])
            adafruit.publish(username + "/f/lux", msg["lux"])
            adafruit.publish(username + "/f/colorTemp", msg["colorTemp"])
            adafruit.publish(username + "/f/moist", msg["moist"])
    except (SystemExit, KeyboardInterrupt):
        myStop()
    except TypeError:
        pass
    except KeyError:
        print json.dumps(msg)
        pass
    except:
        print "Exception", sys.exc_info()[0]
        myStop()

signal.signal(signal.SIGINT, handler)

adafruit = mqtt.Client()
adafruit.username_pw_set(username, password)
adafruit.on_connect = on_connect
adafruit.on_message = on_message
adafruit.connect("io.adafruit.com", 1883, 60)
adafruit.loop_start()

myClient = mqtt.Client()
myClient.on_connect = on_connect
myClient.on_message = on_message
myClient.connect("localhost", 1883, 60)
myClient.loop_start()

while True:
    try:
        time.sleep(1)
    except (SystemExit, KeyboardInterrupt):
        myStop("Interrupt")
    except:
        myStop(sys.exc_info()[0])

