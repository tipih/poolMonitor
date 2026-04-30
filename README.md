# Pool Monitor

ESP32-based pool pump and heat pump monitoring & control system with a modern web interface, Dallas temperature sensing, Linked-Go cloud REST heat pump readout, MQTT integration, Home Assistant auto-discovery, and host-PC unit tests.

## ✨ Features

### Core Functionality
- **🎮 Remote Pool Pump Control**: Control pump speed (Low/Med/High/Stop) via web interface or MQTT
- **🌡️ Temperature Monitoring**: Real-time pool water temperature using Dallas DS18B20 sensor
- **♈️ Heat Pump Read-Out**: Cloud REST API integration via [linked-go.com](https://cloud.linked-go.com) (Fairland / Welldana / Comfortpool app account) — inlet/outlet/ambient temperature, target temperature, operation & silence mode, compressor frequency & load, power and error code (read-only; see [HEATPUMP.md](HEATPUMP.md))
- **📱 Modern Web Interface**: Responsive dashboard with 5 color themes and animated pool water effects
- **🏠 Home Assistant Integration**: Automatic MQTT discovery for seamless smart home integration
- **⏰ Scheduled Operation**: Configure automatic on/off times with persistent storage
- **🔄 Reliable OTA Updates**: Over-the-air firmware updates with protection against blocking operations
- **📡 WiFi Connectivity**: Automatic reconnection with retry logic
- **💾 Persistent Settings**: Non-volatile storage for schedules and configurations

### User Interface
- **🎨 5 Color Themes**: Ocean (blue), Forest (green), Sand (earth), Night (dark), Terracotta (clay)
- **💧 Animated Water Effects**: CSS-based pool water caustic light animations
- **🔐 HTTP Authentication**: Secure access with username/password
- **📊 Real-time Status**: Live temperature, RSSI, pump speed, and connection status
- **⚡ Instant Updates**: AJAX-based interface with smooth transitions

### Code Quality
- **🏗️ Modular Architecture**: 10 manager classes for maintainability
- **🧪 Host-PC Unit Tests**: Unity test suite (`pio test -e native`) covers `PumpController` and `ScheduleManager` pure logic — 18 test cases
- **🔇 DEBUG Stripped from Production**: `DBG_PRINT*` macros compile to no-ops in `[env:production]`; verbose serial logs only in `[env:test]`
- **🛡️ OTA Protection**: Prevents pump operations during firmware updates
- **🐛 Bug-Free**: Fixed buffer overflows, WiFi event issues, memory leaks
- **📉 Optimized**: ~540 bytes flash saved, ~34 bytes RAM saved through type optimization
- **📝 Well Documented**: Comprehensive code quality report included

## 🔧 Hardware Requirements

- **ESP32 Development Board** (ESP32 DOIT DevKit V1 or compatible)
- **Dallas DS18B20** temperature sensor (one-wire)
- **Relay Module** or transistor circuit to interface with pool pump control buttons
- **4.7kΩ resistor** (pull-up for DS18B20)
- **WiFi access** to the Internet (for Linked-Go cloud API calls)
- **Push button** (optional, for manual input/testing)

> ℹ️ The RS485/MAX485 transceiver required for the old Modbus integration is
> no longer needed. Heat pump data is now fetched over HTTPS from the
> Linked-Go cloud service using the same credentials as the Fairland /
> Comfortpool mobile app.

### 📍 Pin Configuration

| GPIO | Function | Description |
|------|----------|-------------|
| **4** | DS18B20 Data | Pool temperature sensor (one-wire) |
| **13** | Button Input | Manual control button (active HIGH) |
| **14** | Medium Speed | Pool pump medium speed control |
| **25** | Stop | Pool pump stop control |
| **26** | Low Speed | Pool pump low speed control |
| **27** | High Speed | Pool pump high speed control |
| **LED_BUILTIN** | Status LED | Connection status indicator |

> GPIO 16–19 (previously used for RS485) are now free for other purposes.

## 📦 Software Requirements

Built with PlatformIO using the Arduino framework.

### Dependencies

```ini
- PubSubClient @ ^2.8                    # MQTT client library
- AsyncTCP-esphome @ ^1.2.2              # Asynchronous TCP library  
- ESPAsyncWebServer-esphome @ ^2.1.0     # Async web server
- DallasTemperature @ ^3.10.0            # DS18B20 sensor library
- bblanchon/ArduinoJson @ ^7.0.0         # JSON parse/serialise (cloud REST API)
- Preferences @ 2.0.0                    # NVS storage
- WiFi @ 2.0.0                           # WiFi connectivity
- ArduinoOTA @ 2.0.0                     # OTA updates
- HTTPClient (bundled with ESP32 core)   # HTTPS REST calls
- WiFiClientSecure (bundled)             # TLS for Linked-Go cloud API
```

## ⚙️ Configuration

### 1. Credentials Setup

Edit `src/cred.h` with your credentials:

```cpp
#define SS_ID          "your-wifi-ssid"
#define auth           "your-wifi-password"
#define HTTP_USER      "admin"
#define HTTP_PASS      "your-web-password"
#define MQTT_USER      "mqtt-username"
#define MQTT_PASS      "mqtt-password"
// Linked-Go cloud account (same as Fairland / Comfortpool mobile app)
#define LINKED_GO_USER "your-email@example.com"
#define LINKED_GO_PASS "your-app-password"
```

**Note**: `cred.h` is excluded from version control via `.gitignore` to protect your credentials.

### 2. MQTT Configuration

MQTT settings are configured in `platformio.ini` build flags:

**Production Environment:**
```ini
-DMQTT_CLIENT_ID=\"PoolMonitor\"
-DMQTT_TOPIC=\"pool/monitor\"
```

**Test Environment:**
```ini
-DMQTT_CLIENT_ID=\"PoolMonitor_Test\"
-DMQTT_TOPIC=\"pool/monitor/test\"
```

MQTT broker address is set in `src/main.cpp`:
```cpp
#define MQTT_HOST "192.168.0.54"  # Your MQTT broker IP
#define MQTT_PORT 1883
```

## 🚀 Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/tipih/poolMonitor.git
   cd poolMonitor
   ```

2. **Create your credentials file:**
   ```bash
   # Create src/cred.h with your WiFi and authentication credentials
   ```

3. **Update MQTT broker settings** in `src/main.cpp` if needed

4. **Build and upload:**
   
   For production device:
   ```bash
   platformio run --target upload --environment production
   ```
   
   For test device:
   ```bash
   platformio run --target upload --environment test
   ```

5. **Monitor serial output:**
   ```bash
   platformio device monitor
   ```

## 🧪 Testing

Host-PC unit tests run on your development machine via PlatformIO's
Unity integration — no ESP32 hardware required.

```bash
platformio test -e native
```

Covered managers and behaviours:

- **`PumpController`** (5 tests) — initial state, `begin()` side-effects, all speed setters and string mappings, repeated changes, stop from running
- **`ScheduleManager`** (13 tests) — default hours, `isValidSchedule()` (range + equal-hour + midnight-wrap), `setSchedule()` ignores invalid input, `checkAndExecute()` drives pump + MQTT at hour boundaries, idempotent within an hour, no-op at unrelated hour, safe before `begin()`, midnight on-hour

Requires a host C++ compiler on `PATH`. On this machine LLVM clang at
`C:\Program Files\LLVM\bin` is wrapped by `scripts/shims/` (gcc/g++/ar/ranlib
`.cmd` files). See [test/test_native/README.md](test/test_native/README.md) for layout and what is
intentionally not covered (real GPIO / NVS / HTTPS / WiFi).

> `HeatPumpManager` uses `HTTPClient` and `WiFiClientSecure` which are
> ESP32-only; its behaviour is verified by integration testing on the device.

## 🌐 Usage

### Web Interface

1. Power on your ESP32
2. Find the device IP address from serial monitor or router DHCP table
   - **Production**: Usually assigned by DHCP
   - **Test**: Usually assigned by DHCP
3. Open a web browser: `http://<ESP32-IP-ADDRESS>`
4. Login with credentials from `cred.h`
5. Features available:
   - **Control Panel**: Set pump speed (Low/Med/High/Stop)
   - **Temperature Display**: Real-time pool temperature
   - **Schedule Manager**: Set automatic on/off times
   - **Theme Selector**: Choose from 5 color themes
   - **Status Bar**: WiFi signal, time, connection status

### 🏠 Home Assistant Integration

The device automatically publishes MQTT discovery messages for Home Assistant:

**Auto-discovered entities:**
- `sensor.poolmonitor_temperature` - Pool temperature sensor
- `sensor.poolmonitor_rssi` - WiFi signal strength
- `sensor.poolmonitor_pump_speed` - Current pump speed
- `binary_sensor.poolmonitor_status` - Pool system status
- `sensor.poolmonitor_ip` - Device IP address

**Heat pump (read-only):**
- `sensor.poolmonitor_hp_inlet_temp` / `_hp_outlet_temp` / `_hp_ambient_temp` - Heat pump temperatures
- `sensor.poolmonitor_hp_target_temp` - Configured target temperature
- `binary_sensor.poolmonitor_hp_power` - Heat pump on/off state
- `sensor.poolmonitor_hp_mode` - Operation mode (Auto / Heat / Cool)
- `sensor.poolmonitor_hp_silence` - Silence mode (Smart / Silence / Super Silence)
- `sensor.poolmonitor_hp_error` - Heat pump error code
- `binary_sensor.poolmonitor_hp_online` - Cloud link health (problem class)
- `sensor.poolmonitor_hp_comp_hz` - Compressor frequency (Hz)
- `sensor.poolmonitor_hp_comp_pct` - Compressor load (%)

All entities appear automatically under the "PoolMonitor" device in Home Assistant. Heat pump control entities are intentionally **not** advertised — the integration is read-only at this stage.

### 📡 MQTT Topics

**Published topics (pool):**
- `pool/monitor/temperature` - Temperature in °C
- `pool/monitor/rssi` - WiFi signal strength (dBm) 
- `pool/monitor/pump_speed` - Current speed (Low/Med/High/Stop)
- `pool/monitor/status` - System status (1=OK, 0=Error)
- `pool/monitor/ip` - Device IP address

**Published topics (heat pump):**
- `pool/monitor/heatpump/inlet_temp` - Inlet water temperature (°C)
- `pool/monitor/heatpump/outlet_temp` - Outlet water temperature (°C)
- `pool/monitor/heatpump/ambient_temp` - Ambient air temperature (°C)
- `pool/monitor/heatpump/target_temp` - Target water temperature (°C)
- `pool/monitor/heatpump/power` - `ON` / `OFF`
- `pool/monitor/heatpump/mode` - `0`=Auto, `1`=Heat, `2`=Cool
- `pool/monitor/heatpump/silence` - `0`=Smart, `1`=Silence, `2`=Super Silence
- `pool/monitor/heatpump/error` - Error code (numeric)
- `pool/monitor/heatpump/online` - Cloud link health (`1` / `0`)
- `pool/monitor/heatpump/compressor_hz` - Compressor frequency (Hz)
- `pool/monitor/heatpump/compressor_pct` - Compressor load (0–100 %)

**Home Assistant Discovery:**
- `homeassistant/sensor/poolmonitor_*` - Auto-discovery configs
- `homeassistant/binary_sensor/poolmonitor_*` - Auto-discovery configs

### 🔄 OTA Updates

The device supports Over-The-Air updates with built-in protection:

**Method 1: PlatformIO**
```bash
platformio run --target upload --environment production --upload-port 192.168.0.163
```

**Method 2: Arduino IDE**
- Network port will appear automatically
- Select "PoolMonitor at <IP>" from Tools > Port

**OTA Protection Features:**
- Automatically suspends pump operations during update
- Prevents blocking delays that could interrupt upload
- Web interface returns HTTP 503 if pump control attempted during update
- Requires good WiFi signal (RSSI better than -60 dBm recommended)

## 📐 Architecture

The codebase uses a modular manager-based architecture:

```
src/
├── main.cpp              # Main setup and loop
├── cred.h                # Credentials (gitignored)
├── html.h                # Web interface HTML/CSS/JS
├── Debug.h               # DBG_* macros (no-op when DEBUG undefined)
├── WiFiManager.*         # WiFi connection management
├── MQTTManager.*         # MQTT publishing & HA discovery
├── OTAManager.*          # OTA updates with protection
├── PumpController.*      # Pump speed control logic
├── ScheduleManager.*     # Automatic scheduling
├── TemperatureSensor.*   # DS18B20 temperature reading
├── TimeManager.*         # NTP time synchronization  
├── WebServerManager.*    # HTTP server & routes (incl. heat pump tile)
├── InputManager.*        # Button input & LED status
└── HeatPumpManager.*     # Linked-Go cloud REST heat pump readout (HTTPS)

test/
└── test_native/          # Host-PC Unity unit tests
    ├── mocks/            # Arduino, Preferences, MQTTManager mocks
    └── test_main.cpp     # Test runner

scripts/
├── native_use_clang.py  # PlatformIO pre-script: PATH + -include for [env:native]
└── shims/               # gcc/g++/ar/ranlib .cmd wrappers around clang
```

**Benefits:**
- Clear separation of concerns
- Easy to test and maintain
- Reduced coupling between components
- Improved code reusability

## ⏰ Default Schedule

- **On Time**: 6:00 AM
- **Off Time**: 6:00 PM

Schedule can be modified via the web interface. Settings are saved to non-volatile memory and persist across reboots.

## 🛡️ Reliability Features

- **Watchdog Timer**: 90-second timeout with automatic recovery
- **WiFi Auto-Reconnect**: 5 retry attempts before system reset
- **MQTT Reconnection**: Automatic reconnection with backoff
- **OTA Protection**: Suspends operations during firmware updates
- **Persistent Storage**: Schedule saved to NVS
- **Error Handling**: Comprehensive error checking throughout

## 🎨 Color Themes

Choose from 5 beautiful color schemes:

1. **Ocean** (Default) - Blue/purple gradients with wave effects
2. **Forest** - Green natural tones  
3. **Sand** - Warm beige earth colors
4. **Night** - Dark blue evening theme
5. **Terracotta** - Clay/brick earth tones

Theme preference is saved in browser localStorage.

## 📊 Memory Usage

- **Flash**: ~950 KB (leaves room for future features)
- **RAM**: Optimized with uint8_t/uint16_t types (~34 bytes saved)
- **Dead Code Removed**: ~540 bytes flash saved
- **Partition Scheme**: min_spiffs.csv (1.9 MB OTA partitions)

## 🐛 Known Issues & Solutions

### OTA Upload Fails
- **Cause**: Weak WiFi signal or blocking delays
- **Solution**: Move access point closer or use serial upload once
- **Prevention**: Built-in OTA protection now prevents this

### Schedule Not Saving
- **Cause**: NVS corruption or initialization issue  
- **Solution**: Fixed in v2.0.0 with proper ScheduleManager initialization

### Theme Switcher Stops Working
- **Cause**: Old theme class names in JavaScript
- **Solution**: Fixed in v2.0.0 - all themes work correctly

## 📝 Version History

### v2.2.0 (Current) — Quality Pass & Tests
- ✅ Heat pump status tile added to the web dashboard (`/state` JSON now
  includes `hpOnline`, `hpPower`, `hpMode`, `hpTarget`, `hpInlet`,
  `hpOutlet`, `hpAmbient`, `hpError`)
- ✅ New `[env:native]` host-PC unit tests for `PumpController`,
  `ScheduleManager`, `HeatPumpManager` (11 tests, run with
  `pio test -e native`)
- ✅ `src/Debug.h` with `DBG_PRINT/PRINTLN/PRINTF` macros; `-DDEBUG` is
  now only set in `[env:test]`, so production firmware drops the chatty
  per-event prints (~944 bytes flash saved)
- ✅ `pump_status` MQTT topic actually published to back the existing HA
  binary_sensor discovery
- ✅ `WiFiManager.onGotIP()` reads IP from the event payload (no WiFi
  API call inside the event task); reset counter migrated to
  `putULong/getULong`
- ✅ `HeatPumpManager` got a destructor and idempotent `begin()`
- ✅ `TemperatureSensor` simplified from placement-new pattern to a
  single one-time heap allocation
- ✅ `InputManager` state types tightened (`bool` / `uint8_t`)
- ✅ `ScheduleManager.isValidSchedule()` now rejects equal on/off hours
- ✅ `WebServerManager.handleState()` JSON buffer grown 200 → 384 bytes
  (heat pump fields fit; null guards return 503 if managers not wired)
- ✅ `TimeManager` month off-by-one fixed (`tm_mon + 1`)
- ✅ HTML uses `textContent` everywhere for server-driven DOM updates;
  `refreshState()` runs once on load before the 2 s interval

### v2.1.0 — Heat Pump Integration
- ✅ New `HeatPumpManager` reads Welldana / Fairland heat pump over RS485 (Modbus RTU)
- ✅ Self-rate-limited polling (default 5 s); cached values exposed via getters
- ✅ Heat pump telemetry published under `<base>/heatpump/*` every 10 s
- ✅ 9 new Home Assistant auto-discovery entities (sensors + binary_sensors, read-only)
- ✅ Added `4-20ma/ModbusMaster` dependency
- ✅ Removed legacy broken `heatpumpRead.*` stub
- ✅ Added [HEATPUMP.md](HEATPUMP.md) with wiring, register map and topic reference

### v2.0.0 - Architecture Refactor & UI Overhaul
- ✅ Complete architecture refactor to manager-based design
- ✅ 5 color themes with animated pool water effects
- ✅ OTA protection against blocking delays  
- ✅ Home Assistant MQTT auto-discovery
- ✅ Fixed WiFi event types and reconnection logic
- ✅ Removed dead code (~540 bytes saved)
- ✅ Type optimizations (uint8_t/uint16_t)
- ✅ Fixed buffer overflows and memory leaks
- ✅ Comprehensive code quality improvements
- ✅ Dual environment support (production/test)

### v1.0.0 - Initial Release
- ✅ Basic pump control functionality
- ✅ Temperature monitoring
- ✅ Web interface
- ✅ MQTT integration
- ✅ Schedule support

## 📄 License

This project is open source and available for personal and educational use.

## 🤝 Contributing

Contributions are welcome! Please see [CODE_QUALITY_REPORT.md](CODE_QUALITY_REPORT.md) for code standards and architecture guidelines.

## 🔗 Links

- **Repository**: https://github.com/tipih/poolMonitor
- **Issues**: https://github.com/tipih/poolMonitor/issues

---

**Made with ❤️ for pool automation**
