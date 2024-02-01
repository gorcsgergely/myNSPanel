
//#define DEBUG 1
#define ESP32 1


#include <WiFi.h>  
#include <ESPmDNS.h>  // Sends host name in WiFi setup
#include <PubSubClient.h> // MQTT
#include <ArduinoOTA.h>   // (Over The Air) update
#include <HTTPUpdateServer.h>

#include <ArduinoJson.h>
#include <math.h>
#include "config.h"
#include "crc.h"
#include "web.h"

#include "ESPNexUpload.h"
//#include <SPIFFS.h>

#include "EasyNextionLibrary.h"  // Include EasyNextionLibrary

#include "pitches.h"

#include <PicoMQTT.h>

EasyNex myNex(Serial2); // Create an object of EasyNex class with the name < myNex > 
//bool button_state = false;

            
unsigned long lastUpdate = 0; // timestamp - last MQTT update
unsigned long lastCallback = 0; // timestamp - last MQTT callback received
unsigned long lastWiFiDisconnect=0;
unsigned long lastWiFiConnect=0;
unsigned long lastMQTTDisconnect=0; // last time MQTT was disconnected
unsigned long WiFiLEDOn=0;
unsigned long k1_up_pushed=0;
unsigned long k1_down_pushed=0;

unsigned long previousWifiAttempt = 0;
unsigned long previousMQTTAttempt = 0;

String lastCommand = "";
String crcStatus="";

configuration cfg,web_cfg;

// used only internally
int fileSize  = 0;
bool result   = true;
// init Nextion object
ESPNexUpload nextion(115200);

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


//PubSubClient mqttClient(espClient);   // MQTT client
//PubSubClient mqttClient(_mqtt_server_,1883,callback,espClient);   // MQTT client
WebServer server(80);    // Web Server
HTTPUpdateServer httpUpdater;

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

/*
String getContentType(String filename){
  if(server.hasArg(F("download"))) return F("application/octet-stream");
  else if(filename.endsWith(F(".htm"))) return F("text/html");
  else if(filename.endsWith(".html")) return F("text/html");
  else if(filename.endsWith(F(".css"))) return F("text/css");
  else if(filename.endsWith(F(".js"))) return F("application/javascript");
  else if(filename.endsWith(F(".png"))) return F("image/png");
  else if(filename.endsWith(F(".gif"))) return F("image/gif");
  else if(filename.endsWith(F(".jpg"))) return F("image/jpeg");
  else if(filename.endsWith(F(".ico"))) return F("image/x-icon");
  else if(filename.endsWith(F(".xml"))) return F("text/xml");
  else if(filename.endsWith(F(".pdf"))) return F("application/x-pdf");
  else if(filename.endsWith(F(".zip"))) return F("application/x-zip");
  else if(filename.endsWith(F(".gz"))) return F("application/x-gzip");
  return F("text/plain");
}*/

/*
bool handleFileRead(String path) {                          // send the right file to the client (if it exists)
  Serial.print("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";             // If a folder is requested, send the index file
  String contentType = getContentType(path);                // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) {   // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                          // If there's a compressed version available
      path += ".gz";                                        // Use the compressed verion
    File file = SPIFFS.open(path, "r");                     // Open the file
    size_t sent = server.streamFile(file, contentType);     // Send it to the client
    file.close();                                           // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);      // If the file doesn't exist, return false
  return false;
}
*/

// handle the file uploads
bool handleFileUpload(){
   digitalWrite(GPIO_REL1, LOW);  // Relay1 off
     digitalWrite(GPIO_REL2, HIGH);  // Relay1 off
  HTTPUpload& upload = server.upload();
  
  // Check if file seems valid nextion tft file
  if(!upload.filename.endsWith(F(".tft"))){
    server.send(500, F("text/plain"), F("ONLY TFT FILES ALLOWED\n"));
    return false;
  }
  
  if(!result){
    // Redirect the client to the failure page
    server.sendHeader(F("Location"),"/failure?reason=" + nextion.statusMessage);
    server.send(303);
    return false;
  }

  if(upload.status == UPLOAD_FILE_START){

    Serial.println(F("\nFile received. Update Nextion..."));

    // Prepare the Nextion display by seting up serial and telling it the file size to expect
    result = nextion.prepareUpload(fileSize);
    
    if(result){
      Serial.print(F("Start upload. File size is: "));
      Serial.print(fileSize);
      Serial.println(F(" bytes"));
    }else{
      Serial.println(nextion.statusMessage + "\n");
      return false;
    }
    
  }else if(upload.status == UPLOAD_FILE_WRITE){

    // Write the received bytes to the nextion
    result = nextion.upload(upload.buf, upload.currentSize);
    
    if(result){
      Serial.print(F("."));
    }else{
      Serial.println(nextion.statusMessage + "\n");
      return false;
    }
  
  }else if(upload.status == UPLOAD_FILE_END){

    // End the serial connection to the Nextion and softrest it
    nextion.end();
    
    Serial.println("");
    //Serial.println(nextion.statusMessage);
    return true;
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
    /*defaultConfig(&cfg);
    saveConfig();
    Restart();
    delay(10000);*/
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
  loadConfig();   // loading config to cfg
  copyConfig(&cfg,&web_cfg); // copy config to web_cfg as well

  pinMode(GPIO_KEY1, INPUT_PULLUP);
  pinMode(GPIO_KEY2, INPUT_PULLUP);

  myNex.begin(112500); // Begin the object with a baud rate of 115200

  Serial.begin(115200);
  Serial.setDebugOutput(false);
/*
  if(!SPIFFS.begin()){
       Serial.println(F("An Error has occurred while mounting SPIFFS"));
       Serial.println(F("Did you upload the data directory that came with this example?"));
       return;
  } 
*/
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
    Serial.println("Start updating " + type);
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
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
  });
  ArduinoOTA.begin(); 


