#include <Arduino.h>
#ifndef CONFIG_H
#define CONFIG_H

#define _WEB_ 1

// Constants (default values, stored to EEPROM during the first run, can be configured from the web interface)
const char _host_name_[] = "NSPanel";  // Has to be unique for each device

// filters out noise (100 ms)
const boolean button_press_delay = true;   
#define _button_delay_ 100  
#define NUMBER_OF_BLINDS 7

//MQTT parameters (has to be unique for each device)
const char _publish_position_[] = "blinds/cover1/state";
const char _publish_tilt_[] = "blinds/cover1/tilt-state";
const char _subscribe_command_[] = "blinds/cover1/set";
const char _subscribe_position_[] = "blinds/cover1/position";
const char _subscribe_tilt_[] = "blinds/cover1/tilt";

extern char _blind_names_[NUMBER_OF_BLINDS][16];

const char _subscribe_calibrate_[] = "blinds/cover/calibrate";
const char _subscribe_reset_[] = "blinds/cover/reset";
const char _subscribe_reboot_[] = "blinds/cover/reboot";

const char payload_open[] = "open";
const char payload_close[] = "close";
const char payload_stop[] = "stop";


// Change these for your WIFI, IP and MQTT
//const char _ssid1_[] = "GG-2.4G";
//const char _password1_[] = "FhmX8rjjezmd";
const char _ssid1_[] = "home";
const char _password1_[] = "12345678";
const char WEB_UPGRADE_USER[] = "admin";
const char WEB_UPGRADE_PASS[] = "admin";
//char OTA_password[] = "admin"; // Only accepts [a-z][A-Z][0-9]
const char _mqtt_server_[] = "192.168.0.89";
const char _mqtt_user_[] = "user";
const char _mqtt_password_[] = "pass";

#define WIFI_RETRY_INTERVAL 20000
#define MQTT_RETRY_INTERVAL 10000

#define update_interval_loop 50
#define update_interval_active 1000
#define update_interval_passive 300000

//Ignore pulses shorter than 100ms
#define _button_delay_ 100

// Relay GPIO ports

#define GPIO_DISPLAY_INVERTED 4  //Display enable

#define GPIO_REL1 22  // Relay1
#define GPIO_REL2 19  // Relay2

#define GPIO_BUZZER 21  // buzer
#define GPIO_USER 23  // user

// Buttons GPIO ports
#define GPIO_KEY1 14  // Button1
#define GPIO_KEY2 27  // Button2

#define GPIO_ADCTEMP 38  // ADC Temp

#define KEY_PRESSED  LOW

struct configuration {
  char host_name[25];
  char wifi_ssid1[25];
  char wifi_password1[25];
  char mqtt_server[25];
  char mqtt_user[25];
  char mqtt_password[25];
  char blind_names[NUMBER_OF_BLINDS][16];
  char publish_position[50];
  char publish_tilt[50];
  char subscribe_command[50];
  char subscribe_position[50];
  char subscribe_tilt[50]; 
  char subscribe_calibrate[50];
  char subscribe_reboot[50];
  char subscribe_reset[50]; // currently not used
};

extern configuration cfg,web_cfg;

#endif