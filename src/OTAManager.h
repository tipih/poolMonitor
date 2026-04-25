#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OTAManager {
public:
  // Constructor
  OTAManager();

  // Initialize OTA with optional hostname and password
  void begin(const char* hostname = nullptr, const char* password = nullptr);

  // Handle OTA updates (call in loop)
  void handle();

  // Set password after initialization
  void setPassword(const char* password);

  // Set hostname after initialization
  void setHostname(const char* hostname);

private:
  bool _initialized;
};

#endif // OTA_MANAGER_H
