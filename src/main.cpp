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
#define WDT_TIMEOUT 30

const char *ssid = SS_ID;
const char *password = auth;
const char *http_username = user;
const char *http_password = pass;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
const long timeoutTime = 2000;
const long ledSpeed = 500;

unsigned long currentSec = 1;
unsigned long currentDay = 1;
unsigned long currentMd = 1;
unsigned long currentYr = 1;
unsigned long currentHour = 1;
unsigned long currentMinute = 1;
unsigned long currentSec = 1;
unsigned long currentDay = 1;
unsigned long currentMd = 1;
unsigned long currentYr = 1;


unsigned long currentTime = millis();
unsigned long previousTime = 0;
unsigned long onHour = 6;
unsigned long offHour = 17;
unsigned long resetcounter = 0;
signed int rssi = 0;
int currentRelaxStatus = 0;

const char *input_parameter = "state";
float currentTemp = -999;
const int output = 14;

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);
unsigned long lastDallasRead = millis();

Preferences preferences;

typedef enum
{
  lowSpeed,
  highSpeed,
  noSpeed,
  medSpeed,
  stop
} pumpSpeed;
pumpSpeed currentSpeed = noSpeed;

// Variables will change:
int LED_state = LOW;
int button_state;
int lastbutton_state = LOW;

const int Push_button_GPIO = 13;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

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
// Function to set Low speed for the pump, will simulatate button press
void goLowSpeed()
{
  digitalWrite(27, HIGH); // HignSpeed button
  delay(100);
  digitalWrite(14, HIGH); // MedSpeed button
  delay(100);
  digitalWrite(25, HIGH); // Stop button
  delay(100);
  digitalWrite(26, LOW);  //Low Speed button
  delay(500);
  digitalWrite(26, HIGH);  //Low speed button
  delay(500);
  currentSpeed = lowSpeed;
  // digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
}
//******************************************************************************


//******************************************************************************
// Function to set Hihg speed for the pump, will simulatate button press
void goHighSpeed()
{
  digitalWrite(26, HIGH); // HighSpeed Button 
  delay(100);
  digitalWrite(14, HIGH); // MedSpeed button
  delay(100);
  digitalWrite(25, HIGH); // Stop button
  delay(100);
  digitalWrite(27, LOW);  //High Speed button
  delay(500);
  digitalWrite(27, HIGH);  //HighSpeed button
  delay(500);
  currentSpeed = highSpeed;
  // digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
}
//******************************************************************************

//******************************************************************************
// Function to set Med speed for the pump, will simulatate button press
void goMedSpeed()
{
  digitalWrite(26, HIGH); // Lowspeed button
  delay(100);
  digitalWrite(25, HIGH); // Stop button
  delay(100);
  digitalWrite(27, HIGH); // Highspeed button
  delay(100);
  digitalWrite(14, LOW);  // Medspeed button
  delay(500);
  digitalWrite(14, HIGH);  //Medspeed button
  delay(500);
  currentSpeed = medSpeed;
  // digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
}
//******************************************************************************

//******************************************************************************
// Function to set Stop for the pump, will simulatate button press
void goStop()
{
  digitalWrite(26, HIGH); // Lowspeed button
  delay(100);
  digitalWrite(14, HIGH); // Medspeed button
  delay(100);
  digitalWrite(27, HIGH); // Highspeed button
  delay(100);
  digitalWrite(25, LOW);  //Stop Button
  delay(500);
  digitalWrite(25, HIGH);  //Stop Button
  delay(500);
  currentSpeed = stop;
  // digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
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
  // Handler for the update request, check og auth is ok
  // if ok get the input params, and check the values
  // change the pumpspeed according the the button pressed
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
                  goLowSpeed();
                }
                else if ((p->value() == "HighOff"))
                {
                  goHighSpeed();
                }
                else if ((p->value() == "MedOff"))
                {
                  goMedSpeed();
                }
                else if ((p->value() == "StopOff"))
                {
                  goStop();
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
  // Check if client is auth, if ok the send values to client in a json format
  server.on("/state", HTTP_GET, [](AsyncWebServerRequest *request)
            {
      Serial.println("Got an update reuest");
      if (!request->authenticate(http_username, http_password))
       return request->requestAuthentication();
      char buffer[200];
      //Format a json string, with all the data to be updated in the client
      sprintf(buffer, "{\"poolRelaxStatus\":\"%d\",\"pumpSpeed\":\"%d\",\"onTime\":\"%d\",\"offTime\":\"%d\",\"rssi\":\"%d\",\"hh\":\"%02d\",\"mm\":\"%02d\",\"ss\":\"%02d\",\"dd\":\"%02d\",\"md\":\"%02d\",\"yy\":\"%02d\",\"currentTemp\":\"%.2f\"}",currentRelaxStatus, currentSpeed, onHour, offHour, rssi,currentHour,currentMinute,currentSec,currentDay,currentMd,currentYr,currentTemp);
      
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
// Function to get the time from the network, this is called each 30 sec
void printLocalTime()
{
  struct tm timeinfo;

  if (millis() - previousTime > 1000)
  {
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }
    previousTime = millis();
    
    
    currentHour = timeinfo.tm_hour;
    currentMinute = timeinfo.tm_min;
    currentSec =timeinfo.tm_sec;
    currentDay =timeinfo.tm_mday;
    currentMd=timeinfo.tm_mon;
    currentYr=timeinfo.tm_year+1900;

    Serial.println(currentHour);
    Serial.println(currentMinute);
    Serial.println(currentYr);
    Serial.println(currentHour);
    Serial.println(currentMinute);
    Serial.println(currentYr);
    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }


}
//******************************************************************************

//******************************************************************************
// Handler for wifi connection
void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Connected to AP successfully!");
}
//******************************************************************************

//******************************************************************************
// Handler for wifi got an IP adress, when IP is ok, get the network time, and
// setup the server
// When server is running also start the OTA, and setup the OTA handlers

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  setupServer();
  server.begin();
  Serial.println("Server started");

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

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
//******************************************************************************

//******************************************************************************
// Handler for wifi disconnect
// If this happen, store a reset counter and reset the ESP
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
  Serial.println("Disconnected from WiFi access point");
  Serial.print("WiFi lost connection. Reason: ");
  Serial.println(info.wifi_sta_disconnected.reason);
  Serial.println("Trying to Reconnect");
  preferences.begin("reset", false);
  resetcounter++;
  preferences.putLong("resetCounter", resetcounter);
  preferences.end();
  // WiFi.begin(ssid, password);
  // Let reset the ESP, to get to a known state
  delay(2000);
  ESP.restart();
}
//******************************************************************************

