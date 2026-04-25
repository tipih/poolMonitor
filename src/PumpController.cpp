#include "PumpController.h"

PumpController::PumpController() 
  : _highSpeedPin(0),
    _lowSpeedPin(0),
    _medSpeedPin(0),
    _stopPin(0),
    _currentSpeed(NO_SPEED) {
}

void PumpController::begin(uint8_t highSpeedPin, uint8_t lowSpeedPin, uint8_t medSpeedPin, uint8_t stopPin) {
  _highSpeedPin = highSpeedPin;
  _lowSpeedPin = lowSpeedPin;
  _medSpeedPin = medSpeedPin;
  _stopPin = stopPin;

  // Initialize all pump control pins as outputs with HIGH (inactive)
  pinMode(_highSpeedPin, OUTPUT);
  pinMode(_lowSpeedPin, OUTPUT);
  pinMode(_medSpeedPin, OUTPUT);
  pinMode(_stopPin, OUTPUT);
  
  digitalWrite(_highSpeedPin, HIGH);
  digitalWrite(_lowSpeedPin, HIGH);
  digitalWrite(_medSpeedPin, HIGH);
  digitalWrite(_stopPin, HIGH);
}

void PumpController::simulateButtonPress(uint8_t pin) {
  digitalWrite(pin, LOW);
  delay(500);
  digitalWrite(pin, HIGH);
  delay(500);
}

void PumpController::setLowSpeed() {
  // Set all other pins HIGH first
  digitalWrite(_highSpeedPin, HIGH);
  delay(100);
  digitalWrite(_medSpeedPin, HIGH);
  delay(100);
  digitalWrite(_stopPin, HIGH);
  delay(100);
  
  // Simulate low speed button press
  simulateButtonPress(_lowSpeedPin);
  _currentSpeed = LOW_SPEED;
}

void PumpController::setHighSpeed() {
  // Set all other pins HIGH first
  digitalWrite(_lowSpeedPin, HIGH);
  delay(100);
  digitalWrite(_medSpeedPin, HIGH);
  delay(100);
  digitalWrite(_stopPin, HIGH);
  delay(100);
  
  // Simulate high speed button press
  simulateButtonPress(_highSpeedPin);
  _currentSpeed = HIGH_SPEED;
}

void PumpController::setMedSpeed() {
  // Set all other pins HIGH first
  digitalWrite(_lowSpeedPin, HIGH);
  delay(100);
  digitalWrite(_stopPin, HIGH);
  delay(100);
  digitalWrite(_highSpeedPin, HIGH);
  delay(100);
  
  // Simulate medium speed button press
  simulateButtonPress(_medSpeedPin);
  _currentSpeed = MED_SPEED;
}

void PumpController::setStop() {
  // Set all other pins HIGH first
  digitalWrite(_lowSpeedPin, HIGH);
  delay(100);
  digitalWrite(_medSpeedPin, HIGH);
  delay(100);
  digitalWrite(_highSpeedPin, HIGH);
  delay(100);
  
  // Simulate stop button press
  simulateButtonPress(_stopPin);
  _currentSpeed = STOP;
}

PumpController::PumpSpeed PumpController::getCurrentSpeed() const {
  return _currentSpeed;
}

const char* PumpController::getSpeedString() const {
  switch(_currentSpeed) {
    case LOW_SPEED: return "Low";
    case HIGH_SPEED: return "High";
    case MED_SPEED: return "Medium";
    case STOP: return "Stopped";
    case NO_SPEED: return "None";
    default: return "unknown";
  }
}
