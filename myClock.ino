/*   myClock -- ESP8266 WiFi NTP Clock for pixel displays
     Copyright (c) 2018 David M Denney <dragondaud@gmail.com>
     distributed under the terms of the MIT License
*/

#if defined(ESP8266)
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#else
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <WiFi.h>    
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#endif

#include <ArduinoOTA.h>
#include <time.h>
#include <FS.h>
#include <pgmspace.h>
#include <ArduinoJson.h>        // https://github.com/bblanchon/ArduinoJson/
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>
#include "display.h"

#define APPNAME "myClock"
#define VERSION "0.10.3"
//#define DS18                      // enable DS18B20 temperature sensor
//#define SYSLOG                    // enable SYSLOG support
#define LIGHT                     // enable LDR light sensor
#define ADMIN_USER "admin"

#define myOUT 1   // {0 = NullStream, 1 = Serial, 2 = Bluetooth}

#if myOUT == 0                    // NullStream output
NullStream NullStream;
Stream & OUT = NullStream;
#elif myOUT == 2                  // Bluetooth output, only on ESP32
#include "BluetoothSerial.h"
BluetoothSerial SerialBT;
Stream & OUT = SerialBT;
#else                             // Serial output default
Stream & OUT = Serial;
#endif

// in NTP.ino
void printTime(time_t);
time_t getNow(void);

String tzKey;                     // API key from https://timezonedb.com/register
String owKey;                     // API key from https://home.openweathermap.org/api_keys
String softAPpass = "ConFigMe";   // password for SoftAP config
uint8_t brightness = 255;         // 0-255 display brightness
bool milTime = true;              // set false for 12hour clock
String location;                  // zipcode or empty for geoIP location
String timezone;                  // timezone from https://timezonedb.com/time-zones or empty for geoIP
int threshold = 500;              // below this value display will dim, incrementally
bool celsius = false;             // set true to display temp in celsius
String language = "en";           // font does not support all languages
String countryCode = "US";        // default US, automatically set based on public IP address
String APpass ="";
String APname ="";
String HTTPpass = "";

// Syslog
#ifdef SYSLOG
#include <Syslog.h>             // https://github.com/arcao/Syslog
WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_PROTO_IETF);
String syslogSrv = "syslog";
uint16_t syslogPort = 514;
#endif

// DS18B20
#ifdef DS18
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS D3
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int Temp;
#endif

#if defined (ESP8266)
static const char* UserAgent PROGMEM = "myClock/1.0 (Arduino ESP8266)";
#else
static const char* UserAgent PROGMEM = "myClock/1.0 (Arduino ESP32)";
#endif

time_t TWOAM, pNow, wDelay;
uint8_t pHH, pMM, pSS;
uint16_t light;
long offset;
char HOST[20];
uint8_t dim;

void setup() {

#ifdef ESP32
  // disable brownout detector reset
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  // default ESP32 ADC is 12 bits, set to 10 bits
  analogSetWidth(10);
#else
  system_update_cpu_freq(SYS_CPU_160MHZ);               // force 160Mhz to prevent display flicker
#endif

#if myOUT == 0
  Serial.end();
#else
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println();
#endif

#if myOUT == 2
  if (!SerialBT.begin(APPNAME)) {
    Serial.println(F("Bluetooth failed"));
    delay(5000);
    ESP.restart();
  }
  delay(5000); // wait for client to connect
#endif

  OUT.println(UserAgent);
  readSPIFFS();

#ifdef DISPLAY_64x64
  // Define your display layout here, e.g. 1/32 step
  display.begin(32);
  OUT.println(F("Using 64x64 Display"));
#else
  // Define your display layout here, e.g. 1/16 step
  display.begin(16);
  OUT.println(F("Using 64x32 Display"));
#endif

  // Define your scan pattern here {LINE, ZIGZAG, ZAGGIZ} (default is LINE)
  //display.setScanPattern(LINE);

  // Define multiplex implemention here {BINARY, STRAIGHT} (default is BINARY)
  //display.setMuxPattern(BINARY);

  display.setFastUpdate(false);
  display_ticker.attach(0.002, display_updater);
  display.clearDisplay();
  display.setFont(&TomThumb);
  display.setTextWrap(false);
  display.setTextColor(myColor);
  setBrightness();

  //drawImage(0, 0); // display splash image while connecting

#ifdef DS18
  sensors.begin();
#endif

  // Create Hostname
  uint64_t chipid;  
  #ifdef ESP32
  chipid=ESP.getEfuseMac();
  #else
  chipid=ESP.getChipId();
  #endif
  sprintf_P(HOST, PSTR("%s-%06x"), APPNAME, (uint32_t) (chipid & 0xFFFFFF) );

  startWiFi();

  display.setCursor(2, row1);
  display.setTextColor(myGREEN);
  display.print(HOST);
  display.setCursor(2, row2);
  display.setTextColor(myLTBLUE);
  display.print(F("V"));
  display.print(VERSION);
  display.setCursor(2, row3);
  display.setTextColor(myMAGENTA);
  display.print(timezone);

#ifdef SYSLOG
  syslog.server(syslogSrv.c_str(), syslogPort);
  syslog.deviceHostname(HOST);
  syslog.appName(APPNAME);
  syslog.defaultPriority(LOG_INFO);
#endif

  display.setCursor(2, row4);
  display.setTextColor(myBLUE);
  display.print(WiFi.localIP());

#ifdef DISPLAY_64x64
  display.setCursor(2, row5);
  display.setTextColor(myGREEN);

  if (location == "") {
    display.print(F("-----"));
    OUT.println(F("location not set getIPlocation()"));
    location = getIPlocation();
    display.setCursor(2, row5);
    display.print(location);
  } else {
    display.print(location);
    display.setCursor(2, row6);
    display.setTextColor(myBLUE);
    display.print(F("TimeZone"));
    while (timezone == "") {
      OUT.println(F("timezone not set getIPlocation()"));
      getIPlocation();
    }
  }
  display.setCursor(2, row6);
  display.print(timezone);
  display.setCursor(2, row7);
  display.setTextColor(myMAGENTA);
  display.print(F("Get NTP Offset"));
#else
  if (location == "") {
    OUT.println(F("location not set getIPlocation()"));
    location = getIPlocation();
  } else {
    while (timezone == "") {
      OUT.println(F("timezone not set getIPlocation()"));
      getIPlocation();
    }
  }
#endif

  OUT.printf_P(PSTR("setup: Location:%s, TZ:%s, MilTime:%s \r\n"), location.c_str(), timezone.c_str(), milTime ? "true" : "false");
#ifdef SYSLOG
  syslog.logf("setup: %s|%s|%s", location.c_str(), timezone.c_str(), milTime ? "true" : "false");
#endif

  setNTP(timezone);
  #ifdef DISPLAY_64x64
  display.setCursor(2, row8);
  display.setTextColor(myLTBLUE);
  display.print(offset/3600);
  display.print(F(" Hour"));
  delay(2000);
  #else
  delay(1000);
  #endif
  startWebServer(ADMIN_USER, HTTPpass.c_str());
  displayDraw();
  getWeather();
} // setup

