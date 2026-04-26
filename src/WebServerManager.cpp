#include "WebServerManager.h"
#include "PumpController.h"
#include "MQTTManager.h"
#include "ScheduleManager.h"
#include "TemperatureSensor.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "html.h"

WebServerManager::WebServerManager(PumpController &pump, MQTTManager &mqtt, ScheduleManager &schedule, TemperatureSensor &temp)
    : _server(80), _pumpController(pump), _mqttManager(mqtt), _scheduleManager(schedule), _tempSensor(temp),
      _httpUsername(nullptr), _httpPassword(nullptr),
      _timeManager(nullptr), _inputManager(nullptr), _rssi(nullptr)
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

void WebServerManager::setManagerReferences(TimeManager &timeMgr, InputManager &inputMgr, int *rssi)
{
  _timeManager = &timeMgr;
  _inputManager = &inputMgr;
  _rssi = rssi;
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
      _mqttManager.publishToSubtopic("pump_speed", _pumpController.getSpeedString());
    }
    else if (value == "HighOff")
    {
      _pumpController.setHighSpeed();
      _mqttManager.publishToSubtopic("pump_speed", _pumpController.getSpeedString());
    }
    else if (value == "MedOff")
    {
      _pumpController.setMedSpeed();
      _mqttManager.publishToSubtopic("pump_speed", _pumpController.getSpeedString());
    }
    else if (value == "StopOff")
    {
      _pumpController.setStop();
      _mqttManager.publishToSubtopic("pump_speed", _pumpController.getSpeedString());
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
    uint8_t onTime = request->getParam(0)->value().toInt();
    uint8_t offTime = request->getParam(1)->value().toInt();

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
  snprintf(buffer, sizeof(buffer), "{\"poolRelaxStatus\":\"%d\",\"pumpSpeed\":\"%d\",\"onTime\":\"%u\",\"offTime\":\"%u\",\"rssi\":\"%d\",\"hh\":\"%02u\",\"mm\":\"%02u\",\"ss\":\"%02u\",\"dd\":\"%02u\",\"md\":\"%02u\",\"yy\":\"%u\",\"currentTemp\":\"%.2f\"}",
          _inputManager->getRelaxStatus(),
          _pumpController.getCurrentSpeed(),
          _scheduleManager.getOnHour(),
          _scheduleManager.getOffHour(),
          *_rssi,
          _timeManager->getHour(),
          _timeManager->getMinute(),
          _timeManager->getSecond(),
          _timeManager->getDay(),
          _timeManager->getMonth(),
          _timeManager->getYear(),
          _tempSensor.getTemperature());

  request->send(200, "application/json", buffer);
}
