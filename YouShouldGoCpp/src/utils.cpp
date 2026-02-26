#include "utils.h"
#include "app.h"


#define PIN_POWER 15
#define PIN_BACKLIGHT 38

Arduino_DataBus *bus = new Arduino_ESP32PAR8Q(7, 6, 8, 9, 39, 40, 41, 42, 45, 46, 47, 48);
Arduino_GFX *gfx = new Arduino_ST7789(bus, 5, 0, true, 170, 320, 35, 0, 35, 0);

Preferences preferences;

const char* getNVSErrorString(esp_err_t err) {
  switch (err) {
    case ESP_OK:
      return "OK";
    case ESP_ERR_NVS_NOT_FOUND:
      return "NOT_FOUND";
    case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
      return "NOT_ENOUGH_SPACE";
    case ESP_ERR_NVS_INVALID_HANDLE:
      return "INVALID_HANDLE";
    case ESP_ERR_NVS_INVALID_NAME:
      return "INVALID_NAME";
    case ESP_ERR_NVS_INVALID_LENGTH:
      return "INVALID_LENGTH";
    case ESP_ERR_NVS_NOT_INITIALIZED:
      return "NOT_INITIALIZED";
    default:
      return "UNKNOWN";
  }
}

void nvsOpen() {
  if (nvsHandle) {
    Serial.println("[NVS] Handle already open");
    return;
  }
  Serial.println("[NVS] Opening namespace 'transit'...");
  nvsErr = nvs_open("transit", NVS_READWRITE, &nvsHandle);
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_open failed: " + String(getNVSErrorString(nvsErr)));
    nvsHandle = 0;
  } else {
    Serial.println("[NVS] Namespace opened successfully");
  }
}

void initNVS() {
  Serial.println("[NVS] Initializing NVS...");
  nvsErr = nvs_flash_init();
  if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    Serial.println("[NVS] NVS flash corrupted, erasing...");
    nvs_flash_erase();
    nvs_flash_init();
  }
  nvsOpen();
  Serial.println("[NVS] NVS ready");
}

void clearNVS() {
  nvsOpen();
  Serial.println("[NVS] Clearing all NVS data...");
  nvsErr = nvs_erase_all(nvsHandle);
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_erase_all failed: " + String(getNVSErrorString(nvsErr)));
  } else {
    // Commit to write changes to flash
    nvsErr = nvs_commit(nvsHandle);
    if (nvsErr != ESP_OK) {
      Serial.println("[NVS] ERROR: nvs_commit failed: " + String(getNVSErrorString(nvsErr)));
    } else {
      Serial.println("[NVS] NVS cleared and committed successfully");
    }
  }
  
  routes.clear();
  trips.clear();
  stations.clear();
  routesLoaded = false;
  tripsLoaded = false;
  stationsLoaded = false;
  currentRouteIndex = 0;
  currentTripIndex = 0;
  currentStationIndex = 0;
  
  Serial.println("[NVS] NVS cleared and all data reset!");
}

void saveRoutesToNVS() {
  nvsOpen();
  Serial.println("[NVS] Saving routes to NVS...");
  
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  
  for (const Route &r : routes) {
    JsonObject obj = array.add<JsonObject>();
    obj["route_id"] = r.route_id;
    obj["route_short_name"] = r.route_short_name;
    obj["route_long_name"] = r.route_long_name;
    obj["route_type"] = r.route_type;
    obj["hasVehicle"] = r.hasVehicle;
  }
  
  String json;
  serializeJson(doc, json);
  
  Serial.println("[NVS] Routes JSON size: " + String(json.length()) + " bytes");
  
  // Use putBytes with direct NVS API
  size_t written = 0;
  const char* jsonData = json.c_str();
  size_t jsonLen = json.length();
  
  nvsErr = nvs_set_blob(nvsHandle, "routes", (const void*)jsonData, jsonLen);
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_set_blob failed: " + String(getNVSErrorString(nvsErr)));
  } else {
    Serial.println("[NVS] nvs_set_blob succeeded, committing to flash...");
    nvsErr = nvs_commit(nvsHandle);
    
    if (nvsErr != ESP_OK) {
      Serial.println("[NVS] ERROR: nvs_commit failed: " + String(getNVSErrorString(nvsErr)));
    } else {
      Serial.println("[NVS] Successfully saved " + String(routes.size()) + " routes to NVS");
      written = jsonLen;
    }
  }
  
  if (written == 0) {
    Serial.println("[NVS] WARNING: Write returned 0 bytes");
  }
}

