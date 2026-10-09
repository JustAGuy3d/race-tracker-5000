#pragma once
// Wi-Fi networks added from the on-screen setup, saved in flash (newest first)
// and tried alongside the ones in config.h.

#include <Arduino.h>
#include <WiFiMulti.h>

constexpr int WIFI_SAVED_MAX = 5;

void wifiStoreBegin();                                     // once, from setup()
void wifiStoreRemember(const String& ssid, const String& pass);
void wifiStoreForget(int index);
int wifiStoreList(String* ssids, int max);                 // names, newest first
void wifiStoreAddTo(WiFiMulti& multi);

struct WifiNet {
  char ssid[33];
  int  rssi;
  bool secure;
};

// Blocking scan (~2-3 s); strongest first, duplicates and hidden networks removed.
int wifiScan(WifiNet* out, int max);
