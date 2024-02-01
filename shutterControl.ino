#include "shutterControl.h"
#include "config.h"

void setCoverPosition(int selected_cover, int position){
    char topic[40];
    char payload[10];
    snprintf(topic,50,"blinds/%s/position", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%d",position);
   // mqttClient.publish(topic, payload , false);
}

void setCoverTilt(int selected_cover, int tilt){
    char topic[40];
    char payload[10];
    snprintf(topic,40,"blinds/%s/tilt", cfg.blind_names[selected_cover-1]);
    snprintf(payload,10,"%d",tilt);
    //mqttClient.publish(topic, payload , false);
}