#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

const char* ssid = "Eric 2.4GHz";
const char* password = "ENTER WIFI PASSWORD"; // ENTER WIFI PASSWORD !!!
const char* mdnsName = "boiler-heater-pomp";
IPAddress ip(192, 168, 8, 56); // "192.168.8.56" : "boiler-heater-pomp.local"
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server = ESP8266WebServer(80);

void handleHeaterStatus()
{
  server.send(200, "application/json", heaterJsonStatus());
}

void handleHeaterOn()
{
    server.send(200, "application/json", heaterJsonStatus());
    digitalWrite(04, HIGH);
    setHeater(true);
}

void handleHeaterOff()
{
    server.send(200, "application/json", heaterJsonStatus());
    digitalWrite(04, LOW);
    setHeater(false);
}

String heaterJsonStatus ()
{
  int statusNumber = getHeater() == true ? 1 : 0;
  String msg = "{ \"status\": "; msg += statusNumber; msg += "}";
  return msg;
}

void setHeater(bool status)
{ 
  EEPROM.write(2, 0); // clear
  EEPROM.commit();
  EEPROM.write(2,status == true ? 1: 0); //write
  EEPROM.commit();
}

bool getHeater()
{
 String msg = "restored status to : "; if (EEPROM.read(2) == 1) {msg += "1";} else {msg += "0";} Serial.println(msg);
 return EEPROM.read(2) == 1 ? true : false;
}

void handlePompStatus()
{
  server.send(200, "application/json", pompJsonStatus());
}

void handlePompOn()
{
    server.send(200, "application/json", pompJsonStatus());
    digitalWrite(05, HIGH);
    setPomp(true);
}

void handlePompOff()
{
    server.send(200, "application/json", pompJsonStatus());
    digitalWrite(05, LOW);
    setPomp(false);
}

String pompJsonStatus ()
{
  int statusNumber = getPomp() == true ? 1 : 0;
  String msg = "{ \"status\": "; msg += statusNumber; msg += "}";
  return msg;
}

void setPomp(bool status)
{ 
  EEPROM.write(3, 0); // clear
  EEPROM.commit();
  EEPROM.write(3,status == true ? 1: 0); //write
  EEPROM.commit();
}

bool getPomp()
{
 String msg = "restored status to : "; if (EEPROM.read(3) == 1) {msg += "1";} else {msg += "0";} Serial.println(msg);
 return EEPROM.read(3) == 1 ? true : false;
}

void setup(void)
{
  pinMode(04, OUTPUT);
  pinMode(05, OUTPUT);
  EEPROM.begin(512);
 // EEPROM.end();
  //Serial.printf("status restored to: %d\n", EEPROM.read(0));
  if (getHeater() == true) {digitalWrite(04, HIGH);} else {digitalWrite(04, LOW);}
  if (getPomp() == true) {digitalWrite(05, HIGH);} else {digitalWrite(05, LOW);}
  
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
  MDNS.addService("http", "tcp", 80);

  server.on("/11", handlePompOn);
  server.on("/10", handlePompOff);
  server.on("/01", handleHeaterOn);
  server.on("/00", handleHeaterOff);
  server.on("/0status", handleHeaterStatus);
  server.on("/1status", handlePompStatus);
  //server.on("/statusForHomebridge", handleStatusForHomebridge);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
 
  Serial.printf("Flash chip ID: %d\n", ESP.getFlashChipId());
  Serial.printf("Flash chip size (in bytes): %d\n", ESP.getFlashChipSize());
  Serial.printf("Flash chip speed (in Hz): %d\n", ESP.getFlashChipSpeed());
}

void loop(void)
{
  server.handleClient();
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



