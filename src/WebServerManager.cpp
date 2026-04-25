#include "WebServerManager.h"
#include "PumpController.h"
#include "MQTTManager.h"
#include "ScheduleManager.h"
#include "TemperatureSensor.h"
#include "html.h"

WebServerManager::WebServerManager(PumpController &pump, MQTTManager &mqtt, ScheduleManager &schedule, TemperatureSensor &temp)
    : _server(80), _pumpController(pump), _mqttManager(mqtt), _scheduleManager(schedule), _tempSensor(temp),
      _httpUsername(nullptr), _httpPassword(nullptr),
      _currentRelaxStatus(nullptr), _rssi(nullptr),
      _currentHour(nullptr), _currentMinute(nullptr), _currentSec(nullptr),
      _currentDay(nullptr), _currentMd(nullptr), _currentYr(nullptr)
{
}

void WebServerManager::begin(const char *username, const char *password)
{
  _httpUsername = username;
  _httpPassword = password;

  // Setup routes using lambdas to bind member functions
  _server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleRoot(request);
  });

  _server.on("/logout", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleLogout(request);
  });

  _server.on("/logged-out", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleLoggedOut(request);
  });

  _server.on("/update", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleUpdate(request);
  });

  _server.on("/timeAdjust", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleTimeAdjust(request);
  });

  _server.on("/state", HTTP_GET, [this](AsyncWebServerRequest *request) {
    this->handleState(request);
  });

  _server.begin();
  Serial.println("Web server started on port 80");
}

void WebServerManager::setReferences(int *relax, int *rssi, unsigned long *hour, unsigned long *minute, unsigned long *sec,
                                      unsigned long *day, unsigned long *md, unsigned long *yr)
{
  _currentRelaxStatus = relax;
  _rssi = rssi;
  _currentHour = hour;
  _currentMinute = minute;
  _currentSec = sec;
  _currentDay = day;
  _currentMd = md;
  _currentYr = yr;
}

// Handler for root page - show main interface
void WebServerManager::handleRoot(AsyncWebServerRequest *request)
{
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();
  request->send_P(200, "text/html", index_html);
}

// Handler for logout
void WebServerManager::handleLogout(AsyncWebServerRequest *request)
{
  request->send(401);
}

// Handler for logged out page
void WebServerManager::handleLoggedOut(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", logout_html);
}

// Handler for pump control updates
void WebServerManager::handleUpdate(AsyncWebServerRequest *request)
{
  Serial.println("Got an update");
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();

  if (request->params() == 1)
  {
    AsyncWebParameter *p = request->getParam(0);
    String value = p->value();

    Serial.print("Button: ");
    Serial.println(value);

    if (value == "LowOff")
    {
      _pumpController.setLowSpeed();
      _mqttManager.publishToSubtopic("pump_speed", "Low");
    }
    else if (value == "HighOff")
    {
      _pumpController.setHighSpeed();
      _mqttManager.publishToSubtopic("pump_speed", "High");
    }
    else if (value == "MedOff")
    {
      _pumpController.setMedSpeed();
      _mqttManager.publishToSubtopic("pump_speed", "Medium");
    }
    else if (value == "StopOff")
    {
      _pumpController.setStop();
      _mqttManager.publishToSubtopic("pump_speed", "Stopped");
    }
  }

  request->send(200, "text/plain", "OK");
}

// Handler for time schedule adjustments
void WebServerManager::handleTimeAdjust(AsyncWebServerRequest *request)
{
  Serial.println("Got a time update");
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();

  if (request->params() > 1)
  {
    unsigned long onTime = request->getParam(0)->value().toInt();
    unsigned long offTime = request->getParam(1)->value().toInt();

    Serial.print("New schedule: ON=");
    Serial.print(onTime);
    Serial.print(", OFF=");
    Serial.println(offTime);

    _scheduleManager.setSchedule(onTime, offTime);
  }

  request->send(200, "text/plain", "OK");
}

// Handler for state polling
void WebServerManager::handleState(AsyncWebServerRequest *request)
{
  Serial.println("Got a state request");
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();

  char buffer[200];
  sprintf(buffer, "{\"poolRelaxStatus\":\"%d\",\"pumpSpeed\":\"%d\",\"onTime\":\"%lu\",\"offTime\":\"%lu\",\"rssi\":\"%d\",\"hh\":\"%02lu\",\"mm\":\"%02lu\",\"ss\":\"%02lu\",\"dd\":\"%02lu\",\"md\":\"%02lu\",\"yy\":\"%02lu\",\"currentTemp\":\"%.2f\"}",
          *_currentRelaxStatus,
          _pumpController.getCurrentSpeed(),
          _scheduleManager.getOnHour(),
          _scheduleManager.getOffHour(),
          *_rssi,
          *_currentHour,
          *_currentMinute,
          *_currentSec,
          *_currentDay,
          *_currentMd,
          *_currentYr,
          _tempSensor.getTemperature());

  request->send(200, "application/json", buffer);
}
