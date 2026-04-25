#include <Arduino.h>

#include <WiFi.h>
#include "AsyncTCP.h"
#include <ESPAsyncWebServer.h>
#include "time.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "html.h"
#include "Preferences.h"
#include <esp_task_wdt.h>
#include "cred.h"
#include <DallasTemperature.h>
#include "WiFiManager.h"
#include "MQTTManager.h"
#include "PumpController.h"

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
unsigned long onHour = 6;
unsigned long offHour = 17;
signed int rssi = 0;
int currentRelaxStatus = 0;

float currentTemp = -999;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(GPIO_ONE_WIRE);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
unsigned long lastDallasRead = millis();

Preferences preferences;

// Variables will change:
int LED_state = LOW;
int button_state;
int lastbutton_state = LOW;

unsigned long lastDebounceTime = 0;
unsigned long lastLEDToggle = 0;

AsyncWebServer server(80);

//******************************************************************************
// Get data from NVM memory and update the VARS, if not data set default values
void getPreference()
{
  preferences.begin("poolPump", false);
  onHour = preferences.getLong("onHour", 6);
  offHour = preferences.getLong("offHour", 23);
  preferences.end();

  if ((onHour > 0) && (offHour < 24) && (onHour < 24) && (offHour > 0))
  {
    Serial.println("Got some preferences");
    Serial.println(onHour);
    Serial.print(offHour);
  }
  else
  {
    onHour = 6;
    offHour = 18;
  }
}
//******************************************************************************

//******************************************************************************
// Store data to NVM
void setPreference()
{
  preferences.begin("poolPump", false);
  preferences.putLong("onHour", onHour);
  preferences.putLong("offHour", offHour);
  preferences.end();
}
//******************************************************************************


//******************************************************************************
// Get Dallas temp data
float getTemperatur()
{
 sensors.requestTemperatures(); // Send the command to get temperatures
 float tempC = sensors.getTempCByIndex(0);
 if (tempC != DEVICE_DISCONNECTED_C)
  {
    Serial.print("Temperature for the device 1 (index 0) is: ");
    Serial.println(tempC);
    return tempC;
  }
  else
  {
    Serial.println("Error: Could not read temperature data");
    return -999;
  }
}
//******************************************************************************


//******************************************************************************
// Function to set on/off time for the pump, and store the new values to NVM
void setOnOffTime(unsigned long ontime, unsigned long offtime)
{

  onHour = ontime;   // Store global ontime
  offHour = offtime; // Store global offtime
  setPreference();   // Save on off for next time
}
//******************************************************************************




