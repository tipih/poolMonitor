#include "WiFiManager.h"
#include <Arduino.h>

// Static member initialization
Preferences *WiFiManager::_staticPrefs = nullptr;
unsigned long *WiFiManager::_staticResetCounter = nullptr;
int WiFiManager::_reconnectAttempts = 0;
volatile bool WiFiManager::_shouldReconnect = false;
volatile bool WiFiManager::_shouldRestart = false;

WiFiManager::WiFiManager()
{
  _ssid = nullptr;
  _password = nullptr;
  _preferences = nullptr;
  _resetCounter = 0;
}

WiFiManager::~WiFiManager()
{
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
  WiFi.onEvent(onStationConnected, ARDUINO_EVENT_WIFI_STA_CONNECTED);
  WiFi.onEvent(onGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(onStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

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
  
  // Reset reconnect attempts on successful connection
  _reconnectAttempts = 0;
}

void WiFiManager::onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  // WARNING: This runs inside the ESP-IDF WiFi task.
  // Do NOT call WiFi API, delay(), NVS/Preferences, or ESP.restart() here —
  // they can deadlock or corrupt state. Only set flags; handle() acts on them.
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);

  _reconnectAttempts++;
  Serial.print("Reconnect attempt: ");
  Serial.print(_reconnectAttempts);
  Serial.print("/");
  Serial.println(_maxReconnectAttempts);

  if (_reconnectAttempts < _maxReconnectAttempts)
    _shouldReconnect = true;
  else
    _shouldRestart = true;
}

void WiFiManager::handle()
{
  if (_shouldReconnect)
  {
    _shouldReconnect = false;
    Serial.println("[WiFiManager] Reconnecting from main task...");
    WiFi.disconnect();
    delay(1000);
    WiFi.begin(_ssid, _password);
  }

  if (_shouldRestart)
  {
    _shouldRestart = false;
    Serial.println("[WiFiManager] Max reconnect attempts exceeded. Restarting...");

    // Safe to write NVS and restart here — we're in the main task
    if (_staticPrefs && _staticResetCounter)
    {
      (*_staticResetCounter)++;
      _staticPrefs->begin("reset", false);
      _staticPrefs->putLong("resetCounter", *_staticResetCounter);
      _staticPrefs->end();
    }

    delay(500);
    ESP.restart();
  }
}
