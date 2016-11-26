#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Arduino.h>  // for type definitions
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define TEMPERATURE_MODULE_OR_VIBRATION_MODULE false
IPAddress ip(10, 0, 1, 14); // 10.0.1.10 : Salon / 10.0.1.11 : Salle à manger / 10.0.1.12 : Bureau / 10.0.1.13 : Cuisine
IPAddress gateway(10, 0, 1, 1);
IPAddress subnet(255, 255, 255, 0);
const char* ssid = "Airport Extreme";
const char* password = "ENTER WIFI PASSWORD"; // ENTER WIFI PASSWORD !!!
MDNSResponder mdns;

#if TEMPERATURE_MODULE_OR_VIBRATION_MODULE == true

#define DHTPIN 0         // Pin which is connected to the DHT sensor.
// Uncomment the type of sensor in use:
//#define DHTTYPE           DHT11     // DHT 11
#define DHTTYPE           DHT22     // DHT 22 (AM2302)
//#define DHTTYPE           DHT21     // DHT 21 (AM2301)
// See guide for details on sensor wiring and usage:
//   https://learn.adafruit.com/dht/overview
DHT_Unified dht(DHTPIN, DHTTYPE);
// for DHT11,
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
//int pinDHT11 = 0;
//SimpleDHT11 dht11;
float temperature = 0;
float humidity = 0;

#else

int EP = 0;
int measurements[60];
int indexMeasurement = 0;
long measurementsSumMinute = 0;
long measurementSeconde = 0;

#endif

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
#if TEMPERATURE_MODULE_OR_VIBRATION_MODULE == true
  Serial.print((int)temperature); Serial.print(" *C, ");
  Serial.print((int)humidity); Serial.println(" %");
  String msg = "{ \"open\": "; msg += getStatus(); msg += ", \"temperature\": "; msg += temperature ; msg += ", \"humidity\": "; msg += humidity ; msg += "}";
  return msg;
#else
  Serial.print("measurementsSumMinute: "); Serial.print(measurementsSumMinute);
  Serial.print(" - measurementSeconde: "); Serial.println(measurementSeconde);
  String msg = "{ \"open\": "; msg += getStatus(); msg += ", \"measurementsSumMinute\": "; msg += measurementsSumMinute ; msg += ", \"measurementSeconde\": "; msg += measurementSeconde ; msg += "}";
  return msg;
#endif
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
  }
  else {
    msg += "0";
  }
  Serial.println(msg);
  return EEPROM.read(0) == 1 ? true : false;
}

void setup(void)
{
  pinMode(05, OUTPUT);
  pinMode(04, OUTPUT);
  digitalWrite(05, LOW);
  digitalWrite(04, LOW);

  Serial.begin(9600);
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

#if TEMPERATURE_MODULE_OR_VIBRATION_MODULE == true
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
#else
  pinMode(EP, INPUT); //set EP input for measurment
  for (int i = 0; i < 60; i++) {measurements[i] = 0;}
#endif
}

void loop(void)
{
  server.handleClient();

#if TEMPERATURE_MODULE_OR_VIBRATION_MODULE == true
  /*
    temperature = 0;
    humidity = 0;
    if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    Serial.print("Read DHT11 failed.");
    temperature = 0;
    humidity = 0;
    }
  */
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
#else
  delay(50);
  long measurement = pulseIn (EP, HIGH, 1000000); //wait for the pin to get HIGH and returns measurement
  measurementSeconde = measurement;
  if (measurement > 0) {
    delay(1000);
  }
  indexMeasurement = indexMeasurement + 1; if (indexMeasurement == 60) {
    indexMeasurement = 0;
  }
  measurements[indexMeasurement] = measurement;
  measurementsSumMinute = 0;
  for (int i = 0; i < 60; i++) {
    measurementsSumMinute = measurementsSumMinute + measurements[i];
  }
  Serial.print("measurementsSumMinute: "); Serial.print(measurementsSumMinute);
  Serial.print(" - measurementSeconde: "); Serial.println(measurementSeconde);
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



