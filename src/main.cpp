#include <Arduino.h>

#include <WiFi.h>
#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
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

const char *ssid = SS_ID;
const char *password = auth;
const char *http_username = HTTP_USER;
const char *http_password = HTTP_PASS;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

unsigned long currentSec = 1;
unsigned long currentDay = 1;
unsigned long currentMd = 1;
unsigned long currentYr = 1;
unsigned long currentHour = 1;
unsigned long currentMinute = 1;

unsigned long previousTime = 0;
signed int rssi = 0;
int currentRelaxStatus = 0;

unsigned long lastDallasRead = millis();

// Variables will change:
int LED_state = LOW;
int button_state;
int lastbutton_state = LOW;

unsigned long lastDebounceTime = 0;
unsigned long lastLEDToggle = 0;

AsyncWebServer server(80);

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
  snprintf(statusPayload, sizeof(statusPayload), "%d", currentRelaxStatus);
  mqttManager.publishToSubtopic("status", statusPayload);

  // Publish IP address
  IPAddress ip = wifiManager.getLocalIP();
  char ipPayload[16];
  snprintf(ipPayload, sizeof(ipPayload), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  mqttManager.publishToSubtopic("ip", ipPayload);
}
//******************************************************************************

//******************************************************************************
// Web server handlers
//******************************************************************************

// Handler for root page - show main interface
void handleRoot(AsyncWebServerRequest *request)
{
  if (!request->authenticate(http_username, http_password))
    return request->requestAuthentication();
  request->send_P(200, "text/html", index_html);
}

// Handler for logout
void handleLogout(AsyncWebServerRequest *request)
{
  request->send(401);
}

// Handler for logged out page
void handleLoggedOut(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", logout_html);
}

// Handler for pump control updates
void handleUpdate(AsyncWebServerRequest *request)
{
  Serial.println("Got an update");
  if (!request->authenticate(http_username, http_password))
    return request->requestAuthentication();

  if (request->params() == 1)
  {
    AsyncWebParameter *p = request->getParam(0);
    String value = p->value();

    Serial.print("Button: ");
    Serial.println(value);

    if (value == "LowOff")
    {
      pumpController.setLowSpeed();
      mqttManager.publishToSubtopic("pump_speed", "Low");
    }
    else if (value == "HighOff")
    {
      pumpController.setHighSpeed();
      mqttManager.publishToSubtopic("pump_speed", "High");
    }
    else if (value == "MedOff")
    {
      pumpController.setMedSpeed();
      mqttManager.publishToSubtopic("pump_speed", "Medium");
    }
    else if (value == "StopOff")
    {
      pumpController.setStop();
      mqttManager.publishToSubtopic("pump_speed", "Stopped");
    }
  }

  request->send(200, "text/plain", "OK");
}

// Handler for time schedule adjustments
void handleTimeAdjust(AsyncWebServerRequest *request)
{
  Serial.println("Got a time update");
  if (!request->authenticate(http_username, http_password))
    return request->requestAuthentication();

  if (request->params() > 1)
  {
    unsigned long onTime = request->getParam(0)->value().toInt();
    unsigned long offTime = request->getParam(1)->value().toInt();

    Serial.print("New schedule: ON=");
    Serial.print(onTime);
    Serial.print(", OFF=");
    Serial.println(offTime);

    scheduleManager.setSchedule(onTime, offTime);
  }

  request->send(200, "text/plain", "OK");
}

// Handler for state polling
void handleState(AsyncWebServerRequest *request)
{
  Serial.println("Got a state request");
  if (!request->authenticate(http_username, http_password))
    return request->requestAuthentication();

  char buffer[200];
  sprintf(buffer, "{\"poolRelaxStatus\":\"%d\",\"pumpSpeed\":\"%d\",\"onTime\":\"%lu\",\"offTime\":\"%lu\",\"rssi\":\"%d\",\"hh\":\"%02lu\",\"mm\":\"%02lu\",\"ss\":\"%02lu\",\"dd\":\"%02lu\",\"md\":\"%02lu\",\"yy\":\"%02lu\",\"currentTemp\":\"%.2f\"}",
          currentRelaxStatus,
          pumpController.getCurrentSpeed(),
          scheduleManager.getOnHour(),
          scheduleManager.getOffHour(),
          rssi,
          currentHour,
          currentMinute,
          currentSec,
          currentDay,
          currentMd,
          currentYr,
          temperatureSensor.getTemperature());

  request->send(200, "application/json", buffer);
}

