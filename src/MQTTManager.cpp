#include "MQTTManager.h"
#include <Arduino.h>

MQTTManager::MQTTManager() : _mqttClient(_espClient)
{
  _host = nullptr;
  _port = 1883;
  _clientId = nullptr;
  _baseTopic = nullptr;
  _user = nullptr;
  _password = nullptr;
  _lastReconnectAttempt = 0;
  _discoveryPublished = false;
}

MQTTManager::~MQTTManager()
{
  _mqttClient.disconnect();
}

void MQTTManager::begin(const char *host, int port, const char *clientId, const char *baseTopic,
                        const char *user, const char *password)
{
  _host = host;
  _port = port;
  _clientId = clientId;
  _baseTopic = baseTopic;
  _user = user;
  _password = password;

  // Configure MQTT client
  _mqttClient.setServer(_host, _port);
  _mqttClient.setBufferSize(512); // Match MQTT_MAX_PACKET_SIZE
  _mqttClient.setKeepAlive(60);   // Increase keepalive to 60 seconds for stability

  Serial.print("MQTT configured for broker: ");
  Serial.print(_host);
  Serial.print(":");
  Serial.println(_port);
}

bool MQTTManager::isConnected()
{
  return _mqttClient.connected();
}

void MQTTManager::handle()
{
  if (_mqttClient.connected())
  {
    _mqttClient.loop();
  }
  else
  {
    reconnect();
  }
}

void MQTTManager::reconnect()
{
  // Only attempt reconnection if enough time has passed
  unsigned long currentTime = millis();

  if ((currentTime - _lastReconnectAttempt) >= RECONNECT_INTERVAL)
  {
    _lastReconnectAttempt = currentTime;
    Serial.print("Attempting MQTT connection...");

    // Single non-blocking connection attempt
    bool connected = false;
    if (_user && _password)
    {
      connected = _mqttClient.connect(_clientId, _user, _password);
    }
    else
    {
      connected = _mqttClient.connect(_clientId);
    }

    if (connected)
    {
      Serial.println("connected");

      // Subscribe to command topic
      char topic[80];
      snprintf(topic, sizeof(topic), "%s/command", _baseTopic);
      _mqttClient.subscribe(topic);

      // Publish Home Assistant discovery configs only once
      if (!_discoveryPublished)
      {
        publishHADiscovery();
        _discoveryPublished = true;
      }
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(_mqttClient.state());
      Serial.println(" - will retry in 10 seconds");
    }
  }
}

bool MQTTManager::publish(const char *topic, const char *payload, bool retain)
{
  if (_mqttClient.connected())
  {
    if (_mqttClient.publish(topic, payload, retain))
    {
      Serial.print("Published to ");
      Serial.print(topic);
      Serial.print(": ");
      Serial.println(payload);
      return true;
    }
  }
  else
  {
    Serial.println("MQTT not connected, cannot publish");
  }
  return false;
}

bool MQTTManager::publishToSubtopic(const char *subtopic, const char *payload, bool retain)
{
  char topic[80];
  snprintf(topic, sizeof(topic), "%s/%s", _baseTopic, subtopic);
  return publish(topic, payload, retain);
}

