#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

class WiFiManager
{
public:
  WiFiManager();
  ~WiFiManager();

  // Initialize WiFi with SSID and password
  void begin(const char *ssid, const char *password, Preferences *prefs);

  // Check if WiFi is connected
  bool isConnected();

  // Get WiFi RSSI (signal strength)
  int getRSSI();

  // Get local IP address
  IPAddress getLocalIP();

  // Must be called from loop() to safely act on reconnect/restart flags
  void handle();

private:
  const char *_ssid;
  const char *_password;
  Preferences *_preferences;
  unsigned long _resetCounter;
  static int _reconnectAttempts;
  static const int _maxReconnectAttempts = 5;

  // Flags set by event handlers (ISR-safe bools), acted on in handle()
  static volatile bool _shouldReconnect;
  static volatile bool _shouldRestart;

  // Event handlers — must only set flags, no WiFi API calls or delays
  static void onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
  static void onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

  // Static members for event handlers
  static Preferences *_staticPrefs;
  static unsigned long *_staticResetCounter;
};

#endif // WIFI_MANAGER_H
