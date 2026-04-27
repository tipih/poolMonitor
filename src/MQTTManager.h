#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>

/**
 * Manages the MQTT client lifecycle and Home Assistant integration:
 * non-blocking connect/reconnect with retry interval, topic subscription,
 * publishing helpers (raw topic and base-topic-relative subtopic), and
 * one-shot publication of Home Assistant MQTT discovery configs so the
 * device's sensors appear automatically in HA.
 *
 * handle() must be called frequently from loop() to service the
 * underlying PubSubClient and trigger reconnects when needed.
 */
class MQTTManager
{
public:
  MQTTManager();
  ~MQTTManager();

  // Initialize MQTT with broker settings
  void begin(const char *host, int port, const char *clientId, const char *baseTopic,
             const char *user = nullptr, const char *password = nullptr);

  // Handle MQTT connection and reconnection (call from main loop)
  void handle();

  // Check if MQTT is connected
  bool isConnected();

  // Publish a message to a topic
  bool publish(const char *topic, const char *payload, bool retain = false);

  // Publish to a subtopic under the base topic
  bool publishToSubtopic(const char *subtopic, const char *payload, bool retain = false);

  // Publish Home Assistant discovery configurations
  void publishHADiscovery();

  // Force republish of HA discovery on next connection
  void resetDiscovery() { _discoveryPublished = false; }

private:
  WiFiClient _espClient;
  PubSubClient _mqttClient;

  const char *_host;
  int _port;
  const char *_clientId;
  const char *_baseTopic;
  const char *_user;
  const char *_password;

  unsigned long _lastReconnectAttempt;
  bool _discoveryPublished;
  static const unsigned long RECONNECT_INTERVAL = 10000; // 10 seconds

  // Internal reconnect handler (non-blocking)
  void reconnect();
};

#endif // MQTT_MANAGER_H
