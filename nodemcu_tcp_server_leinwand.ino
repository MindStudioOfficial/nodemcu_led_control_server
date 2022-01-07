String deviceName = "LED Control Leinwand";
String deviceType = "ledv1";

#include <EEPROM.h>

// TCP ===================

#include <ESP8266WiFi.h>

#ifndef STASSID
#define STASSID "heimwehLAN"
#define STAPSK "97631074249600999423"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

// UDP ====================

#include <WiFiUdp.h>

unsigned int udpPort = 4301;
WiFiUDP udp;

IPAddress localIP;

// LEDS ===================

#include <FastLED.h>

#define NUM_LEDS 142
#define NUM_LEDS2 141
#define DATA_PIN D1  // im Schrank
#define DATA_PIN2 D2 // Tisch

#define MOD_RGB 0
#define MOD_HSV 1
#define MOD_FULLRAINBOW 2
#define MOD_CIRCLERAINBOW 3
#define MOD_STRIPES 4

CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS2];

WiFiServer wifiServer(4300);

uint8_t modus = 0; // 0 = SOLID RGB | 1 = SOLID HSV | 2 = fullrainbow | 3 = circlerainbow | 4 = stripes

int8_t direction = 1;
uint8_t o = 0;
uint8_t speed = 1;

uint8_t cci = 0;   // 0 no cc | 1 ledstrip | 2 pixelstring
uint8_t tempi = 0; // 0 uncorrected | 1 tungsten40w | 2 tungsten100w | 3 halogen | 4 carbonarc | 5 highnoonsun | 6 directsun | 7 overcast | 8 clearbluesky

uint8_t brightness = 255;

CRGB c1 = CRGB(0, 0, 0);
CRGB c2 = CRGB(0, 0, 0);

void setup()
{
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PIN2, GRB>(leds2, NUM_LEDS2);

  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setTemperature(HighNoonSun);
  FastLED.setBrightness(255);

  Serial.begin(115200);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  c1 = CRGB(0, 0, 255);
  setFullColor(c1);
  update();

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.println(".");
  }

  Serial.print("Connected to WiFi. IP:");
  localIP = WiFi.localIP();
  Serial.println(localIP);
  wifiServer.begin();
  c1 = CRGB(128, 128, 128);
  readStateEEPROM(24);
  update();
  updateCC();
  updateTemp();
  FastLED.setBrightness(brightness);

  udp.begin(udpPort);

  Serial.print("Listening to UDP on ");
  Serial.println(udpPort);
}

WiFiClient client;

