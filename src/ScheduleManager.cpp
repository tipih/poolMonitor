#include "ScheduleManager.h"
#include "PumpController.h"
#include "MQTTManager.h"

ScheduleManager::ScheduleManager()
  : _onHour(6),
    _offHour(18),
    _preferencesNamespace("poolPump"),
    _pumpController(nullptr),
    _mqttManager(nullptr) {
}

void ScheduleManager::begin(PumpController &pumpController, MQTTManager &mqttManager, const char* preferencesNamespace) {
  _pumpController = &pumpController;
  _mqttManager = &mqttManager;
  _preferencesNamespace = preferencesNamespace;
  loadFromNVM();
}

uint8_t ScheduleManager::getOnHour() const {
  return _onHour;
}

uint8_t ScheduleManager::getOffHour() const {
  return _offHour;
}

void ScheduleManager::setSchedule(uint8_t onHour, uint8_t offHour) {
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
  _onHour = _preferences.getUChar("onHour", 6);
  _offHour = _preferences.getUChar("offHour", 18);
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
  _preferences.putUChar("onHour", _onHour);
  _preferences.putUChar("offHour", _offHour);
  _preferences.end();
  
  Serial.println("Schedule saved to NVM");
}

bool ScheduleManager::isValidSchedule(uint8_t onHour, uint8_t offHour) const {
  return (onHour < 24 && offHour < 24);
}

void ScheduleManager::checkAndExecute(uint8_t currentHour) {
  if (!_pumpController || !_mqttManager) {
    Serial.println("Error: ScheduleManager not properly initialized");
    return;
  }

  // Check if it is time to turn on to medium speed
  if ((currentHour == _onHour) && (_pumpController->getCurrentSpeed() != PumpController::MED_SPEED))
  {
    _pumpController->setMedSpeed();
    _mqttManager->publishToSubtopic("pump_speed", _pumpController->getSpeedString());
  }
  
  // Check if it is time to turn off to low speed
  if ((currentHour == _offHour) && (_pumpController->getCurrentSpeed() != PumpController::LOW_SPEED))
  {
    _pumpController->setLowSpeed();
    _mqttManager->publishToSubtopic("pump_speed", _pumpController->getSpeedString());
  }
}
