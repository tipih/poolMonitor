#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

class InputManager
{
public:
  InputManager();

  void begin(uint8_t buttonPin, uint8_t ledPin, unsigned long debounceDelay, unsigned long ledBlinkInterval);
  void update();

  int getRelaxStatus() const { return _currentRelaxStatus; }

private:
  uint8_t _buttonPin;
  uint8_t _ledPin;
  unsigned long _debounceDelay;
  unsigned long _ledBlinkInterval;

  int _ledState;
  int _buttonState;
  int _lastButtonState;
  int _currentRelaxStatus;

  unsigned long _lastDebounceTime;
  unsigned long _lastLEDToggle;
};

#endif // INPUT_MANAGER_H
