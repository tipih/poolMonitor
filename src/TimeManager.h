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
  uint8_t getHour() const { return _currentHour; }
  uint8_t getMinute() const { return _currentMinute; }
  uint8_t getSecond() const { return _currentSec; }
  uint8_t getDay() const { return _currentDay; }
  uint8_t getMonth() const { return _currentMd; }
  uint16_t getYear() const { return _currentYr; }

private:
  uint8_t _currentSec;
  uint8_t _currentDay;
  uint8_t _currentMd;
  uint16_t _currentYr;
  uint8_t _currentHour;
  uint8_t _currentMinute;
  unsigned long _previousTime;
};

#endif // TIME_MANAGER_H
