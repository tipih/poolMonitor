# Code Quality Analysis Report

## Dead Code (Unused Functions/Variables)

All previously reported dead code has been removed. No new dead code found.

### Previously Reported — Now Fixed

- **`WiFiManager::handle()`** — **FIXED**: Fully implemented with exponential-backoff
  reconnect logic and called from `loop()` in main.cpp.
- **`WiFiManager::_instance`** — **FIXED**: Static singleton pointer removed; replaced
  with purpose-specific static helpers (`_staticPrefs`, `_staticResetCounter`).
- **`TemperatureSensor::getLastReadTime()`** — **FIXED**: Method removed.
- **`TemperatureSensor::isConnected()`** — **FIXED**: Method removed.
- **Duplicate header guard in TemperatureSensor.h** — **FIXED**: Now a single
  `#endif // TEMPERATURE_SENSOR_H`.
- **`MQTTManager::getClient()`** — **FIXED**: Method removed.
- **`OTAManager::setPassword()`** — **FIXED**: Functionality folded into `begin()`.
- **`OTAManager::setHostname()`** — **FIXED**: Functionality folded into `begin()`.

## Code Quality Issues

### 1. PumpController — Blocking Delays
**Severity: HIGH** | **Status: ACCEPTED (won't-fix, by-design)**
- Multiple `delay()` calls in every speed-change method.
- Location: PumpController.cpp lines 31–33, 39–46, 51–58, 64–71, 78–85
- Each speed change blocks ~1300 ms total (3 × 100 ms pin release + 2 × 500 ms pulse)
- **Impact**: Blocks main loop during the call; OTA handling, MQTT `loop()`, watchdog
  resets, and schedule checks are all deferred for over a second.
- The class-level doc comment acknowledges this limitation.
- **Decision (accepted)**: The blocking timing is required by the panel's physical
  button-press hardware contract. Speed changes are infrequent (schedule-driven,
  not per-loop), and the 90 s watchdog has ~70× headroom over the worst-case
  ~1.3 s blocking window. A non-blocking refactor was considered and rejected as
  not worth the added state-machine complexity.

### 2. HTML Form Variables — Hardcoded Defaults
**Severity: MEDIUM** | **Status: FIXED**
- `timeon` and `timeoff` are initialised to compile-time constants `6` / `18`,
  so the page briefly showed those values on every load before the first
  poll response arrived.
- Location: html.h lines 453–454
- **Resolution**: The polling body was extracted into a named `refreshState()`
  function; the script now calls it once immediately and only then schedules
  `setInterval(refreshState, 2000)`. The dashboard renders real values within
  one HTTP round-trip instead of waiting for the first interval tick (commit
  `c5e142b`).

### 3. InputManager — Mixed int / uint8_t State Types
**Severity: LOW** | **Status: FIXED**
- Pin members already used `uint8_t`; the logical state variables
  (`_ledState`, `_buttonState`, `_lastButtonState`, `_currentRelaxStatus`) were
  declared as `int`.
- Location: InputManager.h lines 30–33
- **Resolution**: `_ledState` is now `bool`; the remaining three are `uint8_t`,
  and `getRelaxStatus()` returns `uint8_t`. Callers in `main.cpp` and
  `WebServerManager.cpp` updated to use `%u` format specifiers (commit
  `b5a135d`).

### 4. Millis() Overflow Handling
**Severity: LOW** | **Status: NO ISSUE**
- All timing comparisons use `(millis() - last) >= interval` pattern, which handles
  the 49-day rollover correctly. ✓

### 5. Serial Print Statements
**Severity: LOW** | **Status: FIXED**
- Verbose `Serial.print()` calls were unguarded throughout the managers.
- **Resolution**: Added `src/Debug.h` with `DBG_PRINT/PRINTLN/PRINTF` macros
  that compile to no-ops when `DEBUG` is undefined. `-DDEBUG` is set only in
  the `[env:test]` build_flags, so chatty per-event prints (HTTP request
  handlers, per-temperature reads, per-MQTT-publish logs) are stripped from
  production firmware (~944 bytes of flash saved). Boot/init/error prints
  are intentionally left as `Serial.print*` so they remain visible in both
  environments. To temporarily enable verbose logs in production, add
  `-DDEBUG` to that env's `build_flags` (commits `3af07cd`, `f766eb7`).

## Potential Bugs

### 1. WiFi Reconnect Counter Not Reset
**Status: FIXED** — `_reconnectAttempts` is reset to 0 inside `onGotIP()`.

### 2. Buffer Overflow Risk in handleState()
**Status: FIXED** — `WebServerManager::handleState()` uses `snprintf()` throughout,
and the response buffer was grown from 200 to 256 bytes (commit `eaaccb3`) to
leave comfortable headroom over the ~175-byte worst-case payload.

### 3. No Null Check Before Using _pumpController/_mqttManager
**Status: FIXED** — `ScheduleManager::checkAndExecute()` guards with nullptr checks.

## Memory Usage

### Optimizations Applied
- TimeManager: Reduced from 24 bytes to 9 bytes (uint8_t/uint16_t types)
- ScheduleManager: Reduced from 8 bytes to 2 bytes (uint8_t types)
- TemperatureSensor: Heap-allocates `OneWire` and `DallasTemperature` once
  in `begin()` (idempotent — second call is a no-op). The previous
  placement-new-into-aligned-buffers pattern was removed as
  over-engineered for an object that lives for the entire firmware
  lifetime.
- All dead code removed (~500 bytes flash estimated saving)
- InputManager pin numbers changed to `uint8_t` (saves 4 bytes RAM)
- InputManager state variables (`_ledState`, `_buttonState`,
  `_lastButtonState`, `_currentRelaxStatus`) changed from `int` to
  `bool` / `uint8_t`; `getRelaxStatus()` now returns `uint8_t`.

### Remaining Opportunities
_(none currently tracked)_

## Architectural Issues

### 1. PumpController Coupling
**Severity: MEDIUM** | **Status: ACCEPTED (won't-fix)**
- Blocking hardware delays are embedded directly in the public speed-change API,
  making callers responsible for timing context.
- **Recommendation (deferred)**: Separate the GPIO pulsing mechanism
  (`PumpHardware`) from the higher-level speed-selection logic (`PumpControl`)
  so the state machine can live in the control layer without touching hardware
  timing directly.
- **Decision (accepted)**: Same root cause as the blocking-delays item above.
  Splitting the class only pays off if the timing model itself becomes
  non-blocking, and that change is intentionally not being made.

### 2. Global Variables in main.cpp
**Severity: LOW** | **Status: ACCEPTABLE**
- All manager instances and config are file-scope globals.
- Standard and idiomatic pattern for Arduino/ESP32 sketches; no change needed.

## Summary

**Critical Issues**: 0
**High Priority**: 0 open (1 accepted: PumpController blocking delays)
**Medium Priority**: 0 open (1 accepted: PumpController architecture)
**Low Priority**: 0 open

**Accepted (won't-fix) items**:
1. PumpController blocking delays — required by panel button-press hardware
   timing; 90 s watchdog has ~70× headroom over the ~1.3 s worst case.
2. PumpController coupling/architecture — same root cause; splitting the class
   is only worthwhile if the timing model becomes non-blocking, which is not
   planned.

**Recommendations Priority**:
_(no open recommendations)_

## Test Coverage

Host-PC unit tests (`pio test -e native`) currently cover:

- **PumpController** — initial `NO_SPEED`, speed setters update state,
  `getSpeedString()` mapping for all five enum values.
- **ScheduleManager** — `isValidSchedule()` for normal / out-of-range /
  equal hours, `setSchedule()` rejects invalid input without mutating
  state, `checkAndExecute()` drives the pump and publishes MQTT on hour
  boundaries and is idempotent within an hour.
- **HeatPumpManager** — initial offline / NaN / default state,
  `resultToString()` for known and unknown Modbus codes, `poll()` no-op
  before `begin()`.

Intentionally not covered (no useful pure-logic surface): GPIO timing,
NVS persistence across reboots, real Modbus traffic, WiFi / MQTT broker
/ OTA / AsyncWebServer, TimeManager (SNTP), TemperatureSensor (OneWire).
