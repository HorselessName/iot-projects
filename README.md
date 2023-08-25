
# IoT Projects for Research and Development

This repository holds multiple projects done for studying and researching.

## Current Projects in the Repository

- Temperature Monitoring
- Automated Toyota ANDON Production software


## About/Deploying the Softwares

### Temperature Monitoring

For the Temperature Monitoring project, you will need a ESP8266 Arduino and a BMP180 sensor.
After getting those hardwares, just compile the project inside the ESP and install it.

It creates a REST API with informations about Temperature and some other things.
Useful in scenarios of `Data Centers`, `Farms`, `Medical Storages`, etc.

The modules you need for this project to compile correctly, are:

- [SparkFun BPM180 Arduino Library](https://github.com/sparkfun/BMP180_Breakout_Arduino_Library)
- [ESP8266 WiFi Library](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)
- [The JSON Serialization Arduino Library](https://arduinojson.org/)
- [The ESP Async Library for creating our REST API](https://github.com/me-no-dev/ESPAsyncWebServer)

### Automated Toyota ANDON Software

For this you will need a ESP8266 and electrical devices for buttons, alerts and signals. Since this goes beyond code, you can take a read at this document that explain briefly on how the ANDON concept works.

It is basically doing a logic of getting electrical signals from physical buttons, and communicating with a external API that handles its requests and the IoT flow of messages using a low-profile network protocol called MQTT, and it's Broker, which is called MQTT Broker.

For this fully to work, you will need:

- A API that handles communication with the MQTT Protocol.
- [ESP8266 WiFi Library](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/readme.html)
  - Important: For this to be functional on ESP8266, we recommend you to install ESP8266 Community on V2.4.0.
- [The Official MQTT Library for using MQTT Protocol messages](https://www.arduino.cc/reference/en/libraries/pubsubclient/)

The REST API part is a bit more complicated. Here are some useful links that can help you understand its architecture a little better, and also help you choose your language/library of choice:

- [HiveMQ - A MQTT Library in Java](https://docs.hivemq.com/hivemq/4.18/rest-api/introduction.html)
- [PahoMQTT - A MQTT Library in Python](https://www.cloudmqtt.com/docs/python.html)

There are others out there, but you need to do a quick search.
