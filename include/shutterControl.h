#ifndef SHUTTER_CONTROL_H
#define SHUTTER_CONTROL_H
#include <PubSubClient.h> 

class shutterControl{
    public:
        //shutterControl(PicoMQTT::Server* mqttServer);
        shutterControl(PubSubClient* mqttClient);
        void setCoverPosition(int selected_cover, int position);
        void setCoverTilt(int selected_cover, int tilt);
        void openCover(int selected_cover);
        void closeCover(int selected_cover);
        void stopCover(int selected_cover);
    private:
        //PicoMQTT::Server* mqttBroker;
        PubSubClient* mqttBroker;
};

extern shutterControl shuttercontrol;

#endif