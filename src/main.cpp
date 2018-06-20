
#include "config.h"

// Setup the ir pin
IRsend irsend(IRPin);
//Type 0 is IR remote APA1616 24 Button Remote
RgbIrLed rgbled(LEDStrip);

// initialise mqtt pubsub client
WiFiClient espClient;
PubSubClient client(espClient);

WiFiClient kodiclient;

// initialise webserver
ESP8266WebServer httpServer(80);
MDNSResponder mdns;
ESP8266HTTPUpdateServer httpUpdater;

WiFiUDP ntpUDP;

// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
//NTPClient timeClient(ntpUDP, "us.pool.ntp.org", 3600, 60000);
NTPClient timeClient(ntpUDP, "ntp1.meraka.csir.co.za", 7200, 60000);

fauxmoESP fauxmo;

// Function declaration
void TxCode(uint16_t irSignal[68]);
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void setupWebUpdater();
void setupMQTTclient();
void kodiRunning();
void setPage();
String getPage();
void fauxmoCallback(uint8_t device_id, const char * device_name, bool state);

void setup() {
  irsend.begin();
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Static IP Setup Info Here...
  WiFi.config(ip, dns, gateway, subnet);
  //WiFi.mode(WIFI_STA);
  // Start Server
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Starting UDP");
  timeClient.begin();

  setupWebUpdater();
  setupMQTTclient();
  setPage();

  // -----------------------------------------------------------------------------
  // Alexa Device Names
  // -----------------------------------------------------------------------------
  // Fauxmo
  fauxmo.addDevice("TV");
  fauxmo.addDevice("Youtube");
  fauxmo.addDevice("Movie Time");
  fauxmo.onMessage(fauxmoCallback);
}

void setupMQTTclient(){
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setupWebUpdater(){
  Serial.println("Setting up http updater!");
  MDNS.begin(hostname);
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  MDNS.addService("http", "tcp", 80);

  Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", hostname);
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
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
    for (uint16_t i = 0; i < 2; i++) {
      TxCode(VOL_PLUS);
      delay(10);
      }
    }
  else if (receivedData == "VOL_MINUS") {
    Serial.println(receivedData);
    for (uint16_t i = 0; i < 2; i++) {
      TxCode(VOL_MINUS);
      delay(10);
      }
    }
  else if (receivedData == "MUTE") {
    Serial.println(receivedData);
    TxCode(MUTE);
    delay(10);
    }
/*  else if ((receivedData == "PP") || (receivedData == "PAUSE")) {
    //Serial.println(receivedData);
    //TxCode(PLAY_PAUSE);
    delay(10);
  }*/
  else if (receivedData == "TVOnOff") {
      Serial.println(receivedData);
      client.publish(mqttTopicTV, "Toggle TV On/Off");
      irsend.sendGC(Samsung_power_toggle, 71);
      delay(50);
      irsend.sendNEC(rgbled.Off, freq_strip);
      }
  else if (receivedData == "TVLedOn") {
    Serial.println(receivedData);
    client.publish(mqttTopicTV, "Time for movie night");
    irsend.sendNEC(rgbled.On, freq_strip);
    delay(100);
    irsend.sendNEC(rgbled.Blue, freq_strip);
    delay(50);
    // switch off the lights and do other stuffs
  }
  else if (receivedData == "TVLedOff") {
    Serial.println(receivedData);
    client.publish(mqttTopicTV, "No movie night");
    irsend.sendNEC(rgbled.Off, freq_strip);
    delay(500);
    // switch off the lights and do other stuffs
  }
  else if (receivedData == "CHAN_UP") {
      // Change Channel
      Serial.println("TX IR Signal");
      TxCode(P_PLUS);
      delay(100);
      }
  else if (receivedData == "CHAN_DOWN") {
      // Change Channel
      Serial.println("TX IR Signal");
      TxCode(P_MINUS);
      delay(500);
      }
  else if (receivedData.length() == 3) {
    if (receivedData == "161") {
      // Mzanzi Magic 161
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_6);
      delay(500);
      TxCode(NUM_1);
      delay(500);
      }
    else if (receivedData == "191") {
      // SABC1 191
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_1);
      delay(500);
      }
    else if (receivedData == "192") {
      // SABC2 192
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_2);
      delay(500);
      }
    else if (receivedData == "193") {
      // SABC3 193
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_3);
      delay(500);
      }
    else if (receivedData == "194") {
      // eTV 194
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_9);
      delay(500);
      TxCode(NUM_4);
      delay(500);
      }
    else if (receivedData == "403") {
      // eNCA 403
      Serial.println("TX IR Signal");
      TxCode(NUM_4);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_3);
      delay(500);
      }
    else if (receivedData == "175") {
      // Food Network 175
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_7);
      delay(500);
      TxCode(NUM_5);
      delay(500);
      }
    else if (receivedData == "136") {
      // Discovery Family 136
      Serial.println("TX IR Signal");
      TxCode(NUM_1);
      delay(500);
      TxCode(NUM_3);
      delay(500);
      TxCode(NUM_6);
      delay(500);
      }
    else if (receivedData == "302") {
      // Boomerang 302
      Serial.println("TX IR Signal");
      TxCode(NUM_3);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_2);
      delay(500);
      }
    else if (receivedData == "801") {
      // MetroFM
      Serial.println("TX IR Signal");
      TxCode(NUM_8);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_1);
      delay(500);
      }
    else Serial.println("Error: Access denied");
    }
  else Serial.println("Error: Access denied");
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
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
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
/*
// FOr KODI
void kodiRunning() {
  if (kodiclient.connect(xbmchost, 8080)){
    kodiclient.setTimeout(1000);
    kodiclient.print(F("GET /jsonrpc?request={\"jsonrpc\":\"2.0\",\"method\":\"Player.GetActivePlayers\", \"id\":1} HTTP/1.1"));
    kodiclient.println(F("Host: XBMC"));
    kodiclient.println(F("Connection: close"));
    kodiclient.println();
    delay(50);
    char status[32] = {0};
    kodiclient.readBytesUntil('\r', status, sizeof(status));
    if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
      Serial.print(F("Unexpected response: "));
      Serial.println(status);
    }

    // Skip HTTP headers
    char endOfHeaders[] = "\r\n\r\n";
    if (!kodiclient.find(endOfHeaders)) {
      Serial.println(F("Invalid response"));
      return;
    }
    // Allocate JsonBuffer
    // Use arduinojson.org/assistant to compute the capacity.
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonBuffer jsonBuffer(capacity);
    // Parse JSON object
    JsonObject& root = jsonBuffer.parseObject(kodiclient);
    if (!root.success()) {
      Serial.println(F("Parsing failed!"));
      return;
      }

    // Debugging
    //while(kodiclient.available()){
      //String line = kodiclient.readString);
    //  String line = kodiclient.readString();
    //  Serial.print(line);
    //}
    if(root["result"].as<String>() != "[]"){
      Serial.println("I guess it is Movie night");
      irsend.sendNEC(rgbled.On, freq_strip);
      delay(10);
      irsend.sendNEC(rgbled.Blue, freq_strip);
      running = true;
      stopped = false;
    }
    Serial.println("Closing Kodi Client connection");
    kodiclient.stop();
  }
  else {
    kodiclient.stop();
  }
}
*/

