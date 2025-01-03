#include <Arduino.h>
//#define DEBUG 1
#define ESP32 1

#include <WiFi.h>  
#include <ESPmDNS.h> 
#include <ArduinoOTA.h>  
#include <HTTPUpdateServer.h>
#include <ArduinoJson.h>
#include <PubSubClient.h> // MQTT

#include "config.h"
#include "crc.h"
#include "web.h"

#include "EasyNextionLibrary.h" 

#include "pitches.h"
#include "printf.h"

#include "httpServer.h"
#include "shutterControl.h"

EasyNex myNex(Serial2); // Create an object of EasyNex class with the name < myNex > 
configuration cfg,web_cfg; // structure storing config settings
WiFiClient espClient;         // WiFi
httpServer httpserver;
WebPage webpage=WebPage(httpserver.getServer());
HTTPUpdateServer httpUpdater;
unsigned long previousWifiAttempt = 0;
unsigned long previousMQTTAttempt = 0;
boolean wifiConnected=false;  // wifi status changes
boolean mqttConnected=false;  // mqtt status changes
unsigned long lastMQTTDisconnect=0; // last time MQTT was disconnected
PubSubClient mqttClient(espClient);   // MQTT client
shutterControl shuttercontrol(&mqttClient);

// notes in the melody:
int melody[] = {
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_G5, NOTE_C5, NOTE_D5,
  NOTE_E5,
  NOTE_F5, NOTE_F5, NOTE_F5, NOTE_F5,
  NOTE_F5, NOTE_E5, NOTE_E5, NOTE_E5, NOTE_E5,
  NOTE_E5, NOTE_D5, NOTE_D5, NOTE_E5,
  NOTE_D5, NOTE_G5
};

// note durations: 4 = quarter note, 8 = eighth note, etc, also called tempo:
int noteDurations[] = {
  8, 8, 4,
  8, 8, 4,
  8, 8, 8, 8,
  2,
  8, 8, 8, 8,
  8, 8, 8, 16, 16,
  8, 8, 8, 8,
  4, 4
};

void playMelody() {
  int size = sizeof(noteDurations) / sizeof(int);
  for (int thisNote = 0; thisNote < size; thisNote++) {// iterate over the notes of the melody
    // to calculate the note duration, take 2 seconds divided by the note type.
    //e.g. quarter note = 2000 / 4, eighth note = 2000/8, etc.
    int noteDuration = 2000 / noteDurations[thisNote];
    tone(GPIO_BUZZER, melody[thisNote], noteDuration);
    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(GPIO_BUZZER);
  }
}

/************************ 
* S E T U P   W I F I  
*************************/
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
    Serial.println();
    Serial.print("Connecting to WiFi");
  #endif

  WiFi.hostname(cfg.host_name);
  WiFi.mode(WIFI_STA);
  WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1); //use config values loaded from flash (EEPROM imitation)

  int i=0;
  while (WiFi.status() != WL_CONNECTED && i<60) {
    i++;
    delay(500); //retry for 30 seconds
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }
  //Try default ssid and password if the ones from loaded config does not work, maybe there is a new router
  if (WiFi.status() != WL_CONNECTED && (strcmp(cfg.wifi_ssid1,_ssid1_)!=0 || strcmp(cfg.wifi_password1,_password1_)!=0))  {
    #ifdef DEBUG
       Serial.println();
       Serial.print("Loading defaults and restarting...");
    #endif
    //defaultConfig(&cfg);
    //saveConfig();
    //ESP.restart(); 
    //delay(10000);
    WiFi.disconnect();
    WiFi.begin(_ssid1_, _password1_);
    while (WiFi.status() != WL_CONNECTED && i<60) {
      i++;
      delay(500);
      #ifdef DEBUG
        Serial.print(".");
      #endif
    }
    return; //not connected afterall TODO better error handling
  }
  MDNS.begin(cfg.host_name); // user-freindly hostname.local can be used instead of IP address only
  MDNS.addService("http", "tcp", 80); // Ensures that the mDNS will run after initialization. Without this call, the mDNS will be active for only a few minutes.

  #ifdef DEBUG
    Serial.println("");
    Serial.printf("Connected to %s\n", WiFi.SSID().c_str());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

