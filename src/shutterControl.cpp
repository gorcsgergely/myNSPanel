#include "shutterControl.h"
#include "config.h"
#include "printf.h"
#include <PubSubClient.h>

shutterControl::shutterControl(PubSubClient* mqttClient){
    mqttBroker = mqttClient;
}

void shutterControl::setCoverPosition(int selected_cover, int position){
    char topic[40];
    char payload[10];
    snprintf(topic,50,"blinds/%s/position", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%d",position);
    mqttBroker->publish(topic, payload , false);
}

void shutterControl::setCoverTilt(int selected_cover, int tilt){
    char topic[40];
    char payload[10];
    snprintf(topic,40,"blinds/%s/tilt", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%d",tilt);
    mqttBroker->publish(topic, payload , false);
}

void shutterControl::openCover(int selected_cover){
    char topic[40];
    char payload[10];
    snprintf(topic,40,"blinds/%s/set", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%s","open");
    mqttBroker->publish(topic, payload , false);
}

void shutterControl::closeCover(int selected_cover){
    char topic[40];
    char payload[10];
    snprintf(topic,40,"blinds/%s/set", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%s","close");
    mqttBroker->publish(topic, payload , false);
}

void shutterControl::stopCover(int selected_cover){
    char topic[40];
    char payload[10];
    snprintf(topic,40,"blinds/%s/set", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%s","stop");
    mqttBroker->publish(topic, payload , false);
}

void shutterControl::setScene(int selected_scene){
    char topic[40];
    char payload[10];
    snprintf(topic,50,"blinds/scene");
    snprintf(payload,10,"%d",selected_scene);
    mqttBroker->publish(topic, payload , false);
}