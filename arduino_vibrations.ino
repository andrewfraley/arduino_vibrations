// ACCELEROMETER
#include <Adafruit_LSM303_Accel.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// TFT
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

// WIFI
#include "myconfig.hpp"
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
#include <ArduinoOTA.h>


const char* ssid = MYSSID;
const char* wifikey = WIFIKEY;

int seconds_to_gather = 5;  // interval to collect max sensor data in seconds
float init_x = 0;
float init_y = 0;
float init_z = 0;


// Use dedicated hardware SPI pins
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

/* Assign a unique ID to this sensor at the same time */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);


void setup(void) {
  Serial.begin(115200);
  delay(500);
  Serial.println("Booting...");

  // Accelerometer SETUP
  Serial.println("Initializing Accelerometer...");
  while (!accel.begin()) {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
  }
  accel.setRange(LSM303_RANGE_2G);  // 2G, 4G, 8G, 16G
  accel.setMode(LSM303_MODE_HIGH_RESOLUTION); // LSM303_MODE_NORMA, LSM303_MODE_LOW_POWER, LSM303_MODE_HIGH_RESOLUTION
  sensors_event_t event;
  accel.getEvent(&event);
  init_x = event.acceleration.x;
  init_y = event.acceleration.y;
  init_z = event.acceleration.z;
  Serial.println("Accelerometer OK");


  // TFT SETUP
  Serial.println("Initializing TFT Display...");
  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);
  tft.setTextSize(2);
  Serial.println("TFT Initialized");
  tft.println("Screen initialized.");

  // WIFI
  tft.println("Wifi initializing...");
  Serial.println("Wifi Initializing");

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, wifikey);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  
  tft.println("Wifi initialized!");
  

  // OTA
  Serial.println("Initializing ArduinoOTA...");
  tft.println("Initializing ArduinoOTA...");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  Serial.println("ArduinoOTA OK");
  tft.println("ArduinoOTA OK");

  Serial.println("System Ready!");
  tft.println("System ready!");
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);
  
}



void output_values(String x, String y, String z, String max_x, String max_y, String max_z) {
  /* Display the results (acceleration is measured in m/s^2) */
  // Serial.print("X: " + x + "  ");
  // Serial.print("Y: " + y + "  ");
  // Serial.print("Z: " + z + "  ");
  // Serial.println("m/s^2");

  tft.setCursor(0, 0);
  
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.println("X: " + x + "  ");

  tft.setTextSize(1);
  tft.println();
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_BLUE, ST77XX_BLACK);
  tft.println("Y: " + y + "  ");

  tft.setTextSize(1);
  tft.println();
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.println("Z: " + z + "  ");

  // Max Values
  tft.setTextSize(2);
  tft.println();

  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  tft.print("(" + String(seconds_to_gather) + "s):");

  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  tft.print(max_x + " ");

  tft.setTextColor(ST77XX_BLUE, ST77XX_BLACK);
  tft.print(max_y + " ");

  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  tft.print(max_z);

  tft.println();
}

void drawstatus(int status) {
  int x = 190;
  int y = 45;
  int radius = 15;


  if (status == 0) {
    tft.fillCircle(x, y, radius, ST77XX_GREEN);
  } else {
     tft.fillCircle(x, y, radius, ST77XX_RED);
  }
}

void influx_send(String x, String y, String z) {
  
  String measurement="accelerometer";
  String tags="location=grinder_discharge";
  String payload = measurement + "," + tags + " x=" + x + ",y=" + y + ",z=" + z;
  Serial.println(payload);
  String url = "http://grafana.localdomain:8086/write?db=arduino";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  Serial.println("Sending payload");

  int httpCode = http.POST(payload);

  // httpCode will be negative on error
  if(httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if(httpCode == 204) {
        Serial.println("Influx send OK");
        drawstatus(0);
      } else {
        Serial.print("Influx send FAILED  Code: ");
        Serial.println(httpCode);
        drawstatus(1);
      }
  } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      drawstatus(1);
  }
  http.end();

}



void loop(void) {
  ArduinoOTA.handle();
  int delay_ms = 25;
  int loops = seconds_to_gather * 1000 / delay_ms;
  float max_x = 0;
  float max_y = 0;
  float max_z = 0;
  float x;
  float y;
  float z;
  sensors_event_t event;

  
  
  for (int i=0; i<loops; i++) {
  
    accel.getEvent(&event);
    x = abs(event.acceleration.x - init_x);
    y = abs(event.acceleration.y - init_y);
    z = abs(event.acceleration.z - init_z);

    max_x = max(x, max_x);
    max_y = max(x, max_y);
    max_z = max(x, max_z);

    output_values(String(x), String(y), String(z), String(max_x), String(max_y), String(max_z));
    delay(delay_ms);
  }
  
  Serial.println("----MAX VALUES--------");
  Serial.println("X: " + String(x) + "  Y: " + String(y) + "  Z: " + String(z));
  Serial.println("----------------------");

  influx_send(String(max_x), String(max_y), String(max_z));

  delay(100);
}
