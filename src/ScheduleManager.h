#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// Forward declarations
class PumpController;
class MQTTManager;

class ScheduleManager {
public:
  // Constructor
  ScheduleManager();

  // Initialize with preferences namespace and required dependencies
  void begin(PumpController &pumpController, MQTTManager &mqttManager, const char* preferencesNamespace = "poolPump");

  // Get/Set schedule hours
  uint8_t getOnHour() const;
  uint8_t getOffHour() const;
  void setSchedule(uint8_t onHour, uint8_t offHour);

  // Load schedule from NVM
  void loadFromNVM();

  // Save schedule to NVM
  void saveToNVM();

  // Validate schedule hours
  bool isValidSchedule(uint8_t onHour, uint8_t offHour) const;

  // Check and execute schedule based on current hour
  void checkAndExecute(uint8_t currentHour);

private:
  uint8_t _onHour;
  uint8_t _offHour;
  const char* _preferencesNamespace;
  Preferences _preferences;
  PumpController* _pumpController;
  MQTTManager* _mqttManager;
};

#endif // SCHEDULE_MANAGER_H
