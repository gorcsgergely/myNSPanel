#ifndef CONFIG_H
#define CONFIG_H

#define _WEB_ 1


// Constants (default values, stored to EEPROM during the first run, can be configured from the web interface)
char _host_name_[] = "NSPanel";  // Has to be unique for each device

//comment for covers with no tilting blades
#define _tilt_ 1

// filters out noise (100 ms)
boolean button_press_delay = true;   
#define _button_delay_ 100  
#define NUMBER_OF_BLINDS 7

//MQTT parameters (has to be unique for each device)
char _publish_position_[] = "blinds/cover1/state";
char _publish_tilt_[] = "blinds/cover1/tilt-state";
char _subscribe_command_[] = "blinds/cover1/set";
char _subscribe_position_[] = "blinds/cover1/position";
char _subscribe_tilt_[] = "blinds/cover1/tilt";
char _blind_names_[NUMBER_OF_BLINDS][16] ={"blind1","blind2","blind3","blind4","blind5","blind6","blind7"};

char _subscribe_calibrate_[] = "blinds/cover/calibrate";
char _subscribe_reset_[] = "blinds/cover/reset";
char _subscribe_reboot_[] = "blinds/cover/reboot";

// Time for each rolling shutter to go down and up - you need to measure this and configure - default values - can be changed via web (if enabled)
#define _Shutter1_duration_down_ 51200
#define _Shutter1_duration_up_ 51770
#define _Shutter1_duration_tilt_ 1650

//#define _reverse_position_mapping_ 1
#define _auto_hold_buttons_ 1

char payload_open[] = "open";
char payload_close[] = "close";
char payload_stop[] = "stop";


// Change these for your WIFI, IP and MQTT
//char _ssid1_[] = "GG-2.4G";
//char _password1_[] = "FhmX8rjjezmd";
char _ssid1_[] = "home";
char _password1_[] = "12345678";
char WEB_UPGRADE_USER[] = "admin";
char WEB_UPGRADE_PASS[] = "admin";
//char OTA_password[] = "admin"; // Only accepts [a-z][A-Z][0-9]
char _mqtt_server_[] = "192.168.0.89";
char _mqtt_user_[] = "user";
char _mqtt_password_[] = "pass";

#define WIFI_RETRY_INTERVAL 20000
#define MQTT_RETRY_INTERVAL 10000


char movementUp[] ="up";
char movementDown[] = "down";
char movementStopped[] = "stopped";

#define update_interval_loop 50
#define update_interval_active 1000
#define update_interval_passive 300000

//Ignore pulses shorter than 100ms
#define _button_delay_ 100


/********************************
SONOFF DUAL R3 PCB ver 1.x, 2.x
GPIO13	Status LED (blue/inverted)
GPIO00	Push Button (inverted)
GPIO27	Relay 1 / LED 1 (red)
GPIO14	Relay 2 / LED 2 (red)
GPIO32	Switch 1 (inverted)
GPIO33	Switch 2 (inverted)
GPIO25	power sensor UART Tx
GPIO26	power sensor UART Rx
**********************************/

// Relay GPIO ports

#define GPIO_DISPLAY_INVERTED 4  // r1 up wire

#define GPIO_REL1 22  // Relay1
#define GPIO_REL2 19  // Relay2

#define GPIO_BUZZER 21  // buzer
#define GPIO_USER 23  // buzer

// Buttons GPIO ports
#define GPIO_KEY1 14  // Button1
#define GPIO_KEY2 27  // Button2

#define GPIO_ADCTEMP 38  // ADC Temp

#define KEY_PRESSED  LOW

struct configuration {
  boolean tilt;
  boolean reverse_position_mapping; // currently not used
  boolean auto_hold_buttons;
  char host_name[25];
  char wifi_ssid1[25];
  char wifi_password1[25];
  char wifi_ssid2[25];
  char wifi_password2[25];
  boolean wifi_multi;
  char mqtt_server[25];
  char mqtt_user[25];
  char mqtt_password[25];
  char publish_position[50];
  char publish_tilt[50];
  char subscribe_command[50];
  char subscribe_position[50];
  char subscribe_tilt[50]; 
  char subscribe_calibrate[50];
  char subscribe_reboot[50];
  char subscribe_reset[50]; // currently not used
  char blind_names[NUMBER_OF_BLINDS][16];
  unsigned long Shutter1_duration_down;
  unsigned long Shutter1_duration_up;
  unsigned long Shutter1_duration_tilt;
};

#endif