#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

#define TEMPERATURE_MODULE true
const char* ssid = "Eric 2.4GHz";
const char* password = "ENTER WIFI PASSWORD"; // ENTER WIFI PASSWORD !!!
const char* mdnsName = "living-room"; // living-room dining-room office kitchen bedroom
IPAddress ip(192, 168, 8, 50); // "192.168.8.50":"living-room.local" / ".51":"dining-room.local" / ".52":"office.local" /  ".53":"kitchen.local" / ".54":"bedroom.local" 
IPAddress gateway(192, 168, 8, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server = ESP8266WebServer(80);

#if TEMPERATURE_MODULE == true

#include <Arduino.h>  // for type definitions
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 0         // Pin which is connected to the DHT sensor.
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
DHT_Unified dht(DHTPIN, DHTTYPE);

#endif
float temperature = 0;
float humidity = 0;



void handleStatusForHomebridge()
{
  String statusString = getOpen() == true ? "1" : "0";
  server.send(200, "text/plain", statusString);
}

void handleStatus()
{
  server.send(200, "application/json", jsonStatus());
}

void handleClose()
{
  setOpen(false);
  server.send(200, "application/json", jsonStatus());
  digitalWrite(05, LOW); digitalWrite(04, LOW);
  digitalWrite(05, HIGH); delay(500); digitalWrite(05, LOW);

}

void handleOpen()
{
  setOpen(true);
  server.send(200, "application/json", jsonStatus());
  digitalWrite(04, LOW); digitalWrite(05, LOW);
  digitalWrite(04, HIGH); delay(500); digitalWrite(04, LOW);

}

String jsonStatus ()
{
  Serial.print((int)temperature); Serial.print(" *C, ");
  Serial.print((int)humidity); Serial.println(" %");
  String msg = "{ \"open\": "; msg += getOpen(); msg += ", \"temperature\": "; msg += temperature ; msg += ", \"humidity\": "; msg += humidity ; msg += "}";
  return msg;
}

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

void setOpen(bool status)
{
  EEPROM.write(0, 0); // clear
  EEPROM.commit();
  EEPROM.write(0, status == true ? 1 : 0); //write
  EEPROM.commit();
}

bool getOpen()
{
  String msg = "restored status to : ";
  if (EEPROM.read(0) == 1) {
    msg += "1";
  }
  else {
    msg += "0";
  }
  Serial.println(msg);
  return EEPROM.read(0) == 1 ? true : false;
}

void setup(void)
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(05, OUTPUT);
  pinMode(04, OUTPUT);
  digitalWrite(05, LOW);
  digitalWrite(04, LOW);

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

#if TEMPERATURE_MODULE == true
  // Initialize device.
  dht.begin();
  Serial.println("DHTxx Unified Sensor Example");
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");
  Serial.println("------------------------------------");
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");
  Serial.println("------------------------------------");
#endif

}

void loop(void)
{
  server.handleClient();

#if TEMPERATURE_MODULE == true
  temperature = 0;
  humidity = 0;
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println("Error reading temperature!");
  }
  else
  {
    temperature = event.temperature;
    Serial.print("Temperature: ");
    // Serial.print(event.temperature);
    Serial.print(temperature);
    Serial.println(" *C");
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity))
  {
    Serial.println("Error reading humidity!");
  }
  else
  {
    humidity = event.relative_humidity;
    Serial.print("Humidity: ");
    // Serial.print(event.relative_humidity);
    Serial.print(humidity);
    Serial.println("%");
  }
  delay(2000);
#endif
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



