//  NTP and location functions

String getIPlocation() { // Using ip-api.com to discover public IP's location and timezone
  WiFiClient wifi;
  HTTPClient http;
  static const char URL[] PROGMEM = "http://ip-api.com/json";
  String payload;
  http.setUserAgent(UserAgent);

  OUT.println(F("getIPlocation:"));
  OUT.println(URL);

  if (!http.begin(wifi, URL)) {
#ifdef SYSLOG
    syslog.log(F("getIPlocation HTTP failed"));
#endif
    OUT.println(F("getIPlocation: HTTP failed"));
  } else {
    int stat = http.GET();
    if (stat == HTTP_CODE_OK) {
      payload = http.getString();
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(payload);
      if (root.success()) {
        root.prettyPrintTo(OUT);
        OUT.println();
        String isp = root["isp"];
        String region = root["regionName"];
        String country = root["countryCode"];
        String tz = root["timezone"];
        String zip = root["zip"];
        timezone = tz;
        countryCode = country;
        http.end();
#ifdef SYSLOG
        syslog.logf("getIPlocation: %s, %s, %s, %s", isp.c_str(), region.c_str(), country.c_str(), tz.c_str());
#endif
        return zip;
      } else {
#ifdef SYSLOG
        syslog.log(F("getIPlocation JSON parse failed"));
        syslog.log(payload);
#endif
        OUT.println(F("getIPlocation: JSON parse failed!"));
        OUT.println(payload);
      }
    } else {
#ifdef SYSLOG
      syslog.logf("getIPlocation failed, GET reply %d", stat);
#endif
      OUT.printf_P(PSTR("getIPlocation: GET reply %d\r\n"), stat);
    }
  }
  http.end();
  delay(1000);
} // getIPlocation

int getOffset(const String tz) { // using timezonedb.com, return offset for zone name
  WiFiClient wifi;
  HTTPClient http;
  String URL = PSTR("http://api.timezonedb.com/v2/list-time-zone?key=") + tzKey + PSTR("&format=json&zone=") + tz;
  String payload;
  int stat;
  http.setUserAgent(UserAgent);

  OUT.print(F("getOffset:"));
  OUT.println(URL);

  if (!http.begin(wifi, URL)) {
#ifdef SYSLOG
    syslog.log(F("getOffset HTTP failed"));
#endif
    OUT.println(F("getOffset: HTTP failed"));
  } else {
    stat = http.GET();
    if (stat == HTTP_CODE_OK) {
      payload = http.getString();
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(payload);
      if (root.success()) {
        root.prettyPrintTo(OUT);
        OUT.println();
        JsonObject& zones = root["zones"][0];
        offset = zones["gmtOffset"];
      } else {
#ifdef SYSLOG
        syslog.log(F("getOffset JSON parse failed"));
        syslog.log(payload);
#endif
        OUT.println(F("getOffset: JSON parse failed!"));
        OUT.println(payload);
      }
    } else {
#ifdef SYSLOG
      syslog.logf("getOffset failed, GET reply %d", stat);
#endif
      OUT.printf_P(PSTR("getOffset: GET reply %d\r\n"), stat);
    }
  }
  http.end();
  return stat;
} // getOffset


time_t getNow(void) {
  time_t now = time(nullptr);
  #ifdef ESP8266
  while (time(nullptr) < (30 * 365 * 24 * 60 * 60)) {
    delay(500);
  }
  #endif
  #ifdef ESP32
  // Looks like on ESP32 tz offset not taken into account
  now += offset;
  #endif
  return now;
}

void printTime(time_t ts )
{
  struct tm * timeinfo;
  timeinfo = gmtime(&ts);
  OUT.println(timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setNextNTP(time_t ts) {
  time_t now = getNow();
  TWOAM = now + ts;
  OUT.printf_P(PSTR("%s (%d) next timezone check @ "), timezone.c_str(), offset );
  printTime(TWOAM);
#ifdef SYSLOG
  syslog.logf("setNTP: %s (%d)", timezone.c_str(), int(offset / 3600));
#endif
} // setNTP

void setNTP(const String tz) {
  if (getOffset(tz) == HTTP_CODE_OK) {
    OUT.print(F("setNTP: TimeZone Offset "));
    OUT.println(offset);
    OUT.print(F("setNTP: configure NTP ..."));
    configTime(offset, 0, PSTR("pool.ntp.org"));
    OUT.println(F(" OK"));
  } else {
    OUT.print(F("setNTP: can't getOffset()"));
  }

  // Next sync in 1h * 60m * 60s
  setNextNTP(1 * 60 * 60);
} // setNTP
