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

// Creds stored in myconfig.hpp
// #define MYSSID "some_ssid";
// #define WIFIKEY "some_key";
const char* ssid = MYSSID;
const char* wifikey = WIFIKEY;

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

  Serial.println("OTA wait on boot...");
  tft.println("OTA wait on boot...");
  unsigned long startTime = millis(); // Get the current time in milliseconds
  while (millis() - startTime < 5000) { // Loop while the difference between the current time and the start time is less than 5000 milliseconds (5 seconds)
    ArduinoOTA.handle();
  }


  Serial.println("System Ready!");
  tft.println("System ready!");
  delay(1000);
  tft.fillScreen(ST77XX_BLACK);

}



void output_values(float x, float y, float z, float acc) {
  /* Display the results (acceleration is measured in m/s^2) */
  // Serial.print("X: " + x + "  ");
  // Serial.print("Y: " + y + "  ");
  // Serial.print("Z: " + z + "  ");
  // Serial.println("m/s^2");

  char output[50];
  tft.setCursor(0, 0);

  tft.setTextSize(3);
  tft.setTextColor(ST77XX_GREEN, ST77XX_BLACK);
  sprintf(output, "X: %.2f  ", x);
  tft.println(output);

  tft.setTextSize(1);
  tft.println();
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_BLUE, ST77XX_BLACK);
  sprintf(output, "Y: %.2f  ", y);
  tft.println(output);

  tft.setTextSize(1);
  tft.println();
  tft.setTextSize(3);
  tft.setTextColor(ST77XX_RED, ST77XX_BLACK);
  sprintf(output, "Z: %.2f  ", z);
  tft.println(output);

  // Max Values
  tft.setTextSize(2);
  tft.println();

  tft.setTextColor(ST77XX_MAGENTA, ST77XX_BLACK);
  sprintf(output, "Accel (5s): %.4f", acc);
  tft.println(output);
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

void influx_send(float x, float y, float z, float acc) {

  char payloadc[200];
  sprintf(payloadc, "accelerometer,location=grinder_discharge x=%.4f,y=%.4f,z=%.4f,acc=%.4f", x, y, z, acc);
  String url = "http://grafana.localdomain:8086/write?db=arduino";
  String payload = String(payloadc);
  Serial.println(payload);

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
  int loop_time = 5000;  // ms to loop for
  float max_x = 0;
  float max_y = 0;
  float max_z = 0;
  float x = 0;
  float y = 0;
  float z = 0;
  float acc = 0;
  float max_acc = 0;
  sensors_event_t event;
  int loops = 0;



  unsigned long startTime = millis(); // Get the current time in milliseconds
  while (millis() - startTime < loop_time) { // Loop while the difference between the current time and the start time is less than 5000 milliseconds (5 seconds)

    accel.getEvent(&event);
    x = abs(event.acceleration.x);
    y = abs(event.acceleration.y);
    z = abs(event.acceleration.z);

    max_x = max(x, max_x);
    max_y = max(y, max_y);
    max_z = max(z, max_z);

    acc = (sqrt(event.acceleration.x * event.acceleration.x +
                        event.acceleration.y * event.acceleration.y +
                        event.acceleration.z * event.acceleration.z)) / 9.806;

    max_acc = max(acc, max_acc);
    if (loops % 10 == 0) output_values(x, y, z, max_acc);
    loops++;
  }
  ArduinoOTA.handle();
  output_values(max_x, y, z, max_acc);
  Serial.println("------MAX VALUES------");
  Serial.printf("X: %.4f  Y: %.4f  Z: %.4f  ACC: %.4f\n", x, y, z, acc);
  Serial.println("----------------------");
  Serial.println(loops);

  influx_send(max_x, max_y, max_z, max_acc);
}
