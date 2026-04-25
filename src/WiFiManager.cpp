#include "WiFiManager.h"
#include <Arduino.h>

// Static member initialization
WiFiManager *WiFiManager::_instance = nullptr;
Preferences *WiFiManager::_staticPrefs = nullptr;
unsigned long *WiFiManager::_staticResetCounter = nullptr;

WiFiManager::WiFiManager()
{
  _instance = this;
  _ssid = nullptr;
  _password = nullptr;
  _preferences = nullptr;
  _resetCounter = 0;
}

WiFiManager::~WiFiManager()
{
  _instance = nullptr;
}

void WiFiManager::begin(const char *ssid, const char *password, Preferences *prefs)
{
  _ssid = ssid;
  _password = password;
  _preferences = prefs;
  _staticPrefs = prefs;

  // Get reset counter from preferences
  if (_preferences)
  {
    _preferences->begin("reset", false);
    _resetCounter = _preferences->getLong("resetCounter", 0);
    _preferences->end();
    _staticResetCounter = &_resetCounter;

    Serial.print("Reset cnt=");
    Serial.println(_resetCounter);
  }

  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(_ssid);

  // Register WiFi event handlers
  WiFi.onEvent(onStationConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(onGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onStationDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

  // Start WiFi connection
  WiFi.begin(_ssid, _password);

  Serial.println("Waiting for WiFi connection...");
  delay(3000);
}

bool WiFiManager::isConnected()
{
  return WiFi.isConnected();
}

int WiFiManager::getRSSI()
{
  return WiFi.RSSI();
}

IPAddress WiFiManager::getLocalIP()
{
  return WiFi.localIP();
}

void WiFiManager::handle()
{
  // Currently no periodic handling needed
  // WiFi events are handled automatically
}

// Static event handlers
void WiFiManager::onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Connected to AP successfully!");
}

void WiFiManager::onGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void WiFiManager::onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");

  // Increment reset counter and save to preferences
  if (_staticPrefs && _staticResetCounter)
  {
    (*_staticResetCounter)++;
    _staticPrefs->begin("reset", false);
    _staticPrefs->putLong("resetCounter", *_staticResetCounter);
    _staticPrefs->end();
  }

  // Restart ESP to reconnect
  delay(2000);
  ESP.restart();
}