// Turn on the Web Server
  server.on("/upload", HTTP_POST, [](){ 
       Serial.println(F("Succesfully updated Nextion!\n"));
    // Redirect the client to the success page after handeling the file upload
    server.sendHeader(F("Location"),F("/success"));
    server.send(303);
    return true;
  },
    // Receive and save the file
    handleFileUpload
  );
  server.on("/", handleRootPath);    //Associate the handler function to the path
  server.on("/upload", handleTftUploadPath);
  server.on("/success", handleSuccessPath);
  server.on("/failure", handleFailurePath);
  server.on("/configure",handleConfigurePath);
  server.on("/readMain", readMain);
  server.on("/readConfig",readConfig);
  server.on("/pressButton",pressButton);
  server.on("/updateField",updateField);
  server.on("/updateConfig", HTTP_POST, updateConfig);

  httpUpdater.setup(&server,"/upgrade",WEB_UPGRADE_USER, WEB_UPGRADE_PASS);



  // receive fileSize once a file is selected (Workaround as the file content-length is of by +/- 200 bytes. Known issue: https://github.com/esp8266/Arduino/issues/3787)
  server.on("/fs", HTTP_POST, [](){
    fileSize = server.arg(F("fileSize")).toInt();
    server.send(200, F("text/plain"), "");
  });
/*
  // called when the url is not defined here
  // use it to load content from SPIFFS
  server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, F("text/plain"), F("FileNotFound"));
  });*/

  server.begin(); //Start the server

 //mqttBroker.subscribe("#", [](const char * topic, const char * payload) {
       // Serial.printf("Received message in topic '%s': %s\n", topic, payload);  
  //});


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

void Restart() {
  /*
    mqttClient.publish(cfg.subscribe_command , "" , true);
    mqttClient.publish(cfg.subscribe_position , "" , true);
    mqttClient.publish(cfg.publish_position , "" , true);
    if (cfg.tilt) {
      mqttClient.publish(cfg.subscribe_tilt , "" , true);
      mqttClient.publish(cfg.publish_tilt , "" , true);

    }       
    ESP.restart();  */
}

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
  lastCommand+=param;

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
  lastCommand="connected:";
  lastCommand+=client_id;
}

void disconnectedCallback(const char* client_id){
  lastCommand="disconnected:";
  lastCommand+=client_id;
}

/********************************************************
CONVERTS SIGNAL FROM dB TO %
*********************************************************/
int WifiGetRssiAsQuality(int rssi)
{
  int quality = 0;

  if (rssi <= -100) {
    quality = 0;
  } else if (rssi >= -50) {
    quality = 100;
  } else {
    quality = 2 * (rssi + 100);
  }
  return quality;
}

void timeDiff(char *buf,size_t len,unsigned long lastUpdate){
    //####d, ##:##:##0
    unsigned long t = millis();
    if(lastUpdate>t) {
      snprintf(buf,len,"N/A");
      return;
    }
    t=(t-lastUpdate)/1000;  // Converted to difference in seconds

    int d=t/(60*60*24);
    t=t%(60*60*24);
    int h=t/(60*60);
    t=t%(60*60);
    int m=t/60;
    t=t%60;
    if(d>0) {
      snprintf(buf,len,"%dd, %02d:%02d:%02d",d,h,m,t);
    } else if (h>0) {
      snprintf(buf,len,"%02d:%02d:%02d",h,m,t); 
    } else {
      snprintf(buf,len,"%02d:%02d",m,t); 
    }
}

/******************************************
* NEXTION CODE printh 23 02 54 00
******************************************/
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
  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    lastWiFiConnect=now;  // Not used at the moment
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

    server.handleClient();         // Web handling
    myNex.NextionListen();
  } else {
    if ((WiFi.status() != WL_CONNECTED) && ((unsigned long)(now - previousWifiAttempt) > WIFI_RETRY_INTERVAL)) {//every 20 sec
      //digitalWrite(SLED, HIGH);   // Turn the Status Led off
      WiFi.disconnect();
      //WiFi.reconnect();
      WiFi.begin(cfg.wifi_ssid1, cfg.wifi_password1);
      previousWifiAttempt = now;
    } 
  } 
  //delay(update_interval_loop); // 25 ms (short because of tilt) (1.5 degrees in 25 ms)
}
