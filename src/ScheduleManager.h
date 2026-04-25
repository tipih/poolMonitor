#ifndef SCHEDULE_MANAGER_H
#define SCHEDULE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

class ScheduleManager {
public:
  // Constructor
  ScheduleManager();

  // Initialize with preferences namespace
  void begin(const char* preferencesNamespace = "poolPump");

  // Get/Set schedule hours
  unsigned long getOnHour() const;
  unsigned long getOffHour() const;
  void setSchedule(unsigned long onHour, unsigned long offHour);

  // Load schedule from NVM
  void loadFromNVM();

  // Save schedule to NVM
  void saveToNVM();

  // Validate schedule hours
  bool isValidSchedule(unsigned long onHour, unsigned long offHour) const;

private:
  unsigned long _onHour;
  unsigned long _offHour;
  const char* _preferencesNamespace;
  Preferences _preferences;
};

#endif // SCHEDULE_MANAGER_H
