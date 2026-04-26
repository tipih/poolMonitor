#include "TemperatureSensor.h"

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
  if (_sensors) {
    _sensors->~DallasTemperature();
    _sensors = nullptr;
  }
  if (_oneWire) {
    _oneWire->~OneWire();
    _oneWire = nullptr;
  }
}

void TemperatureSensor::begin(uint8_t oneWirePin, float calibrationOffset) {
  _pin = oneWirePin;
  _calibrationOffset = calibrationOffset;
  
  // Use placement new to construct objects in pre-allocated buffers (no heap allocation)
  _oneWire = new (_oneWireBuffer) OneWire(_pin);
  _sensors = new (_sensorsBuffer) DallasTemperature(_oneWire);
  
  // Initialize the sensor
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
    
    Serial.print("Temperature reading: ");
    Serial.print(tempC);
    Serial.print("°C (calibrated: ");
    Serial.print(_currentTemp);
    Serial.println("°C)");
    
    return _currentTemp;
  } else {
    Serial.println("Error: Could not read temperature data - sensor disconnected");
    _currentTemp = -999.0;
    return _currentTemp;
  }
}
