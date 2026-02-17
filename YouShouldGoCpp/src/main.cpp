#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "Arduino_GFX_Library.h"

#ifndef WIFI_SSID
#define WIFI_SSID "x"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "x"
#endif

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* serverUrl = "x";

// Display pins
#define PIN_POWER 15
#define PIN_BACKLIGHT 38

Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5, 0, true, 170, 320, 35, 0, 35, 0);

void showMessage(const String &text, uint16_t color = WHITE, int textSize = 2, int y = 60) {
  gfx->fillScreen(BLACK);
  gfx->setTextSize(textSize);
  gfx->setTextColor(color);
  gfx->setCursor(10, y);
  gfx->println(text);
}

void displayWrappedText(const String &text, int startY = 40) {
  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);
  
  int y = startY;
  String line = "";
  const int maxCharsPerLine = 50; // ~6px per char, 300px width
  
  for (unsigned int i = 0; i < text.length() && y < gfx->height() - 20; i++) {
    char c = text[i];
    
    if (c == '\n' || line.length() >= maxCharsPerLine) {
      gfx->setCursor(10, y);
      gfx->println(line);
      y += 12;
      line = (c == '\n') ? "" : String(c);
    } else {
      line += c;
    }
  }
  
  if (line.length() > 0 && y < gfx->height() - 20) {
    gfx->setCursor(10, y);
    gfx->println(line);
  }
}

void initDisplay() {
  pinMode(PIN_POWER, OUTPUT);
  digitalWrite(PIN_POWER, HIGH);
  pinMode(PIN_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_BACKLIGHT, HIGH);
  
  gfx->begin();
  gfx->setRotation(3);
  gfx->fillScreen(BLACK);
}

void setup() {
  Serial.begin(115200);
  initDisplay();
  
  showMessage("Connecting WiFi...");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi connected");
  showMessage("WiFi Connected!", GREEN);
  delay(1000);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected");
    delay(15000);
    return;
  }
  
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;
  
  if (!http.begin(*client, serverUrl)) {
    delete client;
    delay(15000);
    return;
  }
  
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("payload: " + payload);
    
    gfx->fillScreen(BLACK);
    gfx->setTextSize(2);
    gfx->setTextColor(GREEN);
    gfx->setCursor(10, 10);
    gfx->println("Server Response:");
    
    displayWrappedText(payload);
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    showMessage("HTTP Error:\n" + http.errorToString(httpCode), RED);
  }
  
  http.end();
  delete client;
  delay(15000);
}