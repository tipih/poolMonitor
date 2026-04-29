# Native unit tests

These tests run on the host PC (not on the ESP32) via PlatformIO's Unity
integration. They cover pure logic and string-mapping contracts that
don't require real GPIO, NVS, MQTT, or RS485 traffic.

## Run

```pwsh
pio test -e native
```

Requires a host C++ compiler (gcc, clang, or MSVC) on `PATH`.

## What's covered

- `PumpController`
  - Initial speed is `NO_SPEED`.
  - Speed setters update state and `getSpeedString()` mapping.
- `ScheduleManager`
  - `isValidSchedule()` for normal, out-of-range, and equal hours.
  - `setSchedule()` rejects invalid input without mutating state.
  - `checkAndExecute()` triggers MED on ON-hour, LOW on OFF-hour, and is
    idempotent within the same hour.
- `HeatPumpManager`
  - Initial cached state (offline, NaN temps, error 0, default modes).
  - `resultToString()` mapping for known and unknown Modbus result codes.
  - `poll()` is a no-op before `begin()`.

## What's intentionally NOT covered

- Real GPIO pulses or pump-panel timing (PumpController's `delay()`s).
- NVS persistence across reboots (Preferences is mocked in-memory).
- Real Modbus RTU traffic / MAX485 DE/RE switching.
- WiFi, MQTT broker, OTA, AsyncWebServer routing.
- TimeManager (SNTP) and TemperatureSensor (OneWire) — both depend on
  hardware-specific calls with no useful pure-logic surface.

## Layout

- `mocks/` — minimal Arduino, Preferences, HardwareSerial, ModbusMaster,
  and MQTTManager headers used in place of the real platform headers.
- `test_main.cpp` — Unity test cases and runner.