void loop()
{
  //#region [rgba(255,0,0,0.05)]
  int packetSize = udp.parsePacket();
  if (packetSize > 0)
  {
    IPAddress hostIP = udp.remoteIP();
    unsigned int hostPort = udp.remotePort();
    Serial.print("New Packet from ");
    Serial.print(hostIP);
    Serial.print(":");
    Serial.print(hostPort);
    Serial.print(" ");
    Serial.print(packetSize);
    Serial.println(" Bytes");

    String message = "";
    for (int i = 0; i < packetSize; i++)
    {
      char c = udp.read();
      message += c;
    }
    Serial.println(message);
    if (message == "getIoTs") // send IOT device info on request
    {
      udp.beginPacket(hostIP, hostPort);
      char headerbuf[3] = {'I', 'O', 'T'};
      char ipbuf[6] = {'I', 'P', localIP[0], localIP[1], localIP[2], localIP[3]};

      uint8_t namel = deviceName.length();
      char nheadbuf[5] = {'N', 'A', 'M', 'E', namel};

      char namebuf[namel + 1];
      deviceName.toCharArray(namebuf, namel + 1);

      uint8_t typl = deviceType.length();
      char theadbuf[4] = {'T', 'Y', 'P', typl};
      char typbuf[typl + 1];
      deviceType.toCharArray(typbuf, typl + 1);

      udp.write(headerbuf, 3);
      udp.write(ipbuf, 6);
      udp.write(nheadbuf, 5);
      udp.write(namebuf, namel + 1);
      udp.write(theadbuf, 4);
      udp.write(typbuf, typl + 1);
      if (udp.endPacket() > 0)
        Serial.println("Sent IOTINFO");
    }
  }
  //#endregion
  //#region [rgba(0,255,128,0.05)]
  client = wifiServer.available();
  if (client)
  {
    Serial.println("New Client");
    while (client.connected())
    {
      if (client.available() > 0)
      {
        String commandString = "";
        while (client.available() > 0)
        {
          char c = client.read();
          Serial.write(c);
          commandString += c;
        }
        Serial.println("");
        delay(10);
        if (commandString.length() < 3)
          break;

        int start = commandString.indexOf("<");
        int end = commandString.indexOf(">");
        if (start < 0 || end < 0)
          break;

        String command = commandString.substring(start + 1, end);

        int a = countSplitCharacters(command, ',') + 1;
        String arguments[a];

        for (int i = 0; i < a; i++)
        {
          arguments[i] = getValue(command, ',', i);
        }
        if (arguments[0] == "fullrgb" && a == 4)
        {
          c1 = CRGB(arguments[1].toInt(), arguments[2].toInt(), arguments[3].toInt());
          modus = MOD_RGB;
          update();
        }
        if (arguments[0] == "fullhsv" && a == 4)
        {
          c1.setHSV(arguments[1].toInt(), arguments[2].toInt(), arguments[3].toInt());
          modus = MOD_HSV;
          update();
        }
        if (arguments[0] == "fullrainbow")
        {
          modus = MOD_FULLRAINBOW;
          writeStateEEPROM();
        }
        if (arguments[0] == "circlerainbow" && a == 3)
        {
          modus = MOD_CIRCLERAINBOW;
          if (arguments[1] == "left")
          {
            direction = 1;
          }
          if (arguments[1] == "right")
          {
            direction = -1;
          }
          speed = arguments[2].toInt();
          writeStateEEPROM();
        }
        if (arguments[0] == "stripes" && a == 9)
        {
          modus = 4;
          c1 = CRGB(arguments[1].toInt(), arguments[2].toInt(), arguments[3].toInt());
          c2 = CRGB(arguments[5].toInt(), arguments[6].toInt(), arguments[7].toInt());
          setStripes(c1, arguments[4].toInt(), c2, arguments[8].toInt(), 0);
          update();
        }
        if (arguments[0] == "settemp" && a == 2)
        {
          tempi = arguments[1].toInt();
          updateTemp();
          writeStateEEPROM();
        }
        if (arguments[0] == "setcc" && a == 2)
        {
          cci = arguments[1].toInt();
          updateCC();
          writeStateEEPROM();
        }
        if (arguments[0] == "setbright" && a == 2)
        {
          brightness = arguments[1].toInt();
          FastLED.setBrightness(brightness);
          writeStateEEPROM();
        }
        if (arguments[0] == "getstate")
        {
          String m = "{\"iotstate\":{\"c1\":[" + String(c1.r) + "," + String(c1.g) + "," + String(c1.b) +
                     "],\"c2\":[" + String(c2.r) + "," + String(c2.g) + "," + String(c2.b) +
                     "],\"mode\":" + String(modus) +
                     ",\"direction\":" + String(direction) +
                     ",\"speed\":" + String(speed) +
                     ",\"cc\":" + String(cci) +
                     ",\"temp\":" + String(tempi) +
                     ",\"brightness\":" + String(brightness) +
                     "}}";

          client.println(m);
        }
      }
      animate();
    }
  }
  //#endregion
  animate();
}

uint8_t hue = 0;

void updateCC()
{
  switch (cci)
  {
  case 0:
    FastLED.setCorrection(UncorrectedColor);
    break;
  case 1:
    FastLED.setCorrection(TypicalLEDStrip);
    break;
  case 2:
    FastLED.setCorrection(TypicalPixelString);

  default:
    FastLED.setCorrection(UncorrectedColor);
    break;
  }
}

void updateTemp()
{
  switch (tempi)
  {
  case 0:
    FastLED.setTemperature(UncorrectedTemperature);
    break;
  case 1:
    FastLED.setTemperature(Tungsten40W);
    break;
  case 2:
    FastLED.setTemperature(Tungsten100W);
    break;
  case 3:
    FastLED.setTemperature(Halogen);
    break;
  case 4:
    FastLED.setTemperature(CarbonArc);
    break;
  case 5:
    FastLED.setTemperature(HighNoonSun);
    break;
  case 6:
    FastLED.setTemperature(DirectSunlight);
    break;
  case 7:
    FastLED.setTemperature(OvercastSky);
    break;
  case 8:
    FastLED.setTemperature(ClearBlueSky);
    break;

  default:
    FastLED.setTemperature(UncorrectedTemperature);
    break;
  }
}

