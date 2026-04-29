// Minimal Arduino.h stub for native (host) unit tests.
// Provides only the symbols used by the production code under test.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cmath>
#include <cstring>
#include <string>

#define HIGH 1
#define LOW  0
#define OUTPUT 1
#define INPUT  0
#define INPUT_PULLUP 2

// PROGMEM helpers used by html.h etc.
#define PROGMEM
#define F(x) (x)

using boolean = bool;

inline void pinMode(uint8_t /*pin*/, uint8_t /*mode*/) {}
inline void digitalWrite(uint8_t /*pin*/, uint8_t /*val*/) {}
inline int  digitalRead(uint8_t /*pin*/) { return LOW; }
inline void delay(unsigned long /*ms*/) {}
inline void delayMicroseconds(unsigned long /*us*/) {}
inline unsigned long millis() { return 0; }
inline unsigned long micros() { return 0; }

// Tiny Serial stub.
struct SerialStub {
  void begin(unsigned long) {}
  template <typename T> void print(T) {}
  template <typename T> void print(T, int) {}
  template <typename T> void println(T) {}
  void println() {}
  void printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
  }
};

inline SerialStub Serial;

// Minimal String shim (only what tests need; production code under test
// uses const char* almost exclusively).
class String {
public:
  String() = default;
  String(const char* s) : _s(s ? s : "") {}
  const char* c_str() const { return _s.c_str(); }
private:
  std::string _s;
};
