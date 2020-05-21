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
4. SDD1306 OLED Disaply (Sink)
5. Elasticsearch to log messages  (Sink)
6. MQTT Client (Source and Sink)
7. HCSR04 Ultrasonic Range Finder (Source)
8. REST Service (Source and Sink)
9. SX1278 LoRa Transceiver (Source and Sink)
10. Camera (Source)
11. IR Motion (Source)

I will also think about adding triggers, so thing slike an IR montion sensor can trigger a reading of a source. This can also be used to save battery power by waking up once to synchronise a set of work and goign back to sleep.

## Getting Started

This uses esp-idf V4.0 so follow the instructions here:
[https://docs.espressif.com/projects/esp-idf/en/stable/get-started/](https://docs.espressif.com/projects/esp-idf/en/stable/get-started/)
The link above contains instructions on getting the IDF, installing the toolchain, and setting the enviroment variables.
Once all that is done, you can use:
\>idf\.py flash && idf\.py monitor
In the usual way to compile, flash and monitor.

I use VSCode in Windows 10, but connected to a WSL Ubuntu 18.04, and all the command line stuff happens in the Ubutnu WSL.
Some of the code is structured in clunky ways to satify VSCode and esp compiler so neither give warnings.

<br>
<br>
