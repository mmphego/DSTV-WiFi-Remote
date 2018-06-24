#include "config.h"


// Setup the ir pin
IRsend irsend(IRPin);
//Type 0 is IR remote APA1616 24 Button Remote
RgbIrLed rgbled(LEDStrip);

// initialise mqtt pubsub client
WiFiClient espClient;
PubSubClient client(espClient);

//WiFiClient kodiclient;

// initialise webserver
ESP8266WebServer httpServer(80);
MDNSResponder mdns;
ESP8266HTTPUpdateServer httpUpdater;

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
//NTPClient timeClient(ntpUDP, "us.pool.ntp.org", 3600, 60000);
// NTPClient timeClient(ntpUDP, ntpServerName, 7200, 60000);

// Initialising Websocket to connect to Sinric.com
WebSocketsClient webSocket;
//WiFiClient client;

WiFiUDP Udp;

// Function declaration
void TxCode(uint16_t irSignal[68]);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setupWebUpdater();
void setupMQTTclient();
void setupWebSockets();
void turnOn(String deviceId);
void turnOff(String deviceId);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);
void WakeOnLan(byte mac[]);
// void checkTimer(String TimerUpdate);
//void kodiRunning();
//void setPage();
//String getPage();
//void fauxmoCallback(uint8_t device_id, const char * device_name, bool state);

void setup() {
  Serial.begin(115200);
  irsend.begin();
  WiFi.config(ip, gateway, subnet, dns);
  WiFi.begin(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);
  // Waiting for Wifi connect
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected. ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //timeClient.begin();
  setupWebSockets();
  setupWebUpdater();
  setupMQTTclient();
}

void loop() {
  httpServer.handleClient();
  if (!client.connected()) {
    reconnect();
    delay(50);
  }
  client.loop();
  webSocket.loop();
  if(isConnected) {
      uint64_t now = millis();
      // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
      if((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
          heartbeatTimestamp = now;
          webSocket.sendTXT("H");
        }
      }
  // timeClient.update();
  // String TimerUpdate = timeClient.getFormattedTime();
  // checkTimer(TimerUpdate);
  }

void setupMQTTclient(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setupWebUpdater(){
  Serial.println("Setting up http updater!");
  client.publish(mqttTopicLog,"[ESP8266-IRremote] Setting up http updater!");
  MDNS.begin(Hostname);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);

  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", Hostname);
  client.publish(mqttTopicLog, "[ESP8266-IRremote] HTTPUpdateServer ready! Open http://Hostname.local/update in your browser\n");
  Serial.println("Ready");
  client.publish(mqttTopicLog, "[ESP8266-IRremote] Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived from topic: [");
  Serial.print(topic);
  Serial.println("]");
  for(unsigned long i = 0; i < length; i++) {
    receivedChar[i] = payload[i];
    }
  String receivedData = receivedChar;
  if (receivedData == "VOL_PLUS") {
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Volume Up");
    for (uint16_t i = 0; i < 2; i++) {
      TxCode(VOL_PLUS);
      delay(10);
      }
    }
  else if (receivedData == "VOL_MINUS") {
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Volume Down");
    for (uint16_t i = 0; i < 2; i++) {
      TxCode(VOL_MINUS);
      delay(10);
      }
    }
  else if (receivedData == "MUTE") {
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Volume Muted");
    TxCode(MUTE);
    delay(10);
  }
  else if (receivedData == "TVOnOff") {
      client.publish(mqttTopic, "[ESP8266-IRremote] Toggle TV On/Off");
      irsend.sendGC(Samsung_power_toggle, 71);
  }
  else if (receivedData == "TVLedOn" || receivedData == "LedOn") {
    client.publish(mqttTopic, "[ESP8266-IRremote] TV Ambient Lights On!!");
    irsend.sendNEC(rgbled.On, freq_strip);
    delay(100);
    irsend.sendNEC(rgbled.Green, freq_strip);
    delay(50);
  }
  else if (receivedData == "TVLedOff" || receivedData == "LedOff") {
    client.publish(mqttTopic, "[ESP8266-IRremote] TV Ambient Lights Off!!");
    irsend.sendNEC(rgbled.Off, freq_strip);
    delay(500);
  }
  else if (receivedData == "CHAN_UP") {
      // Change Channel
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel Up");
      TxCode(P_PLUS);
      delay(100);
      }
  else if (receivedData == "CHAN_DOWN") {
      // Change Channel
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel Down");
      TxCode(P_MINUS);
      delay(500);
      }
  else if (receivedData == "KODISERVER") {
      // Change Channel
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Kodi Server");
      WakeOnLan(kodi_server_mac);
      }
  else if (receivedData.length() == 3) {
    if (receivedData == "161") {
      // Mzanzi Magic 161
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 161");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_6);
      delay(500);
      TxCode(NUM_1);
      delay(500);
      }
    else if (receivedData == "191") {
      // SABC1 191
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 191");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_1);
      delay(500);
      }
    else if (receivedData == "192") {
      // SABC2 192
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 192");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_2);
      delay(500);
      }
    else if (receivedData == "193") {
      // SABC3 193
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 193");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_3);
      delay(500);
      }
    else if (receivedData == "194") {
      // eTV 194
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 194");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_4);
      delay(500);
      }
    else if (receivedData == "403") {
      // eNCA 403
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 403");
      TxCode(NUM_4);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_3);
      delay(500);
      }
    else if (receivedData == "404") {
      // SABC News 404
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 404");
      TxCode(NUM_4);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_4);
      delay(500);
      }
    else if (receivedData == "175") {
      // Food Network 175
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 175");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_7);
      delay(500);
      TxCode(NUM_5);
      delay(500);
      }
    else if (receivedData == "136") {
      // Discovery Family 136
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 136");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_3);
      delay(500);
      TxCode(NUM_6);
      delay(500);
    }
    else if (receivedData == "302") {
      // Boomerang 302
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 302");
      TxCode(NUM_3);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_2);
      delay(500);
    }
    else if (receivedData == "801") {
      // MetroFM
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 801");
      TxCode(NUM_8);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_1);
      delay(500);
    }
    else{
      String msg = "[ESP8266-IRremote] Didnt understand " + receivedData;
      const char* Msg = msg.c_str();
      Serial.println(Msg);
      client.publish(mqttTopicLog, Msg);
    }
  }
  else {
    String msg = "[ESP8266-IRremote] Didnt understand " +  receivedData;
    const char* Msg = msg.c_str();
    Serial.println(Msg);
    client.publish(mqttTopicLog, Msg);
  }
  // Clearing all characters receiver previously.
  for (unsigned long i=0; i<sizeof(receivedChar); ++i ) {
    receivedChar[i] = (char)0;
  }
  Serial.println("");
}

