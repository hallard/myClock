// SPIFFS config file
// auto format if unable to write

void readSPIFFS() {
  if (SPIFFS.begin()) {
    Serial.println(F("readSPIFFS: mounted"));
    if (SPIFFS.exists("/config.json")) {
      File configFile = SPIFFS.open(F("/config.json"), "r");
      if (!configFile) return;
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      parseJson(json);
      configFile.close();
    }
  } else {
    Serial.println(F("readSPIFFS: failed to mount SPIFFS"));
  }
}


bool parseJson(JsonObject& json) {
  if (json.success()) {
    const char * value;
    value = json["softAPpass"];
    if (value != nullptr) softAPpass = value;
    value = json["APpass"];
    if (value != nullptr) APpass = value;
    value = json["APname"];
    if (value != nullptr) APname = value;
    value = json["location"];
    if (value != nullptr) location = value;
    value = json["timezone"];
    if (value != nullptr) timezone = value;
    value = json["tzKey"];
    if (value != nullptr) tzKey = value;
    value = json["owKey"];
    if (value != nullptr) owKey = value;
    value = json["language"];
    if (value != nullptr) language = value;
    value = json["countryCode"];
    if (value != nullptr) countryCode = value;
    brightness = json["brightness"] | 255;
    milTime = json["milTime"] | true;
    myColor = json["myColor"] | 65535;
    threshold = json["threshold"] | 500;
    celsius = json["celsius"] | false;
#ifdef SYSLOG
    value = json["syslogSrv"];
    if (value != nullptr) syslogSrv = value;
    syslogPort = json["syslogPort"] | 514;
#endif
  }
}

bool writeSPIFFS() {
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["softAPpass"] = softAPpass;
  json["APpass"] = APpass;
  json["APname"] = APname;
  json["location"] = location;
  json["timezone"] = timezone;
  json["tzKey"] = tzKey;
  json["owKey"] = owKey;
  json["brightness"] = brightness;
  json["milTime"] = milTime;
  json["myColor"] = myColor;
  json["threshold"] = threshold;
  json["celsius"] = celsius;
  json["language"] = language;
  json["countryCode"] = countryCode;
#ifdef SYSLOG
  json["syslogSrv"] = syslogSrv;
  json["syslogPort"] = syslogPort;
#endif
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    display.setCursor(2, row2);
    display.print(F("config failed"));
    Serial.println(F("failed to open config.json for writing"));
    if (SPIFFS.format()) {
      Serial.println(F("SPIFFS formated"));
    }
    Serial.flush();
    ESP.reset();
  } else {
#ifdef SYSLOG
    syslog.log(F("save config"));
#endif
    json.prettyPrintTo(configFile);
    configFile.close();
    delay(100);
  }
  return true;
}

String getSPIFFS() {
  String payload;
  if (SPIFFS.exists("/config.json")) {
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      if (json.success()) json.prettyPrintTo(payload);
      configFile.close();
    }
  }
  return payload;
}
