#pragma once
//pragma to only include once
#include "app.h"
// utils.h includes function declarations
void showMessage(const String &text, uint16_t color = WHITE, int textSize = 2, int y = 60);
void displayWrappedText(const String &text, int startY = 40);
void drawClearPopup(unsigned long remainingMs);
void initDisplay();
void initNVS();
void clearNVS();
void saveRoutesToNVS();
bool loadRoutesFromNVS();
void saveTripsToNVS(int routeId);
bool loadTripsFromNVS(int routeId);
void saveStationsToNVS();
bool loadStationsFromNVS();
void loadRoutes();
void displayCurrentRoute();
void loadTripsForRoute(int routeId);
void displayCurrentTrip();
void loadStations();
void displayCurrentStation();
String fetchStatus();
void selectStation();
