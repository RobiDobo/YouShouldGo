#include "app.h"
#include "utils.h"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;
const char* serverUrl = "https://youshouldgo.onrender.com";

#define BTN_NEXT 14
#define BTN_SELECT 0

std::vector<Route> routes;
int currentRouteIndex = 0;
bool routesLoaded = false;

std::vector<Trip> trips;
int currentTripIndex = 0;
bool tripsLoaded = false;

std::vector<Station> stations;
int currentStationIndex = 0;
bool stationsLoaded = false;
unsigned long lastButtonPress = 0;
const unsigned long DEBOUNCE_DELAY = 200;
const unsigned long LONG_PRESS_MS = 800;
const unsigned long CLEAR_NVS_PRESS_MS = 10000;
const unsigned long STATUS_POLL_INTERVAL = 2000;
Screen currentScreen = SCREEN_ROUTES;

unsigned long lastStatusFetch = 0;
String lastStatus = "";

bool selectPressed = false;
bool selectLongHandled = false;
bool clearWarningShown = false;
unsigned long selectPressStart = 0;
unsigned long lastPopupUpdate = 0;
const unsigned long POPUP_UPDATE_INTERVAL = 200;
const unsigned long POPUP_START_DELAY = 1000;

// NVS globals
nvs_handle_t nvsHandle = 0;
esp_err_t nvsErr = ESP_OK;

void initButtons() {
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);
}

void startWiFi() {
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWi-Fi connected");
}

bool checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected");
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(5000);  //wait for serial to be ready
  initDisplay();
  initNVS();
  initButtons();
  startWiFi();
  delay(1000);
  loadRoutes();
}