void messageCallback(char* topic, byte* payload, unsigned int length) {

  char *blind_name;
  char *dummy;
  char *param;
  int blind_num=0;

  #ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payload_copy);
  #endif

  //webpage.lastCommand="Topic:";
  //webpage.lastCommand+=topic;
  //webpage.lastCommand+=",  Payload:";
  webpage.lastCallback= millis();

  //parse blinds/blind_name/state topic
  if ((strncmp(topic,"blinds/",7)==0) && (strcmp(topic,"blinds/scene")!=0))
  {
    dummy = strtok_r(topic, "/", &topic);// parsing a string into a sequence of tokens (thread safe) -> blinds
    blind_name = strtok_r(topic, "/", &topic);// blind name
    if(blind_name == NULL) return;
    dummy = strtok_r(topic, "/", &topic);// state
    if(dummy == NULL) return;
    if(strcmp(dummy,"state")!=0) return; //only interpreting state changes
    param = strtok_r(topic, "/", &topic);//position or tilt
    if(param == NULL) return;
 
    //find which blind number is it
    for(int i=0; i<NUMBER_OF_BLINDS;i++){
      if(strcmp(blind_name,cfg.blind_names[i])==0)
      {
        blind_num=i+1;
      }
    }

    //found a matching name
    if(blind_num!=0){
      char numberarray[3];
      if(strcmp(param,"position")==0){ //position changed make a percentage string out of it
        char* payload_copy = (char*)malloc(length+2);
        memcpy(payload_copy,payload,length);
        payload_copy[length]='%';
        payload_copy[length+1] = '\0';

        //update the right text label in the screen with tha string
        String cmdstring = "statuspage.pos";
        cmdstring+=itoa(blind_num,numberarray, 10);
        cmdstring+=".txt";
        myNex.writeStr(cmdstring, payload_copy);  
        free(payload_copy);
      } else if(strcmp(param,"tilt")==0) {//tilt changed make a number string
        char* payload_copy = (char*)malloc(length+1);
        memcpy(payload_copy,payload,length);
        payload_copy[length] = '\0';

        //update the right text label on the screen
        String cmdstring = "statuspage.tilt";
        cmdstring+=itoa(blind_num,numberarray, 10);
        cmdstring+=".txt";
        myNex.writeStr(cmdstring, payload_copy);
        free(payload_copy);
      }
    }
  }
/*
  else if (strcmp(topic, cfg.subscribe_command) == 0) {
    //
  } else if (strcmp(topic, cfg.subscribe_position) == 0) {
    //
  } else if (cfg.tilt && strcmp(topic, cfg.subscribe_tilt) == 0) {
    //
  }  else if (strcmp(topic, cfg.subscribe_calibrate) == 0) {
    //
  } else if (strcmp(topic, cfg.subscribe_reboot) == 0) {    
     Restart();
  }*/

}

/********************************************
* M A I N   A R D U I N O   S E T U P 
********************************************/
void setup() {
// HW initialization
  pinMode(GPIO_DISPLAY_INVERTED, OUTPUT);
  digitalWrite(GPIO_DISPLAY_INVERTED, LOW);  // Display on

  pinMode(GPIO_REL1, OUTPUT);
  pinMode(GPIO_REL2, OUTPUT);
  pinMode(GPIO_KEY1, INPUT_PULLUP);
  pinMode(GPIO_KEY2, INPUT_PULLUP);

  myNex.begin(112500); //begin the object with a baud rate of 115200 on Serial2 towards Nextion display

// Open EEPROM
  openMemory();
  if (loadConfig()){
    webpage.crcStatus+="CRC config OK! ";
    myNex.writeStr("settings.wifi_ssid.txt",cfg.wifi_ssid1);
    myNex.writeStr("settings.wifi_pass.txt",cfg.wifi_password1);
    myNex.writeNum("blind_number",NUMBER_OF_BLINDS); //set the number of blinds to be visible in the display
    for(int i=0; i<NUMBER_OF_BLINDS;i++) //set blind name text on status page buttons
    {
      char numberarray[3];
      String textname="statuspage.button0";
      textname+=itoa(i+1,numberarray, 10);
      textname+="text.txt";
      webpage.crcStatus+=textname;
      myNex.writeStr(textname, cfg.blind_names[i]);
    }
  }
  else{
    webpage.crcStatus+="CRC config failed. ";
  };  

  copyConfig(&cfg,&web_cfg); // copy config to web_cfg as well
  Serial.begin(115200); //begin main serial for debug
  Serial.setDebugOutput(false);  //turn of debug output from the WiFi library

  setup_wifi();
  mqttClient.setServer(cfg.mqtt_server,1883);
  mqttClient.setCallback(messageCallback);

  //playMelody();
    
  // Over The Air Update
  ArduinoOTA.setHostname(cfg.host_name);
  //ArduinoOTA.setPassword(OTA_password);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    #ifdef DEBUG
    Serial.println("Start updating " + type);
    #endif
  });

  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
    Serial.println("\nEnd");
    #endif
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });

  ArduinoOTA.onError([](ota_error_t error) {
    #ifdef DEBUG
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
    #endif
  });
  ArduinoOTA.begin(); 

  // Turn on the Web Server
  httpserver.setup();
  webpage.setup();
  httpUpdater.setup(httpserver.getServer(),"/upgrade"/*,WEB_UPGRADE_USER, WEB_UPGRADE_PASS*/);
  httpserver.begin(); //Start the server

  //setup MQTT broker
 /* mqttBroker.subscribe("#", [](char* topic, void* payload, size_t payload_size){
    messageCallback(topic, (char*) payload, payload_size);
  });
  mqttBroker.begin();*/
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

