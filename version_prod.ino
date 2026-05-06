#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <DHT12.h>
#include <RCSwitch.h>
DHT12 dht12;
RCSwitch mySwitch = RCSwitch();
uint8_t toggleState_1;
unsigned long time1 = 0;
char mqtt_server[40];
char mqtt_port[6];
char api_token[34] = "YOUR_API_TOKEN";
char time_delay[6] = "100";

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}



void callback(char* topic, byte* payload, unsigned int length) {
  String myString = String(topic);
  if (myString == "blynk/cmactive") {
    if ((char)payload[0] == '0') {
      digitalWrite(D5, HIGH);
      delay(300);
      digitalWrite(D5, LOW);
      SendCm("0");
    }
    if ((char)payload[0] == '1') {
      digitalWrite(D6, HIGH);
      delay(300);
      digitalWrite(D6, LOW);
      SendCm("1");
    }
    if ((char)payload[0] == '2') {
      digitalWrite(D7, HIGH);
      delay(300);
      digitalWrite(D7, LOW);
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
  randomSeed(micros());

  dht12.begin();

  // =========================
  // Mount SPIFFS + đọc config
  // =========================
  if (SPIFFS.begin()) {
    Serial.println(F("SPIFFS mounted"));

    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open("/config.json", "r");

      if (configFile) {
        size_t size = configFile.size();

        if (size > 0 && size < 1024) {
          std::unique_ptr<char[]> buf(new char[size + 1]);
          configFile.readBytes(buf.get(), size);
          buf[size] = '\0';

          DynamicJsonDocument json(512);
          DeserializationError err = deserializeJson(json, buf.get());

          if (!err) {
            strlcpy(mqtt_server, json["mqtt_server"] | "", sizeof(mqtt_server));
            strlcpy(mqtt_port, json["mqtt_port"] | "", sizeof(mqtt_port));
            strlcpy(api_token, json["api_token"] | "", sizeof(api_token));
            strlcpy(time_delay, json["time_delay"] | "", sizeof(time_delay));

            Serial.println(F("Config loaded"));
          } else {
            Serial.println(F("Failed to parse config"));
          }
        }

        configFile.close();
      }
    }
  } else {
    Serial.println(F("SPIFFS mount failed"));
  }

  // =========================
  // WiFiManager
  // =========================
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, sizeof(mqtt_server));
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, sizeof(mqtt_port));
  WiFiManagerParameter custom_api_token("apikey", "API token", api_token, sizeof(api_token));
  WiFiManagerParameter custom_api_time("time", "time delay", time_delay, sizeof(time_delay));

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_api_token);
  wifiManager.addParameter(&custom_api_time);

  wifiManager.setConnectRetries(5);
  wifiManager.setConnectTimeout(30);
  wifiManager.setConfigPortalTimeout(180);

  if (!wifiManager.autoConnect("Thanh Trang Electronic", "")) {
    ESP.restart();
  }

  // =========================
  // Lấy config từ portal
  // =========================
  strlcpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server));
  strlcpy(mqtt_port, custom_mqtt_port.getValue(), sizeof(mqtt_port));
  strlcpy(api_token, custom_api_token.getValue(), sizeof(api_token));
  strlcpy(time_delay, custom_api_time.getValue(), sizeof(time_delay));

  Serial.println(F("Current config:"));
  Serial.printf("mqtt_server : %s\n", mqtt_server);
  Serial.printf("mqtt_port   : %s\n", mqtt_port);
  Serial.printf("api_token   : %s\n", api_token);
  Serial.printf("time_delay  : %s\n", time_delay);

  // =========================
  // Save config nếu cần
  // =========================
  if (shouldSaveConfig) {
    DynamicJsonDocument json(512);

    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["api_token"] = api_token;
    json["time_delay"] = time_delay;

    File configFile = SPIFFS.open("/config.json", "w");

    if (configFile) {
      serializeJson(json, configFile);
      configFile.close();
      Serial.println(F("Config saved"));
    } else {
      Serial.println(F("Failed to save config"));
    }
  }

  // =========================
  // GPIO
  // =========================
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);

  mySwitch.enableTransmit(D8);

  // =========================
  // MQTT
  // =========================
  int port = atoi(mqtt_port);
  if (port <= 0) {
    port = 1883;
  }

  client.setServer(mqtt_server, port);
  client.setCallback(callback);

  reconnect();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(100);
    return;
  }
  if (!client.connected()) {
    reconnect();
    return;
  }
  client.loop();

  if ((unsigned long)(millis() - time1) >= 10000) {
    digitalWrite(D4, !digitalRead(D4));
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", WiFi.RSSI());
    client.publish("blynk/rssid", buf);
    dtostrf(dht12.getTemperature(), 0, 2, buf);
    client.publish("blynk/temp", buf);
    dtostrf(dht12.getHumidity(), 0, 2, buf);
    client.publish("blynk/humi", buf);
    snprintf(buf, sizeof(buf), "%u", random(0xffff));
    client.publish("blynk/checkstatus", buf);
    time1 = millis();
  }
}

void reconnect() {
  static unsigned long lastAttempt = 0;
  if (client.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  unsigned long now = millis();
  if (now - lastAttempt < 5000) return;
  lastAttempt = now;
  char clientId[32];
  snprintf(clientId, sizeof(clientId), "ESP8266Client-%04X", random(0xFFFF));
  if (client.connect(clientId, "thanhcom", "laodaicaha")) {
    client.setKeepAlive(300);
    client.subscribe("blynk/cmactive");
  }
}

void SendCm(const char* msg) {
  client.publish("blynk/cm", msg);
}
