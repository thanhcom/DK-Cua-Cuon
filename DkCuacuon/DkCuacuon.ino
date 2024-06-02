#define BLYNK_TEMPLATE_ID "TMPL68gx4jN9o"
#define BLYNK_TEMPLATE_NAME "DK Cua Cuon"
#define BLYNK_FIRMWARE_VERSION        "1.0.0"
//#define BLYNK_AUTH_TOKEN "smyXCNBNUSn-1C4dmApjgNSIRPt3bLwx"
 
int  toggleState_1;
unsigned long time1 = 0;
//#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiManager.h>

BLYNK_WRITE(V0)
{
  toggleState_1 = param.asInt();
  if(toggleState_1==0)
  {
    Blynk.virtualWrite(V1,"Lên Cửa");
    digitalWrite(2,HIGH);
    delay(300);
    digitalWrite(2,LOW);

  }if(toggleState_1==1)
  {
    Blynk.virtualWrite(V1,"Dừng Cửa");
    digitalWrite(4,HIGH);
    delay(300);
    digitalWrite(4,LOW);

  }
  if(toggleState_1==2)
  {
    
    Blynk.virtualWrite(V1,"Xuống Cửa");
    digitalWrite(18,HIGH);
    delay(300);
    digitalWrite(18,LOW);

  }
}

void setup() {
   delay(180000);

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("Thanh Trang Electronic",""); 
      if(!res) {
        ESP.restart();
    } 
   pinMode(2, OUTPUT);
   pinMode(4, OUTPUT);
   pinMode(18, OUTPUT);
   Blynk.begin(aut,WiFi.SSID().c_str(), WiFi.psk().c_str());
  }

void loop() {
  // put your main code here, to run repeatedly:
if ( (unsigned long) (millis() - time1) > 10000 )
    {
      Blynk.virtualWrite(V2,WiFi.RSSI());
      time1 = millis();
    }
if(time1<0)
{
  time1 = millis();
}    

    Blynk.run();
    if(WiFi.status() != WL_CONNECTED) {
    delay(50000);
    ESP.restart();
  }
}