//******************************************************************************
// Setup the async server handler functions, and then start the server
void setupServer()
{
  server.on("/", HTTP_GET, handleRoot);
  server.on("/logout", HTTP_GET, handleLogout);
  server.on("/logged-out", HTTP_GET, handleLoggedOut);
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/timeAdjust", HTTP_GET, handleTimeAdjust);
  server.on("/state", HTTP_GET, handleState);

  server.begin();
}
//******************************************************************************

//******************************************************************************
// Function to get the time from the network, this is called periodically
void printLocalTime()
{
  struct tm timeinfo;

  if (millis() - previousTime > 5000)  // Update every 5 seconds instead of 1 second
  {
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }
    previousTime = millis();
    
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSec = timeinfo.tm_sec;
    currentDay = timeinfo.tm_mday;
    currentMd = timeinfo.tm_mon;
    currentYr = timeinfo.tm_year + 1900;

    // Disabled verbose time logging to reduce serial output and improve OTA responsiveness
    // Serial.printf("Time: %02d:%02d:%02d\n", currentHour, currentMinute, currentSec);
  }
}
//******************************************************************************

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

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();

  // Setup web server
  setupServer();
  server.begin();
  Serial.println("Web server started");

  // Initialize OTA Manager
  otaManager.begin();

  // Initialize pump controller
  pumpController.begin(GPIO_HIGH_SPEED, GPIO_LOW_SPEED, GPIO_MED_SPEED, GPIO_STOP);

  // Setup digital pins
  pinMode(GPIO_BUTTON, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

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

  // Get the time
  printLocalTime();

  // Read in status from pool relax
  int data = digitalRead(GPIO_BUTTON);
  if (data != lastbutton_state)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY)
  {
    // Get the wifi rssi, this is used to kick the HW watchdog
    rssi = wifiManager.getRSSI();

    // Check if rssi reading is ok, if not the do not kick the HW watchdog
    if (rssi != 0)
      esp_task_wdt_reset();

    if (data != button_state)
    {
      button_state = data;
      currentRelaxStatus = data;

      if (button_state == HIGH)
      {
        LED_state = !LED_state;
      }
    }

    lastDebounceTime = millis();
  }

  // Visible LED blink every 500ms (independent of button)
  if ((millis() - lastLEDToggle) >= LED_BLINK_INTERVAL)
  {
    lastLEDToggle = millis();
    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state);
  }

  if((millis() - lastDallasRead) > TEMP_READ_INTERVAL){
     digitalWrite(LED_BUILTIN, HIGH);
     lastDallasRead = millis();
     
     // Read temperature from sensor
     temperatureSensor.readTemperature();

     // Publish all status data to MQTT
     publishStatusToMQTT();

     digitalWrite(LED_BUILTIN, LOW);
   }

  lastbutton_state = data;

  // Check if it is time to turn on or off the pump speed (every 500ms)
  if ((millis() - lastLoopDelay) >= PUMP_CHECK_INTERVAL)
  {
    lastLoopDelay = millis();

    // Check if it is time to turn on or off the pump speed
    if ((currentHour == scheduleManager.getOnHour()) && (pumpController.getCurrentSpeed() != PumpController::MED_SPEED))
    {
      pumpController.setMedSpeed();
      mqttManager.publishToSubtopic("pump_speed", "Medium");
    }
    if ((currentHour == scheduleManager.getOffHour()) && (pumpController.getCurrentSpeed() != PumpController::LOW_SPEED))
    {
      pumpController.setLowSpeed();
      mqttManager.publishToSubtopic("pump_speed", "Low");
    }
  }
}