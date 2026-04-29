#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

/**
 * Handles the on-board user-interface peripherals: a debounced push
 * button (whose latched state is exposed as the "pool relax status")
 * and a status LED that blinks at a fixed interval as a heartbeat.
 *
 * update() must be called from loop() to service debouncing and the
 * LED toggle; both are millis()-based and non-blocking.
 */
class InputManager
{
public:
  InputManager();

  void begin(uint8_t buttonPin, uint8_t ledPin, unsigned long debounceDelay, unsigned long ledBlinkInterval);
  void update();

  uint8_t getRelaxStatus() const { return _currentRelaxStatus; }

private:
  uint8_t _buttonPin;
  uint8_t _ledPin;
  unsigned long _debounceDelay;
  unsigned long _ledBlinkInterval;

  bool _ledState;            // current LED level (HIGH/LOW)
  uint8_t _buttonState;      // last debounced raw read (HIGH/LOW)
  uint8_t _lastButtonState;  // last raw read, used for debounce edge
  uint8_t _currentRelaxStatus; // 0/1, mirrors _buttonState after debounce

  unsigned long _lastDebounceTime;
  unsigned long _lastLEDToggle;
};

#endif // INPUT_MANAGER_H
