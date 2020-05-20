# Beeline ESP32 IOT Messaging Platform

## End Goal

The goal of this project is a universal, low power multi-purpose messaging platform.

## Project Brief

It is a platform to make it simple to develop sensor nodes from temperature sensors to cameras.
The routing is handeled by a messaging bus that is handeled internally, All messages are in JSON.
Its based around modules which can be message input and/or outputs. The web interface will allow you to choose where messages go.
E.g. A temperature sensor in the field may send its data to a LoRa radio, and another device may have its input LoRa radio set to send to an MQTT server.

The inital modules i will be devloping are:

1. Die Temperature sensor (Source)
2. Die Hall Effect Sensor (Source)
3. ADCs, with 1 reserved for battery level (Source)
4. Elasticsearch to log messages  (Sink)
5. MQTT Client (Source and Sink)
6. HCSR04 Ultrasonic Range Finder (Source)
7. REST Service (Source and Sink)
8. SX1278 LoRa Transceiver (Source and Sink)
9. Camera (Source)
10. IR Motion (Source)

I will also think about adding triggers, so thing slike an IR montion sensor can trigger a reading of a source. This can also be used to save battery power by waking up once to synchronise a set of work and goign back to sleep.
