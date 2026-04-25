#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include <Arduino.h>

class InputManager
{
public:
  InputManager();

  void begin(int buttonPin, int ledPin, unsigned long debounceDelay, unsigned long ledBlinkInterval);
  void update();

  int getRelaxStatus() const { return _currentRelaxStatus; }

private:
  int _buttonPin;
  int _ledPin;
  unsigned long _debounceDelay;
  unsigned long _ledBlinkInterval;

  int _ledState;
  int _buttonState;
  int _lastButtonState;
  int _currentRelaxStatus;

  unsigned long _lastDebounceTime;
  unsigned long _lastLEDToggle;
};

#endif
