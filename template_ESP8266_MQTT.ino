#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <ArduinoJson.h>

WiFiClient espClient;
PubSubClient client(espClient);

char mqtt_server[40] = "192.168.1.10";
char mqtt_port[6] = "1883";
char mqtt_user[20] = "user";
char mqtt_pass[20] = "pass";

unsigned long lastWifiCheck = 0;
unsigned long lastMqttReconnect = 0;
unsigned long lastSensorSend = 0;

bool shouldSaveConfig = false;

void saveConfigCallback() {
  shouldSaveConfig = true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  Serial.print("Topic: ");
  Serial.println(topic);

}

void loadConfig() {

  if (!SPIFFS.begin()) return;

  if (!SPIFFS.exists("/config.json")) return;

  File file = SPIFFS.open("/config.json", "r");

  DynamicJsonDocument doc(512);
  deserializeJson(doc, file);

  strcpy(mqtt_server, doc["mqtt_server"]);
  strcpy(mqtt_port, doc["mqtt_port"]);
  strcpy(mqtt_user, doc["mqtt_user"]);
  strcpy(mqtt_pass, doc["mqtt_pass"]);

  file.close();
}

void saveConfig() {

  DynamicJsonDocument doc(512);

  doc["mqtt_server"] = mqtt_server;
  doc["mqtt_port"] = mqtt_port;
  doc["mqtt_user"] = mqtt_user;
  doc["mqtt_pass"] = mqtt_pass;

  File file = SPIFFS.open("/config.json", "w");

  serializeJson(doc, file);

  file.close();
}

void connectMQTT() {

  String clientId = "esp8266-";
  clientId += String(ESP.getChipId());

  if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {

    Serial.println("MQTT connected");
    client.subscribe("device/cmd");

  } else {

    Serial.print("MQTT error: ");
    Serial.println(client.state());

  }
}

void setup() {

  Serial.begin(115200);

  loadConfig();

  WiFiManager wm;

  WiFiManagerParameter custom_mqtt_server("server","MQTT server",mqtt_server,40);
  WiFiManagerParameter custom_mqtt_port("port","MQTT port",mqtt_port,6);
  WiFiManagerParameter custom_mqtt_user("user","MQTT user",mqtt_user,20);
  WiFiManagerParameter custom_mqtt_pass("pass","MQTT pass",mqtt_pass,20);

  wm.setSaveConfigCallback(saveConfigCallback);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);

  bool res = wm.autoConnect("ESP8266_SETUP");

  if (!res) {
    ESP.restart();
  }

  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());

  if (shouldSaveConfig) saveConfig();

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  client.setServer(mqtt_server, atoi(mqtt_port));
  client.setCallback(mqttCallback);

  ArduinoOTA.setHostname("esp8266-device");
  ArduinoOTA.begin();

}

void loop() {

  ArduinoOTA.handle();

  // WiFi check
  if (millis() - lastWifiCheck > 5000) {

    lastWifiCheck = millis();

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }

  // MQTT reconnect
  if (!client.connected()) {

    if (millis() - lastMqttReconnect > 5000) {

      lastMqttReconnect = millis();
      connectMQTT();
    }
  }

  client.loop();

  // publish mỗi 10s
  if (millis() - lastSensorSend > 10000) {

    lastSensorSend = millis();

    String msg = "alive-" + String(random(0xffff), HEX);

    client.publish("device/status", msg.c_str());
  }

}