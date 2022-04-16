#define LARGE_JSON_BUFFERS 1

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>
#include <IRac.h>
#include <IRremoteESP8266.h>
#include <IRutils.h>

#include "Thing.h"
#include "WebThingAdapter.h"

const uint16_t ledPin = LED_BUILTIN;
const uint16_t irSendPin = 0;

IRac ac(irSendPin);

WebThingAdapter *adapter;
const char *acTypes[] = {"Thermostat", nullptr};
ThingDevice acDevice("ac", "AC", acTypes);
ThingProperty acCurrentTemp("nowtemp", "Current Temperature in Celcius",
                            INTEGER, "TemperatureProperty");

void onTemperatureChange(ThingPropertyValue value) {
  int temp = value.integer;
  ac.next.degrees = temp;
  acCurrentTemp.setValue(value);
  ac.sendAc();

  // debug
  Serial.print(acDevice.id);
  Serial.print(": ");
  Serial.print(ac.next.degrees);
  Serial.println("C");
}

void onPowerChange(ThingPropertyValue value) {
  bool power = value.boolean;
  ac.next.power = power;
  ac.sendAc();

  // debug
  Serial.print(acDevice.id);
  Serial.print(": ");
  Serial.println(power);
}

ThingProperty acTargetTemp("targettemp", "Target Temperature in Celcius",
                           INTEGER, "TargetTemperatureProperty",
                           onTemperatureChange);
ThingProperty acMode("on", "AC on/off", BOOLEAN, "OnOffProperty",
                     onPowerChange);

void setupACDevice(const int temperature = 23) {
  ac.next.protocol = decode_type_t::COOLIX;  // Set a protocol to use.
  ac.next.model = 1;  // Some A/Cs have different models. Try just the first.
  ac.next.mode = stdAc::opmode_t::kCool;  // Run in cool mode initially.
  ac.next.celsius = true;  // Use Celsius for temp units. False = Fahrenheit
  ac.next.degrees = 23;    // 25 degrees.
  ac.next.fanspeed = stdAc::fanspeed_t::kMedium;  // Start the fan at medium.
  ac.next.swingv = stdAc::swingv_t::kOff;  // Don't swing the fan up or down.
  ac.next.swingh = stdAc::swingh_t::kOff;  // Don't swing the fan left or right.
  ac.next.light = false;   // Turn off any LED/Lights/Display that we can.
  ac.next.beep = false;    // Turn off any beep from the A/C if we can.
  ac.next.econo = false;   // Turn off any economy modes if we can.
  ac.next.filter = false;  // Turn off any Ion/Mold/Health filters if we can.
  ac.next.turbo = false;   // Don't use any turbo/powerful/etc modes.
  ac.next.quiet = false;   // Don't use any quiet/silent/etc modes.
  ac.next.sleep = -1;      // Don't set any sleep time or modes.
  ac.next.clean = false;   // Turn off any Cleaning options if we can.
  ac.next.clock = -1;      // Don't set any current time if we can avoid it.
  ac.next.power = false;   // Initially start with the unit off.

  acTargetTemp.title = "AC Temp";
  acTargetTemp.unit = "&#176;C";
  acTargetTemp.minimum = 17.0;
  acTargetTemp.minimum = 30.0;
  ThingPropertyValue value;
  value.integer = temperature;
  acTargetTemp.setValue(value);
  acCurrentTemp.setValue(value);

  acDevice.addProperty(&acMode);
  acDevice.addProperty(&acCurrentTemp);
  acDevice.addProperty(&acTargetTemp);
}

void setup(void) {
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  // Wifi config
  AsyncWebServer server(80);
  DNSServer dns;

  AsyncWiFiManager wifiManager(&server, &dns);
  wifiManager.autoConnect();

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  adapter = new WebThingAdapter("w25", WiFi.localIP());
  setupACDevice();
  adapter->addDevice(&acDevice);
  adapter->begin();

  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(acDevice.id);
}

void loop(void) { adapter->update(); }