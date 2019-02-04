// WiFi 

void startWiFi() {   // if WiFi does not connect, establish AP for configuration

#if defined(ESP8266)
  WiFi.hostname(HOST);
#else
  WiFi.setHostname(HOST);
#endif

  OUT.print(softAPpass);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(HOST);
  WiFi.begin(APname.c_str(), APpass.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    OUT.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(APname.c_str(), APpass.c_str());
  }

  MDNS.begin(HOST);

  OUT.printf_P(PSTR("WiFi Connected as %s IP:" ), HOST);
  OUT.println(WiFi.localIP());

  ArduinoOTA.setHostname(HOST);
  ArduinoOTA.onStart([]() {
#ifdef SYSLOG
    syslog.log(F("OTA Update"));
#endif
    display.clearDisplay();   // turn off display during update
    display_ticker.detach();
    OUT.println(F("\nOTA: Start"));
  } );
  ArduinoOTA.onEnd([]() {
    OUT.println(F("\nOTA: End"));
    delay(1000);
    ESP.restart();
  } );
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    OUT.print(PSTR("OTA Progress: ") + String((progress / (total / 100))) + PSTR(" \r"));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    OUT.print(PSTR("\nError[") + String(error) + PSTR("]: "));
    if (error == OTA_AUTH_ERROR) OUT.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) OUT.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) OUT.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) OUT.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) OUT.println(F("End Failed"));
    else OUT.println(F("unknown error"));
    delay(1000);
    ESP.restart();
  });
  ArduinoOTA.begin();
} // startWiFi
