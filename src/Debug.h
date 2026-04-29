#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

/**
 * Debug logging macros. Defined as Serial.print* when DEBUG is set,
 * compiled away to a no-op otherwise.
 *
 * `DEBUG` is set in platformio.ini's common build_flags so the default
 * behaviour matches the previous unconditional Serial.print* calls. To
 * silence the chatty per-event logs (HTTP requests, MQTT publishes,
 * per-temperature-reading prints, ...), add `-UDEBUG` to the build_flags
 * for that environment.
 *
 * Use these for high-frequency / per-event logs only. Boot, init and
 * error messages should keep using Serial.print* directly so they
 * remain visible regardless of the DEBUG flag.
 */
#ifdef DEBUG
  #define DBG_PRINT(...)   Serial.print(__VA_ARGS__)
  #define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define DBG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)   ((void)0)
  #define DBG_PRINTLN(...) ((void)0)
  #define DBG_PRINTF(...)  ((void)0)
#endif

#endif // DEBUG_H