//******************************************************************************
// Arduino main setup function
void setup()
{
  // Init the NVM setup
  preferences.begin("reset", false);
  resetcounter = preferences.getLong("resetCounter", resetcounter);
  preferences.end();

  // Init watchdog handler, timeout is set to 45 sec
  esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
  esp_task_wdt_add(NULL);               // add current thread to WDT watch

  // Init the serial port for debugging
  Serial.begin(115200);
  delay(500);

  // Print out the reset counter
  Serial.print("Reset cnt=");
  Serial.println(resetcounter);
  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // Get the data from NVM
  getPreference();
  // Start the WIFI
  WiFi.begin(ssid, password);

  // Define the wifi callback functions
  WiFi.onEvent(WiFiStationConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
  WiFi.onEvent(WiFiGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
  Serial.println();
  Serial.println();
  Serial.println("Wait for WiFi... ");
  delay(3000);

  // init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
  // Setup digital pins
  pinMode(27, OUTPUT);  //HighSpeed button
  pinMode(26, OUTPUT);  //LowSpeed button
  pinMode(25, OUTPUT);  //MedSpeed button
  pinMode(14, OUTPUT);  //Stop button
  pinMode(Push_button_GPIO, INPUT_PULLUP);
  digitalWrite(26, HIGH); // LowSpeed Button
  digitalWrite(27, HIGH); // HighSpeed Button
  digitalWrite(25, HIGH); // MedSpeed Button
  digitalWrite(14, HIGH); // Stop Button
  pinMode(LED_BUILTIN, OUTPUT);
}
//******************************************************************************



int value = 0;

//******************************************************************************
// Main loop
void loop()
{
  // Check the OTA handler
  ArduinoOTA.handle();
  // Get the time
  printLocalTime();

  // Read in status from poolrelax
  int data = digitalRead(Push_button_GPIO);

  if (data != lastbutton_state)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    // Get the wifi rssi, this is used to kick the HW watchdog
    rssi = WiFi.RSSI();

    // Check if rssi reading is ok, if not the do not kick the HW watchdog
    if (rssi != 0)
      esp_task_wdt_reset();




    LED_state = !LED_state;
    digitalWrite(LED_BUILTIN, LED_state); // turn the LED off by making the voltage LOW

    if (data != button_state)
    {
      button_state = data;
      currentRelaxStatus = data;

      if (button_state == HIGH)
      {
        LED_state = !LED_state;
      }
    }
  }
   if((millis() - lastDallasRead) > 10000){
     digitalWrite(LED_BUILTIN,HIGH);
         lastDallasRead = millis();
     currentTemp = getTemperatur(); 
     currentTemp +=1.5;
     digitalWrite(LED_BUILTIN, LOW);
   }

  // digitalWrite(LED_BUILTIN, LED_state);

  lastbutton_state = data;

  // Check if it is time to turn on or off the pump speed
  if ((currentHour == onHour) && (currentSpeed != medSpeed))
  {
    goMedSpeed();
  }
  if ((currentHour == offHour) && (currentSpeed != lowSpeed))
  {
    goLowSpeed();
  }

  delay(500);
}