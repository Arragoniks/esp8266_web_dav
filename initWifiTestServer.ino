#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

#define APSSID  "esp8266AP"
#define APPASW  "123456789"
#define CONF_PAGE_TIMEOUT  5
#define CONN_TIMEOUT  30

const String ap_ssid = APSSID;
const String ap_pasw = APPASW;
const int configPageTimeout = CONF_PAGE_TIMEOUT;
const int connectionTimeout = CONN_TIMEOUT;

ESP8266WebServer server(80);

const String wifiConfForm1 = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>Wifi config</title>
  </head>

  <body>
    <br>
)=====";

const String wifiConfForm2 = R"=====(
    <br><br>Enter your router credentials:<br><br>
    <form action="/submitQuery/" method="post">
      <label for="ssid">SSID: </label>
      <input type="text" id="ssid" name="ssid"><br><br>
      <label for="pasw">PASW: </label>
      <input type="text" id="pasw" name="pasw"><br><br>
      <input type="submit" value="Connect">
    </form>
  </body>

</html>
)=====";

String rootHTML;

const String wifiConnResponse = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>Wifi config</title>
  </head>

  <body>
    Connecting using new credentials...
  </body>

</html>
)=====";

const String bootResponse = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>Wifi config</title>
  </head>

  <body>
    Stopping server, resuming boot...
  </body>

</html>
)=====";



void handleRoot() {
  server.send(200, "text/html", rootHTML);
}

bool isNewData = false;
bool sta_save;
String sta_ssid;
String sta_pasw;
bool resumeBoot = false;

void handleSubmitQuery() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else if (server.args() == 0) {
    server.send(200, "text/html", bootResponse);
    resumeBoot = true;
    yield();
  }else if( ! server.hasArg("ssid") || ! server.hasArg("pasw") 
      || server.arg("ssid") == NULL || server.arg("pasw") == NULL) {
    
    server.send(400, "text/plain", "400: Invalid Request");
  } else {
    isNewData = true;
    server.send(200, "text/html", wifiConnResponse);
    sta_ssid = server.arg("ssid");
    sta_pasw = server.arg("pasw");
    sta_save = server.arg("saveConfig") == "1" ? true : false;
    yield();
  }
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

bool waitSTAConnection() {
  for (int t = 0; t < connectionTimeout; t++) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    Serial.print(".");
    delay(1000);
  }
  return false;
}

bool connectToWifiSTA(String ssid, String pasw, bool saveConfig) {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.persistent(saveConfig);
  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid, sta_pasw);
  return waitSTAConnection;
}

void runConfigurationServer (unsigned long timeout, String wifiInfo) {
  rootHTML = wifiConfForm1 + wifiInfo + wifiConfForm2;
  Serial.println(rootHTML);
  
  server.on("/", handleRoot);
  server.on("/submitQuery/", handleSubmitQuery);
  server.onNotFound(handleNotFound);
  server.begin();
  
  Serial.println("HTTP server started");

//  WiFiClient temp;
  if (timeout > 0) {
//    unsigned long t = millis();
//    unsigned long stopTime = t + timeout;
//    while (t < stopTime) {
//      temp = server.client();
//      if (temp != NULL) {
//        Serial.println("--------------");
//        Serial.println(temp.available());
//        Serial.println(temp.connected());
//      }
//      server.handleClient();
//      
//      t = millis();
//      delay(100);
//    }
  } else {
    while (true) {
      if (resumeBoot) {
        server.stop();
        break;
      } else if (isNewData) {
        // stop SERVER
//        server.stop();
        if (connectToWifiSTA(sta_ssid, sta_pasw, false)) {
          if (sta_save) {
            connectToWifiSTA(sta_ssid, sta_pasw, true);
          }
          break;
        } else {
          // restore previous WLAN state
          WiFi.disconnect();
          WiFi.softAP(ap_ssid, ap_pasw);
        }
        isNewData = false;
      }
      server.handleClient();
    }
  }
}

void setup(void) {
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // try to connect to the previous one
  WiFi.begin();

  // test if the connection was established
  unsigned long timeout;
  String wifiInfo;
  if (waitSTAConnection) {
    timeout = configPageTimeout;
    wifiInfo = "\nConnected to: " + WiFi.SSID() + "<br>\n";
    wifiInfo += "IP: " + WiFi.localIP().toString() + "\n";
  } else {
    WiFi.disconnect();
    WiFi.softAP(ap_ssid, ap_pasw);
    
    //<-----
    Serial.println("-------------------------------------");
    WiFi.printDiag(Serial);
    Serial.println("-------------------------------------");
    //----->
    
    timeout = 0;     // for an indefinite period of time
    wifiInfo = "\nCreated AP: " + ap_ssid + "<br>\n";
    wifiInfo += "IP: " + WiFi.softAPIP().toString() + "\n";    
  }

  //<-----
    Serial.println("");
    Serial.print(wifiInfo);
  //----->
  
  runConfigurationServer(timeout, wifiInfo);
}

void loop(void) {
  Serial.println("Good");
  delay(500);
}
