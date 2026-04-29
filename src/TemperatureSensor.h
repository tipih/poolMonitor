#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * Reads pool water temperature from a Dallas DS18B20 1-Wire sensor.
 * Owns the OneWire bus and DallasTemperature driver, applies a
 * configurable calibration offset, and caches the most recent reading
 * so getTemperature() is cheap to call frequently.
 *
 * The OneWire/DallasTemperature objects are heap-allocated once in
 * begin(); a second call to begin() is a no-op so the bus is never
 * silently reconfigured at runtime.
 */
class TemperatureSensor {
public:
  TemperatureSensor();
  ~TemperatureSensor();

  // Initialize sensor with OneWire pin. Safe to call only once; later
  // calls are ignored.
  void begin(uint8_t oneWirePin, float calibrationOffset = 0.0);

  // Read temperature (returns cached value if not enough time has passed)
  float getTemperature();

  // Force a new temperature reading
  float readTemperature();

private:
  OneWire* _oneWire;
  DallasTemperature* _sensors;
  float _calibrationOffset;
  float _currentTemp;
  unsigned long _lastReadTime;
  uint8_t _pin;
  bool _initialized;
};

#endif // TEMPERATURE_SENSOR_H
