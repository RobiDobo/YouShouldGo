#pragma once
//pragma to only include once
// app.h includes libraries and shared data types, globals
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <Preferences.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <vector>

#ifndef WIFI_SSID
#define WIFI_SSID "MyNetwork"
#endif
#ifndef WIFI_PASS
#define WIFI_PASS "1976@bond"
#endif

struct Route {
  int route_id;
  String route_short_name;
  String route_long_name;
  int route_type;
  int hasVehicle;
};

struct Trip {
  String trip_id;
  int route_id;
  int direction_id;
  String trip_headsign;
};

struct Station {
  int sequence;
  String name;
  double lat;
  double lon;
  int hasVehicle;
};

enum Screen {
  SCREEN_ROUTES,
  SCREEN_TRIPS,
  SCREEN_STATIONS,
  SCREEN_STATUS
};

//use extern to declare the variables once, and define them in main.cpp
extern const char* ssid;
extern const char* password;
extern const char* serverUrl;

extern std::vector<Route> routes;
extern int currentRouteIndex;
extern bool routesLoaded;

extern std::vector<Trip> trips;
extern int currentTripIndex;
extern bool tripsLoaded;

extern std::vector<Station> stations;
extern int currentStationIndex;
extern bool stationsLoaded;
extern unsigned long lastButtonPress;
extern Screen currentScreen;

extern Arduino_DataBus *bus;
extern Arduino_GFX *gfx;

// NVS handle and error code for direct NVS operations
extern nvs_handle_t nvsHandle;
extern esp_err_t nvsErr;
extern void getStatus();
void selectStation();