#ifndef TIME_MANAGER_H
#define TIME_MANAGER_H

#include <Arduino.h>
#include "time.h"

/**
 * Maintains current wall-clock time via SNTP. update() polls the system
 * time roughly every 5 seconds with a short non-blocking timeout so it
 * never stalls the main loop when NTP has not yet synced.
 *
 * isTimeValid() returns true only after the first successful sync;
 * callers (e.g. ScheduleManager) should gate time-dependent actions on
 * it to avoid acting on the placeholder values used before sync.
 */
class TimeManager
{
public:
  TimeManager();

  void begin(const char *ntpServer, long gmtOffset, int daylightOffset);
  void update();

  // True once getLocalTime() has succeeded at least once (i.e. NTP synced)
  bool isTimeValid() const { return _timeValid; }

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
  bool _timeValid;
};

#endif // TIME_MANAGER_H
