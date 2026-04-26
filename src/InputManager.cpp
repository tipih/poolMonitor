#include "InputManager.h"

InputManager::InputManager()
    : _buttonPin(0), _ledPin(0), _debounceDelay(50), _ledBlinkInterval(500),
      _ledState(LOW), _buttonState(0), _lastButtonState(LOW), _currentRelaxStatus(0),
      _lastDebounceTime(0), _lastLEDToggle(0)
{
}

void InputManager::begin(uint8_t buttonPin, uint8_t ledPin, unsigned long debounceDelay, unsigned long ledBlinkInterval)
{
  _buttonPin = buttonPin;
  _ledPin = ledPin;
  _debounceDelay = debounceDelay;
  _ledBlinkInterval = ledBlinkInterval;

  pinMode(_buttonPin, INPUT_PULLUP);
  pinMode(_ledPin, OUTPUT);

  Serial.println("Input manager initialized");
}

void InputManager::update()
{
  // Read button status
  int data = digitalRead(_buttonPin);

  if (data != _lastButtonState)
  {
    // Reset the debouncing timer
    _lastDebounceTime = millis();
  }

  if ((millis() - _lastDebounceTime) > _debounceDelay)
  {
    if (data != _buttonState)
    {
      _buttonState = data;
      _currentRelaxStatus = data;
    }
  }

  // Visible LED blink every interval (independent of button)
  if ((millis() - _lastLEDToggle) >= _ledBlinkInterval)
  {
    _lastLEDToggle = millis();
    _ledState = !_ledState;
    digitalWrite(_ledPin, _ledState);
  }

  _lastButtonState = data;
}
