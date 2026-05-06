#include <FS.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <DHT12.h>
#include <RCSwitch.h>

DHT12 dht12;
RCSwitch mySwitch = RCSwitch();

WiFiClient espClient;
PubSubClient client(espClient);

unsigned long time1 = 0;
bool shouldSaveConfig = false;

char mqtt_server[40] = "thanhcom1989.ddns.net";
char mqtt_port[6] = "1882";
char api_token[34] = "YOUR_API_TOKEN";
char time_delay[6] = "10";

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void SendCm(const char* msg) {
  if (client.connected()) {
    client.publish("blynk/cm", msg);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "blynk/cmactive") != 0 || length == 0) {
    return;
  }

  char cmd = (char)payload[0];

  switch (cmd) {
    case '0':
      digitalWrite(D5, HIGH);
      delay(300);
      digitalWrite(D5, LOW);
      SendCm("0");
      break;

    case '1':
      digitalWrite(D6, HIGH);
      delay(300);
      digitalWrite(D6, LOW);
      SendCm("1");
      break;

    case '2':
      digitalWrite(D7, HIGH);
      delay(300);
      digitalWrite(D7, LOW);
      SendCm("2");
      break;

    case '3':
    case '4':
      mySwitch.send(5592323, 24);
      break;

    case '5':
    case '6':
      mySwitch.send(5592332, 24);
      break;

    case '7':
    case '8':
      mySwitch.send(5592335, 24);
      break;

    case '9':
    case 'a':
      mySwitch.send(5592368, 24);
      break;
  }
}

void loadConfig() {
  if (!SPIFFS.begin()) {
    Serial.println(F("SPIFFS mount failed"));
    return;
  }

  if (!SPIFFS.exists("/config.json")) {
    return;
  }

  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    return;
  }

  size_t size = configFile.size();

  if (size == 0 || size > 512) {
    configFile.close();
    return;
  }

  std::unique_ptr<char[]> buf(new char[size + 1]);
  configFile.readBytes(buf.get(), size);
  buf[size] = '\0';
  configFile.close();

  DynamicJsonDocument json(512);
  DeserializationError err = deserializeJson(json, buf.get());

  if (err) {
    Serial.println(F("Config parse failed"));
    return;
  }

  strlcpy(mqtt_server, json["mqtt_server"] | mqtt_server, sizeof(mqtt_server));
  strlcpy(mqtt_port, json["mqtt_port"] | mqtt_port, sizeof(mqtt_port));
  strlcpy(api_token, json["api_token"] | api_token, sizeof(api_token));
  strlcpy(time_delay, json["time_delay"] | time_delay, sizeof(time_delay));
}

void saveConfig() {
  DynamicJsonDocument json(512);

  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["api_token"] = api_token;
  json["time_delay"] = time_delay;

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    return;
  }

  serializeJson(json, configFile);
  configFile.close();
}

void setupWifi() {
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

  strlcpy(mqtt_server, custom_mqtt_server.getValue(), sizeof(mqtt_server));
  strlcpy(mqtt_port, custom_mqtt_port.getValue(), sizeof(mqtt_port));
  strlcpy(api_token, custom_api_token.getValue(), sizeof(api_token));
  strlcpy(time_delay, custom_api_time.getValue(), sizeof(time_delay));

  if (shouldSaveConfig) {
    saveConfig();
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

void publishSensor() {
  char buf[32];

  snprintf(buf, sizeof(buf), "%d", WiFi.RSSI());
  client.publish("blynk/rssid", buf);

  dtostrf(dht12.getTemperature(), 0, 2, buf);
  client.publish("blynk/temp", buf);

  dtostrf(dht12.getHumidity(), 0, 2, buf);
  client.publish("blynk/humi", buf);

  snprintf(buf, sizeof(buf), "%u", random(0xFFFF));
  client.publish("blynk/checkstatus", buf);
}

void setup() {
  Serial.begin(9600);
  randomSeed(micros());

  dht12.begin();

  loadConfig();
  setupWifi();

  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);

  mySwitch.enableTransmit(D8);

  int port = atoi(mqtt_port);
  if (port <= 0) port = 1883;

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

  unsigned long interval = (unsigned long)atoi(time_delay) * 1000UL;
  if (interval == 0) interval = 10000;

  if ((unsigned long)(millis() - time1) >= interval) {
    digitalWrite(D4, !digitalRead(D4));
    publishSensor();
    time1 = millis();
  }
}
