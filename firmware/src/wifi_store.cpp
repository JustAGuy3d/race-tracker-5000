#include "wifi_store.h"

#include <Preferences.h>
#include <WiFi.h>

namespace {

Preferences store;

String key(const char* prefix, int i) { return String(prefix) + i; }

int count() { return store.getInt("n", 0); }

void load(int i, String& ssid, String& pass) {
  ssid = store.getString(key("s", i).c_str(), "");
  pass = store.getString(key("p", i).c_str(), "");
}

void write(int i, const String& ssid, const String& pass) {
  store.putString(key("s", i).c_str(), ssid);
  store.putString(key("p", i).c_str(), pass);
}

}  // namespace

void wifiStoreBegin() { store.begin("wifi", false); }

// Re-saving a network moves it to the front with the new password.
void wifiStoreRemember(const String& ssid, const String& pass) {
  String ssids[WIFI_SAVED_MAX], passes[WIFI_SAVED_MAX];
  int n = 0;
  ssids[n] = ssid;
  passes[n++] = pass;
  for (int i = 0; i < count() && n < WIFI_SAVED_MAX; i++) {
    String s, p;
    load(i, s, p);
    if (s != ssid) {
      ssids[n] = s;
      passes[n++] = p;
    }
  }
  for (int i = 0; i < n; i++) write(i, ssids[i], passes[i]);
  store.putInt("n", n);
}

void wifiStoreForget(int index) {
  const int n = count();
  if (index < 0 || index >= n) return;
  for (int i = index; i < n - 1; i++) {
    String s, p;
    load(i + 1, s, p);
    write(i, s, p);
  }
  store.remove(key("s", n - 1).c_str());
  store.remove(key("p", n - 1).c_str());
  store.putInt("n", n - 1);
}

int wifiStoreList(String* ssids, int max) {
  int n = 0;
  for (int i = 0; i < count() && n < max; i++) {
    String p;
    load(i, ssids[n++], p);
  }
  return n;
}

void wifiStoreAddTo(WiFiMulti& multi) {
  for (int i = 0; i < count(); i++) {
    String s, p;
    load(i, s, p);
    if (s.length()) multi.addAP(s.c_str(), p.c_str());
  }
}

int wifiScan(WifiNet* out, int max) {
  const int found = WiFi.scanNetworks();  // results come strongest first
  int n = 0;
  for (int i = 0; i < found && n < max; i++) {
    const String s = WiFi.SSID(i);
    if (!s.length()) continue;
    bool dup = false;
    for (int j = 0; j < n && !dup; j++) dup = s == out[j].ssid;
    if (dup) continue;
    snprintf(out[n].ssid, sizeof out[n].ssid, "%s", s.c_str());
    out[n].rssi = WiFi.RSSI(i);
    out[n].secure = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    n++;
  }
  WiFi.scanDelete();
  return n;
}
