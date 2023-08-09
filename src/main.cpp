#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <ArduinoHA.h>

// Infomation about the LED
#define PANEL_MATRIX_WIDTH 32
#define PANEL_MATRIX_HEIGHT 8
#define PANELS_WIDTH 4 //Panels layout
#define PANELS_HEIGHT 1
#define TOTAL_LEDS (PANEL_MATRIX_HEIGHT * PANEL_MATRIX_WIDTH * PANELS_WIDTH * PANELS_HEIGHT)
#define TOTAL_WIDTH (PANEL_MATRIX_WIDTH * PANELS_WIDTH)
#define TOTAL_HEIGHT (PANEL_MATRIX_HEIGHT * PANELS_HEIGHT)#

//MQTT Information
#define BROKER_ADDR         IPAddress(192,168,1,5)
#define BROKER_USERNAME     "deviceuser" // replace with your credentials
#define BROKER_PASSWORD     "Aoshi2012"

// PIN LED is located on
#define DATA_PIN 2

// WIFI Settings
const char* ESP_HOST_NAME = "esp32-dev-1-abbycottontail";
const char* WIFI_SSID = "IANetworks";
const char* WIFI_PASSWORD = "addyandinka20092907";

//NTP Servers:
static const char ntpServerName[] = "us.pool.ntp.org";

const int timeZone = -5; //EST - Eastern Standard Time

// Initiate WifiClient
WiFiUDP Udp;
unsigned int localPort = 8888;  // local port to listen for UDP packets

WiFiClient client;
HADevice device;
HAMqtt mqtt(client, device);


//Predefined Functions
void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length);
void onMqttConnected();
time_t getNtpTime();
void digitalClockDisplay();
void printDigits(int digits);
void sendNTPpacket(IPAddress &address);
void connectingMessage(bool tickOn);

void connectWifi() {
  bool tickOnOff = false;
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    connectingMessage(tickOnOff);
    if(tickOnOff) {
      tickOnOff = false;
    } else {
      tickOnOff = true;
    }
    delay(500);
  }
  Udp.begin(localPort);
}

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)


// Example for NeoPixel Shield.  In this application we'd like to use it
// as a 5x8 tall matrix, with the USB port positioned at the top of the
// Arduino.  When held that way, the first pixel is at the top right, and
// lines are arranged in columns, progressive order.  The shield uses
// 800 KHz (v2) pixels that expect GRB color data.

// Set Block of Memory for storing and manipulating LED Data
CRGB matrixleds[TOTAL_LEDS];
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, PANEL_MATRIX_WIDTH, PANEL_MATRIX_HEIGHT, 
  PANELS_WIDTH, PANELS_HEIGHT, 
    NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
      NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG );

#define DEFAULT_SPEED 5

// Process data
// Setup buffer for display
// Output the display - Due to timing issues, interrupt is disabled while data is being sent to the LEDs (Limitations of 1 wire data design)
const uint16_t colors[] = {
  matrix->Color(255, 0, 0), matrix->Color(0, 255, 0), matrix->Color(0, 0, 255) };

void setup() {
  byte mac[6];
  WiFi.macAddress(mac);
  device.setUniqueId(mac, sizeof(mac));

  device.setName("TickerBoard");
  device.setSoftwareVersion("0.0.1");

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(matrixleds, TOTAL_LEDS);  // GRB ordering is typical
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->setBrightness(40);
  matrix->setTextColor(colors[0]);

  connectWifi();

  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  matrix->print(("Connecting to MQTT"));
  mqtt.onMessage(onMqttMessage);
  mqtt.onConnected(onMqttConnected);
  mqtt.begin(BROKER_ADDR, BROKER_USERNAME, BROKER_PASSWORD);

  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  matrix->print(("Syncing with Provider"));
  matrix->show();
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  matrix->print(("??:??:??"));
  matrix->show();
}

int x = TOTAL_WIDTH;
int pass = 0;


time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
  mqtt.loop();
}

void onMqttMessage(const char* topic, const uint8_t* payload, uint16_t length) {
  // This callback is called when message from MQTT broker is received.
  // Please note that you should always verify if the message's topic is the one you expect.
  // For example: if (memcmp(topic, "myCustomTopic") == 0) { ... }
  matrix->setTextColor(colors[2]);
  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  matrix->print("New message on topic: ");
  Serial.println(topic);
  matrix->show();
  sleep(3);
  matrix->setTextColor(colors[0]);
  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  matrix->print("Data: ");
  matrix->print((const char*)payload);
  matrix->show();
  sleep(5);
  mqtt.publish("myPublishTopic", "hello");
  matrix->setTextColor(colors[2]);
  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  matrix->print("publishing hello to MQTT");
  matrix->show();
  sleep(3);
}

void onMqttConnected() {
    Serial.println("Connected to the broker!");

    // You can subscribe to custom topic if you need
    mqtt.subscribe("myCustomTopic");
}


void digitalClockDisplay()
{
  matrix->setTextColor(colors[1]);
  // digital clock display of the time
  matrix->fillScreen(0);
  matrix->setCursor(0, 1);
  printDigits(hour());
  matrix->print(":");
  printDigits(minute());
  matrix->print(":");
  printDigits(second());
  matrix->show();
}


void printDigits(int digits)
{
  // utility for digital clock display: print leading 0
  if (digits < 10)
    matrix->print('0');
  matrix->print(digits);
}

/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address

  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}

void connectingMessage(bool tickOn)
{
  if (tickOn)
  {
    matrix->fillScreen(0);
    matrix->setCursor(0, 1);
    matrix->print(F("Connecting to Wifi \\"));
    matrix->show();
  } else {
    matrix->fillScreen(0);
    matrix->setCursor(0, 1);
    matrix->print(F("Connecting to Wifi /"));
    matrix->show();
  }
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
