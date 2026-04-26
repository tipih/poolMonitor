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

private:
  bool _initialized;
};

#endif // OTA_MANAGER_H
