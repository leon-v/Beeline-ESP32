# ESP32 Modular IoT Firmware

This is a codebase i have developed to make it easier for myself to create sensor networks that can be integrated with various components.

Its a simple OS layer above esp-idf / Freertos that provides a universal messaging format so messages can be routed to and from devices and protocals like MQTT, LoRa, Elasticsearch can be disaplyed on an OLED.

I am currently looking into writing more documentation, so please let me know how i can improove this documentation.

## Enviroment
I use Ubuntu shell in Windows 10.

Setup Instructions:

`git clone https://github.com/leon-v/Beeline-ESP32.git`
`cd Beeline-ESP32`
`./install.sh`

inshall.sh will install dependencies and setup esp-idf.
Once completed successfully, run:

`./idf.sh menuconfig` where you can disable components that you don't need.
then

`./idf.sh flash 5 && ./idf.sh monitor 5` To compile, flash and monitor the console log.



## Components

### datetime / Date Time
  Keeps the system up to date using an NTP server. Requires WiFi
  
### device / Device
  Provides a unique name for the sensor / router

### die_hall / Die Hall
  Provides measurments talken by the integrated hall effect sensor on the ESP32
  
### die_temperature / Die Temp
  Provides measurments talken by the integrated temperature sensor on the ESP32
  
### display / Disaply
  Parses a template ro disaply onto a SSD1603 OLED disaply and reaplces markdown for the sensor message values so you can disaply your data on screen.
  
### elastic / Elastic
  Provesed integration with Elasticsearch to create documents on the Elasticserrch server. Can be used for graphing, logging and machine learning data analsys.
  
### hcsr04 / HCSR04
  Provides messaging from an HCSR04 module to meansure distances ultrasonically.
  
### http_server / HTTP Server
  Provides configuration and statistc information to the user.

### mqtt_connection / MQTT Client
  Provides two way messaging with an MQTT server.

### radio / Radio
  Provides two way messaging using an SX1278. AES encryption available.

### wake_timer / Wake Timer
  Provides timed intervals that other components can be triggered from so that the IC does not have to wake up at various intervals and all the wakup events are syncronised.
  
### wifi / WiFi
  Provides WiFi connectivity. Will provide an open access oint when GPIO0 / PRG pin is triggered imediately after reboot to give access to all configuration data.
 
