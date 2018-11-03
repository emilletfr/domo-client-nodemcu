//#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#//include <EEPROM.h>
#include <HX711.h>
#include <WebSocketsServer.h>

const char* ssid = "Eric 2.4GHz";
const char* password = "ENTER WIFI PASSWORD"; // ENTER WIFI PASSWORD !!!
const char* mdnsName = "bed-occupancy";
IPAddress ip(192, 168, 8, 55); // "192.168.8.55" : "bed-occupancy.local"
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server = ESP8266WebServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);
HX711 scale;

int pressionThreshold = 45; // (scale.read()/10000) 34 lit seul, 50  Moi à droite, 69 moi à gauche
int pressionCurrentValue = 0;

void handleStatus()
{
  server.send(200, "application/json", jsonStatus());
}

void handleHtmlStatus()
{
  // send index.html
  String text = "<script src=https:\/\/www.gstatic.com\/charts\/loader.js><\/script><script>function drawChart(){var o=google.visualization.arrayToDataTable([[\"Label\",\"Value\"],[\"WIFI\",0]]),e={width:500,height:500,redFrom:-100,redTo:-85,yellowFrom:-85,yellowTo:-75,greenFrom:-75,greenTo:-50,minorTicks:10,min:-100,max:-50,animation:{duration:0},majorTicks:[-100,-50]},a=new google.visualization.Gauge(document.getElementById(\"chart_div\"));a.draw(o,e);var n=new WebSocket(\"ws:\/\/\"+location.hostname+\":81\/\",[\"arduino\"]);n.onopen=function(){n.send(\"Connect \"+new Date)},n.onerror=function(o){console.log(\"WebSocket Error \",o)},n.onmessage=function(n){console.log(\"Server: \",n.data),o.setValue(0,1,n.data),a.draw(o,e)}.bind(this)}google.charts.load(\"current\",{packages:[\"gauge\"]}),google.charts.setOnLoadCallback(drawChart)<\/script><div id=chart_div style=width:500px;height:500px><\/div>";
  server.send(200, "text/html", text);
}

String jsonStatus ()
{
  int inBed = pressionCurrentValue > pressionThreshold ? 1 : 0;
  String msg = "{ \"inBed\": "; msg += inBed ; msg += ", \"pressionThreshold\": "; msg += pressionThreshold ; msg += ", \"currentPression\": "; msg += pressionCurrentValue; msg += "}";
  return msg;
}
/*
  void setPressionThreshold(int status)
  {
  EEPROM.write(1, 0); EEPROM.write(2, 0); // clear
  EEPROM.commit();
  EEPROM.write(1, status / 256); EEPROM.write(2, status % 256); //write
  EEPROM.commit();
  }

  int getPressionThreshold()
  {
  return EEPROM.read(1) * 256 + EEPROM.read(2);
  }
*/

uint8_t webSocketConNum = 0;
bool webSocketIsCon = false;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght)
{
  switch (type) {
    case WStype_DISCONNECTED:
      webSocketIsCon = false;
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        webSocketIsCon = true;
        IPAddress ip = webSocket.remoteIP(num);
        webSocketConNum = num;
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

        // send message to client
        //  webSocket.sendTXT(num, "Connected");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      if (payload[0] == '#') {
        // we get RGB data

        // decode rgb data
        uint32_t rgb = (uint32_t) strtol((const char *) &payload[1], NULL, 16);
      }
      break;
  }
}


void setup(void)
{
  pinMode(D1, OUTPUT);
  pinMode(D2, INPUT);
  scale.begin(D1, D2);
  //pinMode(LED_BUILTIN, OUTPUT);
  //digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);

  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(""); Serial.print("Connected to "); Serial.println(ssid); Serial.print("IP address: "); Serial.println(WiFi.localIP());
  
  if (!MDNS.begin(mdnsName)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");

  // start webSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.on("/status", handleStatus);
  server.on("/", handleHtmlStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);
  Serial.println("HTTP server started");
  Serial.printf("Flash chip ID: %d\n", ESP.getFlashChipId());
  Serial.printf("Flash chip size (in bytes): %d\n", ESP.getFlashChipSize());
  Serial.printf("Flash chip speed (in Hz): %d\n", ESP.getFlashChipSpeed());

  scale.power_up();
}

int loopCounter = 1;
void loop(void)
{
  delay(1); loopCounter++; if (loopCounter > 1000) {
    loopCounter = 1;
  }
  webSocket.loop();
  server.handleClient();
  if (loopCounter % 1000 == 0) // 1sec
  {
    // scale.power_up();
    int scaleRead = scale.read() / 10000;
    if (scaleRead > 0) {
      pressionCurrentValue = scaleRead;
    }
    //  scale.power_down();
  }

  if (loopCounter % 20 == 0) // 20msec
  {
    String value = String(WiFi.RSSI());
    if (webSocketIsCon == true) {
      // webSocket.sendTXT(webSocketConNum, value);
      webSocket.broadcastTXT(value);
    }
  }
}

void handleNotFound()
{
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



