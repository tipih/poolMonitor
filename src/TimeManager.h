#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include "time.h"

class TimeManager
{
public:
  TimeManager();

  void begin(const char *ntpServer, long gmtOffset, int daylightOffset);
  void update();

  // Getters for time components
  unsigned long getHour() const { return _currentHour; }
  unsigned long getMinute() const { return _currentMinute; }
  unsigned long getSecond() const { return _currentSec; }
  unsigned long getDay() const { return _currentDay; }
  unsigned long getMonth() const { return _currentMd; }
  unsigned long getYear() const { return _currentYr; }

private:
  unsigned long _currentSec;
  unsigned long _currentDay;
  unsigned long _currentMd;
  unsigned long _currentYr;
  unsigned long _currentHour;
  unsigned long _currentMinute;
  unsigned long _previousTime;
};

#endif // TIME_MANAGER_H