void loop() {
  if (!checkWiFi()) {
    delay(5000);
    return;
  }
  
  unsigned long now = millis();

  // Handle SELECT button presses
  bool selectDown = (digitalRead(BTN_SELECT) == LOW);
  if (selectDown) {
    if (!selectPressed) {
      selectPressed = true;
      selectPressStart = now;
      selectLongHandled = false;
      clearWarningShown = false;
      lastPopupUpdate = 0;
    } else {
      unsigned long elapsed = now - selectPressStart;
      
      if (!clearWarningShown && elapsed >= CLEAR_NVS_PRESS_MS) {
        clearWarningShown = true;
        gfx->fillScreen(BLACK);
        gfx->setTextSize(2);
        gfx->setTextColor(YELLOW);
        gfx->setCursor(6, 60);
        gfx->println("Clearing cache...");
        Serial.println("User requested NVS clear (10s hold)");
        
        clearNVS();
        delay(1000);
        
        loadRoutes();
        selectLongHandled = true;
        lastButtonPress = now;
      } else if (!selectLongHandled && elapsed >= LONG_PRESS_MS) {
        selectLongHandled = true;
        lastButtonPress = now;
        if (currentScreen != SCREEN_ROUTES) {
          currentScreen = SCREEN_ROUTES;
          displayCurrentRoute();
        }
      } else if (elapsed >= POPUP_START_DELAY && !clearWarningShown && (now - lastPopupUpdate >= POPUP_UPDATE_INTERVAL)) {
        lastPopupUpdate = now;
        unsigned long remaining = CLEAR_NVS_PRESS_MS - elapsed;
        drawClearPopup(remaining);
      }
    }
  } else if (selectPressed) {
    if (!selectLongHandled && (now - lastButtonPress > DEBOUNCE_DELAY)) {
      lastButtonPress = now;

      if (currentScreen == SCREEN_ROUTES) {
        if (routesLoaded && !routes.empty()) {
          loadTripsForRoute(routes[currentRouteIndex].route_id);
        } else {
          loadRoutes();
        }
      } else if (currentScreen == SCREEN_TRIPS) {
        if (tripsLoaded && !trips.empty()) {
          loadStations();
        } else if (routesLoaded && !routes.empty()) {
          loadTripsForRoute(routes[currentRouteIndex].route_id);
        }
      } else if (currentScreen == SCREEN_STATIONS) {
        if (stationsLoaded && !stations.empty()) {
          selectStation();
        }
      } else if (currentScreen == SCREEN_STATUS) {
        currentScreen = SCREEN_STATIONS;
        displayCurrentStation();
        lastStatus = "";
      }
    }
    
    // Clear popup if it was shown but user released early
    if (clearWarningShown && !selectLongHandled) {
      clearWarningShown = false;
      selectLongHandled = false;
      if (currentScreen == SCREEN_ROUTES) {
        displayCurrentRoute();
      } else if (currentScreen == SCREEN_TRIPS) {
        displayCurrentTrip();
      } else if (currentScreen == SCREEN_STATIONS) {
        displayCurrentStation();
      } else if (currentScreen == SCREEN_STATUS) {
        // Will be redrawn by polling
      }
    }
    
    selectPressed = false;
  }
  
  // Handle NEXT button (navigation)
  if (now - lastButtonPress > DEBOUNCE_DELAY) {
    if (digitalRead(BTN_NEXT) == LOW) {
      lastButtonPress = now;

      if (currentScreen == SCREEN_ROUTES) {
        if (routesLoaded && !routes.empty()) {
          currentRouteIndex = (currentRouteIndex + 1) % routes.size();
          displayCurrentRoute();
          Serial.println("Next route: " + String(currentRouteIndex));
        } else {
          loadRoutes();
        }
      } else if (currentScreen == SCREEN_TRIPS) {
        if (tripsLoaded && !trips.empty()) {
          currentTripIndex = (currentTripIndex + 1) % trips.size();
          displayCurrentTrip();
          Serial.println("Next trip: " + String(currentTripIndex));
        } else if (routesLoaded && !routes.empty()) {
          loadTripsForRoute(routes[currentRouteIndex].route_id);
        }
      } else if (currentScreen == SCREEN_STATIONS) {
        if (stationsLoaded && !stations.empty()) {
          currentStationIndex = (currentStationIndex + 1) % stations.size();
          displayCurrentStation();
          Serial.println("Next station: " + String(currentStationIndex + 1));
        } else {
          loadStations();
        }
      } else if (currentScreen == SCREEN_STATUS) {
        currentScreen = SCREEN_STATIONS;
        displayCurrentStation();
        lastStatus = "";
      }

      delay(200);
    }
  }

  // Poll status screen updates
  if (currentScreen == SCREEN_STATUS) {
    if (now - lastStatusFetch >= STATUS_POLL_INTERVAL) {
      lastStatusFetch = now;

      WiFiClientSecure *client = new WiFiClientSecure;
      client->setInsecure();
      HTTPClient http;

      String url = String(serverUrl) + "/api/status";

      if (http.begin(*client, url)) {
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
          String status = http.getString();
          
          if (status != lastStatus) {
            lastStatus = status;
            Serial.println("[STATUS] Updated: " + status);

            gfx->fillScreen(BLACK);
            gfx->setTextSize(2);
            gfx->setTextColor(GREEN);
            gfx->setCursor(10, 10);
            gfx->println("Tram Status:");

            gfx->setTextSize(3);
            gfx->setTextColor(YELLOW);
            gfx->setCursor(10, 50);
            gfx->println(status);

            gfx->setTextSize(1);
            gfx->setTextColor(WHITE);
            gfx->setCursor(10, 145);
            gfx->println("BTN1: Back  BTN2: Refresh");
          }
        }
      }

      http.end();
      delete client;
    }
  }
  
  delay(50);
}

void getStatus() {
  if (!checkWiFi()) {
    showMessage("WiFi disconnected", RED);
    return;
  }

  currentScreen = SCREEN_STATUS;
  lastStatusFetch = 0;  // Force immediate poll on first call
  lastStatus = "";
}

void selectStation() {
  if (!stationsLoaded || stations.empty()) return;

  Station &station = stations[currentStationIndex];

  showMessage("Selecting...\nStop " + String(station.sequence), YELLOW, 2, 40);

  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  String url = String(serverUrl) + "/api/user-location";
  url += "?lat=" + String(station.lat, 6);
  url += "&lon=" + String(station.lon, 6);
  url += "&name=" + station.name;

  url.replace(" ", "%20");

  if (!http.begin(*client, url)) {
    showMessage("Connection failed", RED);
    delete client;
    delay(2000);
    displayCurrentStation();
    return;
  }

  int httpCode = http.POST("");

  if (httpCode == HTTP_CODE_OK) {
    showMessage("Selected!\n" + station.name, GREEN, 2, 40);
    Serial.println("Station selected: " + station.name);
    delay(2000);

    getStatus();
  } else {
    showMessage("Error: " + String(httpCode), RED);
    delay(2000);
    displayCurrentStation();
  }

  http.end();
  delete client;
}