bool loadRoutesFromNVS() {
  nvsOpen();
  Serial.println("[NVS] Attempting to load routes from NVS...");
  
  size_t required_size = 0;
  nvsErr = nvs_get_blob(nvsHandle, "routes", NULL, &required_size);
  
  if (nvsErr == ESP_ERR_NVS_NOT_FOUND) {
    Serial.println("[NVS] No routes in NVS");
    return false;
  }
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_get_blob (size check) failed: " + String(getNVSErrorString(nvsErr)));
    return false;
  }
  
  Serial.println("[NVS] Routes blob size: " + String(required_size) + " bytes");
  
  std::vector<char> buf(required_size + 1);
  nvsErr = nvs_get_blob(nvsHandle, "routes", buf.data(), &required_size);
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_get_blob (read) failed: " + String(getNVSErrorString(nvsErr)));
    return false;
  }
  
  buf[required_size] = '\0';
  String json(buf.data());
  Serial.println("[NVS] Routes JSON loaded: " + String(json.length()) + " chars");
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.println("[NVS] ERROR: JSON parse failed: " + String(error.c_str()));
    return false;
  }
  
  routes.clear();
  JsonArray array = doc.as<JsonArray>();
  
  for (JsonObject obj : array) {
    Route r;
    r.route_id = obj["route_id"];
    r.route_short_name = obj["route_short_name"].as<String>();
    r.route_long_name = obj["route_long_name"].as<String>();
    r.route_type = obj["route_type"] | 0;
    r.hasVehicle = obj["hasVehicle"] | 0;
    routes.push_back(r);
  }
  
  routesLoaded = true;
  Serial.println("[NVS] Successfully loaded " + String(routes.size()) + " routes from NVS");
  return true;
}

void saveTripsToNVS(int routeId) {
  nvsOpen();
  Serial.println("[NVS] Saving trips for route " + String(routeId) + "...");
  
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  
  for (const Trip &t : trips) {
    JsonObject obj = array.add<JsonObject>();
    obj["trip_id"] = t.trip_id;
    obj["route_id"] = t.route_id;
    obj["direction_id"] = t.direction_id;
    obj["trip_headsign"] = t.trip_headsign;
  }
  
  String json;
  serializeJson(doc, json);
  String key = "trips_" + String(routeId);
  
  Serial.println("[NVS] Trips JSON size: " + String(json.length()) + " bytes, key: " + key);
  
  nvsErr = nvs_set_blob(nvsHandle, key.c_str(), (const void*)json.c_str(), json.length());
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_set_blob failed: " + String(getNVSErrorString(nvsErr)));
  } else {
    Serial.println("[NVS] nvs_set_blob succeeded, committing...");
    nvsErr = nvs_commit(nvsHandle);
    
    if (nvsErr != ESP_OK) {
      Serial.println("[NVS] ERROR: nvs_commit failed: " + String(getNVSErrorString(nvsErr)));
    } else {
      Serial.println("[NVS] Successfully saved " + String(trips.size()) + " trips for route " + String(routeId));
    }
  }
}

bool loadTripsFromNVS(int routeId) {
  nvsOpen();
  String key = "trips_" + String(routeId);
  Serial.println("[NVS] Attempting to load trips for route " + String(routeId) + "...");
  
  size_t required_size = 0;
  nvsErr = nvs_get_blob(nvsHandle, key.c_str(), NULL, &required_size);
  
  if (nvsErr == ESP_ERR_NVS_NOT_FOUND) {
    Serial.println("[NVS] No trips for route " + String(routeId) + " in NVS");
    return false;
  }
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_get_blob (size check) failed: " + String(getNVSErrorString(nvsErr)));
    return false;
  }
  
  Serial.println("[NVS] Trips blob size: " + String(required_size) + " bytes");
  
  std::vector<char> buf(required_size + 1);
  nvsErr = nvs_get_blob(nvsHandle, key.c_str(), buf.data(), &required_size);
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_get_blob (read) failed: " + String(getNVSErrorString(nvsErr)));
    return false;
  }
  
  buf[required_size] = '\0';
  String json(buf.data());
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.println("[NVS] ERROR: JSON parse failed for trips: " + String(error.c_str()));
    return false;
  }
  
  trips.clear();
  JsonArray array = doc.as<JsonArray>();
  
  for (JsonObject obj : array) {
    Trip t;
    t.trip_id = obj["trip_id"].as<String>();
    t.route_id = obj["route_id"] | routeId;
    t.direction_id = obj["direction_id"] | 0;
    t.trip_headsign = obj["trip_headsign"].as<String>();
    trips.push_back(t);
  }
  
  tripsLoaded = true;
  Serial.println("[NVS] Successfully loaded " + String(trips.size()) + " trips for route " + String(routeId));
  return true;
}

