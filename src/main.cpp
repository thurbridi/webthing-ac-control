#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>

#include "Thing.h"
#include "WebThingAdapter.h"

const int ledPin = LED_BUILTIN;

WebThingAdapter *adapter;

const char *ledTypes[] = {"OnOffSwitch", "Light", nullptr};
ThingDevice led("led", "Built-in LED", ledTypes);
ThingProperty ledOn("on", "", BOOLEAN, "OnOffProperty");

bool lastOn = false;

void setup(void) {
  AsyncWebServer server(80);
  DNSServer dns;

  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.autoConnect();

  pinMode(ledPin, OUTPUT);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  adapter = new WebThingAdapter("w25", WiFi.localIP());

  led.addProperty(&ledOn);
  adapter->addDevice(&led);
  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(led.id);
}

void loop(void) {
  adapter->update();
  bool on = ledOn.getValue().boolean;
  digitalWrite(ledPin, on ? LOW : HIGH);  // active low led
  if (on != lastOn) {
    Serial.print(led.id);
    Serial.print(": ");
    Serial.println(on);
  }
  lastOn = on;
}