void setPage() {
    if (mdns.begin("esp8266", WiFi.localIP())) {
      Serial.println("MDNS responder started");
      Serial.println(WiFi.localIP());
    }

    httpServer.on("/", [](){
      httpServer.send(200, "text/html", getPage());
    });
    httpServer.on("/TVOnOff", [](){
      irsend.sendGC(Samsung_power_toggle, 71);
      Serial.printf("TV On/Off\n");
      httpServer.send(200, "text/html", getPage());
    });
    httpServer.on("/TVLedOn", [](){
      irsend.sendNEC(rgbled.On, freq_strip);
      Serial.printf("LED Strip On");
      httpServer.send(200, "text/html", getPage());
    });
    httpServer.on("/TVLedOff", [](){
      irsend.sendNEC(rgbled.Off, freq_strip);
      Serial.printf("LED Strip Off");
      httpServer.send(200, "text/html", getPage());
    });
    httpServer.on("/5", [](){
      httpServer.send(200, "text/html", getPage());
    });
      httpServer.on("/0", [](){
      httpServer.send(200, "text/html", getPage());
    });
     httpServer.on("/A", [](){
      httpServer.send(200, "text/html", getPage());
    });
      httpServer.on("/U", [](){
      httpServer.send(200, "text/html", getPage());
    });
      httpServer.on("/D", [](){
      httpServer.send(200, "text/html", getPage());
    });
    httpServer.on("/Modo", [](){
      httpServer.send(200, "text/html", getPage());
    });
     httpServer.on("/Day", [](){
      httpServer.send(200, "text/html", getPage());
    });
     httpServer.on("/update", [](){
      httpServer.send(200, "text/html", getPage());
    });

    httpUpdater.setup(&httpServer);
    httpServer.begin();
    Serial.println("HTTP server started");
    // we are connected
    Serial.println();
}

