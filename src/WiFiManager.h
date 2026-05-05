#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Preferences.h>

/**
 * Manages the WiFi connection lifecycle: initial association, automatic
 * reconnection with exponential backoff, and full device restart after
 * repeated failures (with a persistent reset counter stored in NVS).
 *
 * Reconnect/restart decisions are made inside WiFi event callbacks (which
 * run on the ESP-IDF WiFi task) by setting flags only — no WiFi API,
 * delay(), NVS or ESP.restart() calls happen there. handle() must be
 * called from loop() to act on those flags safely from the main task.
 *
 * The Preferences pointer passed to begin() must outlive this object.
 */
class WiFiManager
{
public:
  WiFiManager();
  ~WiFiManager();

  // Initialize WiFi with SSID, password, and optional STA hostname
  // (the name the device announces over DHCP, e.g. "PoolMonitor").
  void begin(const char *ssid, const char *password, Preferences *prefs,
             const char *hostname = nullptr);

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
  static unsigned long _reconnectAt;  // millis() timestamp for next reconnect attempt
  static bool _initialized;           // true after begin() completes; ignores events before then

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
