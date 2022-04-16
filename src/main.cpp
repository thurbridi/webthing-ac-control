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
ThingDevice acThing("ac", "AC", acTypes);
ThingProperty acCurrentTemp("nowtemp", "Current Temperature in Celcius",
                            INTEGER, "TemperatureProperty");
ThingProperty acTargetTemp("targettemp", "Target Temperature in Celcius",
                           INTEGER, "TargetTemperatureProperty");
ThingProperty acMode("on", "AC on/off", BOOLEAN, "OnOffProperty");

bool lastOn = false;
int lastTemp = 22;

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

  acTargetTemp.title = "AC Temp";
  acTargetTemp.unit = "&#176;C";
  acTargetTemp.minimum = 17;
  acTargetTemp.minimum = 30;
  ThingPropertyValue value;
  value.integer = lastTemp;
  acTargetTemp.setValue(value);
  acCurrentTemp.setValue(value);

  acThing.addProperty(&acMode);
  acThing.addProperty(&acCurrentTemp);
  acThing.addProperty(&acTargetTemp);
  adapter->addDevice(&acThing);
  adapter->begin();
  Serial.println("HTTP server started");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.print("/things/");
  Serial.println(acThing.id);

  ac.next.protocol = decode_type_t::COOLIX;  // Set a protocol to use.
  ac.next.model = 1;  // Some A/Cs have different models. Try just the first.
  ac.next.mode = stdAc::opmode_t::kCool;  // Run in cool mode initially.
  ac.next.celsius = true;      // Use Celsius for temp units. False = Fahrenheit
  ac.next.degrees = lastTemp;  // 25 degrees.
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
  ac.next.power = lastOn;  // Initially start with the unit off.
}

void loop(void) {
  adapter->update();
  bool mode = acMode.getValue().boolean;
  digitalWrite(ledPin, mode ? LOW : HIGH);  // active low led
  ac.next.power = mode ? true : false;      // We want to turn on the A/C
  ac.next.degrees = acTargetTemp.getValue().integer;

  ThingPropertyValue temp;
  temp.integer = (int)ac.next.degrees;
  acCurrentTemp.setValue(temp);

  if (mode != lastOn) {
    ac.sendAc();
    Serial.print(acThing.id);
    Serial.print(": ");
    Serial.println(mode);
  }
  if (ac.next.degrees != lastTemp) {
    ac.sendAc();
    Serial.print(acThing.id);
    Serial.print(": ");
    Serial.print(ac.next.degrees);
    Serial.println("C");
  }
  lastTemp = ac.next.degrees;
  lastOn = mode;
}