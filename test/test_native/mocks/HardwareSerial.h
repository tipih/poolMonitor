// HardwareSerial stub for native unit tests.
#pragma once

#include <cstdint>

#define SERIAL_8N1 0x06

class HardwareSerial {
public:
  HardwareSerial(int /*uartNum*/) {}
  void begin(unsigned long /*baud*/, uint8_t /*config*/ = SERIAL_8N1,
             int /*rxPin*/ = -1, int /*txPin*/ = -1) {}
  void end() {}
};
