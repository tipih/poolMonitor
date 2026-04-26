#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <ESPAsyncWebServer.h>

// Forward declarations
class PumpController;
class MQTTManager;
class ScheduleManager;
class TemperatureSensor;
class TimeManager;
class InputManager;
class OTAManager;

class WebServerManager
{
public:
  WebServerManager(PumpController &pump, MQTTManager &mqtt, ScheduleManager &schedule, TemperatureSensor &temp);

  void begin(const char *username, const char *password);
  void setManagerReferences(TimeManager &timeMgr, InputManager &inputMgr, OTAManager &otaMgr, int *rssi);

private:
  AsyncWebServer _server;
  PumpController &_pumpController;
  MQTTManager &_mqttManager;
  ScheduleManager &_scheduleManager;
  TemperatureSensor &_tempSensor;

  const char *_httpUsername;
  const char *_httpPassword;

  // References to managers for state
  TimeManager *_timeManager;
  InputManager *_inputManager;
  OTAManager *_otaManager;
  int *_rssi;

  // Handler methods
  void handleRoot(AsyncWebServerRequest *request);
  void handleLogout(AsyncWebServerRequest *request);
  void handleLoggedOut(AsyncWebServerRequest *request);
  void handleUpdate(AsyncWebServerRequest *request);
  void handleTimeAdjust(AsyncWebServerRequest *request);
  void handleState(AsyncWebServerRequest *request);
};

#endif // WEB_SERVER_MANAGER_H
