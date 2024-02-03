#ifndef SHUTTER_CONTROL_H
#define SHUTTER_CONTROL_H
#include "config.h"

void setCoverPosition(int selected_cover, int position, configuration *cfg);
void setCoverTilt(int selected_cover, int tilt);

#endif