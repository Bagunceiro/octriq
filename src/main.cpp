#include <Arduino.h>
#include <LITTLEFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>

#include "networks.h"
#include "ntpclient.h"
#include "cmd.h"

#include "seqvm.h"

const char *mDNSName = "octrick";

#define serr Serial

// Interval between attempts to reconnect WiFi (ms)
const unsigned int WIFI_CONNECT_ATTEMPT_INT = (5 * 60 * 1000);
const int TZ = -3;

WiFiMulti wifimulti;
networkList configuredNets;
WiFiUDP udp;

NTPClient timeClient(udp, TZ * 60 * 60);

networkList &networkConfRead()
{
  File netsFile = LITTLEFS.open("/networks.json", "r");
  if (!netsFile)
  {
    perror("");
    serr.println("Config file open for read failed");
  }
  else
  {
    configuredNets.clear();
    StaticJsonDocument<1500> doc;
    DeserializationError error = deserializeJson(doc, netsFile);
    if (error)
    {
      serr.println(F("Failed to read network file"));
    }
    else
    {
    }
    JsonArray array = doc.as<JsonArray>();
    for (JsonObject net : array)
    {
      const char *ssid = (const char *)net["ssid"];
      const char *psk = (const char *)net["psk"];
      // Serial.printf("Configured network: %s/%s\n", ssid, psk);
      WiFiNetworkDef network(ssid, psk);
      configuredNets.push_back(network);
    }
    netsFile.close();
  }
  return configuredNets;
}

void connectToWiFi()
{
  static long then = 0;
  long now = millis();
  if ((then == 0) || ((now - then) >= WIFI_CONNECT_ATTEMPT_INT))
  {
    then = now;
    networkConfRead();
    unsigned int numNets = configuredNets.size();
    if (numNets > 0)
    {
      for (unsigned int i = 0; i < numNets; i++)
      {
        Serial.printf("Connect to %s/%s\n", configuredNets[i].ssid.c_str(), configuredNets[i].psk.c_str());
        wifimulti.addAP(configuredNets[i].ssid.c_str(), configuredNets[i].psk.c_str());
      }
      delay(500);
      wifimulti.run();
      MDNS.begin(mDNSName);
      MDNS.addService("octrick", "tcp", 24);
    }
    else
    {
      // dev.configurator.start();
    }
  }
}

bool checkLittleEndian()
{
  uint8_t test = 0x1;
  unsigned int z = test;
  uint8_t* s = reinterpret_cast<uint8_t*>(&z);
  uint8_t c = s[sizeof(z)-1];
  return(c = test);
}

void setup()
{
  // const char *filename = "/seq.bin";
  extern void initSysUpdate();
  Serial.begin(115200);

  if (!checkLittleEndian())
  {
    Serial.println("Code is not little endian");
    exit(-1);
  }

  LITTLEFS.begin();
  connectToWiFi();
  initSysUpdate();
  startCmdTask();
  VM::buildOpMap();
  /*
  File f = LITTLEFS.open(filename);
  if (f)
  {
    VM* vm = new VM(f);

    vm->settrace(true);
    vm->startAsTask(0);
  }
  else
    Serial.printf("Could not open binary file %s\n", filename);
    */
}

void loop()
{
  static bool wifiWasConnected = false;
  static bool ntpstarted = false;
  if (WiFi.status() == WL_CONNECTED)
  {
    if (!wifiWasConnected)
    {
      wifiWasConnected = true;
      // serr.begin("Octrick");
      serr.println("WiFi connected");
    }
    if (!ntpstarted)
    {
      // timeClient.setUpdateCallback(ntpUpdated);
      timeClient.begin();
      timeClient.setUpdateInterval(3600000);
      timeClient.setTimeOffset(TZ * 60 * 60);
      ntpstarted = true;
    }
    // in NTPClient_Generic false return is not (necessarily) a failure - it just means
    // not updated, which happens most spins of the loop because no attempt is made.
    // if (!timeClient.update()) serr.println("NTP failure");
    timeClient.update();

    // serr.loop();
  }
  else
  {
    if (wifiWasConnected)
    {
      serr.println("WiFi connection lost");
      wifiWasConnected = false;
    }
  }
}