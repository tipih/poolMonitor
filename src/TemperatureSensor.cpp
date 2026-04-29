#include "TemperatureSensor.h"
#include "Debug.h"

TemperatureSensor::TemperatureSensor()
  : _oneWire(nullptr),
    _sensors(nullptr),
    _calibrationOffset(0.0),
    _currentTemp(-999.0),
    _lastReadTime(0),
    _pin(0),
    _initialized(false) {
}

TemperatureSensor::~TemperatureSensor() {
  // Allocated once in begin(); free in the standard way. In practice
  // this object lives for the entire firmware lifetime, so the
  // destructor never actually runs on the ESP32.
  delete _sensors;
  delete _oneWire;
  _sensors = nullptr;
  _oneWire = nullptr;
}

void TemperatureSensor::begin(uint8_t oneWirePin, float calibrationOffset) {
  if (_initialized) {
    Serial.println("Warning: TemperatureSensor::begin() called twice; ignoring");
    return;
  }

  _pin = oneWirePin;
  _calibrationOffset = calibrationOffset;

  _oneWire = new OneWire(_pin);
  _sensors = new DallasTemperature(_oneWire);

  _sensors->begin();
  _initialized = true;

  Serial.print("Temperature sensor initialized on pin ");
  Serial.println(_pin);
}

float TemperatureSensor::getTemperature() {
  return _currentTemp;
}

float TemperatureSensor::readTemperature() {
  if (!_initialized) {
    Serial.println("Error: Temperature sensor not initialized");
    return -999.0;
  }

  _sensors->requestTemperatures();
  float tempC = _sensors->getTempCByIndex(0);
  
  if (tempC != DEVICE_DISCONNECTED_C) {
    _currentTemp = tempC + _calibrationOffset;
    _lastReadTime = millis();
    
    DBG_PRINT("Temperature reading: ");
    DBG_PRINT(tempC);
    DBG_PRINT("°C (calibrated: ");
    DBG_PRINT(_currentTemp);
    DBG_PRINTLN("°C)");
    
    return _currentTemp;
  } else {
    Serial.println("Error: Could not read temperature data - sensor disconnected");
    _currentTemp = -999.0;
    return _currentTemp;
  }
}
