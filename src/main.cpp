#include <Arduino.h>

#include <WiFi.h>
#include "time.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "html.h"
#include "Preferences.h"
#include <esp_task_wdt.h>
#include "cred.h"
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "PumpController.h"
#include "TemperatureSensor.h"
#include "ScheduleManager.h"
#include "OTAManager.h"
#include "WebServerManager.h"
#include "TimeManager.h"
#include "InputManager.h"

#define WDT_TIMEOUT 90

// MQTT Configuration
#define MQTT_HOST "192.168.0.54"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "PoolMonitor_Test"
#define MQTT_TOPIC "pool/monitor/test"

// GPIO Pin Definitions
#define GPIO_HIGH_SPEED 27
#define GPIO_LOW_SPEED 26
#define GPIO_STOP 25
#define GPIO_MED_SPEED 14
#define GPIO_BUTTON 13
#define GPIO_ONE_WIRE 4

// Timing Constants
#define TEMP_READ_INTERVAL 10000     // Temperature reading interval (ms)
#define TEMP_CALIBRATION_OFFSET 1.5  // Temperature calibration offset
#define PUMP_CHECK_INTERVAL 500      // Pump schedule check interval (ms)
#define LED_BLINK_INTERVAL 500       // LED blink interval (ms)
#define DEBOUNCE_DELAY 50            // Button debounce delay (ms)

// Manager instances
WiFiManager wifiManager;
MQTTManager mqttManager;
PumpController pumpController;
TemperatureSensor temperatureSensor;
ScheduleManager scheduleManager;
OTAManager otaManager;
WebServerManager webServerManager(pumpController, mqttManager, scheduleManager, temperatureSensor);
TimeManager timeManager;
InputManager inputManager;

const char *ssid = SS_ID;
const char *password = auth;
const char *http_username = HTTP_USER;
const char *http_password = HTTP_PASS;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

signed int rssi = 0;
unsigned long lastDallasRead = millis();

//******************************************************************************
// Publish all sensor data to MQTT
void publishStatusToMQTT()
{
  // Publish temperature
  char tempPayload[32];
  snprintf(tempPayload, sizeof(tempPayload), "%.2f", temperatureSensor.getTemperature());
  mqttManager.publishToSubtopic("temperature", tempPayload);

  // Publish RSSI (WiFi signal strength)
  char rssiPayload[16];
  snprintf(rssiPayload, sizeof(rssiPayload), "%d", rssi);
  mqttManager.publishToSubtopic("rssi", rssiPayload);

  // Publish current pump speed
  char speedPayload[16];
  snprintf(speedPayload, sizeof(speedPayload), "%s", pumpController.getSpeedString());
  mqttManager.publishToSubtopic("pump_speed", speedPayload);

  // Publish pool relax status (1 = ok, 0 = error)
  char statusPayload[8];
  snprintf(statusPayload, sizeof(statusPayload), "%d", inputManager.getRelaxStatus());
  mqttManager.publishToSubtopic("status", statusPayload);

  // Publish IP address
  IPAddress ip = wifiManager.getLocalIP();
  char ipPayload[16];
  snprintf(ipPayload, sizeof(ipPayload), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  mqttManager.publishToSubtopic("ip", ipPayload);
}
//*****************************************************************************/

//******************************************************************************
// Arduino main setup function
void setup()
{
  // Init watchdog handler
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch

  // Init the serial port for debugging
  Serial.begin(115200);
  delay(500);

  // Initialize schedule manager and load from NVM
  scheduleManager.begin();

  // Initialize temperature sensor
  temperatureSensor.begin(GPIO_ONE_WIRE, TEMP_CALIBRATION_OFFSET);

  // Initialize WiFi Manager
  Preferences preferences;
  wifiManager.begin(ssid, password, &preferences);

  // Initialize MQTT Manager
  mqttManager.begin(MQTT_HOST, MQTT_PORT, MQTT_CLIENT_ID, MQTT_TOPIC, MQTT_USER, MQTT_PASS);

  // Wait for WiFi connection
  delay(3000);

  // Initialize time manager with NTP
  timeManager.begin(ntpServer, gmtOffset_sec, daylightOffset_sec);
  timeManager.update();

  // Initialize input manager (button and LED)
  inputManager.begin(GPIO_BUTTON, LED_BUILTIN, DEBOUNCE_DELAY, LED_BLINK_INTERVAL);

  // Setup web server
  webServerManager.begin(http_username, http_password);
  webServerManager.setManagerReferences(timeManager, inputManager, &rssi);

  // Initialize OTA Manager
  otaManager.begin();

  // Initialize pump controller
  pumpController.begin(GPIO_HIGH_SPEED, GPIO_LOW_SPEED, GPIO_MED_SPEED, GPIO_STOP);

  Serial.println("Setup complete!");
}
//******************************************************************************

unsigned long lastLoopDelay = 0;

//******************************************************************************
// Main loop
void loop()
{
  // Check the OTA handler (MUST be called frequently - FIRST priority)
  otaManager.handle();

  // Handle MQTT connection (non-blocking with timeout interval)
  if (wifiManager.isConnected())
  {
    mqttManager.handle();
  }

  // Update time from NTP
  timeManager.update();

  // Update button and LED status
  inputManager.update();

  // Get the wifi rssi, this is used to kick the HW watchdog
  rssi = wifiManager.getRSSI();

  // Check if rssi reading is ok, if not then do not kick the HW watchdog
  if (rssi != 0)
    esp_task_wdt_reset();

  if((millis() - lastDallasRead) > TEMP_READ_INTERVAL){
     lastDallasRead = millis();
     
     // Read temperature from sensor
     temperatureSensor.readTemperature();

     // Publish all status data to MQTT
     publishStatusToMQTT();
   }

  // Check and execute pump schedule (every 500ms)
  if ((millis() - lastLoopDelay) >= PUMP_CHECK_INTERVAL)
  {
    lastLoopDelay = millis();
    scheduleManager.checkAndExecute(timeManager.getHour(), pumpController, mqttManager);
  }
}