void MQTTManager::publishHADiscovery()
{
  if (!_mqttClient.connected())
    return;

  const char *haId = "pool_monitor";

  // Device object for Home Assistant
  char deviceJson[256];
  snprintf(deviceJson, sizeof(deviceJson),
           "{\"identifiers\":[\"%s\",\"%s\"],\"name\":\"Pool Monitor\"}",
           haId, _clientId);

  char topic[160];
  char payload[512];

  // Temperature Sensor
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_temperature/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Pool Temperature\",\"state_topic\":\"%s/temperature\","
           "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
           "\"value_template\":\"{{ value }}\",\"unique_id\":\"%s_temperature\","
           "\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Pump Status (on/off) - Binary Sensor
  snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/%s_pump_status/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Pool Pump\",\"state_topic\":\"%s/pump_status\","
           "\"payload_on\":\"on\",\"payload_off\":\"off\",\"device_class\":\"running\","
           "\"unique_id\":\"%s_pump_status\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Current Pump Speed
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_pump_speed/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Pump Speed\",\"state_topic\":\"%s/pump_speed\","
           "\"value_template\":\"{{ value }}\",\"unique_id\":\"%s_pump_speed\","
           "\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // WiFi Signal Strength (RSSI)
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_rssi/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"WiFi Signal Strength\",\"state_topic\":\"%s/rssi\","
           "\"unit_of_measurement\":\"dBm\",\"device_class\":\"signal_strength\","
           "\"value_template\":\"{{ value }}\",\"unique_id\":\"%s_rssi\","
           "\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Pool Monitor Status (1=ok, 0=error)
  snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/%s_status/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Pool Monitor Status\",\"state_topic\":\"%s/status\","
           "\"payload_on\":\"1\",\"payload_off\":\"0\",\"device_class\":\"problem\","
           "\"value_template\":\"{% if value == '1' %}off{% else %}on{% endif %}\","
           "\"unique_id\":\"%s_status\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // IP Address
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_ip/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"IP Address\",\"state_topic\":\"%s/ip\","
           "\"value_template\":\"{{ value }}\",\"unique_id\":\"%s_ip\","
           "\"icon\":\"mdi:ip-network\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // -------- Heat pump (read-only) --------
  // All entities are sensors / binary_sensors only — no command topics are
  // advertised, so HA will not offer any controls for these.

  // Inlet water temperature
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_inlet_temp/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Inlet Temperature\",\"state_topic\":\"%s/heatpump/inlet_temp\","
           "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
           "\"state_class\":\"measurement\",\"value_template\":\"{{ value }}\","
           "\"unique_id\":\"%s_hp_inlet_temp\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Outlet water temperature
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_outlet_temp/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Outlet Temperature\",\"state_topic\":\"%s/heatpump/outlet_temp\","
           "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
           "\"state_class\":\"measurement\",\"value_template\":\"{{ value }}\","
           "\"unique_id\":\"%s_hp_outlet_temp\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Ambient temperature
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_ambient_temp/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Ambient Temperature\",\"state_topic\":\"%s/heatpump/ambient_temp\","
           "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
           "\"state_class\":\"measurement\",\"value_template\":\"{{ value }}\","
           "\"unique_id\":\"%s_hp_ambient_temp\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Target water temperature (read-only sensor, NOT a number entity)
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_target_temp/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Target Temperature\",\"state_topic\":\"%s/heatpump/target_temp\","
           "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
           "\"state_class\":\"measurement\",\"value_template\":\"{{ value }}\","
           "\"unique_id\":\"%s_hp_target_temp\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Power state (binary_sensor, read-only — not a switch)
  snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/%s_hp_power/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Power\",\"state_topic\":\"%s/heatpump/power\","
           "\"payload_on\":\"ON\",\"payload_off\":\"OFF\",\"device_class\":\"power\","
           "\"unique_id\":\"%s_hp_power\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Operation mode (read-only sensor; map enum -> label in HA)
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_mode/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Mode\",\"state_topic\":\"%s/heatpump/mode\","
           "\"value_template\":\"{%% set m={'0':'Auto','1':'Heat','2':'Cool'} %%}"
           "{{ m.get(value, value) }}\","
           "\"unique_id\":\"%s_hp_mode\",\"icon\":\"mdi:thermostat\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Silence / working mode (read-only sensor)
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_silence/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Silence Mode\",\"state_topic\":\"%s/heatpump/silence\","
           "\"value_template\":\"{%% set m={'0':'Smart','1':'Silence','2':'Super Silence'} %%}"
           "{{ m.get(value, value) }}\","
           "\"unique_id\":\"%s_hp_silence\",\"icon\":\"mdi:volume-low\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Error code (numeric sensor; E3 = no flow, etc.)
  snprintf(topic, sizeof(topic), "homeassistant/sensor/%s_hp_error/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Error Code\",\"state_topic\":\"%s/heatpump/error\","
           "\"value_template\":\"{{ value }}\",\"unique_id\":\"%s_hp_error\","
           "\"icon\":\"mdi:alert-circle-outline\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  // Modbus link health (binary_sensor, problem class — on when offline)
  snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/%s_hp_online/config", haId);
  snprintf(payload, sizeof(payload),
           "{\"name\":\"Heat Pump Modbus\",\"state_topic\":\"%s/heatpump/online\","
           "\"payload_on\":\"0\",\"payload_off\":\"1\",\"device_class\":\"problem\","
           "\"unique_id\":\"%s_hp_online\",\"device\":%s}",
           _baseTopic, haId, deviceJson);
  _mqttClient.publish(topic, payload, true);

  Serial.println("Home Assistant discovery configs published");
}
