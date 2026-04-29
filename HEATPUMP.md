# Heat Pump Integration

Read-only Modbus RTU integration for **Welldana PMH / Fairland IPHCR** family
pool heat pumps. The ESP32 polls the heat pump over RS485 (via a MAX485
transceiver) and republishes the values to MQTT, where Home Assistant picks
them up automatically.

> ⚠️ **Read-only for now.** All write operations exist in the C++ API
> (`setPower`, `setTargetTempC`, `setOperationMode`, `setSilenceMode`) but
> are intentionally **not exposed** via MQTT or the web UI. Home Assistant
> discovery advertises only sensors and binary_sensors, so HA shows no
> controls.

## Hardware

| Signal | ESP32 GPIO | MAX485 pin |
|---|---|---|
| UART2 RX | 16 | RO |
| UART2 TX | 17 | DI |
| Driver Enable | 18 | DE |
| Receiver Enable (active low) | 19 | !RE |

If your MAX485 module ties DE and !RE together, wire that single pin to GPIO 18
and pass `18` for both `dePin` and `rePin` in `HeatPumpManager::begin(...)`.

A/B of the MAX485 connect to the heat pump's RS485 terminals. Use a 120 Ω
termination resistor across A/B at one end of the bus if the cable is long.

- Modbus settings: **9600 baud, 8N1, slave ID 1** (Fairland default).

## Register map (1-based, per Welldana/Fairland docs)

| Type | Reg | Function | Scale |
|---|---|---|---|
| Coil | 0 | Power on/off | 0/1 |
| Input | 2001 | Error code | raw |
| Input | 2010 | Inlet water temp | °C × 10 |
| Input | 2011 | Outlet water temp | °C × 10 |
| Input | 2012 | Ambient air temp | °C × 10 |
| Holding | 3001 | Target water temp | °C × 10 |
| Holding | 3002 | Operation mode | 0=Auto, 1=Heat, 2=Cool |
| Holding | 3003 | Silence mode | 0=Smart, 1=Silence, 2=Super Silence |

`HeatPumpManager` converts to 0-based Modbus addresses internally
(`address = register − 1`).

## Runtime flow

1. `HeatPumpManager::begin()` (called from `setup()`) configures UART2,
   the MAX485 direction pins, and the `ModbusMaster` instance.
2. `loop()` calls `heatPumpManager.poll()` every iteration. Polling is
   **self rate-limited** — the bus is only hit once every 5 s (configurable
   via `setPollInterval(ms)`).
3. A poll cycle issues four Modbus transactions (error, three temps, three
   holding registers, power coil) and caches the decoded values.
4. The 10 s Dallas-temperature tick calls `publishStatusToMQTT()`, which
   reads the cached values via getters and publishes them.

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

## Home Assistant entities

Discovery configs are published once on first MQTT connect (retained), all
attached to the existing `pool_monitor` HA device:

- Heat Pump Inlet / Outlet / Ambient / Target Temperature (`sensor`, °C)
- Heat Pump Power (`binary_sensor`)
- Heat Pump Mode (`sensor`, mapped to *Auto/Heat/Cool*)
- Heat Pump Silence Mode (`sensor`, mapped to *Smart/Silence/Super Silence*)
- Heat Pump Error Code (`sensor`)
- Heat Pump Modbus (`binary_sensor`, *problem* class — on when offline)

## Roadmap (when write access is wanted)

- Subscribe to `heatpump/set/...` topics and route to `HeatPumpManager`
  setters.
- Promote HA discovery: power → `switch`, target temp → `number`,
  mode/silence → `select`.
- Add web UI tile mirroring the same controls.
