#ifndef SHUTTER_CONTROL_H
#define SHUTTER_CONTROL_H
#include <PicoMQTT.h>

class shutterControl{
    public:
        shutterControl(PicoMQTT::Server* mqttServer);
        void setCoverPosition(int selected_cover, int position);
        void setCoverTilt(int selected_cover, int tilt);
    private:
        PicoMQTT::Server* mqttBroker;

};

extern shutterControl shuttercontrol;

#endif