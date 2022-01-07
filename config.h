/*
* create a deviceconfig.h header that holds defines for device specific information
*
* #define DEVICE_NAME "My device Name"
* #define DEVICE_TYPE "ledv1"
* #define STASSID "MyWifiSSID"
* #define STAPSPK "MyWifiPassword"
* #define NUM_LEDS <number of leds in strip 1>
* #define NUM_LEDS2 <number of leds in strip 2>
* #define DATA_PIN <data pin of strip 1>
* #define DATA_PIN2 <data pin of strip 2>
*/
#include "deviceconfig.h"

#ifndef DEVICE_NAME
#error DEVICE_NAME is not defined
#endif

#ifndef DEVICE_TYPE
#error DEVICE_TYPE is not defined
#endif

#ifndef STASSID
#error STASSID is not defined
#endif

#ifndef STAPSK
#error STAPSK is not defined
#endif

#ifndef NUM_LEDS
#error NUM_LEDS is not defined
#endif

#ifndef NUM_LEDS2
#error NUM_LEDS2 is not defined
#endif

#ifndef DATA_PIN
#error DATA_PIN is not defined
#endif

#ifndef DATA_PIN2
#error DATA_PIN2 is not defined
#endif


String deviceName = DEVICE_NAME;
String deviceType = DEVICE_TYPE;

#include <EEPROM.h>

// TCP ===================

#include <ESP8266WiFi.h>

const char* ssid = STASSID;
const char* password = STAPSK;

unsigned int tcpPort = 4300;

WiFiServer wifiServer(tcpPort);

// UDP ====================

#include <WiFiUdp.h>

unsigned int udpPort = 4301;

WiFiUDP udp;

IPAddress localIP;

// LEDS ===================

#include <FastLED.h>

#define MOD_RGB 0
#define MOD_HSV 1
#define MOD_FULLRAINBOW 2
#define MOD_CIRCLERAINBOW 3
#define MOD_STRIPES 4

CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS2];

uint8_t prop_modus = 0; // 0 = SOLID RGB | 1 = SOLID HSV | 2 = fullrainbow | 3 = circlerainbow | 4 = stripes

int8_t prop_direction = 1;
uint8_t o = 0;
uint8_t prop_speed = 1;

uint8_t prop_cc_i = 0;   // 0 no cc | 1 ledstrip | 2 pixelstring
uint8_t prop_temp_i = 0; // 0 uncorrected | 1 tungsten40w | 2 tungsten100w | 3 halogen | 4 carbonarc | 5 highnoonsun | 6 directsun | 7 overcast | 8 clearbluesky

uint8_t prop_brightness = 255;

CRGB prop_color_1 = CRGB(0, 0, 0);
CRGB prop_color_2 = CRGB(0, 0, 0);
