#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Arduino.h>  // for type definitions
#include <Servo.h>

const char* mdnsName = "boiler-temperature";
IPAddress ip(10, 0, 1, 25);
IPAddress gateway(10, 0, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const char* ssid = "Airport Extreme";
const char* password = "ENTER WIFI PASSWORD"; // ENTER WIFI PASSWORD !!!
MDNSResponder mdns;
ESP8266WebServer server = ESP8266WebServer(80);
Servo myservo;  // create servo object to control a servo
// twelve servo objects can be created on most boards


void handleGetTemperature()
{
  int servoDegres = myservo.read();
  int boilerTemperature = int( 90.0 - 30.0 * float(servoDegres) / 115.0 );
  String msg = "[{ \"value\": "; msg += boilerTemperature; msg += "}]";
  server.send(200, "application/json", msg);
}


void handleSetTemperature()
{
  // server.args() -> Get number of parameters
  // server.argName(i) -> Get the name of the parameter
  // server.arg(i) -> Get the value of the parameter

  if (server.args() > 0) // angle 0° -> temperature 90°  -  angle 115° -> temperature 60°
  {
    int boilerTemperature = server.arg(0).toInt();
    if (boilerTemperature < 60) {
      boilerTemperature = 60;
    }
    if (boilerTemperature > 90) {
      boilerTemperature = 90;
    }
    int servoDegres = int((90.0 - float(boilerTemperature)) * 115.0 / 30.0);
    String msg = "[{ \"servoDegres\": "; msg += servoDegres; msg += "}]";
    server.send(200, "application/json", msg);
    myservo.write(servoDegres);
  }
  else
  {
    server.send(404, "text/plain", "no paramaters !!!");
  }
}


void setup()
{
  Serial.begin(115200);
  WiFi.config(ip, gateway, subnet);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
  }
  Serial.println(""); Serial.print("Connected to "); Serial.println(ssid); Serial.print("IP address: "); Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  server.on("/getTemperature", handleGetTemperature);
  server.on("/setTemperature", handleSetTemperature);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  Serial.printf("Flash chip ID: %d\n", ESP.getFlashChipId());
  Serial.printf("Flash chip size (in bytes): %d\n", ESP.getFlashChipSize());
  Serial.printf("Flash chip speed (in Hz): %d\n", ESP.getFlashChipSpeed());

  myservo.attach(5);  // attaches the servo on GIO2 to the servo object
  myservo.write(0); // angle 0° -> temperature 90°  -  angle 115° -> temperature 60°
  delay(60000); // 1mn pour caler le moteur à 90° de temperature de chauffe
}


void loop()
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


