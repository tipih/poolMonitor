// MQTTManager stub for native unit tests.
//
// IMPORTANT: This file is force-included via the [env:native] build_flags
// (-include test/test_native/mocks/MQTTManager.h). It uses the same
// include-guard macro (MQTT_MANAGER_H) as src/MQTTManager.h, so when
// ScheduleManager.cpp later does #include "MQTTManager.h", the guard
// fires and the real (PubSubClient/WiFi-dependent) header is skipped.
#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <string>

class MQTTManager {
public:
  void publishToSubtopic(const char* sub, const char* val) {
    lastSub = sub ? sub : "";
    lastVal = val ? val : "";
    publishCount++;
  }

  static std::string lastSub;
  static std::string lastVal;
  static int         publishCount;
};

#endif // MQTT_MANAGER_H
