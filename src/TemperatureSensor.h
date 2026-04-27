#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <new>

/**
 * Reads pool water temperature from a Dallas DS18B20 1-Wire sensor.
 * Owns the OneWire bus and DallasTemperature driver, applies a
 * configurable calibration offset, and caches the most recent reading
 * so getTemperature() is cheap to call frequently.
 */
class TemperatureSensor {
public:
  // Constructor
  TemperatureSensor();
  
  // Destructor
  ~TemperatureSensor();

  // Initialize sensor with OneWire pin
  void begin(uint8_t oneWirePin, float calibrationOffset = 0.0);

  // Read temperature (returns cached value if not enough time has passed)
  float getTemperature();

  // Force a new temperature reading
  float readTemperature();

private:
  // Use aligned storage to avoid heap allocation
  alignas(OneWire) uint8_t _oneWireBuffer[sizeof(OneWire)];
  alignas(DallasTemperature) uint8_t _sensorsBuffer[sizeof(DallasTemperature)];
  OneWire* _oneWire;
  DallasTemperature* _sensors;
  float _calibrationOffset;
  float _currentTemp;
  unsigned long _lastReadTime;
  uint8_t _pin;
  bool _initialized;
};

#endif // TEMPERATURE_SENSOR_H
