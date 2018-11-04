// Web Server for configuration and updates
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#define ADMIN_USER "admin"

AsyncWebServer server(80);


void handleOptions(AsyncWebServerRequest *request)
{
  String response = "";
  int ret ;

  Serial.println(PSTR("Serving handleOptions"));
  
  // Cant't use PSTR CFG_SAVE here, crash dump hasArg don't have _P
  if ( request->hasArg( "myColor") ) {
    uint8_t params = request->params();
    int i, l;
    char buff[128]; 
    Serial.printf_P(PSTR("Form with %d parameters\r\n"), params);

    // Navigate for all args, and simulate as it was typed from command line
    for ( i = 0; i < params; i++ ) {
      AsyncWebParameter* param = request->getParam(i);
      // We are in async, try to work fast
      // less print, less time 
      Serial.printf_P(PSTR("[%02d] %s %s\r\n"), i, param->name().c_str(), param->value().c_str());

      if (param->name() == "myColor") {
        myColor = htmlColor565(param->value());
      }
      if (param->name() == "brightness") {
        l = param->value().toInt();
        brightness = brightness ? brightness : 255;
      }

      if (param->name() == "threshold") {
        l = param->value().toInt();
        threshold = l ? l : 500;
      }

      if (param->name() == "milTime") {
        milTime = param->value() == "on" ? true : false;
      }
      if (param->name() == "celsius") {
        celsius = param->value() == "on" ? true : false;
      }

      if (param->name() == "tzKey") tzKey = param->value();
      if (param->name() == "owKey") owKey = param->value();
      if (param->name() == "language") language = param->value();
      if (param->name() == "softAPpass") softAPpass = param->value();
      if (param->name() == "APpass") APpass = param->value();
      if (param->name() == "APname") APname = param->value();
      if (param->name() == "location") location = param->value();
      
      if (param->name() == "timezone")  {
        String c = param->value();
        if (c != "") {
          c.trim();
          c.replace(' ', '_');
          if (timezone != c) {
            timezone = c;
            setNTP(timezone);
          }
        }
      }
    } // For all params

    // we're async, to do place a global flag to do this in main loop
    //setNTP(timezone);
    //displayDraw(brightness);
    //getWeather();

    // now we can save the config
    if ( writeSPIFFS() ) {
      ret = 200;
      response = PSTR("OK");
    } else {
      ret = 412;
      response = PSTR("Error while saving configuration");
    }

  } else  {
    ret = 400;
    response = PSTR("Missing Form Field");
  }

  request->send ( ret, "text/plain", response);
}

void handleReset(AsyncWebServerRequest * request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  Serial.println(F("webServer: reset"));

  response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title></head><body>", request->url().c_str());
  response->print("<h2>Restarting<h2>");
  response->print("</body></html>");
  request->send(response);

  delay(100);
  ESP.reset();
  // Should never arrive there
  while (true);
}

void handleNotFound(AsyncWebServerRequest * request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  //response->addHeader("Server","ESP Async Web Server");
  response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title></head><body>", request->url().c_str());
  response->print("<h2>Sorry ");
  response->print(request->client()->remoteIP());
  response->print("We're unable to process this page</h2>");

  response->print("<h3>Information</h3>");
  response->print("<ul>");
  response->printf("<li>Version: HTTP/1.%u</li>", request->version());
  response->printf("<li>Method: %s</li>", request->methodToString());
  response->printf("<li>URL: %s</li>", request->url().c_str());
  response->printf("<li>Host: %s</li>", request->host().c_str());
  response->printf("<li>ContentType: %s</li>", request->contentType().c_str());
  response->printf("<li>ContentLength: %u</li>", request->contentLength());
  response->printf("<li>Multipart: %s</li>", request->multipart() ? "true" : "false");
  response->print("</ul>");

  response->print("<h3>Headers</h3>");
  response->print("<ul>");
  int headers = request->headers();
  for (int i = 0; i < headers; i++) {
    AsyncWebHeader* h = request->getHeader(i);
    response->printf("<li>%s: %s</li>", h->name().c_str(), h->value().c_str());
  }
  response->print("</ul>");

  response->print("<h3>Parameters</h3>");
  response->print("<ul>");
  int params = request->params();
  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if ( p->isFile() ) {
      response->printf("<li>FILE[%s]: %s, size: %u</li>", p->name().c_str(), p->value().c_str(), p->size());
    } else if ( p->isPost() ) {
      response->printf("<li>POST[%s]: %s</li>", p->name().c_str(), p->value().c_str());
    } else {
      response->printf("<li>GET[%s]: %s</li>", p->name().c_str(), p->value().c_str());
    }
  }
  response->print("</ul>");

  response->print("</body></html>");
  //send the response last
  request->send(response);
}


String template_processor(const String& var)
{
  if (var == "version") return F(VERSION);
  if (var == "host") return String(HOST);
  if (var == "brightness") return String(brightness);
  if (var == "threshold") return String(threshold);
  if (var == "location") return String(location);
  if (var == "timezone") return String(timezone);
  if (var == "tzKey") return String(tzKey);
  if (var == "owKey") return String(owKey);
  if (var == "language") return String(language);
  if (var == "milTime" && milTime) return F("checked");
  if (var == "celsius" && celsius) return F("checked");

  if(var == "date") {
    time_t now = time(nullptr);
    String t = ctime(&now);
    t.trim();
    return t;
  }

  // class defined in myclock.css
  if(var == "ds_class") {
    #ifdef DS18
      return F("ds_show");
    #else
      return F("ds_hide");
    #endif
  }

  if(var == "myColor") {
    char c[8];
    sprintf(c, "#%06X", color565to888(myColor));
    return String(c);
  }

  // Default return empty string
  return String();
}

void startWebServer() {

  server.addHandler(new SPIFFSEditor(ADMIN_USER, softAPpass));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/reset", handleReset);
  server.on("/options", handleOptions);
  server.onNotFound(handleNotFound);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm").setTemplateProcessor(template_processor);

  //server.serveStatic("/", SPIFFS, "/www/").setTemplateProcessor(processor);
  
  server.onFileUpload([](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final){
    if(!index)
      Serial.printf("UploadStart: %s\n", filename.c_str());
    Serial.printf("%s", (const char*)data);
    if(final)
      Serial.printf("UploadEnd: %s (%u)\n", filename.c_str(), index+len);
  });

  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
    if(!index)
      Serial.printf("BodyStart: %u\n", total);
    Serial.printf("%s", (const char*)data);
    if(index + len == total)
      Serial.printf("BodyEnd: %u\n", total);
  });

  // Start the Server
  server.begin();
  MDNS.addService("http","tcp",80);
}


