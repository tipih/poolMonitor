#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

/**
 * Wraps ArduinoOTA to provide network firmware updates. Tracks an
 * "update in progress" flag so the main loop can suspend non-essential
 * work (pump control, MQTT, etc.) while a transfer is running, giving
 * the OTA handler maximum CPU time and avoiding mid-flash aborts.
 *
 * handle() must be called as the first thing in loop().
 */
class OTAManager {
public:
  // Constructor
  OTAManager();

  // Initialize OTA with optional hostname and password
  void begin(const char* hostname = nullptr, const char* password = nullptr);

  // Handle OTA updates (call in loop)
  void handle();

  // Check if OTA update is in progress
  bool isUpdating() const { return _isUpdating; }

private:
  bool _initialized;
  bool _isUpdating;
};

#endif // OTA_MANAGER_H
