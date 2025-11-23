#include <HardwareSerial.h>
#include <PZEM004Tv30.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>

// ===== UART cho PZEM trên ESP32-C3 Super Mini =====
HardwareSerial pzemSerial(1);       
PZEM004Tv30 pzem(pzemSerial, 20, 21); // RX=20, TX=21

WiFiClient espClient;
PubSubClient client(espClient);

// ===== Cấu hình MQTT mặc định =====
struct Config {
  char mqtt_server[40] = "thanhcom1989.ddns.net";
  char mqtt_port[6]    = "1882";
  char mqtt_user[32]   = "thanhcom";
  char mqtt_pass[32]   = "laodaicaha";
  char mqtt_topic[64]  = "pzem/data";
} config;

// ===== Tên file cấu hình =====
const char* configFile = "/config.json";
const char* mqtt_cmd   = "pzem/reset"; 

// ===== Đọc file cấu hình =====
bool loadConfig() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  if (!SPIFFS.exists(configFile)) {
    Serial.println("Config file not found, using defaults");
    return false;
  }
  File file = SPIFFS.open(configFile, "r");
  if (!file) return false;

  StaticJsonDocument<256> doc;
  if (deserializeJson(doc, file)) return false;

  strlcpy(config.mqtt_server, doc["mqtt_server"] | config.mqtt_server, sizeof(config.mqtt_server));
  strlcpy(config.mqtt_port,   doc["mqtt_port"]   | config.mqtt_port,   sizeof(config.mqtt_port));
  strlcpy(config.mqtt_user,   doc["mqtt_user"]   | config.mqtt_user,   sizeof(config.mqtt_user));
  strlcpy(config.mqtt_pass,   doc["mqtt_pass"]   | config.mqtt_pass,   sizeof(config.mqtt_pass));
  strlcpy(config.mqtt_topic,  doc["mqtt_topic"]  | config.mqtt_topic,  sizeof(config.mqtt_topic));

  file.close();
  return true;
}

// ===== Lưu cấu hình =====
bool saveConfig() {
  StaticJsonDocument<256> doc;
  doc["mqtt_server"] = config.mqtt_server;
  doc["mqtt_port"]   = config.mqtt_port;
  doc["mqtt_user"]   = config.mqtt_user;
  doc["mqtt_pass"]   = config.mqtt_pass;
  doc["mqtt_topic"]  = config.mqtt_topic;

  File file = SPIFFS.open(configFile, "w");
  if (!file) return false;

  serializeJson(doc, file);
  file.close();
  return true;
}

// ===== MQTT Callback =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.printf("MQTT message received: topic=%s, payload=%s\n", topic, msg.c_str());

  if (String(topic) == mqtt_cmd && msg == "1") {
    if (pzem.resetEnergy()) {
      Serial.println("Energy reset successful!");
    } else {
      Serial.println("Energy reset failed!");
    }
  }
}

// ===== Reconnect MQTT =====
void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32C3Mini-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), config.mqtt_user, config.mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(mqtt_cmd);
    } else {
      Serial.printf("failed, rc=%d, retry in 5s\n", client.state());
      delay(5000);
    }
  }
}

// ===== Setup =====
void setup() {
  Serial.begin(115200);
  delay(5000); // đợi router/modem khởi động lại khi mất điện

  pzemSerial.begin(9600, SERIAL_8N1, 20, 21);
  loadConfig();

  // WiFiManager
  WiFiManager wm;

  // Cấu hình retry để tránh bật AP ngay khi router chưa sẵn sàng
  wm.setConnectRetries(30);        // thử lại 30 lần
  wm.setConnectTimeout(30);        // mỗi lần thử chờ 30 giây
  wm.setConfigPortalTimeout(180);  // chỉ bật AP nếu không kết nối sau 3 phút

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", config.mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", config.mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "MQTT User", config.mqtt_user, 32);
  WiFiManagerParameter custom_mqtt_pass("pass", "MQTT Pass", config.mqtt_pass, 32);
  WiFiManagerParameter custom_mqtt_topic("topic", "MQTT Topic", config.mqtt_topic, 64);

  wm.addParameter(&custom_mqtt_server);
  wm.addParameter(&custom_mqtt_port);
  wm.addParameter(&custom_mqtt_user);
  wm.addParameter(&custom_mqtt_pass);
  wm.addParameter(&custom_mqtt_topic);

  if (!wm.autoConnect("ESP32-C3-SuperMini-PZEM")) {
    Serial.println("Failed to connect, starting AP config mode...");
  }

  // Lưu config mới nếu có thay đổi
  strlcpy(config.mqtt_server, custom_mqtt_server.getValue(), sizeof(config.mqtt_server));
  strlcpy(config.mqtt_port,   custom_mqtt_port.getValue(),   sizeof(config.mqtt_port));
  strlcpy(config.mqtt_user,   custom_mqtt_user.getValue(),   sizeof(config.mqtt_user));
  strlcpy(config.mqtt_pass,   custom_mqtt_pass.getValue(),   sizeof(config.mqtt_pass));
  strlcpy(config.mqtt_topic,  custom_mqtt_topic.getValue(),  sizeof(config.mqtt_topic));
  saveConfig();

  // MQTT
  client.setServer(config.mqtt_server, atoi(config.mqtt_port));
  client.setCallback(mqttCallback);
  client.setKeepAlive(60);
  client.setBufferSize(512);
}

// ===== Loop =====
void loop() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (!client.connected()) reconnect_mqtt();
  client.loop();

  float voltage   = pzem.voltage();   delay(50);
  float current   = pzem.current();   delay(50);
  float power     = pzem.power();     delay(50);
  float energy    = pzem.energy();    delay(50);
  float frequency = pzem.frequency(); delay(50);
  float pf        = pzem.pf();

  if (!isnan(voltage) && !isnan(current) && !isnan(power) &&
      !isnan(energy) && !isnan(frequency) && !isnan(pf)) {
    DynamicJsonDocument doc(512);
    doc["voltage"]     = voltage;
    doc["current"]     = current;
    doc["power"]       = power;
    doc["energy"]      = energy;
    doc["frequency"]   = frequency;
    doc["powerFactor"] = pf;

    char json_string[512];
    serializeJson(doc, json_string);
    client.publish(config.mqtt_topic, json_string);
  }

  delay(5000);
}
