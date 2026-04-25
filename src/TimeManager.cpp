#include "TimeManager.h"

TimeManager::TimeManager()
    : _currentSec(1), _currentDay(1), _currentMd(1), _currentYr(1),
      _currentHour(1), _currentMinute(1), _previousTime(0)
{
}

void TimeManager::begin(const char *ntpServer, long gmtOffset, int daylightOffset)
{
  // Initialize and get the time from NTP server
  configTime(gmtOffset, daylightOffset, ntpServer);
  Serial.println("Time manager initialized with NTP server");
}

void TimeManager::update()
{
  struct tm timeinfo;

  if (millis() - _previousTime > 5000) // Update every 5 seconds
  {
    if (!getLocalTime(&timeinfo))
    {
      Serial.println("Failed to obtain time");
      return;
    }
    _previousTime = millis();

    _currentHour = timeinfo.tm_hour;
    _currentMinute = timeinfo.tm_min;
    _currentSec = timeinfo.tm_sec;
    _currentDay = timeinfo.tm_mday;
    _currentMd = timeinfo.tm_mon;
    _currentYr = timeinfo.tm_year + 1900;

    // Disabled verbose time logging to reduce serial output and improve OTA responsiveness
    // Serial.printf("Time: %02d:%02d:%02d\n", _currentHour, _currentMinute, _currentSec);
  }
}
