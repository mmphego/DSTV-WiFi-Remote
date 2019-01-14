# esp8266 based DSTV A6 IRremote
[![Build Status](https://travis-ci.com/mmphego/DSTV-WiFi-Remote.svg?branch=master)](https://travis-ci.com/mmphego/DSTV-WiFi-Remote)

Control your DSTV and TV(Samsung) via WiFi or Amazon Alexa with an esp8266.

# Circuit
![Circuit](images/Circuit.png)

<!---
# Control
I resorted to using [MQTT Dash](https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=en) for control purposes.

![MQTT Dash Selection](images/mqttdash2.png)

![MQTT Dash](images/mqttdash1.png)

# Future Development
Integrate to my [Node-Red Dashboard](https://nodered.org/) running on a network media server.
-->

## Usage

- **Help**

```bash
make help
```

- **Install libraries**

```bash
make install
```

- **Compiling Clang code**
```bash
make build
```

- **Upload bin to device**
```bash
make upload
```

## Feedback

Feel free to fork it or send me PR to improve it.