void saveStationsToNVS() {
  nvsOpen();
  Serial.println("[NVS] Saving stations to NVS...");
  
  JsonDocument doc;
  JsonArray array = doc.to<JsonArray>();
  
  for (const Station &s : stations) {
    JsonObject obj = array.add<JsonObject>();
    obj["sequence"] = s.sequence;
    obj["name"] = s.name;
    obj["lat"] = s.lat;
    obj["lon"] = s.lon;
    obj["hasVehicle"] = s.hasVehicle;
  }
  
  String json;
  serializeJson(doc, json);
  
  Serial.println("[NVS] Stations JSON size: " + String(json.length()) + " bytes");
  
  nvsErr = nvs_set_blob(nvsHandle, "stations", (const void*)json.c_str(), json.length());
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_set_blob failed: " + String(getNVSErrorString(nvsErr)));
  } else {
    Serial.println("[NVS] nvs_set_blob succeeded, committing...");
    nvsErr = nvs_commit(nvsHandle);
    
    if (nvsErr != ESP_OK) {
      Serial.println("[NVS] ERROR: nvs_commit failed: " + String(getNVSErrorString(nvsErr)));
    } else {
      Serial.println("[NVS] Successfully saved " + String(stations.size()) + " stations to NVS");
    }
  }
}

bool loadStationsFromNVS() {
  nvsOpen();
  Serial.println("[NVS] Attempting to load stations from NVS...");
  
  size_t required_size = 0;
  nvsErr = nvs_get_blob(nvsHandle, "stations", NULL, &required_size);
  
  if (nvsErr == ESP_ERR_NVS_NOT_FOUND) {
    Serial.println("[NVS] No stations in NVS");
    return false;
  }
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_get_blob (size check) failed: " + String(getNVSErrorString(nvsErr)));
    return false;
  }
  
  Serial.println("[NVS] Stations blob size: " + String(required_size) + " bytes");
  
  std::vector<char> buf(required_size + 1);
  nvsErr = nvs_get_blob(nvsHandle, "stations", buf.data(), &required_size);
  
  if (nvsErr != ESP_OK) {
    Serial.println("[NVS] ERROR: nvs_get_blob (read) failed: " + String(getNVSErrorString(nvsErr)));
    return false;
  }
  
  buf[required_size] = '\0';
  String json(buf.data());
  
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, json);
  
  if (error) {
    Serial.println("[NVS] ERROR: JSON parse failed for stations: " + String(error.c_str()));
    return false;
  }
  
  stations.clear();
  JsonArray array = doc.as<JsonArray>();
  
  for (JsonObject obj : array) {
    Station s;
    s.sequence = obj["sequence"];
    s.name = obj["name"].as<String>();
    s.lat = obj["lat"];
    s.lon = obj["lon"];
    s.hasVehicle = obj["hasVehicle"];
    stations.push_back(s);
  }
  
  stationsLoaded = true;
  Serial.println("[NVS] Successfully loaded " + String(stations.size()) + " stations from NVS");
  return true;
}

void showMessage(const String &text, uint16_t color, int textSize, int y) {
  gfx->fillScreen(BLACK);
  gfx->setTextSize(textSize);
  gfx->setTextColor(color);
  gfx->setCursor(10, y);
  gfx->println(text);
}

void displayWrappedText(const String &text, int startY) {
  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);

  int y = startY;
  String line = "";
  const int maxCharsPerLine = 50;

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

