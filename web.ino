#ifdef _WEB_
#include "web.h"
#include "config.h"

void handleRootPath() {            //Handler for the rooth path
   
  String html = MAIN_page;
  server.send(200, "text/html", html);
  
}

void handleTftUploadPath() {            //Handler for the rooth path
   
  String html = TFTUPLOAD_page;
  server.send(200, "text/html", html);
  
}

void handleSuccessPath() {            //Handler for the rooth path
   
  String html = SUCCESS_page;
  server.send(200, "text/html", html);
  
}

void handleFailurePath() {            //Handler for the rooth path
   
  String html = FAILURE_page;
  server.send(200, "text/html", html);
  
}




void handleConfigurePath() {            //Handler for the rooth path
   
  String html = CONFIGURE_page;
  server.send(200, "text/html", html);
  
}

//Sends the complete data package as response to an ajax request, currently 500ms update rate
void readMain() {
  String mqttResults[10] = { F("-4: server didn't respond within the keepalive time"), F("-3: network connection was broken"), F("-2: network connection failed"), F("-1: client is disconnected cleanly"), F("0: client is connected"), F("1: server doesn't support the requested version of MQTT"), F("2: server rejected the client identifier"), F("3: server was unable to accept the connection"), F("4: username/password were rejected"), F("5: client was not authorized to connect") };
  char buf1[25];
  char buf2[25];
  
  DynamicJsonDocument root(500);   // normal 464

  JsonArray keys = root.createNestedArray("keys");
  keys.add("Pressed");
  keys.add(String(5));
 
  root["device"]=String(cfg.host_name);
  root["tilting"]=cfg.tilt?"true":"false";
  root["wifi"]=String(WiFi.SSID());  
  //root["mqtt"]= mqttResults[mqttClient.state()+4];

  timeDiff(buf1,25,lastWiFiDisconnect);
  timeDiff(buf2,25,lastMQTTDisconnect);
  root["disconnect"]=((lastWiFiDisconnect==0)?"N/A":(String(buf1)+" ago"))+","+((lastMQTTDisconnect==0)?"N/A":(String(buf2)+" ago"));
  timeDiff(buf1,25,0);
  root["crc"]=crcStatus + " ("+String(buf1)+" ago)";
  root["mem"]="program: "+String(ESP.getFreeSketchSpace()/1024)+" kB | heap: "+String(ESP.getFreeHeap()/1024)+" kB";
  if(lastCommand=="")
    root["mqttmsg"]= "N/A";
  else
    timeDiff(buf1,25,lastCallback);
    root["mqttmsg"]= lastCommand + " ("+String(buf1)+" ago)";
  root["strength"]=String(WifiGetRssiAsQuality(WiFi.RSSI()));
  root["ip"]=WiFi.localIP().toString();
  
  if(lastUpdate==0) {
    root["update"]="N/A";
  } else {
    timeDiff(buf1,25,lastUpdate);
    root["update"]=String(buf1);
  }

  String out;
  serializeJson(root,out);
#ifdef DEBUG_updates
  Serial.println(out);
#endif  
  server.send(200, "text/plane", out); //Send values to client ajax request
}


void pressButton() {
  String t_state = server.arg("button"); //Refer  request.open("GET", "pressButton?button="+button, true);
  int btn=t_state.toInt();
  #ifdef DEBUG
    Serial.print("Web button: ");
    Serial.println(t_state);
  #endif

 //1,2 - pushed
 //11,12 - released
 switch(btn) {
  case 1:
   break;
  case 2:
    break;
  case 3: break;
  case 4: break;
  case 11:
    break;
  case 12:
    break;
  case 13: break;
  case 14:break;
  case 55:
    break;
  case 66:
    Restart();
    break;
  case 77:
    defaultConfig(&web_cfg);
    break;
  case 88:
    copyConfig(&web_cfg,&cfg);
    saveConfig();    
    Restart();
    break;
 }
 server.send(200, "text/plane", t_state); //Send web page
}

void updateConfig() {

StaticJsonDocument<384> doc;

  if (server.hasArg("plain") == false) {
    return;
  }
  String body = server.arg("plain");

DeserializationError error = deserializeJson(doc, body);

if (error) {
 // Serial.print("deserializeJson() failed: ");
 // Serial.println(error.c_str());
  return;
}
//server.send(200, "application/json", "{}");
JsonArray blinds = doc["blinds"];
for (int i=0; i<sizeof(blinds); i++)
{
  const char* blind_name=blinds[i];
  strncpy(web_cfg.blind_names[i], blind_name,15);
  lastCommand+=blind_name;
}

server.send(200, "application/json", "{}");
/*for (JsonObject blind : doc["blinds"].as<JsonArray>()) {

  const char* blind_name = blind["name"]; // "blind10123456789", "blind20123456789", "blind30123456789", ...
  //int blind_pos = blind["pos"]; // 100, 100, 100, 100, 100, 100, 100
  //int blind_tilt = blind["tilt"]; // 90, 90, 90, 90, 90, 90, 90
  lastCommand+=blind_name;
}*/
  //server.send(200, "application/json", "{}");
}

