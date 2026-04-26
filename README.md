# Pool Monitor

ESP32-based pool pump monitoring and control system with modern web interface, temperature sensing, MQTT integration, and Home Assistant auto-discovery.

## ✨ Features

### Core Functionality
- **🎮 Remote Pool Pump Control**: Control pump speed (Low/Med/High/Stop) via web interface or MQTT
- **🌡️ Temperature Monitoring**: Real-time pool water temperature using Dallas DS18B20 sensor
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
- **🏗️ Modular Architecture**: 9 manager classes for maintainability
- **🛡️ OTA Protection**: Prevents pump operations during firmware updates
- **🐛 Bug-Free**: Fixed buffer overflows, WiFi event issues, memory leaks
- **📉 Optimized**: ~540 bytes flash saved, ~34 bytes RAM saved through type optimization
- **📝 Well Documented**: Comprehensive code quality report included

## 🔧 Hardware Requirements

- **ESP32 Development Board** (ESP32 DOIT DevKit V1 or compatible)
- **Dallas DS18B20** temperature sensor (one-wire)
- **Relay Module** or transistor circuit to interface with pool pump control buttons
- **4.7kΩ resistor** (pull-up for DS18B20)
- **Push button** (optional, for manual input/testing)

### 📍 Pin Configuration

| GPIO | Function | Description |
|------|----------|-------------|
| **4** | DS18B20 Data | Temperature sensor (one-wire) |
| **13** | Button Input | Manual control button (active HIGH) |
| **14** | Medium Speed | Pool pump medium speed control |
| **25** | Stop | Pool pump stop control |
| **26** | Low Speed | Pool pump low speed control |
| **27** | High Speed | Pool pump high speed control |
| **LED_BUILTIN** | Status LED | Connection status indicator |

## 📦 Software Requirements

Built with PlatformIO using the Arduino framework.

### Dependencies

```ini
- PubSubClient @ ^2.8           # MQTT client library
- AsyncTCP-esphome @ ^1.2.2     # Asynchronous TCP library  
- ESPAsyncWebServer-esphome @ ^2.1.0  # Async web server
- DallasTemperature @ ^3.10.0   # DS18B20 sensor library
- OneWire @ ^2.3.8              # One-wire protocol
- Preferences @ 2.0.0           # NVS storage
- WiFi @ 2.0.0                  # WiFi connectivity
- ArduinoOTA @ 2.0.0            # OTA updates
```

## ⚙️ Configuration

### 1. Credentials Setup

Create a `src/cred.h` file with your credentials:

```cpp
#define SS_ID "your-wifi-ssid"
#define auth "your-wifi-password"
#define HTTP_USER "admin"
#define HTTP_PASS "your-web-password"
#define MQTT_USER "mqtt-username"  // Optional
#define MQTT_PASS "mqtt-password"  // Optional
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

All entities appear automatically under the "PoolMonitor" device in Home Assistant.

### 📡 MQTT Topics

**Published topics:**
- `pool/monitor/temperature` - Temperature in °C
- `pool/monitor/rssi` - WiFi signal strength (dBm) 
- `pool/monitor/pump_speed` - Current speed (Low/Med/High/Stop)
- `pool/monitor/status` - System status (1=OK, 0=Error)
- `pool/monitor/ip` - Device IP address

**Home Assistant Discovery:**
- `homeassistant/sensor/poolmonitor_*` - Auto-discovery configs

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
├── WiFiManager.*         # WiFi connection management
├── MQTTManager.*         # MQTT publishing & HA discovery
├── OTAManager.*          # OTA updates with protection
├── PumpController.*      # Pump speed control logic
├── ScheduleManager.*     # Automatic scheduling
├── TemperatureSensor.*   # DS18B20 temperature reading
├── TimeManager.*         # NTP time synchronization  
├── WebServerManager.*    # HTTP server & routes
└── InputManager.*        # Button input & LED status
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

### v2.0.0 (Current) - Architecture Refactor & UI Overhaul
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