void readStateEEPROM(int len)
{
  if (len > 512 || len < 21)
  {
    return;
  }
  Serial.println("Reading from EEPROM");
  EEPROM.begin(512); // CONTENT: L E D S T A T E mode r1 g1 b1 r2 g2 b2 dir speed cc temp bright

  char content[len];
  for (int i = 0; i < len; i++)
  {
    content[i] = EEPROM.read(i);
    Serial.println(content[i], DEC);
    delay(10);
  }

  EEPROM.end();
  if (String(content).substring(0, 8) != "LEDSTATE")
  {
    Serial.println("INVALID EEPROM DATA");
    writeStateEEPROM();
  }
  else
  {
    if (content[8] == 0)
    {
      modus = (uint8_t)content[9];
      c1 = CRGB((uint8_t)content[10], (uint8_t)content[11], (uint8_t)content[12]);
      c2 = CRGB((uint8_t)content[13], (uint8_t)content[14], (uint8_t)content[15]);
      direction = (int)content[16];
      speed = (uint8_t)content[17];
      cci = (uint8_t)content[18];
      tempi = (uint8_t)content[19];
      brightness = (uint8_t)content[20];
    }
  }
}

void writeStateEEPROM()
{
  Serial.println("Writing to EEPROM");
  EEPROM.begin(512); // CONTENT: L E D S T A T E 0 mode r1 g1 b1 r2 g2 b2 dir speed cci tempi brightness

  char data[] = {'L', 'E', 'D', 'S', 'T', 'A', 'T', 'E', 0, modus, c1.r, c1.g, c1.b, c2.r, c2.g, c2.b, direction, speed, cci, tempi, brightness};

  for (int i = 0; i < sizeof(data); i++)
  {
    EEPROM.write(i, data[i]);
  }

  EEPROM.end();
}

void animate()
{
  if (modus == 2)
  { // RAINBOW
    setFullHSV(hue, 255, 255);
    hue++;
    update();
  }
  if (modus == 3)
  { // CIRCLERAINBOW
    float hue2 = o * direction;
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CHSV(round(hue2), 255, 255);
      hue2 += 255.0 / (NUM_LEDS);
    }
    hue2 = o * direction;
    for (int i = NUM_LEDS2 - 1; i >= 0; i--)
    {
      leds2[i] = CHSV(round(hue2), 255, 255);
      hue2 += 255.0 / (NUM_LEDS2);
    }
    o += speed;
    update();
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++)
  {
    if (data.charAt(i) == separator || i == maxIndex)
    {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int countSplitCharacters(String text, char splitChar)
{
  int returnValue = 0;
  int index = 0;

  while (index > -1)
  {

    index = text.indexOf(splitChar, index + 1);

    if (index > -1)
      returnValue += 1;
  }

  return returnValue;
}

void setFullRGB(int r, int g, int b)
{
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  fill_solid(leds2, NUM_LEDS2, CRGB(r, g, b));
}

void setFullHSV(int h, int s, int v)
{
  fill_solid(leds, NUM_LEDS, CHSV(h, s, v));
  fill_solid(leds2, NUM_LEDS2, CHSV(h, s, v));
}

void setFullColor(CRGB c)
{
  fill_solid(leds, NUM_LEDS, c);
  fill_solid(leds2, NUM_LEDS2, c);
}

void update()
{
  if (modus == MOD_RGB)
  {
    setFullColor(c1);
  }
  FastLED.show();
  writeStateEEPROM();
}

void setStripes(CRGB c1, int l1, CRGB c2, int l2, int offset)
{
  int iLED = 0;
  while (iLED < (NUM_LEDS + NUM_LEDS2))
  {

    for (int i = 0; i < l1; i++)
    {
      if (iLED <= NUM_LEDS)
        leds[iLED] = c1;
      if (iLED > NUM_LEDS)
        leds2[iLED - NUM_LEDS] = c1;
      iLED++;
    }
    for (int i = 0; i < l2; i++)
    {
      if (iLED <= NUM_LEDS)
        leds[iLED] = c2;
      if (iLED > NUM_LEDS)
        leds2[iLED - NUM_LEDS] = c2;
      iLED++;
    }
  }
}
