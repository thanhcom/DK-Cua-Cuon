
#include <ArduinoJson.h>
//#include <ESP8266WiFi.h>-8266
//#include <FS.h>--8266
#include <SPIFFS.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <DHT12.h>
#include <RCSwitch.h>
DHT12 dht12;
RCSwitch mySwitch = RCSwitch();
int toggleState_1;
unsigned long time1 = 0;
char mqtt_server[40];
char mqtt_port[6];
char api_token[34] = "YOUR_API_TOKEN";
StaticJsonDocument<192> doc;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void callback(char* topic, byte* payload, unsigned int length) {
  String myString = String(topic);
  if (myString == "test/topic") {
    if ((char)payload[0] == '0') {
      digitalWrite(5, HIGH);
      delay(300);
      digitalWrite(5, LOW);
      SendCm("0");
    }
    if ((char)payload[0] == '1') {
      digitalWrite(6, HIGH);
      delay(300);
      digitalWrite(6, LOW);
      SendCm("1");
    }
    if ((char)payload[0] == '2') {
      digitalWrite(7, HIGH);
      delay(300);
      digitalWrite(7, LOW);
      SendCm("2");
    }
    if ((char)payload[0] == '3' || (char)payload[0] == '4') {
      //btn1
      Serial.println("Button 1 : 5592323");
      mySwitch.send(5592323, 24);
    }
    if ((char)payload[0] == '5' || (char)payload[0] == '6') {
      //btn2
      mySwitch.send(5592332, 24);
      Serial.println("Button  2: 5592332");
    }
    if ((char)payload[0] == '7' || (char)payload[0] == '8') {
      //btn3
      mySwitch.send(5592335, 24);
      Serial.println("Button 3: 5592335");
    }
    if ((char)payload[0] == '9' || (char)payload[0] == 'a') {
      //btn4
      mySwitch.send(5592368, 24);
      Serial.println("Button  4: 5592368");
    }
  }
}


WiFiClient espClient;
PubSubClient client(espClient);


void setup() {
  Serial.begin(9600);
  dht12.begin();
  //delay(180000);

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
        DynamicJsonDocument json(1024);
        auto deserializeError = deserializeJson(json, buf.get());
        serializeJson(json, Serial);
        if (!deserializeError) {
#else
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
#endif
          Serial.println("\nparsed json");
          strcpy(mqtt_server, json["mqtt_server"]);
          strcpy(mqtt_port, json["mqtt_port"]);
          strcpy(api_token, json["api_token"]);
        } else {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_api_token("apikey", "API token", api_token, 32);


  //WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_api_token);

  bool res;
  res = wifiManager.autoConnect("Thanh Trang Electronic", "");
  if (!res) {
    ESP.restart();
  }

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(api_token, custom_api_token.getValue());
  Serial.println("The values in the file are: ");
  Serial.println("\tmqtt_server : " + String(mqtt_server));
  Serial.println("\tmqtt_port : " + String(mqtt_port));
  Serial.println("\tapi_token : " + String(api_token));
  if (shouldSaveConfig) {
    Serial.println("saving config");
#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    DynamicJsonDocument json(1024);
#else
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
#endif
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

#if defined(ARDUINOJSON_VERSION_MAJOR) && ARDUINOJSON_VERSION_MAJOR >= 6
    serializeJson(json, Serial);
    serializeJson(json, configFile);
#else
    json.printTo(Serial);
    json.printTo(configFile);
#endif
    configFile.close();
    //end save
  }



  pinMode(5, OUTPUT);
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(8, OUTPUT);
  mySwitch.enableTransmit(4);

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(callback);

  String str = "Arduino start :" + String(random(0xffff), HEX);
  if (client.connect(str.c_str(), "thanhcom", "laodaicaha")) {
    client.setKeepAlive(300);
    client.subscribe("test/topic");
  }
}

void loop() {
  if ((unsigned long)(millis() - time1) > 10000) {
    digitalWrite(8, !digitalRead(8));

    //Json Process
    doc["sensor"] = "8266 Sensor";

    JsonArray data = doc.createNestedArray("data");
    data[0]["temp"] = random(0xff);
    data[1]["humi"] = random(0xff);
    data[2]["rssid"] = random(0xff);

   // data[0]["temp"] = dht12.readTemperature();
   // data[1]["humi"] = dht12.readHumidity();
   // data[2]["rssid"] = WiFi.RSSI();
    String jsonString;
    serializeJson(doc, jsonString);
    client.publish("blynk/sensor", jsonString.c_str());
    //client.publish("blynk/rssid", String(WiFi.RSSI()).c_str());
    //client.publish("blynk/temp", String(dht12.readTemperature()).c_str());
    //client.publish("blynk/humi", String(dht12.readHumidity()).c_str());
    //client.publish("blynk/checkstatus", String(random(0xffff)).c_str());
    time1 = millis();
  }
  if (time1 < 0) {
    time1 = millis();
  }

  //Blynk.run();
  if (WiFi.status() != WL_CONNECTED) {
    delay(50000);
    ESP.restart();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "thanhcom", "laodaicaha")) {
      client.setKeepAlive(300);
      client.subscribe("test/topic");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void SendCm(String str) {
  //client.publish("blynk/cm", str.c_str());
  client.publish("test/topic1", str.c_str());
}
