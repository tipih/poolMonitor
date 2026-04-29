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
**Severity: HIGH** | **Status: OPEN**
- Multiple `delay()` calls in every speed-change method.
- Location: PumpController.cpp lines 31–33, 39–46, 51–58, 64–71, 78–85
- Each speed change blocks ~1300 ms total (3 × 100 ms pin release + 2 × 500 ms pulse)
- **Impact**: Blocks main loop during the call; OTA handling, MQTT `loop()`, watchdog
  resets, and schedule checks are all deferred for over a second.
- The class-level doc comment acknowledges this limitation.
- **Recommendation**: Implement a non-blocking state machine (e.g. track press phase
  and target pin; advance on each `handle()` call when the elapsed time is met).

### 2. HTML Form Variables — Hardcoded Defaults
**Severity: MEDIUM** | **Status: OPEN**
- `timeon` and `timeoff` are initialised to compile-time constants `6` / `18`.
- Location: html.h lines 453–454
- The `/state` polling interval fires every 2 s, so incorrect defaults are shown
  briefly on every page load before the first XHR response arrives.
- **Recommendation**: Trigger one immediate `GET /state` call on `DOMContentLoaded`
  (before starting the `setInterval`) so the display is seeded from server state
  before the user sees the page.

### 3. InputManager — Mixed int / uint8_t State Types
**Severity: LOW** | **Status: OPEN**
- Pin members correctly use `uint8_t`, but the logical state variables
  (`_ledState`, `_buttonState`, `_lastButtonState`, `_currentRelaxStatus`) are
  declared as `int`.
- Location: InputManager.h lines 30–33
- **Recommendation**: Use `uint8_t` (or `bool` for LED/button state) for consistency
  and a minor RAM saving.

### 4. Millis() Overflow Handling
**Severity: LOW** | **Status: NO ISSUE**
- All timing comparisons use `(millis() - last) >= interval` pattern, which handles
  the 49-day rollover correctly. ✓

### 5. Serial Print Statements
**Severity: LOW** | **Status: OPEN**
- Verbose `Serial.print()` calls throughout every manager.
- **Impact**: Minor performance overhead and increased binary size in production
  firmware.
- **Recommendation**: Wrap with a compile-time `DEBUG` flag (e.g.
  `#ifdef DEBUG … #endif`) so production builds can strip them.

## Potential Bugs

### 1. WiFi Reconnect Counter Not Reset
**Status: FIXED** — `_reconnectAttempts` is reset to 0 inside `onGotIP()`.

### 2. Buffer Overflow Risk in handleState()
**Status: FIXED** — `WebServerManager::handleState()` uses `snprintf()` throughout.

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
**Severity: MEDIUM** | **Status: OPEN**
- Blocking hardware delays are embedded directly in the public speed-change API,
  making callers responsible for timing context.
- **Recommendation**: Separate the GPIO pulsing mechanism (`PumpHardware`) from the
  higher-level speed-selection logic (`PumpControl`) so the state machine can live
  in the control layer without touching hardware timing directly.

### 2. Global Variables in main.cpp
**Severity: LOW** | **Status: ACCEPTABLE**
- All manager instances and config are file-scope globals.
- Standard and idiomatic pattern for Arduino/ESP32 sketches; no change needed.

## Summary

**Critical Issues**: 0
**High Priority**: 1 (Blocking delays in PumpController)
**Medium Priority**: 2 (HTML initialisation, PumpController architecture)
**Low Priority**: 3 (InputManager state types, Serial prints)

**Recommendations Priority**:
1. Refactor PumpController to use a non-blocking state machine
2. Seed HTML page with one immediate `/state` fetch on load
3. Add `DEBUG` compile flag to strip Serial output from production builds
4. Tighten InputManager state variable types to `uint8_t`/`bool`
