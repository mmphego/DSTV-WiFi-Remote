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
NTPClient timeClient(ntpUDP, ntpServerName, 7200, 60000);

// Initialising Websocket to connect to Sinric.com
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
//WiFiClient client;

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
void checkTimer(String TimerUpdate);
//void kodiRunning();
//void setPage();
//String getPage();
//void fauxmoCallback(uint8_t device_id, const char * device_name, bool state);

void setup() {
  Serial.begin(115200);
  irsend.begin();
  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);
  // Waiting for Wifi connect
  while(WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  if(WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  timeClient.begin();
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
  timeClient.update();
  String TimerUpdate = timeClient.getFormattedTime();
  checkTimer(TimerUpdate);
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
  client.publish(mqttTopicLog,"[ESP8266-IRremote] HTTPUpdateServer ready! Open http://Hostname.local/update in your browser\n");
  Serial.println("Ready");
  client.publish(mqttTopicLog,"[ESP8266-IRremote] Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void checkTimer(String TimerUpdate ){
  if (TimerUpdate == "20:00:00"){
    Serial.println("TV LED On!!!");
    client.publish(mqttTopicLog, "[ESP8266-IRremote] TV LED On!!!");
    irsend.sendNEC(rgbled.On, freq_strip);
    delay(500);
  }
  else if (TimerUpdate == "22:00:00"){
    Serial.println("TV LED Off!!!");
    client.publish(mqttTopicLog, "[ESP8266-IRremote] TV LED Off!!!");
    irsend.sendNEC(rgbled.Off, freq_strip);
    delay(500);
  }
  else if (TimerUpdate == "05:00:00"){
    Serial.println("Switch to Metro FM!!!");
    client.publish(mqttTopicLog, "[ESP8266-IRremote] Switch to Metro FM!!!");
    TxCode(NUM_8);
    delay(500);
    TxCode(NUM_0);
    delay(500);
    TxCode(NUM_1);
    delay(500);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived from topic: [");
  Serial.print(topic);
  Serial.println("]");
  for(unsigned long i=0;i<length;i++) {
    receivedChar[i] = payload[i];
    }
  String receivedData = receivedChar;
  if (receivedData == "VOL_PLUS") {
    Serial.println(receivedData);
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Volume Up");
    for (uint16_t i = 0; i < 2; i++) {
      TxCode(VOL_PLUS);
      delay(10);
      }
    }
  else if (receivedData == "VOL_MINUS") {
    Serial.println(receivedData);
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Volume Down");
    for (uint16_t i = 0; i < 2; i++) {
      TxCode(VOL_MINUS);
      delay(10);
      }
    }
  else if (receivedData == "MUTE") {
    Serial.println(receivedData);
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Volume Muted");
    TxCode(MUTE);
    delay(10);
    }

  else if (receivedData == "TVOnOff") {
      Serial.println(receivedData);
      client.publish(mqttTopicTV, "[ESP8266-IRremote] Toggle TV On/Off");
      irsend.sendGC(Samsung_power_toggle, 71);
      delay(50);
      irsend.sendNEC(rgbled.Off, freq_strip);
      }
  else if (receivedData == "TVLedOn") {
    Serial.println(receivedData);
    client.publish(mqttTopicTV, "[ESP8266-IRremote] Time for movie night");
    irsend.sendNEC(rgbled.On, freq_strip);
    delay(100);
    irsend.sendNEC(rgbled.Blue, freq_strip);
    delay(50);
    // switch off the lights and do other stuffs
  }
  else if (receivedData == "TVLedOff") {
    Serial.println(receivedData);
    client.publish(mqttTopicTV, "[ESP8266-IRremote] TV Ambient Lights Off!!");
    irsend.sendNEC(rgbled.Off, freq_strip);
    delay(500);
    // switch off the lights and do other stuffs
  }
  else if (receivedData == "CHAN_UP") {
      // Change Channel
      Serial.println(receivedData);
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel Up");
      TxCode(P_PLUS);
      delay(100);
      }
  else if (receivedData == "CHAN_DOWN") {
      // Change Channel
      Serial.println(receivedData);
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel Down");
      TxCode(P_MINUS);
      delay(500);
      }
  else if (receivedData.length() == 3) {
    if (receivedData == "161") {
      // Mzanzi Magic 161
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
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
      Serial.println(receivedData);
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Channel 801");
      TxCode(NUM_8);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_1);
      delay(500);
    }
    else{
      Serial.println("Error: Access denied");
      client.publish(mqttTopicLog,"[ESP8266-IRremote] Didnt understand");
    }
  }
  else {
    Serial.println("Error: Access denied");
    client.publish(mqttTopicLog,"[ESP8266-IRremote] Didnt understand");
  }
  // Clearing all characters receiver previously.
  for (unsigned long i=0; i<sizeof(receivedChar); ++i ) {
    receivedChar[i] = (char)0;
  }
  Serial.println("");
}

void TxCode(uint16_t irSignal[]) {
  irsend.sendRaw(irSignal, sizeof(irSignal), freq);
  delay(500);
  }

// Loop until we're reconnected
void reconnect() {
    while(WiFiMulti.run() != WL_CONNECTED) {
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
        client.subscribe(mqttTopicDStv);
      } else {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        // Wait 5 seconds before retrying
        delay(5000);
      }
    }
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
    client.publish(mqttTopicTV, "[ESP8266-IRremote-Alexa] Toggle TV On/Off");
    irsend.sendGC(Samsung_power_toggle, 71);
  }
  else if (deviceId == LightId) {// Device ID of second device
    client.publish(mqttTopicTV, "[ESP8266-IRremote-Alexa] TV LEDs On");
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
     client.publish(mqttTopicTV, "[ESP8266-IRremote-Alexa] Toggle TV On/Off");
     irsend.sendGC(Samsung_power_toggle, 71);
   }
   else if (deviceId == LightId) { // Device ID of second device
     client.publish(mqttTopicTV, "[ESP8266-IRremote-Alexa] TV LEDs OFF");
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
