#ifndef WEBSERVERMANAGER_H
#define WEBSERVERMANAGER_H

#include <ESPAsyncWebServer.h>

// Forward declarations
class PumpController;
class MQTTManager;
class ScheduleManager;
class TemperatureSensor;

class WebServerManager
{
public:
  WebServerManager(PumpController &pump, MQTTManager &mqtt, ScheduleManager &schedule, TemperatureSensor &temp);

  void begin(const char *username, const char *password);
  void setReferences(int *relax, int *rssi, unsigned long *hour, unsigned long *minute, unsigned long *sec,
                     unsigned long *day, unsigned long *md, unsigned long *yr);

private:
  AsyncWebServer _server;
  PumpController &_pumpController;
  MQTTManager &_mqttManager;
  ScheduleManager &_scheduleManager;
  TemperatureSensor &_tempSensor;

  const char *_httpUsername;
  const char *_httpPassword;

  // References to global variables needed for state
  int *_currentRelaxStatus;
  int *_rssi;
  unsigned long *_currentHour;
  unsigned long *_currentMinute;
  unsigned long *_currentSec;
  unsigned long *_currentDay;
  unsigned long *_currentMd;
  unsigned long *_currentYr;

  // Handler methods
  void handleRoot(AsyncWebServerRequest *request);
  void handleLogout(AsyncWebServerRequest *request);
  void handleLoggedOut(AsyncWebServerRequest *request);
  void handleUpdate(AsyncWebServerRequest *request);
  void handleTimeAdjust(AsyncWebServerRequest *request);
  void handleState(AsyncWebServerRequest *request);
};

#endif