/********************************
* R E C O N N E C T   M Q T T 
********************************/
void mqtt_reconnect() {
  #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
  #endif

  unsigned long now = millis();
  
 // uint8_t mac[6];
 // WiFi.macAddress(mac);
  String clientName(cfg.host_name);
 // clientName += "-";
 // clientName += macToStr(mac);
 // clientName += "-";
 // clientName += String(micros() & 0xff, 16);
  
  if (mqttClient.connect((char*)clientName.c_str()/*,"user","pass"*/)) {
    #ifdef DEBUG
      Serial.println("connected");
    #endif

    // Once connected, publish an announcement...
    //digitalWrite(SLED, LOW);   // Turn the Status Led on

    // lastUpdate=0;
    // checkSensors(); // send current sensors
    // publishSensor();

    // resubscribe
    mqttClient.subscribe("#"); 

  } else {
   // digitalWrite(SLED, HIGH);   // Turn the Status Led off
    #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
    #endif
  }
}


/******************************************
* NEXTION CODE printh 23 02 54 00
******************************************/
//reset to defaults
void trigger0(){
  defaultConfig(&cfg);
  saveConfig();
  ESP.restart(); 
  delay(3000);
}

void trigger1(){
  shuttercontrol.setScene(0);
  //digitalWrite(GPIO_REL1, HIGH);  // Relay1 on
}

void trigger2(){
  shuttercontrol.setScene(1);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void trigger3(){
  shuttercontrol.setScene(2);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void trigger4(){
  shuttercontrol.setScene(3);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void trigger5(){
  shuttercontrol.setScene(4);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void trigger6(){
  shuttercontrol.setScene(5);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void trigger7(){
  shuttercontrol.setScene(6);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void trigger8(){
  shuttercontrol.setScene(7);
  //digitalWrite(GPIO_REL1, LOW);  // Relay1 off
}

void displayPageChanged(int currentPageId){
  char pagearray[3];
  webpage.lastCommand="Page Changed:";
  webpage.lastCommand+=itoa(currentPageId,pagearray,10);
}

/***************************************
* M A I N   A R D U I N O   L O O P  
***************************************/
void loop() {
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) //just (re)connected
    {
      wifiConnected=true;
      myNex.writeStr("settings.ip_address.txt",WiFi.localIP().toString());
    }
    webpage.lastWiFiConnect=now;  // Not used at the moment
    ArduinoOTA.handle(); // OTA first
    if(!mqttClient.connected()){
      if(mqttConnected) //just disconnected
      {
        myNex.writeStr("settings.mqtt_server.txt","disconnected");
        mqttConnected=false;
      }
      if((unsigned long)(now - previousMQTTAttempt) > MQTT_RETRY_INTERVAL)//every 10 sec
      {
        mqtt_reconnect();  
        previousMQTTAttempt = now;    
      }
    } else{
      if(mqttConnected==false){//just reconnected
        mqttConnected=true;
        myNex.writeStr("settings.mqtt_server.txt",cfg.mqtt_server);
      }
      if (mqttClient.loop()){
        webpage.lastMQTTDisconnect = 0;
      }
    }
    httpserver.handleClient();         // Web handling
  } else {
    if(wifiConnected) //just disconnected
    {
      myNex.writeStr("settings.ip_address.txt","disconnected");
      wifiConnected=false;
    }
    if ((WiFi.status() != WL_CONNECTED) && ((unsigned long)(now - previousWifiAttempt) > WIFI_RETRY_INTERVAL)) {//every 20 sec
      WiFi.disconnect();
      WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1);
      previousWifiAttempt = now;
    } 
  } 
  myNex.NextionListen(); 
  //delay(update_interval_loop); // 25 ms (short because of tilt) (1.5 degrees in 25 ms)
}
