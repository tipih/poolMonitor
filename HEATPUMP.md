# Heat Pump Integration

Read-only cloud integration for **Fairland / Welldana / Comfortpool** pool
heat pumps using the **Linked-Go REST API** (`cloud.linked-go.com`). The
ESP32 authenticates with the same credentials as the mobile app, polls the
API every 30 seconds, and republishes the values to MQTT, where Home
Assistant picks them up automatically.

> ⚠️ **Read-only for now.** All write operations exist in the C++ API
> (`setPower`, `setTargetTempC`, `setOperationMode`, `setSilenceMode`) but
> are intentionally **not exposed** via MQTT or the web UI. Home Assistant
> discovery advertises only sensors and binary_sensors, so HA shows no
> controls.

## Prerequisites

- A **Linked-Go / Fairland / Comfortpool** cloud account (the same one used
  by the mobile app).
- The heat pump must already be commissioned and visible in the app.
- No additional hardware is needed — the ESP32 uses WiFi to reach the cloud.

## Credentials

Add your cloud account to `src/cred.h`:

```cpp
#define LINKED_GO_USER "your-email@example.com"
#define LINKED_GO_PASS "your-app-password"
```

These are passed to `HeatPumpManager::begin()` from `setup()` in `main.cpp`.

## Authentication flow

1. **Login** (`POST /user/login.json`) — exchanges username + password for an
   `x-token`. Repeated automatically when the token is absent or older than
   12 hours.
2. **Device discovery** (`POST /device/deviceList.json`) — retrieves the
   `device_code` of the first device in the account. Retried on the next poll
   cycle if the initial attempt fails.
3. **Telemetry** (`POST /device/getDataByCode.json`) — fetches the protocol
   codes listed below every 30 s. The token is reused for every normal poll;
   re-authentication only happens at step 1's 12-hour boundary.

## Protocol codes fetched

| Code | Field | Getter |
|------|-------|--------|
| `Power` | On/Off | `isPowerOn()` |
| `T02` | Inlet water temperature | `getInletTempC()` |
| `T03` | Outlet water temperature | `getOutletTempC()` |
| `T05` | Ambient air temperature | `getAmbientTempC()` |
| `R02` | Target water temperature | `getTargetTempC()` |
| `Mode` | Operation mode (0=Auto, 1=Heat, 2=Cool) | `getOperationMode()` |
| `Manual-mute` | Silence mode (0=Smart, 1=Silence, 2=Super) | `getSilenceMode()` |
| `Err` | Error code | `getErrorCode()` |
| `O07` | Compressor frequency (Hz, max 85 Hz) | `getCompressorHz()` / `getCompressorPercent()` |

## Runtime flow

1. `HeatPumpManager::begin(user, pass)` (called from `setup()`) stores
   credentials, attempts login, and fetches the device code. WiFi must be
   connected before `begin()` is called.
2. `loop()` calls `heatPumpManager.poll()` every iteration. Polling is
   **self rate-limited** — the cloud API is contacted at most once every 30 s,
   counted from the end of the previous HTTPS exchange.
3. When the 10 s Dallas-temperature tick fires, `publishStatusToMQTT()` reads
   the cached getter values and publishes them to MQTT.

## Serial debug output

Each stage prints progress with timing to the serial monitor at 115200 baud:

```
[HeatPump] Cloud REST client initialized
[HeatPump] Logging in as you@example.com ...
[HeatPump] Login response 312 bytes in 843 ms
[HeatPump] Login OK – token: a3f9c2b1...
[HeatPump] Fetching device list ...
[HeatPump] DeviceList response 487 bytes in 612 ms
[HeatPump] Device code: HP-XXXXXXXXXXXX
...
[HeatPump] Poll start (uptime 120 s, last success 30 s ago)
[HeatPump] Fetching telemetry ...
[HeatPump] Telemetry response 628 bytes in 754 ms
[HeatPump] Telemetry: power=ON inlet=28.5 outlet=32.1 ambient=21.0 target=30.0 mode=0 silence=0 err=0 compressor=42.5 Hz (50%)
[HeatPump] Poll OK in 756 ms
```

On any error the raw JSON response body is also printed to aid diagnosis.

## MQTT topics

Base topic from `platformio.ini` (`MQTT_TOPIC`), e.g. `pool/monitor`:

| Subtopic | Example payload |
|---|---|
| `heatpump/inlet_temp` | `27.4` |
| `heatpump/outlet_temp` | `28.9` |
| `heatpump/ambient_temp` | `21.0` |
| `heatpump/target_temp` | `28.0` |
| `heatpump/error` | `0` |
| `heatpump/power` | `ON` / `OFF` |
| `heatpump/mode` | `0` / `1` / `2` |
| `heatpump/silence` | `0` / `1` / `2` |
| `heatpump/online` | `1` / `0` |
| `heatpump/compressor_hz` | `42.5` |
| `heatpump/compressor_pct` | `50` |

## Home Assistant entities

Discovery configs are published once on first MQTT connect (retained), all
attached to the existing `pool_monitor` HA device:

- Heat Pump Inlet / Outlet / Ambient / Target Temperature (`sensor`, °C)
- Heat Pump Power (`binary_sensor`)
- Heat Pump Mode (`sensor`, mapped to *Auto/Heat/Cool*)
- Heat Pump Silence Mode (`sensor`, mapped to *Smart/Silence/Super Silence*)
- Heat Pump Error Code (`sensor`)
- Heat Pump Cloud (`binary_sensor`, *problem* class — on when offline)
- Heat Pump Compressor Hz (`sensor`, Hz, icon `mdi:sine-wave`)
- Heat Pump Compressor Load (`sensor`, %, icon `mdi:gauge`)

## Roadmap (when write access is wanted)

- Subscribe to `heatpump/set/...` topics and route to `HeatPumpManager`
  setters via `sendControl()`.
- Promote HA discovery: power → `switch`, target temp → `number`,
  mode/silence → `select`.
- Add web UI tile controls mirroring the same commands.