void updateField() {
 String t_field = server.arg("field");
 String t_value = server.arg("value");
 String t_param;
 if (t_field.equals("host_name")) {
    strncpy(web_cfg.host_name,t_value.c_str(),24);
 } else if (t_field.equals("auto_hold_buttons")) {
    web_cfg.auto_hold_buttons = t_value.equals("true");  
 } else if (t_field.equals("tilt")) {
    web_cfg.tilt = t_value.equals("true");
 } else if (t_field.equals("wifi_multi")) {
    web_cfg.wifi_multi = t_value.equals("true");
 } else if (t_field.equals("wifi_ssid1")) {
    strncpy(web_cfg.wifi_ssid1,t_value.c_str(),24);
 } else if (t_field.equals("wifi_password1")) {
    strncpy(web_cfg.wifi_password1,t_value.c_str(),24);
 } else if (t_field.equals("wifi_ssid2")) {
    strncpy(web_cfg.wifi_ssid2,t_value.c_str(),24);
 } else if (t_field.equals("wifi_password2")) {
    strncpy(web_cfg.wifi_password2,t_value.c_str(),24);
 } else if (t_field.equals("mqtt_server")) {
    strncpy(web_cfg.mqtt_server,t_value.c_str(),24);
 } else if (t_field.equals("mqtt_user")) {
    strncpy(web_cfg.mqtt_user,t_value.c_str(),24);
 } else if (t_field.equals("mqtt_password")) {
    strncpy(web_cfg.mqtt_password,t_value.c_str(),24);
 } else if (t_field.equals("publish_position1")) {
    strncpy(web_cfg.publish_position,t_value.c_str(),49);
 } else if (t_field.equals("publish_tilt1")) {
    strncpy(web_cfg.publish_tilt,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_command1")) {
    strncpy(web_cfg.subscribe_command,t_value.c_str(),49);
 }  else if (t_field.equals("subscribe_position1")) {
    strncpy(web_cfg.subscribe_position,t_value.c_str(),49);
 }  else if (t_field.equals("subscribe_tilt1")) {
    strncpy(web_cfg.subscribe_tilt,t_value.c_str(),49);
 }  else if (t_field.equals("subscribe_calibrate")) {
    strncpy(web_cfg.subscribe_calibrate,t_value.c_str(),49);
 } else if (t_field.equals("subscribe_reboot")) {
    strncpy(web_cfg.subscribe_reboot,t_value.c_str(),49);
 } 
 /* else if (t_field.equals("blind_names")) 
  {
    t_param = server.arg("param");
    strncpy(web_cfg.blind_names[t_param.toInt()], t_value.c_str(),16);
  }*/
 else if (t_field.equals("subscribe_reset")) {
    strncpy(web_cfg.subscribe_reset,t_value.c_str(),49);   
 } else if (t_field.equals("Shutter1_duration_down")) {
    web_cfg.Shutter1_duration_down=constrain(t_value.toInt(),0,120000);
 }  else if (t_field.equals("Shutter1_duration_up")) {
    web_cfg.Shutter1_duration_up=constrain(t_value.toInt(),0,120000);
 } else if (t_field.equals("Shutter1_duration_tilt")) {
    web_cfg.Shutter1_duration_tilt=constrain(t_value.toInt(),0,120000);
 } 
 server.send(200, "text/plane", t_field); //Send web page
 server.send(200, "text/plane", t_value); //Send web page  
}

void readConfig() {
  
  DynamicJsonDocument root(1500);   // normal 1279 (but can configure longer strings, so leave it)
  root["host_name"] = web_cfg.host_name;
  root["tilt"] = web_cfg.tilt?"true":"false";
  root["auto_hold_buttons"] = web_cfg.auto_hold_buttons?"true":"false";
  root["wifi_ssid1"] = web_cfg.wifi_ssid1;
  root["wifi_password1"] = web_cfg.wifi_password1;
  root["wifi_ssid2"] = web_cfg.wifi_ssid2;
  root["wifi_password2"] = web_cfg.wifi_password2;
  root["wifi_multi"] = web_cfg.wifi_multi?"true":"false";
  root["mqtt_server"] = web_cfg.mqtt_server;
  root["mqtt_user"] = web_cfg.mqtt_user;
  root["mqtt_password"] = web_cfg.mqtt_password;
  root["publish_position1"] = web_cfg.publish_position;
  root["publish_tilt1"] = web_cfg.publish_tilt;
  root["subscribe_command1"] = web_cfg.subscribe_command;
  root["subscribe_position1"] = web_cfg.subscribe_position;
  root["subscribe_tilt1"] = web_cfg.subscribe_tilt;
  root["subscribe_calibrate"] = web_cfg.subscribe_calibrate;
  root["subscribe_reboot"] = web_cfg.subscribe_reboot;
  root["subscribe_reset"] = web_cfg.subscribe_reset;

  JsonArray blinds = root.createNestedArray("blind_names");
  for (int i=0; i<NUMBER_OF_BLINDS;i++){
   blinds.add(web_cfg.blind_names[i]);
  }
  
  root["Shutter1_duration_down"] = web_cfg.Shutter1_duration_down;
  root["Shutter1_duration_up"] = web_cfg.Shutter1_duration_up;
  root["Shutter1_duration_tilt"] = web_cfg.Shutter1_duration_tilt;
  
  String out;
  serializeJson(root,out);
#ifdef DEBUG_updates
  Serial.println(out);
#endif  
  server.send(200, "text/plane", out); //Send values to client ajax request
}
#endif