String getPage() {
  String webPage = "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css' integrity=' sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u' crossorigin='anonymous'>";
  webPage += "<script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js' integrity='sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa' crossorigin='anonymous'></script>";
  webPage += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  webPage += "<title>Living Room</title><link rel='icon' mask href='https://goo.gl/xhBrd8'>";
  webPage += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'>";
  webPage += "<link rel='stylesheet' href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css'>";
  webPage += "<link href='https://fonts.googleapis.com/icon?family=Material+Icons' rel='stylesheet'>";
  webPage += "<style>.bt{width:85%;}</style>";
  webPage += "<style>.btx{width:15%;}.bta{width:33.3%;}.full{width:100%;}.half{width:50%;}</style>";
  webPage += "<div class='container-fluid'><center>";
  webPage += "<a href='TVOnOff'><button class='btn btn-primary bta'><h2>TV On/Off </i></h2></button></a>";
  webPage += "<a href='TVLedOn'><button class='btn btn-primary bta'><h2>Movie Nite </i></h2></button></a>";
  webPage += "<a href='TVLedOff'><button class='btn btn-primary bta'><h2>No Movie Nite</i></h2></button></a>";
  /*
  webPage += "<a href='0'><button class='btn btn-primary bta'><h2>OFF </h2></button></a>";
  webPage += "<a href='3'><button class='btn btn-primary bta'><h2>3</h2></button></a>";
  webPage += "<a href='5'><button class='btn btn-primary bta'><h2>5 </h2></button></a>";
  webPage += "<a href='0'><button class='btn btn-primary bta'><h2>OFF </h2></button></a>";
  webPage += "<a href='3'><button class='btn btn-primary bta'><h2>3</h2></button></a>";
  webPage += "<a href='5'><button class='btn btn-primary bta'><h2>5 </h2></button></a>";
  webPage += "<a href='3'><button class='btn btn-primary bta'><h2>3</h2></button></a>";
  webPage += "<a href='5'><button class='btn btn-primary bta'><h2>5 </h2></button></a>";
  webPage += "<a href='0'><button class='btn btn-primary bta'><h2>OFF </h2></button></a>";
  webPage += "<a href='3'><button class='btn btn-primary bta'><h2>3</h2></button></a>";
  webPage += "<a href='5'><button class='btn btn-primary bta'><h2>5 </h2></button></a>";
  webPage += "<a href='0'><button class='btn btn-primary bta'><h2>OFF </h2></button></a>";
  */
  webPage += "<a href='update'><button class='btn btn-danger bta'><h2>update</h2></button></a>";
  webPage += "</center></div>";
  return webPage;
}

  // -----------------------------------------------------------------------------
  // Alexa Operation Calls
  // -----------------------------------------------------------------------------

  void fauxmoCallback(uint8_t device_id, const char * device_name, bool state) {
    Serial.printf("[MAIN] %s state: %s\n", device_name, state ? "ON" : "OFF");

    if ( (strcmp(device_name, "TV") == 0) ) {
      if (state) {
        irsend.sendGC(Samsung_power_toggle, 71);
      } else {
        irsend.sendGC(Samsung_power_toggle, 71);
      }
    }
/*
    if ( (strcmp(device_name, "Youtube") == 0) ) {
      // adjust the relay immediately!
      if (state) {
          //
      } else {
        //
      }
    }*/

    if ( (strcmp(device_name, "Movie Nite") == 0) ) {
      // adjust the relay immediately!
      if (state) {
        client.publish(mqttTopicTV, "Time for movie night");
        irsend.sendNEC(rgbled.On, freq_strip);
        delay(100);
        irsend.sendNEC(rgbled.Green, freq_strip);
      } else {
        irsend.sendNEC(rgbled.Off, freq_strip);
      }
    }
}


// ----------------------------------------------------------------------------
void loop() {
    httpServer.handleClient();
    fauxmo.handle();
    timeClient.update();
    if (!client.connected()) {
      reconnect();
      delay(50);
    }
    client.loop();

    if (timeClient.getFormattedTime() == "20:00:00"){
      Serial.println("TV LED On!!!");
      irsend.sendNEC(rgbled.On, freq_strip);
      delay(500);
    }
    else if (timeClient.getFormattedTime() == "22:00:00"){
      Serial.println("TV LED Off!!!");
      irsend.sendNEC(rgbled.Off, freq_strip);
      delay(500);
    }
    else if (timeClient.getFormattedTime() == "05:00:00"){
      Serial.println("Switch to Metro FM!!!");
      TxCode(NUM_8);
      delay(500);
      TxCode(NUM_0);
      delay(500);
      TxCode(NUM_1);
      delay(500);
    }
  }