void drawClearPopup(unsigned long remainingMs) {
  int W = gfx->width();
  int H = gfx->height();
  int boxW = (W * 2) / 3;
  int boxH = 50;
  int boxX = (W - boxW) / 2;
  int boxY = (H - boxH) / 2;
  
  gfx->fillRoundRect(boxX, boxY, boxW, boxH, 6, RED);
  gfx->drawRoundRect(boxX, boxY, boxW, boxH, 6, WHITE);
  gfx->setTextColor(WHITE);
  gfx->setTextSize(2);
  
  char buf[32];
  unsigned long seconds = (remainingMs + 999) / 1000;
  snprintf(buf, sizeof(buf), "Hold %lus more", (unsigned long)seconds);
  
  int textX = boxX + (boxW - strlen(buf) * 12) / 2;
  gfx->setCursor(textX, boxY + (boxH / 2) - 8);
  gfx->println(buf);
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

void loadRoutes() {
  if (loadRoutesFromNVS()) {
    displayCurrentRoute();
    return;
  }

  showMessage("Loading routes...", YELLOW);

  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  String url = String(serverUrl) + "/api/routes-with-vehicles";

  if (!http.begin(*client, url)) {
    showMessage("Connection failed", RED);
    delete client;
    return;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Routes JSON: " + payload);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      showMessage("JSON parse error", RED);
      Serial.println("JSON Error: " + String(error.c_str()));
    } else {
      routes.clear();
      trips.clear();
      stations.clear();
      tripsLoaded = false;
      stationsLoaded = false;
      currentTripIndex = 0;
      currentStationIndex = 0;

      JsonArray array = doc.as<JsonArray>();

      for (JsonObject obj : array) {
        Route r;
        r.route_id = obj["route_id"];
        r.route_short_name = obj["route_short_name"].as<String>();
        r.route_long_name = obj["route_long_name"].as<String>();
        r.route_type = obj["route_type"] | 0;
        r.hasVehicle = obj["hasVehicle"] | 0;
        routes.push_back(r);
      }

      routesLoaded = true;
      currentRouteIndex = 0;
      currentScreen = SCREEN_ROUTES;
      Serial.println("Loaded " + String(routes.size()) + " routes from API");
      saveRoutesToNVS();
      displayCurrentRoute();
    }
  } else {
    showMessage("HTTP Error: " + String(httpCode), RED);
  }

  http.end();
  delete client;
}

void displayCurrentRoute() {
  if (!routesLoaded || routes.empty()) {
    showMessage("No routes loaded", RED);
    return;
  }

  Route &route = routes[currentRouteIndex];
  currentScreen = SCREEN_ROUTES;

  gfx->fillScreen(BLACK);

  gfx->setTextSize(2);
  gfx->setTextColor(CYAN);
  gfx->setCursor(10, 10);
  gfx->printf("%d/%d", currentRouteIndex + 1, (int)routes.size());

  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 40);
  gfx->println(route.route_short_name + " -");

  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 65);
  displayWrappedText(route.route_long_name, 65);

  gfx->setTextSize(1);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(10, 155);
  gfx->println("BTN1: Next  BTN2: Select");
}

void loadTripsForRoute(int routeId) {
  if (loadTripsFromNVS(routeId)) {
    displayCurrentTrip();
    return;
  }

  showMessage("Loading trips...", YELLOW);

  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  String url = String(serverUrl) + "/api/trips?routeId=" + String(routeId);

  if (!http.begin(*client, url)) {
    showMessage("Connection failed", RED);
    delete client;
    return;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Trips JSON: " + payload);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      showMessage("JSON parse error", RED);
      Serial.println("JSON Error: " + String(error.c_str()));
    } else {
      trips.clear();
      stations.clear();
      stationsLoaded = false;
      currentTripIndex = 0;
      currentStationIndex = 0;

      JsonArray array = doc.as<JsonArray>();

      for (JsonObject obj : array) {
        Trip t;
        t.trip_id = obj["trip_id"].as<String>();
        t.route_id = obj["route_id"] | routeId;
        t.direction_id = obj["direction_id"] | 0;
        t.trip_headsign = obj["trip_headsign"].as<String>();
        trips.push_back(t);
      }

      tripsLoaded = true;
      currentScreen = SCREEN_TRIPS;
      Serial.println("Loaded " + String(trips.size()) + " trips from API");
      saveTripsToNVS(routeId);
      displayCurrentTrip();
    }
  } else {
    showMessage("HTTP Error: " + String(httpCode), RED);
  }

  http.end();
  delete client;
}

