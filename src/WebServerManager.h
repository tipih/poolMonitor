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

/**
 * Hosts the device's HTTP control panel on port 80 using
 * ESPAsyncWebServer. Serves the embedded HTML UI, exposes a JSON /state
 * endpoint for polling, and accepts pump-control and schedule-update
 * requests. All endpoints are protected by HTTP Basic auth.
 *
 * Construction takes the always-required collaborators by reference;
 * setManagerReferences() supplies the optional ones. setManagerReferences()
 * MUST be called before begin(), since handlers run on the async server
 * task and would otherwise observe null pointers.
 */
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
