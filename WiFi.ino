// WiFi 
/*
void configModeCallback(WiFiManager *myWiFiManager) {
  display.clearDisplay();
  display.setFont(&TomThumb);
  display.setCursor(2, row1);
  display.print(F("Config WiFi"));
  display.setCursor(2, row2);
  display.print(myWiFiManager->getConfigPortalSSID());
  display.setCursor(2, row3);
  display.print(F("Pass: "));
  display.print(softAPpass);
  display.setCursor(2, row4);
  display.print(F("IP: "));
  display.print(WiFi.softAPIP());
}
*/

void startWiFi() {   // if WiFi does not connect, establish AP for configuration
  String t = WiFi.macAddress();
  t = String(APPNAME) + "-" + t.substring(9, 11) + t.substring(12, 14) + t.substring(15, 17);
  t.toCharArray(HOST, sizeof(HOST));
  Serial.print(softAPpass);
  WiFi.hostname(HOST);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(HOST);
  WiFi.begin(APname.c_str(), APpass.c_str());
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.printf("STA: Failed!\n");
    WiFi.disconnect(false);
    delay(1000);
    WiFi.begin(APname.c_str(), APpass.c_str());
  }

  MDNS.begin(HOST);
  ArduinoOTA.setHostname(HOST);
  ArduinoOTA.onStart([]() {
#ifdef SYSLOG
    syslog.log(F("OTA Update"));
#endif
    display.clearDisplay();   // turn off display during update
    display_ticker.detach();
    Serial.println(F("\nOTA: Start"));
  } );
  ArduinoOTA.onEnd([]() {
    Serial.println(F("\nOTA: End"));
    delay(1000);
    ESP.restart();
  } );
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.print(PSTR("OTA Progress: ") + String((progress / (total / 100))) + PSTR(" \r"));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.print(PSTR("\nError[") + String(error) + PSTR("]: "));
    if (error == OTA_AUTH_ERROR) Serial.println(F("Auth Failed"));
    else if (error == OTA_BEGIN_ERROR) Serial.println(F("Begin Failed"));
    else if (error == OTA_CONNECT_ERROR) Serial.println(F("Connect Failed"));
    else if (error == OTA_RECEIVE_ERROR) Serial.println(F("Receive Failed"));
    else if (error == OTA_END_ERROR) Serial.println(F("End Failed"));
    else Serial.println(F("unknown error"));
    delay(1000);
    ESP.restart();
  });
  ArduinoOTA.begin();
} // startWiFi
