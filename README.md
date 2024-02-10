# myNSPanel

myNSPanel is a custom made small stand-alone home automation system, based on Sonoff NSPanel. Intended for controlling venetian blinds with tilt, shutters and other similar covers over wifi equipped with my custom wifi relay units. It does not require Home Assistant or any other external component and is easy to repurpose for other kind of automation tasks.

The original design is for controlling up to 8 of these wifi Venetian Blind Controller devices. They are made out of the ESP32 based Shelly Plus 2PM or Sonoff Dual R3 smart relays, but can be ported to Sonoff 4CH or other ESP8266 based devices with some minor modifications. You can find the Venetian Blind Controller code in a separate repository. 

myNSPanel is a full custom development project for the EU version of Sonoff NSPanel. The heart of the Sonoff NSPanel is a ESP32-D0WD V3 with 4MB of flash memory connected to a Nextion display over serial port. There are several projects using Tasmota or ESPHome as the basis of similar applications, but this one is a completelly independent, from scratch solution. You have full control over the code, and you are not relying on any of these precompiled firmwares.

**The project has 2 main components:**
+ ESP code: developed on PlatformIO but easy to port to Arduino IDE. See instructions down below.
+ Display UI: Made with the Nextion Editor.

**Main features:**
+ Webserver to configure, see status, upload firmware and upload Nextion TFT file
+ MQTT broker to connect MQTT clients to, so you don't need Mosquito or anything else. It should be working fine with 10 clients at least.
+ mDNS
+ Arduino OTA to upload code wireless
+ Fallback SSID and password

**Here is how to get NSPanel configured:**
+ You need to set GPIO4 on the ESP32 to low to enable the display.
+ Whilst the ESP32 communicates with the display using pins 16 and 17 it is opposite to the standard way this is defined. Serial2 is normally, pin 17 is Tx and pin 16 is Rx but the ESP32 and Nextion display are wired so that pin 17 is RX and pin 16 is Tx.
+ Baudrate is 115200.

I use the Arduino IDE and EasyNextionLibrary to communicate with the display and ESPNexUpload to upload the tft file. 
Both libraries are included the project files and already modifed to swap the pins.

EasyNextionLibrary: In EasyNextionLibrary.cpp change:

```
_serial->begin(baud);
```
to
```
_serial->begin(baud, SERIAL_8N1, 17, 16);
```

ESPNexUpload: in ESPNexUpload.cpp change:

 ```
#define NEXT_RX 16 // Nextion RX pin | Default 16 
 #define NEXT_TX 17 // Nextion TX pin | Default 17
```
to
```
#define NEXT_RX 17 // Nextion RX pin | Default 16
#define NEXT_TX 16 // Nextion TX pin | Default 17
```

## Compiling it with Arduino IDE
+ Create a new folder and copy all files in /src and /include into the new folder.
+ Rename main.cpp to your_folder_name.ino
+ Select ESP32 DEV Module as your board
+ Install dependency: PicoMQTT and ArduinoJSON v7+ (Arduino OTA is not needed, it is using the out of the box library)
