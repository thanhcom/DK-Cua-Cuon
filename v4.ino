#define BLYNK_TEMPLATE_ID "TMPL68gx4jN9o"
#define BLYNK_TEMPLATE_NAME "DK Cua Cuon"
#define BLYNK_AUTH_TOKEN "ByhFrakwe7NX3EC5VL4rQVIria8k0k1b"

#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <BlynkSimpleEsp8266.h>
#include <PubSubClient.h>
#include <DHT12.h>
#include <RCSwitch.h>
DHT12 dht12;
RCSwitch mySwitch = RCSwitch();
char aut[] = "";
int toggleState_1;
unsigned long time1 = 0;
const char* mqtt_server = "server.thanhtrang.online";
//#define BLYNK_PRINT Serial



void callback(char* topic, byte* payload, unsigned int length) {
  String myString = String(topic);  
  if (myString == "blynk/cmactive") {
    if ((char)payload[0] == '0') {
      Blynk.virtualWrite(V1, "Lên Cửa");
      digitalWrite(D5, HIGH);
      delay(300);
      digitalWrite(D5, LOW);
      SendCm("0");
    }
    if ((char)payload[0] == '1') {
      Blynk.virtualWrite(V1, "Dừng Cửa");
      digitalWrite(D6, HIGH);
      delay(300);
      digitalWrite(D6, LOW);
      SendCm("1");
    }
    if ((char)payload[0] == '2') {
      Blynk.virtualWrite(V1, "Xuống Cửa");
      digitalWrite(D7, HIGH);
      delay(300);
      digitalWrite(D7, LOW);
      SendCm("2");
    }
    if ((char)payload[0] == '3'||(char)payload[0] == '4') {
      //btn1
      Serial.println( "Button 1 : 5592323");
      mySwitch.send(5592323, 24);
    }
    if ((char)payload[0] == '5'||(char)payload[0] == '6') {
      //btn2
      mySwitch.send(5592332, 24);
      Serial.println( "Button  2: 5592332");
      
    }
    if ((char)payload[0] == '7'||(char)payload[0] == '8') {
      //btn3
      mySwitch.send(5592335, 24);
      Serial.println( "Button 3: 5592335");
    }
    if ((char)payload[0] == '9'||(char)payload[0] == 'a') {
      //btn4
      mySwitch.send(5592368, 24);
      Serial.println( "Button  4: 5592368");
    }
  }
}


WiFiClient espClient;
PubSubClient client(mqtt_server, 1882, callback, espClient);


void setup() {
  Serial.begin(9600);
  dht12.begin();
  //delay(180000);
  WiFi.mode(WIFI_STA);  // explicitly set mode, esp defaults to STA+AP
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("Thanh Trang Electronic", "");
  if (!res) {
    ESP.restart();
  }
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D4, OUTPUT);
  mySwitch.enableTransmit(D8);
  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());

  //Blynk.begin(BLYNK_AUTH_TOKEN,WiFi.SSID().c_str(), WiFi.psk().c_str());
  String str = "Arduino start :" + String(random(0xffff), HEX);
  if (client.connect(str.c_str(), "thanhcom", "laodaicaha")) {
    client.setKeepAlive(300);
    client.subscribe("blynk/cmactive");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if ((unsigned long)(millis() - time1) > 10000) {
    digitalWrite(D4, !digitalRead(D4));
    Blynk.virtualWrite(V2, WiFi.RSSI());
    Blynk.virtualWrite(V3, dht12.readTemperature());
    Blynk.virtualWrite(V4, dht12.readHumidity());
    time1 = millis();
  }
  if (time1 < 0) {
    time1 = millis();
  }

  Blynk.run();
  if (WiFi.status() != WL_CONNECTED) {
    delay(50000);
    ESP.restart();
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

BLYNK_WRITE(V0) {
  toggleState_1 = param.asInt();
  if (toggleState_1 == 0) {
    Blynk.virtualWrite(V1, "Lên Cửa");
    digitalWrite(D5, HIGH);
    delay(300);
    digitalWrite(D5, LOW);
    client.publish("blynk/cm", "0");
  }
  if (toggleState_1 == 1) {
    Blynk.virtualWrite(V1, "Dừng Cửa");
    digitalWrite(D6, HIGH);
    delay(300);
    digitalWrite(D6, LOW);
    client.publish("blynk/cm", "1");
  }
  if (toggleState_1 == 2) {

    Blynk.virtualWrite(V1, "Xuống Cửa");
    digitalWrite(D7, HIGH);
    delay(300);
    digitalWrite(D7, LOW);
    client.publish("blynk/cm", "2");
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), "thanhcom", "laodaicaha")) {
      client.setKeepAlive(300);
      client.subscribe("blynk/cmactive");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void SendCm(String str) {
  client.publish("blynk/cm", str.c_str());
}