void loop() {
  ArduinoOTA.handle();
  time_t now = getNow();

  if (now != pNow) {
    //OUT.print(F("Time : "));
    //printTime(now);

    if (now > TWOAM) {
      setNTP(timezone);
    }
    int ss = now % 60;
    int mm = (now / 60) % 60;
    int hh = (now / (60 * 60)) % 24;
    //OUT.printf_P(PSTR("NOW   : %02d:%02d:%02d\r\n"), hh,mm,ss);
    //OUT.printf_P(PSTR("TWOAM : %02d:%02d:%02d\r\n"), (TWOAM / (60 * 60)) % 24,(TWOAM / 60) % 60,TWOAM % 60);
    if ((!milTime) && (hh > 12)) hh -= 12;
    if (ss != pSS) {
      int s0 = ss % 10;
      int s1 = ss / 10;
      if (s0 != digit0.Value()) digit0.Morph(s0);
      if (s1 != digit1.Value()) digit1.Morph(s1);
      pSS = ss;
      setBrightness();
    }
    if (mm != pMM) {
      int m0 = mm % 10;
      int m1 = mm / 10;
      if (m0 != digit2.Value()) digit2.Morph(m0);
      if (m1 != digit3.Value()) digit3.Morph(m1);
      pMM = mm;
      OUT.printf_P(PSTR("%02d:%02d %d bytes free\r\n"), hh, mm, ESP.getFreeHeap());
    }
    if (hh != pHH) {
      int h0 = hh % 10;
      int h1 = hh / 10;
      if (h0 != digit4.Value()) digit4.Morph(h0);
      if (h1 != digit5.Value()) digit5.Morph(h1);
      pHH = hh;
    }
#ifdef DS18
    sensors.requestTemperatures();
    int t;
    if (celsius) t = round(sensors.getTempC(0));
    else t = round(sensors.getTempF(0));
    if (t < -66 | t > 150) t = 0;
    if (Temp != t) {
      Temp = t;
      display.setCursor(0, row1);
      display.printf_P(PSTR("% 2d"), Temp);
    }
#endif

#ifdef DISPLAY_64x64
    display.setTextColor(myYELLOW);
    display.fillRect(0, row5 -7, 64, 7, myBLACK);
    //display.drawRect(0, row5 -7, 64, 7, myYELLOW);
    display.setCursor(0, row5);
    display.printf_P(PSTR("Light %d"), light);
    display.setTextColor(myGREEN);
    display.fillRect(0, row6 -7, 64, 7, myBLACK);
    display.setCursor(0, row6);
    display.printf_P(PSTR("Free %dKB"), ESP.getFreeHeap()/1024);
#else 
    //display.fillRect(0, row4 -7, 64, 7, myBLACK);
    //display.setCursor(0, row4);
    //display.printf_P(PSTR("Light %d"), light);
#endif

    pNow = now;
    if (now > wDelay) {
      getWeather();
    }
  }
}

void displayDraw() {
  display.clearDisplay();
  setBrightness();
  time_t now = getNow();
  int ss = now % 60;
  int mm = (now / 60) % 60;
  int hh = (now / (60 * 60)) % 24;
  if ((!milTime) && (hh > 12)) hh -= 12;
  OUT.printf_P(PSTR("%02d:%02d\r\n"), hh, mm);
  digit1.DrawColon(myColor);
  digit3.DrawColon(myColor);
  digit0.Draw(ss % 10, myColor);
  digit1.Draw(ss / 10, myColor);
  digit2.Draw(mm % 10, myColor);
  digit3.Draw(mm / 10, myColor);
  digit4.Draw(hh % 10, myColor);
  digit5.Draw(hh / 10, myColor);
  pNow = now;
}

void setBrightness() {
#ifdef LIGHT
  int lt = analogRead(A0);
  if (lt > 20) {
    light = (light * 3 + lt) >> 2;
    if (light >= threshold) dim = brightness;
    else if (light < (threshold >> 3)) dim = brightness >> 4;
    else if (light < (threshold >> 2)) dim = brightness >> 3;
    else if (light < (threshold >> 1)) dim = brightness >> 2;
    else if (light < threshold) dim = brightness >> 1;
    display.setBrightness(dim);
  }
#else
  display.setBrightness(brightness);
#endif
}
