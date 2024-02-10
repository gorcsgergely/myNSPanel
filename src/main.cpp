#include <Arduino.h>
//#define DEBUG 1
#define ESP32 1

#include <WiFi.h>  
#include <ESPmDNS.h>  // Sends host name in WiFi setup
//#include <PubSubClient.h> // MQTT
#include <ArduinoOTA.h>   // (Over The Air) update
#include <HTTPUpdateServer.h>

#include <ArduinoJson.h>
//#include <math.h>
#include "config.h"
#include "crc.h"
#include "web.h"

//#include <SPIFFS.h>

#include "EasyNextionLibrary.h"  // Include EasyNextionLibrary

#include "pitches.h"
#include "printf.h"

#include <PicoMQTT.h>
#include "httpServer.h"
#include "shutterControl.h"

EasyNex myNex(Serial2); // Create an object of EasyNex class with the name < myNex > 
//bool button_state = false;

configuration cfg,web_cfg;

// init Nextion object
//ESPNexUpload nextion(115200);

// MQTT callback declaratiion (definition below)
//void callback(char* topic, byte* payload, unsigned int length); 
void connectedCallback(const char* client_id);
void disconnectedCallback(const char* client_id);

WiFiClient espClient;         // WiFi

// Here I have to initialize mqtt_server after setup

//PicoMQTT::Server mqttBroker;
class MQTT: public PicoMQTT::Server {
    protected:
        void on_connected(const char * client_id) override {
          connectedCallback(client_id);
        }
         void on_disconnected(const char * client_id) override {
          disconnectedCallback(client_id);
        }
} mqttBroker;

shutterControl shuttercontrol(&mqttBroker);

httpServer httpserver;
WebPage webpage=WebPage(httpserver.getServer());
HTTPUpdateServer httpUpdater;

unsigned long previousWifiAttempt = 0;
unsigned long previousMQTTAttempt = 0;
boolean wifiConnected=false;

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

char _blind_names_[NUMBER_OF_BLINDS][16] ={"blind1","blind2","blind3","blind4","blind5","blind6","blind7"}; 

void playMelody() {
// iterate over the notes of the melody:
  int size = sizeof(noteDurations) / sizeof(int);

  for (int thisNote = 0; thisNote < size; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
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
  WiFi.mode(WIFI_STA); // Pouze WiFi client!!
  WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1);

  int i=0;

  while (WiFi.status() != WL_CONNECTED && i<60) {
    i++;
    delay(500);
    #ifdef DEBUG
      Serial.print(".");
    #endif
  }
  //Try fallback ssid and password
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
    return;
  }
  MDNS.begin(cfg.host_name);

//for web firmware upload
#ifdef _WEB_
  MDNS.addService("http", "tcp", 80);
 // Serial.printf("HTTPUpdateServer ready! Open http://%s.local/update in your browser\n", _host_name_);
#endif

  #ifdef DEBUG
    Serial.println("");
    Serial.printf("Connected to %s\n", WiFi.SSID().c_str());
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  #endif
}

