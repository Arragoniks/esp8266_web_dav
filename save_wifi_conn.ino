#include <ESP8266WiFi.h>

void setup() {
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin("mywifi3", "pasw123456789");

}

void loop() {
  // put your main code here, to run repeatedly:

}
