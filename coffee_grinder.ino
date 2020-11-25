
/* 
 *  Create a WiFi access point and 
 *	provide a web server on it to show weight of grinder-output. 
 *  https://t3n.de/news/web-app-grafiken-android-ios-530322/
 *  https://www.w3schools.com/howto/howto_css_loader.asp
*/

//uncomment for fast development without AP mode 
//#define OTA

#include <FS.h>
#include <ESP8266WiFi.h>

#ifdef OTA
#include <ArduinoOTA.h>
#endif

#include <ESP8266WebServer.h>
#include "HelperFunctions.h"
#include "Coffee_Grinder.h"

const int SCALE_DOUT_PIN = D5;
const int SCALE_SCK_PIN = D2;

// Sonoff
//const int RELAIS_PIN = D6;
//const int AP_LED = D7;
// MCUNode
const int RELAIS_PIN = D0;//invert led
const int AP_LED = D4;

Coffee_Grinder grinder(RELAIS_PIN,1,SCALE_DOUT_PIN, SCALE_SCK_PIN);


const int AP_BUTTON = D3;

int AP_ON = 1;    // variable to store the read value
int Button;       // variable to store the read value

void setup() {
  pinMode(AP_BUTTON, INPUT); // flash button
  pinMode(AP_LED, OUTPUT);// D4 MCUNode LED for AP on
  
  Serial.begin(115200);
  delay(1000);
  
  SPIFFS.begin();
  Serial.println(); Serial.print("Configuring access point...");
  setupWiFi();
#ifdef OTA  
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("coffee-esp");
  // No authentication by default
  ArduinoOTA.setPassword("nobbe");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    //switch Relais OFF
    grinder.stop();
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  IPAddress myIP = WiFi.localIP();
#endif OTA 

#ifndef OTA
  IPAddress myIP = WiFi.softAPIP();
#endif
  
  Serial.print("IP address: "); Serial.println(myIP);

  server.on("/", HTTP_GET, []() {
    handleFileRead("/");
  });

  server.onNotFound([]() {                          // Handle when user requests a file that does not exist
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  server.begin();
  Serial.println("HTTP server started");
  yield();
  
  grinder.setup();
  
}

void loop() {
    
    static unsigned long lled = 0;
    unsigned long t;     // local var: type declaration at compile time
	
	grinder.loop();
    server.handleClient();
    
    if(Serial.available())
    {
      char temp = Serial.read();
      if(temp == '+' || temp == 'a')
        grinder.scale_faktor += 1;
    }
#ifndef OTA
    if(!digitalRead(AP_BUTTON)&&(Button==0)){     // read the input pin
      Button = 1;
      //digitalWrite(AP_LED, 0);
      delay(1000);
    }
    else if(!digitalRead(AP_BUTTON)&&(Button==1)){     // read the input pin
      Button = 0;
      //digitalWrite(AP_LED, 1);
      delay(1000);
    }
    
    if((AP_ON==0)&&(Button==1)){
      //startAP
      AP_ON=1;
      WiFi.forceSleepWake();
      WiFi.mode(WIFI_AP);
      Serial.println("start Wifi");
      digitalWrite(AP_LED, 0);
    }
    else if((AP_ON==1)&&(Button==0)){
      //stopAP
      AP_ON=0;
      WiFi.disconnect();
      WiFi.mode(WIFI_OFF);
      WiFi.forceSleepBegin();
      Serial.println("stop Wifi");
      digitalWrite(AP_LED, 1);
    }
    
	  //flashing LED if client connectet
    t = millis();
    if((WiFi.softAPgetStationNum()>0)&&(AP_ON==1)){
       if((t - lled) > 200) {
          digitalWrite(AP_LED, !digitalRead(AP_LED));
          lled = t;
       }
    }
    else if(AP_ON==1){
      digitalWrite(AP_LED, 0);
    }
#endif   
}