void displayCurrentTrip() {
  if (!tripsLoaded || trips.empty()) {
    showMessage("No trips loaded", RED);
    return;
  }

  Trip &trip = trips[currentTripIndex];
  currentScreen = SCREEN_TRIPS;

  gfx->fillScreen(BLACK);

  gfx->setTextSize(2);
  gfx->setTextColor(CYAN);
  gfx->setCursor(10, 10);
  gfx->printf("%d/%d", currentTripIndex + 1, (int)trips.size());

  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 40);
  gfx->println("Trip");

  gfx->setTextSize(1);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 65);
  displayWrappedText(trip.trip_headsign, 65);

  gfx->setTextSize(1);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(10, 130);
  gfx->println("Dir: " + String(trip.direction_id));

  gfx->setTextSize(1);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(10, 145);
  gfx->println("BTN1: Next  BTN2: Select");
  gfx->setCursor(10, 158);
  gfx->println("Hold BTN2: Routes");
}

void loadStations() {
  if (loadStationsFromNVS()) {
    displayCurrentStation();
    return;
  }

  showMessage("Loading stations...", YELLOW);

  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  String url = String(serverUrl) + "/api/stations-with-vehicles";

  if (!http.begin(*client, url)) {
    showMessage("Connection failed", RED);
    delete client;
    return;
  }

  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.println("Stations JSON: " + payload);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      showMessage("JSON parse error", RED);
      Serial.println("JSON Error: " + String(error.c_str()));
    } else {
      stations.clear();
      JsonArray array = doc.as<JsonArray>();

      for (JsonObject obj : array) {
        Station s;
        s.sequence = obj["sequence"];
        s.name = obj["stationName"].as<String>();
        s.lat = obj["lat"];
        s.lon = obj["lon"];
        s.hasVehicle = obj["hasVehicle"];
        stations.push_back(s);
      }

      stationsLoaded = true;
      currentStationIndex = 0;
      currentScreen = SCREEN_STATIONS;
      Serial.println("Loaded " + String(stations.size()) + " stations from API");
      saveStationsToNVS();
      displayCurrentStation();
    }
  } else {
    showMessage("HTTP Error: " + String(httpCode), RED);
  }

  http.end();
  delete client;
}

void displayCurrentStation() {

  if (!stationsLoaded || stations.empty()) {
    showMessage("No stations loaded", RED);
    return;
  }

  Station &station = stations[currentStationIndex];
  currentScreen = SCREEN_STATIONS;

  gfx->fillScreen(BLACK);

  gfx->setTextSize(2);
  gfx->setTextColor(CYAN);
  gfx->setCursor(10, 10);
  gfx->printf("%d/%d", station.sequence, (int)stations.size());

  gfx->setTextSize(2);
  gfx->setTextColor(WHITE);
  gfx->setCursor(10, 40);

  String name = station.name;
  if (name.length() > 18) {
    int spacePos = name.lastIndexOf(' ', 18);
    if (spacePos > 0) {
      gfx->println(name.substring(0, spacePos));
      gfx->setCursor(10, 60);
      gfx->println(name.substring(spacePos + 1));
    } else {
      gfx->println(name.substring(0, 18));
      gfx->setCursor(10, 60);
      gfx->println(name.substring(18));
    }
  } else {
    gfx->println(name);
  }

  gfx->setTextSize(1);
  gfx->setTextColor(YELLOW);
  gfx->setCursor(10, 145);
  gfx->println("BTN1: Next  BTN2: Select");
  gfx->setCursor(10, 158);
  gfx->println("Hold BTN2: Routes");
}

String fetchStatus() {
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  String url = String(serverUrl) + "/api/status";

  if (!http.begin(*client, url)) {
    delete client;
    return "";
  }

  int httpCode = http.GET();
  String result = "";

  if (httpCode == HTTP_CODE_OK) {
    result = http.getString();
    Serial.println("Status: " + result);
  }

  http.end();
  delete client;
  return result;
}



