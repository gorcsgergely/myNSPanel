#include "config.h"
#include "crc.h"

/*****************************
 *  EEPROM MAP
 *
 *  CONFIGURATION | CRC | POSITION | TILT | CRC
 *
****************************/

struct shutter_position {
    int position;
    int tilt;
} p;


/**** Calculation of CRC of stored values ****/ 

unsigned long eeprom_crc(int o,int s) {
  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;
  byte value;

  for (int index = 0 ; index < s  ; ++index) {
    value = EEPROM.read(o+index);
    crc = crc_table[(crc ^ value) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (value >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

void openMemory() {
    EEPROM.begin(sizeof(configuration)+sizeof(unsigned long)+sizeof(shutter_position)+sizeof(unsigned long));
}


/*********************************************
 * save cfg structure + CRC to eeprom (flash)
**********************************************/
void saveConfig() {
  EEPROM.put(0,cfg);
  EEPROM.commit();
  unsigned long check=eeprom_crc(0,sizeof(configuration));
  EEPROM.put(sizeof(configuration),check);
  EEPROM.commit();  
}

/******************************************************
 * load eeprom (flash) to cfg structure and checks CRC
*******************************************************/
boolean loadConfig() {
  unsigned long check1;
  unsigned long check2;
  
  EEPROM.get(0,cfg);  //read config to cfg
  EEPROM.get(sizeof(configuration),check2); //read crc
  check1=eeprom_crc(0,sizeof(configuration)); //calculate crc on data
  if (check1!=check2) {  //compare calculated crc with stored crc
    defaultConfig(&cfg);
    saveConfig();
    //crcStatus+="CRC config failed. ";
    return false;
  } else {
    //crcStatus+="CRC config OK! ";
    return true;
  }
}

/*************************************************
Load hardcoded default config to cfg structure
*************************************************/
void defaultConfig(configuration* c) {
  strncpy(c->host_name,_host_name_,24);
  strncpy(c->wifi_ssid1,_ssid1_,24);
  strncpy(c->wifi_password1,_password1_,24);
  strncpy(c->mqtt_server,_mqtt_server_,24);
  strncpy(c->mqtt_user,_mqtt_user_,24);
  strncpy(c->mqtt_password,_mqtt_password_,24);
  strncpy(c->publish_position,_publish_position_,49);
  strncpy(c->subscribe_command,_subscribe_command_,49);
  strncpy(c->subscribe_position,_subscribe_position_,49);
  strncpy(c->subscribe_calibrate,_subscribe_calibrate_,49);
  strncpy(c->subscribe_reset,_subscribe_reset_,49);
  strncpy(c->subscribe_reboot,_subscribe_reboot_,49);
  for (int i=0; i<NUMBER_OF_BLINDS;i++)
  {
    strncpy(c->blind_names[i], _blind_names_[i],16);
  }
}

void copyConfig(configuration* from,configuration* to) {
  strncpy(to->host_name,from->host_name,24);
  strncpy(to->wifi_ssid1,from->wifi_ssid1,24);
  strncpy(to->wifi_password1,from->wifi_password1,24);
  strncpy(to->mqtt_server,from->mqtt_server,24);
  strncpy(to->mqtt_user,from->mqtt_user,24);
  strncpy(to->mqtt_password,from->mqtt_password,24);
  strncpy(to->publish_position,from->publish_position,49);
  strncpy(to->subscribe_command,from->subscribe_command,49);
  strncpy(to->subscribe_position,from->subscribe_position,49);
  strncpy(to->subscribe_calibrate,from->subscribe_calibrate,49);
  strncpy(to->subscribe_reset,from->subscribe_reset,49);
  strncpy(to->subscribe_reboot,from->subscribe_reboot,49);
  for (int i=0; i<NUMBER_OF_BLINDS;i++)
  {
    strncpy(to->blind_names[i], from->blind_names[i],16);
  }
  strncpy(to->publish_tilt,from->publish_tilt,49);
  strncpy(to->subscribe_tilt,from->subscribe_tilt,49);
}