//******************************************************************************
// Setup the async server handler functions, and then start the server
void setupServer()
{

  // Main handler function check auth, if ok the send the index page, else show popup
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
          if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    request->send_P(200, "text/html", index_html); });
  // Handler to logoff request
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(401); });
  // Handler to for logged out page, send it
  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", logout_html); });
  // Handler for the update request, check if auth is ok
  // if ok get the input params, and check the values
  // change the pumpspeed according to the button pressed
  // and send ok back to client
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              Serial.println("Got an update");
              if (!request->authenticate(http_username, http_password))
                return request->requestAuthentication();
              String input_message = "";
              String inputParameter;
              int params = request->params();
              // GET input1 value on <ESP_IP>/update?state=<input_message>
              Serial.println(params);
              if (params == 1)
              {
                AsyncWebParameter *p = request->getParam(0);

                Serial.println(p->name());
                Serial.println(p->value());
                if (p->value() == "LowOff")
                {
                  pumpController.setLowSpeed();
                  mqttManager.publishToSubtopic("pump_speed", "Low");
                }
                else if ((p->value() == "HighOff"))
                {
                  pumpController.setHighSpeed();
                  mqttManager.publishToSubtopic("pump_speed", "High");
                }
                else if ((p->value() == "MedOff"))
                {
                  pumpController.setMedSpeed();
                  mqttManager.publishToSubtopic("pump_speed", "Medium");
                }
                else if ((p->value() == "StopOff"))
                {
                  pumpController.setStop();
                  mqttManager.publishToSubtopic("pump_speed", "Stopped");
                }
              
              }
              else
              {
                input_message = "No Data";
              }

              Serial.println(input_message);
              request->send(200, "text/plain", "OK"); });

  // Handler for change the on / off time from the client
  // Check if client is auth, if ok the get params
  server.on("/timeAdjust", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    Serial.println("Got an Time update");
        if(!request->authenticate(http_username, http_password))
      return request->requestAuthentication();
    String input_message="";
    String inputParameter;
    int params = request->params();
    // GET input1 value on <ESP_IP>/update?state=<input_message>
    Serial.println(params);
  if (params>1)
  {
  
   Serial.print(request->getParam(0)->name());
   Serial.println(request->getParam(0)->value());
   Serial.print(request->getParam(1)->name());
   Serial.println(request->getParam(1)->value());
  //Convert the params to integer
  unsigned long onTime = request->getParam(0)->value().toInt();
  unsigned long offTime = request->getParam(1)->value().toInt();
   //Check the values are ok and within the limit
   //if ok then call function to store new values
   if ((onTime >0) && (onTime<24) && (offTime>0) && (offTime<24) ){
    Serial.println("Value are ok");
    Serial.println(onTime);
    Serial.println(offTime);
    setOnOffTime(onTime,offTime);
   }
  }
  else
  {
  input_message="No Data";
  }

  //Send ok to client  
  Serial.println(input_message);
  request->send(200, "text/plain", "OK"); });

  // Handler to state update
  // Check if client is auth, if ok then send values to client in a json format
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      Serial.println("Got an update request");
      if (!request->authenticate(http_username, http_password))
       return request->requestAuthentication();
      char buffer[200];
      //Format a json string, with all the data to be updated in the client
      sprintf(buffer, "{\"poolRelaxStatus\":\"%d\",\"pumpSpeed\":\"%d\",\"onTime\":\"%d\",\"offTime\":\"%d\",\"rssi\":\"%d\",\"hh\":\"%02d\",\"mm\":\"%02d\",\"ss\":\"%02d\",\"dd\":\"%02d\",\"md\":\"%02d\",\"yy\":\"%02d\",\"currentTemp\":\"%.2f\"}",currentRelaxStatus, pumpController.getCurrentSpeed(), onHour, offHour, rssi,currentHour,currentMinute,currentSec,currentDay,currentMd,currentYr,currentTemp);
      
      Serial.print(buffer);
      //Send data to the client
      request->send(200, "application/json", buffer); 
       }
      );

  // Start the server
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
// Initialize OTA (Over-The-Air updates)
void setupOTA()
{
  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  // OTA handler function
  ArduinoOTA
      .onStart([]()
               {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type); })
      .onEnd([]()
             { Serial.println("\nEnd"); })
      .onProgress([](unsigned int progress, unsigned int total)
                  { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); })
      .onError([](ota_error_t error)
               {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
  // Now start the OTA listner.
  ArduinoOTA.begin();

  Serial.println("OTA Ready");
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

  // Get preferences from NVM
  getPreference();

  // Initialize WiFi Manager
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

  // Setup OTA
  setupOTA();

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
  ArduinoOTA.handle();

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
     digitalWrite(LED_BUILTIN,HIGH);
         lastDallasRead = millis();
     currentTemp = getTemperatur(); 
     currentTemp += TEMP_CALIBRATION_OFFSET;

     // Publish temperature to MQTT
     char tempPayload[32];
     snprintf(tempPayload, sizeof(tempPayload), "%.2f", currentTemp);
     mqttManager.publishToSubtopic("temperature", tempPayload);

     // Publish RSSI (WiFi signal strength)
     char rssiPayload[16];
     snprintf(rssiPayload, sizeof(rssiPayload), "%d", rssi);
     mqttManager.publishToSubtopic("rssi", rssiPayload);

     // Publish current pump speed
     char speedPayload[16];
     snprintf(speedPayload, sizeof(speedPayload), "%s", pumpController.getSpeedString());
     mqttManager.publishToSubtopic("current_speed", speedPayload);

     // Publish pool relax status (1 = ok, 0 = error)
     char statusPayload[8];
     snprintf(statusPayload, sizeof(statusPayload), "%d", currentRelaxStatus);
     mqttManager.publishToSubtopic("status", statusPayload);

     // Publish IP address
     IPAddress ip = wifiManager.getLocalIP();
     char ipPayload[16];
     snprintf(ipPayload, sizeof(ipPayload), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
     mqttManager.publishToSubtopic("ip", ipPayload);

     digitalWrite(LED_BUILTIN, LOW);
   }

  lastbutton_state = data;

  // Check if it is time to turn on or off the pump speed (every 500ms)
  if ((millis() - lastLoopDelay) >= PUMP_CHECK_INTERVAL)
  {
    lastLoopDelay = millis();

    // Check if it is time to turn on or off the pump speed
    if ((currentHour == onHour) && (pumpController.getCurrentSpeed() != PumpController::MED_SPEED))
    {
      pumpController.setMedSpeed();
      mqttManager.publishToSubtopic("pump_speed", "Medium");
    }
    if ((currentHour == offHour) && (pumpController.getCurrentSpeed() != PumpController::LOW_SPEED))
    {
      pumpController.setLowSpeed();
      mqttManager.publishToSubtopic("pump_speed", "Low");
    }
  }
}