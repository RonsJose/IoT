#include <WiFi.h>
#include <NetworkClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "webpagecode.h"
#include "WifiCred.h"
#include <DFRobot_DHT11.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

AsyncWebServer server(80);

#define timer 1000
PulseOximeter pox;
uint32_t previousTime = 0;
float heartRate = 0;
float spo2 = 0;
DFRobot_DHT11 DHT;
#define DHT11_PIN 27
const int TRIG_PIN = 25;
const int ECHO_PIN = 14;
long duration;
float cms, inches;

String getTemp() {
   DHT.read(DHT11_PIN);
  Serial.print("temp:");
  Serial.print(DHT.temperature);
  return String(DHT.temperature) + " °C";
}

String getHumid(){
   DHT.read(DHT11_PIN);
  Serial.print("  humi:");
  Serial.println(DHT.humidity);
  return String(DHT.humidity) + " %";
}

String getDis(){
digitalWrite (TRIG_PIN, LOW);
delayMicroseconds (5) ;
digitalWrite (TRIG_PIN, HIGH);
delayMicroseconds (10) ;
digitalWrite (TRIG_PIN, LOW);
duration = pulseIn (ECHO_PIN, HIGH) ;
cms = (duration/2) / 29.1;
return String(cms) + " Cm";
}

String getHeartRate() {
  return String(heartRate, 1) + " bpm";
}

String getSpO2() {
  return String(spo2, 1) + " %";
}


void handleRoot(AsyncWebServerRequest *request) {
    String page = String(homePage);
    page.replace("%TEMPERATURE%", getTemp());
    page.replace("%HUMIDITY%", getHumid());
    page.replace("%DISTANCE%", getDis());
    page.replace("%HEARTRATE%", getHeartRate());
    page.replace("%SPO2%", getSpO2());
    request->send(200, "text/html", page);
}

void handleNotFound(AsyncWebServerRequest *request)  {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->params();
  message += "\n";
  for (uint8_t i = 0; i < request->params(); i++) 
  {
    const AsyncWebParameter* p = request->getParam(i);
    message += " " + p->name() + ": " + p->value() + "\n";
  }
 request->send(404, "text/plain", message);
}

void setup(void) {
  Serial.begin(115200);
  pinMode (TRIG_PIN, OUTPUT);
  pinMode (ECHO_PIN, INPUT);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("iot")) {
    Serial.println("MDNS responder started");
  }
  
  Serial.println("Initializing MAX30100...");

    if (!pox.begin()) {
    Serial.println("FAILED to initialize MAX30100!");
    while (1);
  } else {
    Serial.println("MAX30100 initialized successfully!");
  }
  
  server.on("/", HTTP_GET, handleRoot);

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", getTemp().c_str());
  });

  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", getHumid().c_str());
  });

  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain",getDis().c_str());
  });

  server.on("/heartRate", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", getHeartRate().c_str());
  });
  server.on("/spO2", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", getSpO2().c_str());
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
   pox.update();

  
  if (millis() - previousTime > timer) {
    heartRate = pox.getHeartRate();
    spo2 = pox.getSpO2();
    Serial.printf("Heart Rate: %.1f bpm | SpO2: %.1f%%\n", heartRate, spo2);
   previousTime = millis();
  }
}
