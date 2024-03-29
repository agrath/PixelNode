/*
This example will receive multiple universes via Artnet and control a strip of ws2811 leds via
Paul Stoffregen's excellent OctoWS2811 library: https://www.pjrc.com/teensy/td_libs_OctoWS2811.html
This example may be copied under the terms of the MIT license, see the LICENSE file for details
*/

#include <Artnet.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <OctoWS2811.h>
#include <eeprom.h>
#include <Entropy.h>

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP);
void initTest();
void getMacAddress(byte mac[]);

// OctoWS2811 settings
const int ledsPerStrip = 240; // change for your setup
const byte numStrips = 2;     // change for your setup
const int numLeds = ledsPerStrip * numStrips;
const int numberOfChannels = numLeds * 3; // Total number of channels you want to receive (1 led = 3 channels)
DMAMEM int displayMemory[ledsPerStrip * 6];
int drawingMemory[ledsPerStrip * 6];
const int config = WS2811_GRB | WS2811_800kHz;
OctoWS2811 leds(ledsPerStrip, displayMemory, drawingMemory, config);

// Artnet settings
Artnet artnet;
const int startUniverse = 0; // CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as 0.

// Check if we got all universes
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;

// Change ip and mac address for your setup
//todo: change to dhcp
//todo: add bonjour
byte ip[] = {192, 168, 2, 2};
byte broadcastMask[] = {192, 168, 2, 255};

//mac is filled by getMacAddress() and persisted to EEPROM
byte mac[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void setup()
{
  //Serial.begin(115200);
  Entropy.Initialize(); 
  //todo: begin opc listener
  //todo: config for mapping? 8ch parallel; need to know strip length and synch config and expected artnet universes
  
  //generate and store or fetch mac from eeprom
  getMacAddress(mac);
  
  artnet.setBroadcast(broadcastMask);
  artnet.begin(mac, ip);
  leds.begin();
  initTest();

  // this will be called for each packet received
  artnet.setArtDmxCallback(onDmxFrame);
}

void loop()
{
  // we call the read function inside the loop
  artnet.read();
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data, IPAddress remoteIP)
{
  sendFrame = 1;

  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0; i < maxUniverses; i++)
  {
    if (universesReceived[i] == 0)
    {
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length / 3; i++)
  {
    int led = i + (universe - startUniverse) * (previousDataLength / 3);
    if (led < numLeds)
      leds.setPixel(led, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
  }
  previousDataLength = length;

  if (sendFrame)
  {
    leds.show();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}

void initTest()
{
  for (int i = 0; i < numLeds; i++)
    leds.setPixel(i, 127, 0, 0);
  leds.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
    leds.setPixel(i, 0, 127, 0);
  leds.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
    leds.setPixel(i, 0, 0, 127);
  leds.show();
  delay(500);
  for (int i = 0; i < numLeds; i++)
    leds.setPixel(i, 0, 0, 0);
  leds.show();
}

void getMacAddress(byte mac[])
{
  //header set to OUI from GHEO Sa
  mac[0] = 0x90;
  mac[1] = 0xA2;
  mac[2] = 0xDA;
  //if we have generated and stored mac already (EEPROM begins with #)
  if (EEPROM.read(1) == '#')
  {
    for (int i = 3; i < 6; i++)
    {
      mac[i] = EEPROM.read(i-1);
    }
  }
  else
  {
    //generate a random one
    for (int i = 3; i < 6; i++)
    {
      uint8_t b = Entropy.random(WDT_RETURN_BYTE);
      mac[i] = (byte)b;
      EEPROM.write(i-1, mac[i]);
    }
    //write config marker
    EEPROM.write(1, '#');
  }
}