void TxCode(uint16_t irSignal[]) {
  irsend.sendRaw(irSignal, 68, 38);
  delay(500);
  }

// Loop until we're reconnected
void reconnect() {
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect("ESP8266Client")) {
        Serial.println("connected");
        // Once connected, publish an announcement...
        //client.publish("outTopic", "hello world");
        // ... and resubscribe
        client.subscribe(mqttTopic);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
  }

  void WakeOnLan(byte mac[]) {
      byte preamble[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      Udp.beginPacket(broadcast, 9);
      Udp.write(preamble, sizeof preamble);
      for (char i = 0; i < 16; i++)
        Udp.write(mac, sizeof mac);
      Udp.endPacket();
  }

void setupWebSockets(){
  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");
  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);
  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets

}

// deviceId is the ID assgined to your smart-home-device in sinric.com dashboard. Copy it from dashboard and paste it here
void turnOn(String deviceId) {
  if (deviceId == SwitchId) {// Device ID of first device
    client.publish(mqttTopic, "[ESP8266-IRremote-Alexa] Toggle TV On/Off");
    irsend.sendGC(Samsung_power_toggle, 71);
  }
  else if (deviceId == LightId) {// Device ID of second device
    client.publish(mqttTopic, "[ESP8266-IRremote-Alexa] TV LEDs On");
    irsend.sendNEC(rgbled.On, freq_strip);
    delay(100);
    irsend.sendNEC(rgbled.Blue, freq_strip);
    delay(50);
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);
  }
}

void turnOff(String deviceId) {
   if (deviceId == SwitchId) { // Device ID of first device
     client.publish(mqttTopic, "[ESP8266-IRremote-Alexa] Toggle TV On/Off");
     irsend.sendGC(Samsung_power_toggle, 71);
   }
   else if (deviceId == LightId) { // Device ID of second device
     client.publish(mqttTopic, "[ESP8266-IRremote-Alexa] TV LEDs OFF");
     irsend.sendNEC(rgbled.Off, freq_strip);
  }
  else {
     Serial.print("Turn off for unknown device id: ");
     Serial.println(deviceId);
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      isConnected = false;
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      client.publish(mqttTopicLog, "[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
      isConnected = true;
      Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
      Serial.printf("Waiting for commands from sinric.com ...\n");
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);
        // Example payloads

        // For Switch or Light device types
        // {"deviceId": xxxx, "action": "setPowerState", value: "ON"} // https://developer.amazon.com/docs/device-apis/alexa-powercontroller.html

        // For Light device type
        // Look at the light example in github

        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
        String deviceId = json ["deviceId"];
        String action = json ["action"];
        Serial.println(action);

        if(action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if(value == "ON") {
              Serial.print("Turn on device id: ");
              Serial.println(deviceId);
              turnOn(deviceId);
            } else {
              Serial.print("Turn off device id: ");
              Serial.println(deviceId);
              turnOff(deviceId);
            }
        }
        else if (action == "ChangeChannel") {
          String value = json ["value"];
          Serial.print(value);
        }
        else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
            client.publish(mqttTopicLog,"[WSc] received test command from sinric.com");
          }
        }
      break;

    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;

    default:
      Serial.printf("[Wsc] Defaulting");
      break;
  }
}
