#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

//#define APSSID  "esp8266AP"
//#define APPASW  "123456789"
//#define CONF_PAGE_TIMEOUT  5UL
//#define CONN_TIMEOUT  30UL

const String ap_ssid = "esp8266AP";
const String ap_pasw = "pasw123456789";
//const unsigned long configPageTimeout = 10UL*1000;
const unsigned long connectionTimeout = 10;
//const unsigned long userActionTimeout = 60UL*1000;

ESP8266WebServer server(80);



const String rootHTML = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>Wifi config</title>
  </head>

  <body>
    <br>Enter wifi station credentials:<br><br>
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

const String wifiConnectionResponse = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="refresh" content="30; url=/checkConnectionState/">
    <title>Wifi config</title>
  </head>

  <body>
    Connecting using new credentials... <br>
    Autorefresh in 30 s. <br><br>
    Refresh manually if autorefresh not working: <br>
    <form action="/checkConnectionState/" method="get">
      <input type="submit" value="Check state">
    </form>
  </body>

</html>
)=====";

const String connectionSuccessResponse = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <title>Wifi config</title>
  </head>

  <body>
    Connected to wifi station.<br>
    Saving configuration, stopping server, rebooting...
  </body>

</html>
)=====";

const String connectionFailResponse = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="refresh" content="5; url=/">
    <title>Wifi config</title>
  </head>

  <body>
    Connection to wifi station failed.<br>
    Returning to main page in 5 s.<br><br>
    Refresh manually if autorefresh not working: <br>
    <form action="/" method="get">
      <input type="submit" value="Return to main">
    </form>
  </body>

</html>
)=====";


void handleRoot() {
  server.send(200, "text/html", rootHTML);
  yield();
}

bool isNewData = false;
//bool sta_save;
String sta_ssid;
String sta_pasw;
//bool resumeBoot = false;

void handleSubmitQuery() {
  if( ! server.hasArg("ssid") || ! server.hasArg("pasw") 
      || server.arg("ssid") == NULL || server.arg("pasw") == NULL) {
    server.send(400, "text/plain", "400: Invalid Request");
  } else {
    isNewData = true;
    server.send(200, "text/html", wifiConnectionResponse);
    sta_ssid = server.arg("ssid");
    sta_pasw = server.arg("pasw");
    yield();
  }
}

bool isSTAConnected = false;
bool isNotClientInformed = true;

void handleCheckConnectionState() {
  if (isSTAConnected) {
    isNotClientInformed = false;
    server.send(200, "text/html", connectionSuccessResponse);
  } else {
    server.send(200, "text/html", connectionFailResponse);
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
  WiFi.disconnect();
  Serial.println("Station not responding");
  return false;
}

bool connectToWifiSTA() {
  WiFi.begin(sta_ssid, sta_pasw);
  Serial.println("Waiting for connection");
  if (!waitSTAConnection()) {
    Serial.println("Wifi is unreachable!");
    return false;
  }
  return true;
}

bool saveSTAConfig() {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.persistent(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(sta_ssid, sta_pasw);
  Serial.println("Waiting for connection");
  if (!waitSTAConnection()) {
    Serial.println("Wifi is unreachable!");
    WiFi.persistent(false);
    WiFi.softAP(ap_ssid, ap_pasw);
    return false;
  }
  return true;
}


void runConfigurationServer () {
  // configure server
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submitQuery/", HTTP_POST, handleSubmitQuery);
  server.on("/checkConnectionState/", HTTP_GET, handleCheckConnectionState);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  while (true) {
    if (isNewData) {
      Serial.println("New data");
      if (connectToWifiSTA()) {
        isSTAConnected = true;
        unsigned long t = millis();
        unsigned long timeout = t + 120000UL;
        while(isNotClientInformed && t < timeout) {
          server.handleClient();
          delay(50);
          t = millis();
        }
        if(saveSTAConfig()) {
          Serial.println("Wifi config is completed");
          break;
        } else {
          Serial.println("Wifi config is not completed");
          isSTAConnected = false;
          isNotClientInformed = true;
        }
      } else {
        Serial.println("Connection failed");
      }
      isNewData = false;
    }
    server.handleClient();
  }
  Serial.println("configServer end");
}

void setup(void) {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // try to connect to the previous one
  WiFi.begin();
  if (!waitSTAConnection()) {
    Serial.println("Connection failed");
    WiFi.softAP(ap_ssid, ap_pasw);
    WiFi.printDiag(Serial);
  
    runConfigurationServer();   
  }
  
  WiFi.printDiag(Serial);
  Serial.println("Setup is done");
}

void loop(void) {
  Serial.println("Good");
  delay(500);
}
