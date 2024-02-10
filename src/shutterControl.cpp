#include "shutterControl.h"
#include "config.h"
#include "printf.h"

shutterControl::shutterControl(PicoMQTT::Server* mqttServer){
    mqttBroker = mqttServer;
}

void shutterControl::setCoverPosition(int selected_cover, int position){
    char topic[40];
    char payload[10];
    snprintf(topic,50,"blinds/%s/position", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%d",position);
   // mqttClient.publish(topic, payload , false);
   mqttBroker->publish(topic,payload,0,false,0);
}

void shutterControl::setCoverTilt(int selected_cover, int tilt){
    char topic[40];
    char payload[10];
    snprintf(topic,40,"blinds/%s/tilt", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%d",tilt);
    //mqttClient.publish(topic, payload , false);
    mqttBroker->publish(topic,payload,0,false,0);
}