#include "config.h"


void setup() {
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812B, DATA_PIN2, GRB>(leds2, NUM_LEDS2);

  readStateEEPROM(24);
  setRGBfromC1();
  updateCC();
  updateTemp();
  FastLED.setBrightness(prop_brightness);

  Serial.begin(115200);

  // === CONNECTING TO WIFI ===

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("Connecting");
  // wait for connectiob
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.print("Connected to WiFi. IP:");

  // ==========================

  localIP = WiFi.localIP();
  Serial.println(localIP);
  wifiServer.begin();

  udp.begin(udpPort);

  Serial.print("Listening to UDP on ");
  Serial.println(udpPort);
}

WiFiClient client;

void loop() {
  //#region [rgba(255,0,0,0.05)]
  int packetSize = udp.parsePacket();
  if (packetSize > 0) {
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
    for (int i = 0; i < packetSize; i++) {
      char c = udp.read();
      message += c;
    }
    Serial.println(message);
    if (message == "getIoTs") // send IOT device info on request
    {
      udp.beginPacket(hostIP, hostPort);
      char headerbuf[3] = { 'I', 'O', 'T' };
      char ipbuf[6] = { 'I', 'P', localIP[0], localIP[1], localIP[2], localIP[3] };

      uint8_t namel = deviceName.length();
      char nheadbuf[5] = { 'N', 'A', 'M', 'E', namel };

      char namebuf[namel + 1];
      deviceName.toCharArray(namebuf, namel + 1);

      uint8_t typl = deviceType.length();
      char theadbuf[4] = { 'T', 'Y', 'P', typl };
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
  client = wifiServer.accept();
  if (client) {
    Serial.println("====== New Client ======");
    Serial.println(client.localIP());
    sendHelpMsg();

    unsigned long previousMillis = 0;
    const long interval = 5000;

    while (client.connected()) {
      // * Sends !heartbeat! every 5s
      unsigned long currentMillis = millis();
      if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;
        client.println("!heartbeat!");
        Serial.println("!heartbeat!");
      }
      // * Handle Incoming message
      if (client.available() > 0) {
        String commandString = "";
        // read client message to command string and print to serial
        while (client.available() > 0) {
          char c = client.read();
          commandString += c;
        }
        String rawCommandString = commandString;
        rawCommandString.replace("\n", "\\n");
        rawCommandString.replace("\t", "\\t");
        rawCommandString.replace("\r", "\\r");

        Serial.printf("Received New Command: \"%s\"\n", rawCommandString);

        delay(10);

        // check if message has command format <a0,a1,a2,a3>        

        int start = commandString.indexOf("<");
        int end = commandString.indexOf(">");
        if (commandString.length() < 3 || start < 0 || end < 0) {
          Serial.printf("\"%s\" is not a valid command. Use \"<cmd,a1,a2,...>\" as a command format.\n", rawCommandString);
          client.printf("\"%s\" is not a valid command. Use \"<cmd,a1,a2,...>\" as a command format.\n", rawCommandString);
          continue; // don't close connection, 
        }

        String command = commandString.substring(start + 1, end);

        int argCount = countSplitCharacters(command, ',') + 1;
        String arguments[argCount];

        for (int i = 0; i < argCount; i++) {
          arguments[i] = getValue(command, ',', i);
        }
        if (arguments[0] == "fullrgb" && argCount == 4) {
          prop_color_1 = CRGB(arguments[1].toInt(), arguments[2].toInt(), arguments[3].toInt());
          prop_modus = MOD_RGB;
          writeStateEEPROM();
          setRGBfromC1();
          updateLEDsDelayed();
          Serial.printf("set full rgb (%d, %d, %d)\n", prop_color_1.r, prop_color_1.g, prop_color_1.b);
          client.printf("set full rgb (%d, %d, %d)\n", prop_color_1.r, prop_color_1.g, prop_color_1.b);
        }
        if (arguments[0] == "fullhsv" && argCount == 4) {
          prop_color_1.setHSV(arguments[1].toInt(), arguments[2].toInt(), arguments[3].toInt());
          prop_modus = MOD_HSV;
          writeStateEEPROM();
          setRGBfromC1();
          updateLEDsDelayed();
          Serial.printf("set full hsv (rgb: %d, %d, %d)\n", prop_color_1.r, prop_color_1.g, prop_color_1.b);
          client.printf("set full hsv (rgb: %d, %d, %d)\n", prop_color_1.r, prop_color_1.g, prop_color_1.b);
        }
        if (arguments[0] == "fullrainbow") {
          prop_modus = MOD_FULLRAINBOW;
          writeStateEEPROM();
          // gets updated in [animate]
          Serial.printf("set full rainbow\n");
          client.printf("set full rainbow\n");
        }
        if (arguments[0] == "circlerainbow" && argCount == 3) {
          prop_modus = MOD_CIRCLERAINBOW;
          if (arguments[1] == "left") {
            prop_direction = 1;
          }
          if (arguments[1] == "right") {
            prop_direction = -1;
          }
          prop_speed = arguments[2].toInt();
          writeStateEEPROM();
          // gets updated in [animate]
          Serial.printf("set circle rainbow %s %d\n", prop_direction == 1 ? "left" : "right", prop_speed);
          client.printf("set circle rainbow %s %d\n", prop_direction == 1 ? "left" : "right", prop_speed);
        }
        if (arguments[0] == "stripes" && argCount == 9) {
          prop_modus = MOD_STRIPES;
          prop_color_1 = CRGB(arguments[1].toInt(), arguments[2].toInt(), arguments[3].toInt());
          prop_color_2 = CRGB(arguments[5].toInt(), arguments[6].toInt(), arguments[7].toInt());
          setStripes(prop_color_1, arguments[4].toInt(), prop_color_2, arguments[8].toInt(), 0);
          writeStateEEPROM();
          updateLEDsDelayed();
          Serial.printf("set stripes C1: (%d, %d, %d) len %d, C2: (%d, %d, %d) len %d\n", prop_color_1.r, prop_color_1.g, prop_color_1.b, arguments[4].toInt(), prop_color_2.r, prop_color_2.g, prop_color_2.b, arguments[8].toInt());
          client.printf("set stripes C1: (%d, %d, %d) len %d, C2: (%d, %d, %d) len %d\n", prop_color_1.r, prop_color_1.g, prop_color_1.b, arguments[4].toInt(), prop_color_2.r, prop_color_2.g, prop_color_2.b, arguments[8].toInt());
        }
        if (arguments[0] == "settemp" && argCount == 2) {
          prop_temp_i = arguments[1].toInt();
          writeStateEEPROM();
          updateTemp();
          updateLEDsDelayed();
          Serial.printf("set temp to variant %d\n", prop_temp_i);
          client.printf("set temp to variant %d\n", prop_temp_i);
        }
        if (arguments[0] == "setcc" && argCount == 2) {
          prop_cc_i = arguments[1].toInt();
          writeStateEEPROM();
          updateCC();
          updateLEDsDelayed();
          Serial.printf("set cc to variant %d\n", prop_cc_i);
          client.printf("set cc to variant %d\n", prop_cc_i);
        }
        if (arguments[0] == "setbright" && argCount == 2) {
          prop_brightness = arguments[1].toInt();
          writeStateEEPROM();
          FastLED.setBrightness(prop_brightness);
          updateLEDsDelayed();
          Serial.printf("set brightness to %d\n", prop_brightness);
          client.printf("set brightness to %d\n", prop_brightness);
        }
        if (arguments[0] == "getstate") {
          String m = "{\"iotstate\":{\"c1\":[" + String(prop_color_1.r) + "," + String(prop_color_1.g) + "," + String(prop_color_1.b) +
            "],\"c2\":[" + String(prop_color_2.r) + "," + String(prop_color_2.g) + "," + String(prop_color_2.b) +
            "],\"mode\":" + String(prop_modus) +
            ",\"direction\":" + String(prop_direction) +
            ",\"speed\":" + String(prop_speed) +
            ",\"cc\":" + String(prop_cc_i) +
            ",\"temp\":" + String(prop_temp_i) +
            ",\"brightness\":" + String(prop_brightness) +
            "}}";
          client.flush();
          client.println(m);
          client.flush();
          Serial.println(m);
          Serial.flush();
        }
      }
      animate();
    }
    Serial.println("====== Client Disconnect ======");
  }
  //#endregion
  animate();
}

uint8_t hue = 0;

void updateCC() {
  switch (prop_cc_i) {
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

void updateTemp() {
  switch (prop_temp_i) {
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

void readStateEEPROM(int len) {
  if (len > 512 || len < 21) {
    return;
  }
  Serial.println("Reading from EEPROM");
  EEPROM.begin(512); // CONTENT: L E D S T A T E mode r1 g1 b1 r2 g2 b2 dir speed cc temp bright

  char content[len];
  for (int i = 0; i < len; i++) {
    content[i] = EEPROM.read(i);
    Serial.println(content[i], DEC);
    delay(10);
  }

  EEPROM.end();
  if (String(content).substring(0, 8) != "LEDSTATE") {
    Serial.println("INVALID EEPROM DATA");
    writeStateEEPROM();
  } else {
    if (content[8] == 0) {
      prop_modus = (uint8_t)content[9];
      prop_color_1 = CRGB((uint8_t)content[10], (uint8_t)content[11], (uint8_t)content[12]);
      prop_color_2 = CRGB((uint8_t)content[13], (uint8_t)content[14], (uint8_t)content[15]);
      prop_direction = (int8_t)content[16];
      prop_speed = (uint8_t)content[17];
      prop_cc_i = (uint8_t)content[18];
      prop_temp_i = (uint8_t)content[19];
      prop_brightness = (uint8_t)content[20];
    }
  }
}

void writeStateEEPROM() {
  Serial.println("Writing to EEPROM");
  EEPROM.begin(512); // CONTENT: L E D S T A T E 0 mode r1 g1 b1 r2 g2 b2 dir speed cci tempi brightness

  char data[] = { 'L', 'E', 'D', 'S', 'T', 'A', 'T', 'E', 0, prop_modus, prop_color_1.r, prop_color_1.g, prop_color_1.b, prop_color_2.r, prop_color_2.g, prop_color_2.b, (char)prop_direction, prop_speed, prop_cc_i, prop_temp_i, prop_brightness };

  for (int i = 0; i < sizeof(data); i++) {
    EEPROM.write(i, data[i]);
  }

  EEPROM.end();
}

void animate() {
  if (prop_modus == MOD_FULLRAINBOW) { // RAINBOW
    setFullHSV(hue, 255, 255);
    hue++;
    FastLED.show();
  }
  if (prop_modus == MOD_CIRCLERAINBOW) { // CIRCLERAINBOW
    float hue2 = o * prop_direction;
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(round(hue2), 255, 255);
      hue2 += 255.0 / (NUM_LEDS);
    }
    hue2 = o * prop_direction;
    for (int i = NUM_LEDS2 - 1; i >= 0; i--) {
      leds2[i] = CHSV(round(hue2), 255, 255);
      hue2 += 255.0 / (NUM_LEDS2);
    }
    o += prop_speed;
    FastLED.show();
  }
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int countSplitCharacters(String text, char splitChar) {
  int returnValue = 0;
  int index = 0;

  while (index > -1) {

    index = text.indexOf(splitChar, index + 1);

    if (index > -1)
      returnValue += 1;
  }

  return returnValue;
}

void setFullRGB(int r, int g, int b) {
  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
  fill_solid(leds2, NUM_LEDS2, CRGB(r, g, b));
}

void setFullHSV(int h, int s, int v) {
  fill_solid(leds, NUM_LEDS, CHSV(h, s, v));
  fill_solid(leds2, NUM_LEDS2, CHSV(h, s, v));
}

void setFullColor(CRGB c) {
  fill_solid(leds, NUM_LEDS, c);
  fill_solid(leds2, NUM_LEDS2, c);
}

void setRGBfromC1() {
  if (prop_modus == MOD_RGB || prop_modus == MOD_HSV) {
    setFullColor(prop_color_1);
  }
}

void updateLEDsDelayed() {
  FastLED.show();
  FastLED.delay(10);
}

void updateLEDsDelayed(unsigned long millis) {
  FastLED.delay(millis);
}

void setStripes(CRGB c1, int l1, CRGB c2, int l2, int offset) {
  int iLED = 0;
  while (iLED < (NUM_LEDS + NUM_LEDS2)) {

    for (int i = 0; i < l1; i++) {
      if (iLED <= NUM_LEDS)
        leds[iLED] = c1;
      if (iLED > NUM_LEDS)
        leds2[iLED - NUM_LEDS] = c1;
      iLED++;
    }
    for (int i = 0; i < l2; i++) {
      if (iLED <= NUM_LEDS)
        leds[iLED] = c2;
      if (iLED > NUM_LEDS)
        leds2[iLED - NUM_LEDS] = c2;
      iLED++;
    }
  }
}

void sendHelpMsg() {
  Serial.printf("<getstate>\n\treturns the current state as JSON\n");
  client.printf("<getstate>\n\treturns the current state as JSON\n");

  Serial.printf("<fullrgb,r,g,b> (%d,%d,%d)\n", prop_color_1.r, prop_color_1.g, prop_color_1.b);
  client.printf("<fullrgb,r,g,b> (%d,%d,%d)\n", prop_color_1.r, prop_color_1.g, prop_color_1.b);

  Serial.printf("<fullhsv,h,s,v>\n");
  client.printf("<fullhsv,h,s,v>\n");

  Serial.printf("<fullrainbow>\n");
  client.printf("<fullrainbow>\n");

  Serial.printf("<circlerainbow,left|right,speed>\n");
  client.printf("<circlerainbow,left|right,speed>\n");

  Serial.printf("<stripes,r1,g1,b1,l1,r2,g2,b2,l2>\n");
  client.printf("<stripes,r1,g1,b1,l1,r2,g2,b2,l2>\n");

  Serial.printf("<settemp,temp> (%d)\n\t0: uncorrected\n\t1: tungsten 40W\n\t2: tungsten 100W\n\t3: halogen\n\t4: carbon arc\n\t5: high noon sun\n\t6: direct sun\n\t7: overcast\n\t8: clear blue sky\n", prop_temp_i);
  client.printf("<settemp,temp> (%d)\n\t0: uncorrected\n\t1: tungsten 40W\n\t2: tungsten 100W\n\t3: halogen\n\t4: carbon arc\n\t5: high noon sun\n\t6: direct sun\n\t7: overcast\n\t8: clear blue sky\n", prop_temp_i);

  Serial.printf("<setcc,cc> (%d)\n\t0: uncorrected\n\t1: typical LED strip\n\t2: typical pixel string\n", prop_cc_i);
  client.printf("<setcc,cc> (%d)\n\t0: uncorrected\n\t1: typical LED strip\n\t2: typical pixel string\n", prop_cc_i);

  Serial.printf("<setbright,brightness> (%d)\n\t0-255\n", prop_brightness);
  client.printf("<setbright,brightness> (%d)\n\t0-255\n", prop_brightness);
}