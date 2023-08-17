/*
Code to display heart rate and spo2 values
In the Serial Monitor and 128x32 OLED displaya as well as an adafruit.IO dashboard
*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"
#include <WiFi.h>
#include <PubSubClient.h>

#define REPORTING_PERIOD_MS 1000

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

#define WIFI_SSID "your wifi password"
#define WIFI_PASS "your hotspot password"


#define MQTT_SERVER "io.adafruit.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "your adafruit io username"
#define MQTT_KEY "your adafruit io key"

// initialising sensor object
PulseOximeter pox;
uint32_t tsLastReport = 0;

// for te oled display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// to connect esp to client (Adafruit IO)
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// defining a function to connect the esp32
// to your hotspot
void setupWiFi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP().toString());  // Convert IPAddress to string
}

// to send info to the adafruit io feeds
void publishData(const char* username, const char* topic, int count) {
  char fullTopic[100];
  sprintf(fullTopic, "%s/feeds/%s", username, topic);

  char payload[10];
  sprintf(payload, "%d", count);
  client.publish(fullTopic, payload);
  Serial.println("Published count: " + String(count));
}

// not needed unless you are subscribing
// ie extracting data from adarfuit io
// this code just publishes
void callback(char* topic, byte* payload, unsigned int length) {
  // Handle incoming messages (if needed)
}

/*
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "esp32-client-";                                     // not needed, it messes up the reading and output value of the max30100 sensor
    clientId += String(random(0xffff), HEX);                               // instead just take lines 81, 82, 83 and put it in void setup > works well enough
    if (client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_KEY)) {       // but connection issues could be a prob
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
*/

void setup() {
  Serial.begin(115200);

  // connecting to wifi
  setupWiFi();
  client.setServer("io.adafruit.com", 1883);    // connecting to adafruit io
  client.setCallback(callback);
  String clientId = "esp32-client-";        
  clientId += String(random(0xffff), HEX);
  client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_KEY);

  // oled setup
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {         
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
  display.println("Loading data");
  display.display();
  delay(3000);
  display.clearDisplay();
  display.println("Place your finger");
  display.display();
  delay(2000);
  display.clearDisplay();

  // max30100 sensor setup
  if (!pox.begin()) {
    Serial.println("Pulse oximeter initialization failed");
    for (;;)
      ;
  }
}

void loop() {

  // this is very imp for the sensor
  pox.update();

  /*
    if (!client.connected()) {                // again this messes up the reading of the max30100 sensor (not 100% sure about these 3 lines)
      reconnect();                            // commented out, so connection issues could mess up the results
    }                                         // so just call reconnect() once in void setup
    */

  client.loop();

  float heartrate;
  float spo2;
  
  // reading values from the sensor at correct intervals
  // w/o this condition the sensor readings get messy >> always gives 0
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    heartrate = pox.getHeartRate();
    spo2 = pox.getSpO2();

    Serial.print("Heart rate: ");
    Serial.print(heartrate);
    Serial.print(" bpm / SpO2: ");
    Serial.print(spo2);
    Serial.println("%");


    display.clearDisplay();
    display.setCursor(0, 10);
    display.print("Heart rate:");
    display.print(heartrate);
    display.setCursor(0, 25);
    display.print("bpm / SpO2:");
    display.print(spo2);
    display.print("%");
    display.display();

    publishData(MQTT_USERNAME, "heartrate", heartrate);
    publishData(MQTT_USERNAME, "spo2", spo2);

    tsLastReport = millis();
  }
}
