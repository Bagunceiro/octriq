#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>

struct WiFiNetworkDef
{
    WiFiNetworkDef(const String& s, const String& p = "")
    {
        ssid = s; psk = p; openNet = (psk.length() == 0);
    }
    String ssid;
    String psk;
    int rssi;
    bool openNet;
};

typedef std::vector<WiFiNetworkDef> networkList;

extern bool networkConfWrite(networkList& list);
extern networkList& networkConfRead();
extern networkList& scanNetworks();
extern void addNetwork(networkList& netlist, const String& ssid);
extern void updateWiFiDef(WiFiNetworkDef&);
extern void connectToWiFi();

extern const unsigned int WIFI_CONNECT_ATTEMPT_INT;