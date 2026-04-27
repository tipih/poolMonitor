#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>

/**
 * Drives the pool pump's speed-selector inputs by emulating physical
 * button presses on four GPIO lines (high / low / medium / stop). Each
 * speed change first releases the other lines, then pulses the target
 * line low for a short interval before returning it high.
 *
 * Note: the speed-change methods currently use blocking delay() calls
 * (~1.3 s total) to honour the pump panel's input timing. Callers must
 * tolerate this latency or schedule changes when blocking is acceptable.
 */
class PumpController {
public:
  // Pump speed enumeration
  typedef enum {
    LOW_SPEED,
    HIGH_SPEED,
    NO_SPEED,
    MED_SPEED,
    STOP
  } PumpSpeed;

  // Constructor
  PumpController();

  // Initialize pump controller with GPIO pins
  void begin(uint8_t highSpeedPin, uint8_t lowSpeedPin, uint8_t medSpeedPin, uint8_t stopPin);

  // Pump control methods
  void setLowSpeed();
  void setHighSpeed();
  void setMedSpeed();
  void setStop();

  // Get current pump speed
  PumpSpeed getCurrentSpeed() const;

  // Get speed as string for display/MQTT
  const char* getSpeedString() const;

private:
  // GPIO pin assignments
  uint8_t _highSpeedPin;
  uint8_t _lowSpeedPin;
  uint8_t _medSpeedPin;
  uint8_t _stopPin;

  // Current pump speed state
  PumpSpeed _currentSpeed;

  // Helper method to simulate button press
  void simulateButtonPress(uint8_t pin);
};

#endif // PUMP_CONTROLLER_H
