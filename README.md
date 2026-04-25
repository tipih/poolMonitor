# Pool Monitor

ESP32-based pool pump monitoring and control system with web interface, temperature sensing, and MQTT integration.

## Features

- **Remote Pool Pump Control**: Control pump speed (Low/Med/High/Stop) via web interface or MQTT
- **Temperature Monitoring**: Monitor pool water temperature using Dallas DS18B20 sensor
- **Web Interface**: Responsive web dashboard with authentication for remote access
- **MQTT Integration**: Publish temperature and pump status data to MQTT broker
- **Scheduled Operation**: Configure automatic on/off times for the pump
- **OTA Updates**: Over-the-air firmware updates for easy maintenance
- **WiFi Connectivity**: Connect to your home network for remote monitoring
- **Persistent Settings**: Stores pump schedule in non-volatile memory

## Hardware Requirements

- **ESP32 Development Board** (ESP32 DOIT DevKit V1 or compatible)
- **Dallas DS18B20** temperature sensor (one-wire)
- **Relay Module** or transistor circuit to interface with pool pump control buttons
- 4.7kΩ resistor (pull-up for DS18B20)

### Pin Configuration

- **GPIO 4**: DS18B20 temperature sensor (one-wire data)
- **GPIO 14**: Medium speed control
- **GPIO 25**: Stop control
- **GPIO 26**: Low speed control
- **GPIO 27**: High speed control

## Software Requirements

Built with PlatformIO using the Arduino framework.

### Dependencies

- [PubSubClient](https://github.com/knolleary/pubsubclient) - MQTT client library
- [AsyncTCP-esphome](https://github.com/esphome/AsyncTCP) - Asynchronous TCP library
- [ESPAsyncWebServer-esphome](https://github.com/ottowinter/ESPAsyncWebServer-esphome) - Async web server
- [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) - DS18B20 sensor library

## Configuration

### Credentials Setup

Create a `src/cred.h` file with your WiFi, web authentication, and MQTT credentials:

```cpp
#define SS_ID "your-wifi-ssid"
#define auth "your-wifi-password"
#define user "admin"
#define pass "your-web-password"
```

**Note**: `cred.h` is excluded from version control via `.gitignore` to protect your credentials.

### MQTT Configuration

Edit the MQTT settings in `src/main.cpp`:

```cpp
#define MQTT_HOST "192.168.0.54"  // Your MQTT broker IP
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "PoolMonitor"
#define MQTT_TOPIC "pool/monitor"
```

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/tipih/poolMonitor.git
   cd poolMonitor
   ```

2. Create your `src/cred.h` file with WiFi and authentication credentials

3. Update MQTT broker settings in `src/main.cpp` if needed

4. Build and upload using PlatformIO:
   ```bash
   platformio run --target upload
   ```

5. Monitor serial output:
   ```bash
   platformio device monitor
   ```

## Usage

### Web Interface

1. Connect your ESP32 to power
2. Find the device IP address from serial monitor or router DHCP table
3. Open a web browser and navigate to `http://<ESP32-IP-ADDRESS>`
4. Login with credentials from `cred.h`
5. Control pump speed and view temperature from the dashboard

### MQTT Topics

The device publishes to the following topics:

- `pool/monitor/temperature` - Current pool temperature in Celsius
- `pool/monitor/pump_speed` - Current pump speed (Low/Med/High/Stop)
- `pool/monitor/status` - Device status updates

### OTA Updates

Once connected to WiFi, the device supports Over-The-Air updates:

1. Update code as needed
2. Upload via OTA: `platformio run --target upload --upload-port <ESP32-IP-ADDRESS>`

## Default Schedule

- **On Time**: 6:00 AM
- **Off Time**: 11:00 PM

Schedule can be modified via the web interface and will be saved to non-volatile memory.

## Watchdog Timer

The system includes a 90-second watchdog timer to automatically recover from hangs or crashes.

## License

This project is open source and available for personal and educational use.

## Version

Current version: **v1.0.0** - Initial working pool monitor
