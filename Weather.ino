// obtain and display weather information from openweathermap.org

const char* caridnals PROGMEM = "  N NEE SES SWW NWN ";

void getWeather() {    // Using openweathermap.org
  wDelay = pNow + 900; // delay between weather updates
  display.setCursor(0, row4);   // any error displayed in red on bottom row
  display.setTextColor(myRED);
  WiFiClient wifi;
  HTTPClient http;
  String URL = PSTR("http://api.openweathermap.org/data/2.5/weather?zip=")
               + location + F(",") + countryCode + F("&units=%units%&lang=%lang%&appid=") + owKey;
  URL.replace("%units%", celsius ? "metric" : "imperial");
  URL.replace("%lang%", language);
  String payload;
  long offset;
  http.setUserAgent(UserAgent);
  if (!http.begin(wifi, URL)) {
#ifdef SYSLOG
    syslog.log(F("getWeather HTTP failed"));
#endif
    display.print(F("http fail"));
    OUT.println(F("getWeather: HTTP failed"));
  } else {
    int stat = http.GET();
    if (stat == HTTP_CODE_OK) {
      payload = http.getString();
      DynamicJsonBuffer jsonBuffer;
      JsonObject& root = jsonBuffer.parseObject(payload);
      if ( root.success() ) {
        char dir[3];
        String name = root["name"];
        JsonObject& weather = root["weather"][0];
        JsonObject& main = root["main"];
        int temperature = round( main["temp"].as<float>()) ;
        int humidity = main["humidity"];
        int wind = round( root["wind"]["speed"].as<float>());
        int deg = root["wind"]["deg"];
        degreeDir(deg, &dir[0]);
        //strcpy(dir, "XX");
        int tc;
        if (celsius) tc = round(temperature * 1.8) + 32;
        else tc = round(temperature);
        if (tc <= 32) display.setTextColor(myCYAN);
        else if (tc <= 50) display.setTextColor(myLTBLUE);
        else if (tc <= 60) display.setTextColor(myBLUE);
        else if (tc <= 78) display.setTextColor(myGREEN);
        else if (tc <= 86) display.setTextColor(myYELLOW);
        else if (tc <= 95) display.setTextColor(myORANGE);
        else if (tc > 95) display.setTextColor(myRED);
        else display.setTextColor(myColor);
        display.fillRect(0, 0, 64, 6, myBLACK);

#ifdef DS18
        display.setCursor(0, row1);
        display.printf_P(PSTR("%2d/%2d%c%c %2d%% %2d %s"), Temp, temperature, 142, celsius ? 'C' : 'F', humidity, wind, dir);
#else
        display.setCursor(9, row1);
        display.printf_P(PSTR("%2d%c%c %2d%% %2d %s"), temperature, 142, celsius ? 'C' : 'F', humidity, wind, dir);
#endif
        String description = weather["description"];
        description.replace(F("intensity "), "");   // english description too long sometimes
        description = utf8ascii(description);       // fix UTF-8 characters
        int id = weather["id"];
        int i = id / 100;
        switch (i) {
          case 2: // Thunderstorms
            display.setTextColor(myORANGE);
            break;
          case 3: // Drizzle
            display.setTextColor(myBLUE);
            break;
          case 5: // Rain
            display.setTextColor(myBLUE);
            break;
          case 6: // Snow
            display.setTextColor(myWHITE);
            break;
          case 7: // Atmosphere
            display.setTextColor(myYELLOW);
            break;
          case 8: // Clear/Clouds
            display.setTextColor(myGRAY);
            break;
        }
        int16_t  x1, y1, ww;
        uint16_t w, h;
        display.getTextBounds(description, 0, row4, &x1, &y1, &w, &h);
        display.fillRect(0, 25, 64, 7, myBLACK);
        if (w < 64) x1 = (68 - w) >> 1;         // center weather description (getTextBounds returns too long)
        display.setCursor(x1, row4);
        display.print(description);
#ifdef SYSLOG
        syslog.logf("getWeather: %d%c|%d%%RH|%d%s|%s", temperature, celsius ? 'C' : 'F', humidity, wind, dir, description.c_str());
#endif
        OUT.printf_P(PSTR("%2d%c, %2d%%, %d %s (%d), %s (%d) \r\n"), temperature, celsius ? 'C' : 'F', humidity, wind, dir, deg, description.c_str(), id);
      } else {
        display.print(F("json fail"));
#ifdef SYSLOG
        syslog.log(F("getWeather JSON parse failed"));
        syslog.log(payload);
#endif
        OUT.println(F("getWeather: JSON parse failed!"));
        OUT.println(payload);
      }
    } else {
#ifdef SYSLOG
      syslog.logf("getWeather failed, GET reply %d", stat);
#endif
      display.print(stat);
      OUT.printf_P(PSTR("getWeather: GET failed: %d %s\r\n"), stat, http.errorToString(stat).c_str());
    }
  }
  http.end();
} // getWeather

const char * degreeDir(int degrees, char * buff) {
  int idx = (round((degrees % 360) / 45)) + 1;
  if (idx<1 || idx>8) {
    idx =0;
  }
  buff[0] = caridnals[idx+0];
  buff[1] = caridnals[idx+1];
  buff[2] = '\0';
  return buff;
} // degreeDir

// from http://playground.arduino.cc/Main/Utf8ascii
// ****** UTF8-Decoder: convert UTF8-string to extended ASCII *******
static byte c1;  // Last character buffer

// Convert a single Character from UTF8 to Extended ASCII
// Return "0" if a byte has to be ignored
byte utf8ascii(byte ascii) {
  if ( ascii < 128 ) // Standard ASCII-set 0..0x7F handling
  { c1 = 0;
    return ( ascii );
  }
  // get previous input
  byte last = c1;   // get last char
  c1 = ascii;       // remember actual character
  switch (last)     // conversion depending on first UTF8-character
  { case 0xC2: return  (ascii);  break;
    case 0xC3: return  (ascii | 0xC0) - 34;  break;// TomThumb extended characters off by 34
    case 0x82: if (ascii == 0xAC) return (0x80);   // special case Euro-symbol
  }
  return  (0);                                     // otherwise: return zero, if character has to be ignored
}

// convert String object from UTF8 String to Extended ASCII
String utf8ascii(String s) {
  String r = "";
  char c;
  for (int i = 0; i < s.length(); i++)
  {
    c = utf8ascii(s.charAt(i));
    if (c != 0) r += c;
  }
  return r;
}

// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!)
void utf8ascii(char* s) {
  int k = 0;
  char c;
  for (int i = 0; i < strlen(s); i++)
  {
    c = utf8ascii(s[i]);
    if (c != 0)
      s[k++] = c;
  }
  s[k] = 0;
}
