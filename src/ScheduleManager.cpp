#include "ScheduleManager.h"
#include "PumpController.h"
#include "MQTTManager.h"

ScheduleManager::ScheduleManager()
  : _onHour(6),
    _offHour(18),
    _preferencesNamespace("poolPump") {
}

void ScheduleManager::begin(const char* preferencesNamespace) {
  _preferencesNamespace = preferencesNamespace;
  loadFromNVM();
}

unsigned long ScheduleManager::getOnHour() const {
  return _onHour;
}

unsigned long ScheduleManager::getOffHour() const {
  return _offHour;
}

void ScheduleManager::setSchedule(unsigned long onHour, unsigned long offHour) {
  if (isValidSchedule(onHour, offHour)) {
    _onHour = onHour;
    _offHour = offHour;
    saveToNVM();
    
    Serial.print("Schedule updated: ON=");
    Serial.print(_onHour);
    Serial.print(":00, OFF=");
    Serial.print(_offHour);
    Serial.println(":00");
  } else {
    Serial.println("Error: Invalid schedule times provided");
  }
}

void ScheduleManager::loadFromNVM() {
  _preferences.begin(_preferencesNamespace, false);
  _onHour = _preferences.getLong("onHour", 6);
  _offHour = _preferences.getLong("offHour", 18);
  _preferences.end();

  if (!isValidSchedule(_onHour, _offHour)) {
    Serial.println("Warning: Invalid schedule in NVM, using defaults");
    _onHour = 6;
    _offHour = 18;
    saveToNVM();
  } else {
    Serial.print("Schedule loaded from NVM: ON=");
    Serial.print(_onHour);
    Serial.print(":00, OFF=");
    Serial.print(_offHour);
    Serial.println(":00");
  }
}

void ScheduleManager::saveToNVM() {
  _preferences.begin(_preferencesNamespace, false);
  _preferences.putLong("onHour", _onHour);
  _preferences.putLong("offHour", _offHour);
  _preferences.end();
  
  Serial.println("Schedule saved to NVM");
}

bool ScheduleManager::isValidSchedule(unsigned long onHour, unsigned long offHour) const {
  return (onHour >= 0 && onHour < 24 && offHour >= 0 && offHour < 24);
}

void ScheduleManager::checkAndExecute(unsigned long currentHour, PumpController &pumpController, MQTTManager &mqttManager) {
  // Check if it is time to turn on to medium speed
  if ((currentHour == _onHour) && (pumpController.getCurrentSpeed() != PumpController::MED_SPEED))
  {
    pumpController.setMedSpeed();
    mqttManager.publishToSubtopic("pump_speed", "Medium");
  }
  
  // Check if it is time to turn off to low speed
  if ((currentHour == _offHour) && (pumpController.getCurrentSpeed() != PumpController::LOW_SPEED))
  {
    pumpController.setLowSpeed();
    mqttManager.publishToSubtopic("pump_speed", "Low");
  }
}
