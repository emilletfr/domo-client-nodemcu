#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions
#include <SimpleDHT.h>

// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int pinDHT11 = 0;
SimpleDHT11 dht11;



IPAddress ip(10, 0, 1, 12); // 10.0.1.10 : Salon / 10.0.1.11 : Salle à manger / 10.0.1.12 : Bureau / 10.0.1.13 : Cuisine
IPAddress gateway(10, 0, 1, 1);
IPAddress subnet(255, 255, 255, 0);

const char* ssid = "Airport Extreme";
const char* password = "ENTER WIFI PASSWORD"; // ENTER WIFI PASSWORD !!! 
MDNSResponder mdns;


ESP8266WebServer server(80);


void handleStatusForHomebridge()
{
  String statusString = getStatus() == true ? "1" : "0";
  server.send(200, "text/plain", statusString);
}

void handleStatus()
{
  server.send(200, "application/json", jsonStatus());
}

void handleClose()
{
  setStatus(false);
  server.send(200, "application/json", jsonStatus());
  digitalWrite(05, LOW); digitalWrite(04, LOW);
  digitalWrite(05, HIGH); delay(500); digitalWrite(05, LOW);

}

void handleOpen()
{
  setStatus(true);
  server.send(200, "application/json", jsonStatus());
  digitalWrite(04, LOW); digitalWrite(05, LOW);
  digitalWrite(04, HIGH); delay(500); digitalWrite(04, LOW);

}

String jsonStatus ()
{
// read without samples.
  byte temperature = 0;
  byte humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    Serial.print("Read DHT11 failed.");
    temperature = 0;
    humidity = 0;
  }
  
  Serial.print("Sample OK: ");
  Serial.print((int)temperature); Serial.print(" *C, "); 
  Serial.print((int)humidity); Serial.println(" %");


  
  //int statusNumber = getStatus() == true ? 1 : 0;
  String msg = "{ \"open\": "; msg += getStatus();msg +=", \"temperature\": "; msg += (int)temperature ;msg +=", \"humidity\": "; msg += (int)humidity ; msg += "}";
  return msg;
}

void setStatus(bool status)
{
  EEPROM.write(0, 0); // clear
  EEPROM.commit();
  EEPROM.write(0, status == true ? 1 : 0); //write
  EEPROM.commit();
}

bool getStatus()
{
  String msg = "restored status to : "; if (EEPROM.read(0) == 1) {
    msg += "1";
  } else {
    msg += "0";
  } Serial.println(msg);
  return EEPROM.read(0) == 1 ? true : false;
}

void setup(void)
{
  pinMode(05, OUTPUT);
  pinMode(04, OUTPUT);
  digitalWrite(05, LOW);
  digitalWrite(04, LOW);

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

  server.on("/1", handleOpen);
  server.on("/0", handleClose);
  server.on("/status", handleStatus);
  server.on("/statusForHomebridge", handleStatusForHomebridge);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  Serial.printf("Flash chip ID: %d\n", ESP.getFlashChipId());
  Serial.printf("Flash chip size (in bytes): %d\n", ESP.getFlashChipSize());
  Serial.printf("Flash chip speed (in Hz): %d\n", ESP.getFlashChipSpeed());

  EEPROM.begin(512);
  // EEPROM.end();
  Serial.printf("status restored to: %d\n", EEPROM.read(0));
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