// Callback for processing MQTT message
void messageCallback(char* topic, char* payload, unsigned int length) {

  char *blind_name;
  char *param;

  char* payload_copy = (char*)malloc(length+1);
  memcpy(payload_copy,payload,length);
  payload_copy[length] = '\0';

  #ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payload_copy);
  #endif

  webpage.lastCommand="Topic:";
  webpage.lastCommand+=topic;
  webpage.lastCommand+=",  Payload:";
  webpage.lastCommand+=payload_copy;
  
  webpage.lastCallback= millis();

  //pasrse blinds/c1/position topic
  if (strncmp(topic,"blinds/",7)==0)
  {
    blind_name = strtok_r(topic, "/", &topic);
    param = strtok_r(topic, "/", &topic);
  }
 
  webpage.lastCommand+=blind_name;
  webpage.lastCommand+=param;

/*
  if (strcmp(topic, cfg.subscribe_command) == 0) {
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
  free(payload_copy);
}

void connectedCallback(const char* client_id){
  webpage.lastCommand="connected:";
  webpage.lastCommand+=client_id;
}

void disconnectedCallback(const char* client_id){
  webpage.lastCommand="disconnected:";
  webpage.lastCommand+=client_id;
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


// Open EEPROM
  openMemory();
  if (loadConfig()){
    webpage.crcStatus+="CRC config OK! ";
  }
  else{
    webpage.crcStatus+="CRC config failed. ";
  };   // loading config to cfg
  copyConfig(&cfg,&web_cfg); // copy config to web_cfg as well

  pinMode(GPIO_KEY1, INPUT_PULLUP);
  pinMode(GPIO_KEY2, INPUT_PULLUP);

  myNex.begin(112500); // Begin the object with a baud rate of 115200

  Serial.begin(115200);
  Serial.setDebugOutput(false);

  setup_wifi();

  //playMelody();
  
 /* mqttClient.setServer(cfg.mqtt_server,1883);
  mqttClient.setCallback(callback);*/
  
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
  httpUpdater.setup(httpserver.getServer(),"/upgrade",WEB_UPGRADE_USER, WEB_UPGRADE_PASS);
  httpserver.begin(); //Start the server


   mqttBroker.subscribe("#", [](char* topic, void* payload, size_t payload_size){
    messageCallback(topic, (char*) payload, payload_size);
   });
   mqttBroker.begin();

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
/*
void mqtt_reconnect() {
  #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
  #endif
  char topic[40];
  unsigned long now = millis();
 // if (lastMQTTDisconnect!=0 && lastMQTTDisconnect<now && (unsigned long)(now-lastMQTTDisconnect)<10000) return;
  if (lastMQTTDisconnect!=0 && (unsigned long)(now-lastMQTTDisconnect)<10000) return;
  lastMQTTDisconnect=now;
  // Attempt to connect
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  String clientName(cfg.host_name);
  clientName += "-";
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);
  
  if (mqttClient.connect((char*)clientName.c_str(),cfg.mqtt_user,cfg.mqtt_password)) {
    #ifdef DEBUG
      Serial.println("connected");
    #endif

    // resubscribe
    for (int i=0; i<NUMBER_OF_BLINDS; i++){
      snprintf(topic,40,"blinds/%s/state", cfg.blind_names[i]);
      mqttClient.subscribe(topic);
      snprintf(topic,40,"blinds/%s/tilt-state", cfg.blind_names[i]);
      mqttClient.subscribe(topic);
    }

  } else {
    #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
    #endif
  }
}*/


/*
// Callback for processing MQTT message
void callback(char* topic, byte* payload, unsigned int length) {

  char *blind_name;
  char *param;

  char* payload_copy = (char*)malloc(length+1);
  memcpy(payload_copy,payload,length);
  payload_copy[length] = '\0';

  #ifdef DEBUG
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.println(payload_copy);
  #endif

  lastCommand="Topic:";
  lastCommand+=topic;
  lastCommand+=",  Payload:";
  lastCommand+=payload_copy;
  
  lastCallback= millis();

  //pasrse blinds/c1/position topic
  if (strncmp(topic,"blinds/",7)==0)
  {
    blind_name = strtok_r(topic, "/", &topic);
    param = strtok_r(topic, "/", &topic);
  }
 
  lastCommand+=blind_name;
  lastCommand+=" ";
  lastCommand+=param;
  lastCommand+=" ";

//  if (strcmp(topic, cfg.subscribe_command) == 0) {
    //
 // } else if (strcmp(topic, cfg.subscribe_position) == 0) {
    //
//  } else if (cfg.tilt && strcmp(topic, cfg.subscribe_tilt) == 0) {
    //
 // }  else if (strcmp(topic, cfg.subscribe_calibrate) == 0) {
    //
//  } else if (strcmp(topic, cfg.subscribe_reboot) == 0) {    
 //    Restart();
 // }

  free(payload_copy);
}*/


/******************************************
* NEXTION CODE printh 23 02 54 00
******************************************/
//reset to defaults
void trigger0(){
  defaultConfig(&cfg);
  saveConfig();
  ESP.restart(); 
  delay(10000);
}

void trigger1(){
  digitalWrite(GPIO_REL1, HIGH);  // Relay1 on
 /* if(!button_state){
    myNex.writeNum("b0.bco", 2016); // Set button b0 background color to GREEN (color code: 2016)
    myNex.writeStr("b0.txt", "ON"); // Set button b0 text to "ON"
    button_state=true;
  } else {
    myNex.writeNum("b0.bco", 63488); // Set button b0 background color to RED (color code: 63488)
    myNex.writeStr("b0.txt", "OFF"); // Set button b0 text to "OFF"
    button_state=false;
  }*/
}

void trigger2(){
  digitalWrite(GPIO_REL1, LOW);  // Relay1 off
 /* if(!button_state){
    myNex.writeNum("b0.bco", 2016); // Set button b0 background color to GREEN (color code: 2016)
    myNex.writeStr("b0.txt", "ON"); // Set button b0 text to "ON"
    button_state=true;
  } else {
    myNex.writeNum("b0.bco", 63488); // Set button b0 background color to RED (color code: 63488)
    myNex.writeStr("b0.txt", "OFF"); // Set button b0 text to "OFF"
    button_state=false;
  }*/
}

/***************************************
* M A I N   A R D U I N O   L O O P  
***************************************/
void loop() {
  //char ipbuff[30];
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    if (!wifiConnected) //just (re)connected
    {
      wifiConnected=true;
      //snprintf(ipbuff,30,"ipaddress=%s",WiFi.localIP().toString());
      myNex.writeStr("settings.ip_address.txt",WiFi.localIP().toString());
    }
    webpage.lastWiFiConnect=now;  // Not used at the moment
    ArduinoOTA.handle(); // OTA first
    /*if (mqttClient.loop()) {
      // publishSensor();
    } else {
      if((unsigned long)(now - previousMQTTAttempt) > MQTT_RETRY_INTERVAL)//every 10 sec
      {
        //digitalWrite(SLED, HIGH);   // Turn the Status Led off
        mqtt_reconnect();  
        previousMQTTAttempt = now;    
      }
    } */
    mqttBroker.loop();
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
