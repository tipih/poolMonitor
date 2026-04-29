// ModbusMaster stub for native unit tests.
//
// Reproduces just enough of the 4-20ma/ModbusMaster API for HeatPumpManager
// to compile and for resultToString() / cached-state tests to run. All
// transactions return a programmable status code and getResponseBuffer()
// returns programmable values, so individual tests can simulate bus
// success/error without touching real RS485 hardware.
#pragma once

#include <cstdint>
#include "HardwareSerial.h"

class ModbusMaster {
public:
  // Status constants (matches the real library).
  static constexpr uint8_t ku8MBSuccess             = 0x00;
  static constexpr uint8_t ku8MBIllegalFunction     = 0x01;
  static constexpr uint8_t ku8MBIllegalDataAddress  = 0x02;
  static constexpr uint8_t ku8MBIllegalDataValue    = 0x03;
  static constexpr uint8_t ku8MBSlaveDeviceFailure  = 0x04;
  static constexpr uint8_t ku8MBInvalidSlaveID      = 0xE0;
  static constexpr uint8_t ku8MBInvalidFunction     = 0xE1;
  static constexpr uint8_t ku8MBResponseTimedOut    = 0xE2;
  static constexpr uint8_t ku8MBInvalidCRC          = 0xE3;

  void begin(uint8_t /*slave*/, HardwareSerial& /*serial*/) {}
  void preTransmission(void (*)()) {}
  void postTransmission(void (*)()) {}

  // Programmable hooks for tests.
  static uint8_t  nextResult;
  static uint16_t responseBuffer[8];

  uint8_t readInputRegisters(uint16_t /*addr*/, uint16_t /*qty*/)   { return nextResult; }
  uint8_t readHoldingRegisters(uint16_t /*addr*/, uint16_t /*qty*/) { return nextResult; }
  uint8_t readCoils(uint16_t /*addr*/, uint16_t /*qty*/)            { return nextResult; }
  uint8_t writeSingleCoil(uint16_t /*addr*/, uint8_t /*val*/)       { return nextResult; }
  uint8_t writeSingleRegister(uint16_t /*addr*/, uint16_t /*val*/)  { return nextResult; }

  uint16_t getResponseBuffer(uint8_t i) const {
    return (i < 8) ? responseBuffer[i] : 0;
  }
};
