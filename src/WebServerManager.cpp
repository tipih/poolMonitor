#include "WebServerManager.h"
#include "PumpController.h"
#include "MQTTManager.h"
#include "ScheduleManager.h"
#include "TemperatureSensor.h"
#include "TimeManager.h"
#include "InputManager.h"
#include "OTAManager.h"
#include "HeatPumpManager.h"
#include "Debug.h"
#include "html.h"

WebServerManager::WebServerManager(PumpController &pump, MQTTManager &mqtt, ScheduleManager &schedule, TemperatureSensor &temp)
    : _server(80), _pumpController(pump), _mqttManager(mqtt), _scheduleManager(schedule), _tempSensor(temp),
      _httpUsername(nullptr), _httpPassword(nullptr),
      _timeManager(nullptr), _inputManager(nullptr), _otaManager(nullptr),
      _heatPumpManager(nullptr), _rssi(nullptr)
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

void WebServerManager::setManagerReferences(TimeManager &timeMgr, InputManager &inputMgr, OTAManager &otaMgr, HeatPumpManager &heatPumpMgr, int *rssi)
{
  _timeManager = &timeMgr;
  _inputManager = &inputMgr;
  _otaManager = &otaMgr;
  _heatPumpManager = &heatPumpMgr;
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
  DBG_PRINTLN("Got an update");
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();

  // Block pump operations during OTA updates to prevent blocking delays
  if (_otaManager && _otaManager->isUpdating())
  {
    request->send(503, "text/plain", "Service unavailable during OTA update");
    return;
  }

  if (request->params() == 1)
  {
    AsyncWebParameter *p = request->getParam(0);
    String value = p->value();

    DBG_PRINT("Button: ");
    DBG_PRINTLN(value);

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
  DBG_PRINTLN("Got a time update");
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();

  // Look up by name rather than positional index. Browsers preserve the
  // query-string order today, but relying on that made the endpoint
  // brittle (e.g. proxies or hand-crafted URLs could reorder params and
  // silently swap the on/off hours).
  AsyncWebParameter *onParam = request->getParam("onTime");
  AsyncWebParameter *offParam = request->getParam("offTime");

  if (!onParam || !offParam)
  {
    request->send(400, "text/plain", "Missing onTime or offTime parameter");
    return;
  }

  long onLong = onParam->value().toInt();
  long offLong = offParam->value().toInt();

  if (onLong < 0 || onLong > 23 || offLong < 0 || offLong > 23)
  {
    request->send(400, "text/plain", "Hour out of range (0-23)");
    return;
  }

  uint8_t onTime = (uint8_t)onLong;
  uint8_t offTime = (uint8_t)offLong;

  DBG_PRINT("New schedule: ON=");
  DBG_PRINT(onTime);
  DBG_PRINT(", OFF=");
  DBG_PRINTLN(offTime);

  _scheduleManager.setSchedule(onTime, offTime);

  request->send(200, "text/plain", "OK");
}

// Handler for state polling
void WebServerManager::handleState(AsyncWebServerRequest *request)
{
  DBG_PRINTLN("Got a state request");
  if (!request->authenticate(_httpUsername, _httpPassword))
    return request->requestAuthentication();

  // Defensive null checks: handleState() depends on references wired in
  // via setManagerReferences(). main.cpp guarantees that ordering, but
  // returning 503 here is safer than dereferencing a stale/null pointer
  // if a request races begin() or the wiring is ever changed.
  if (!_timeManager || !_inputManager || !_rssi)
  {
    request->send(503, "text/plain", "State not ready");
    return;
  }

  // Heat pump fields. Only include real values once we've had at least
  // one successful Modbus poll; otherwise emit hpOnline:0 with empty
  // strings so the dashboard can render "--" placeholders.
  bool hpHasData = _heatPumpManager && _heatPumpManager->getLastUpdate() != 0;
  bool hpOnline = _heatPumpManager && _heatPumpManager->isOnline();

  char buffer[448];
  if (hpHasData)
  {
    snprintf(buffer, sizeof(buffer),
             "{\"poolRelaxStatus\":\"%u\",\"pumpSpeed\":\"%d\",\"onTime\":\"%u\",\"offTime\":\"%u\",\"rssi\":\"%d\","
             "\"hh\":\"%02u\",\"mm\":\"%02u\",\"ss\":\"%02u\",\"dd\":\"%02u\",\"md\":\"%02u\",\"yy\":\"%u\","
             "\"currentTemp\":\"%.2f\","
             "\"hpOnline\":\"%u\",\"hpPower\":\"%u\",\"hpInlet\":\"%.1f\",\"hpOutlet\":\"%.1f\","
             "\"hpAmbient\":\"%.1f\",\"hpTarget\":\"%.1f\",\"hpMode\":\"%u\",\"hpError\":\"%u\","
             "\"hpCompHz\":\"%.1f\",\"hpCompPct\":\"%.0f\"}",
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
             _tempSensor.getTemperature(),
             hpOnline ? 1u : 0u,
             _heatPumpManager->isPowerOn() ? 1u : 0u,
             _heatPumpManager->getInletTempC(),
             _heatPumpManager->getOutletTempC(),
             _heatPumpManager->getAmbientTempC(),
             _heatPumpManager->getTargetTempC(),
             (unsigned)_heatPumpManager->getOperationMode(),
             _heatPumpManager->getErrorCode(),
             _heatPumpManager->getCompressorHz(),
             _heatPumpManager->getCompressorPercent());
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
             "{\"poolRelaxStatus\":\"%u\",\"pumpSpeed\":\"%d\",\"onTime\":\"%u\",\"offTime\":\"%u\",\"rssi\":\"%d\","
             "\"hh\":\"%02u\",\"mm\":\"%02u\",\"ss\":\"%02u\",\"dd\":\"%02u\",\"md\":\"%02u\",\"yy\":\"%u\","
             "\"currentTemp\":\"%.2f\",\"hpOnline\":\"0\"}",
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
  }

  request->send(200, "application/json", buffer);
}
