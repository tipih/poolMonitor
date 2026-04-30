#ifndef HEAT_PUMP_MANAGER_H
#define HEAT_PUMP_MANAGER_H

#include <Arduino.h>

/**
 * Reads and controls a Fairland / Linked-Go cloud-connected pool heat pump
 * via the linked-go.com REST API over HTTPS.
 *
 * Authentication flow (automatic, managed internally):
 *   1. POST /user/login.json        -> obtain x-token (refreshed every 12 h)
 *   2. POST /device/deviceList.json -> obtain device_code (once per login)
 *   3. POST /device/getDataByCode.json  (every poll cycle)
 *
 * Protocol codes used for telemetry reads:
 *   Power       -> isPowerOn()
 *   T02         -> getInletTempC()   (water in to heat pump)
 *   T03         -> getOutletTempC()  (water out from heat pump)
 *   T05         -> getAmbientTempC() (ambient air)
 *   R02         -> getTargetTempC()
 *   Mode        -> getOperationMode()
 *   Manual-mute -> getSilenceMode()
 *   Err         -> getErrorCode()
 *   O07         -> getCompressorHz() / getCompressorPercent() (max 85 Hz)
 *
 * Polling is non-blocking: poll() returns immediately if the configured
 * interval has not elapsed. Sensor values are cached.
 */
class HeatPumpManager {
public:
  enum OperationMode : uint8_t {
    OP_AUTO = 0,
    OP_HEAT = 1,
    OP_COOL = 2
  };

  enum SilenceMode : uint8_t {
    SILENCE_SMART         = 0,
    SILENCE_SILENCE       = 1,
    SILENCE_SUPER_SILENCE = 2
  };

  HeatPumpManager();

  /**
   * Store cloud credentials and attempt an initial login + device discovery.
   * WiFi must already be connected before calling begin().
   */
  void begin(const char *username, const char *password);

  /**
   * Service the cloud connection. Call from loop() frequently; refresh
   * happens at most once per pollIntervalMs (default 30 s).
   * Returns true if a fresh poll cycle ran this call.
   */
  bool poll();

  /** Override the minimum interval between poll cycles (ms). */
  void setPollInterval(unsigned long ms) { _pollIntervalMs = ms; }

  /** True if the most recent poll cycle completed successfully. */
  bool isOnline() const { return _online; }

  /** millis() timestamp of the most recent successful poll, 0 if never. */
  unsigned long getLastUpdate() const { return _lastSuccessMs; }

  // --- Cached sensor readings ---
  float    getInletTempC()        const { return _inletTempC;   }
  float    getOutletTempC()       const { return _outletTempC;  }
  float    getAmbientTempC()      const { return _ambientTempC; }
  uint16_t getErrorCode()         const { return _errorCode;    }
  float    getCompressorHz()      const { return _compressorHz; }
  /** Compressor load as 0-100 %, based on O07 max of 85 Hz. */
  float    getCompressorPercent() const { return (_compressorHz / 85.0f) * 100.0f; }

  // --- Cached settings ---
  bool          isPowerOn()        const { return _powerOn;       }
  float         getTargetTempC()   const { return _targetTempC;   }
  OperationMode getOperationMode() const { return _operationMode; }
  SilenceMode   getSilenceMode()   const { return _silenceMode;   }

  // --- Control commands ---
  bool setPower(bool on);
  bool setTargetTempC(float celsius);
  bool setOperationMode(OperationMode mode);
  bool setSilenceMode(SilenceMode mode);

private:
  // Token is refreshed after this period to avoid expiry mid-session.
  static constexpr unsigned long TOKEN_LIFETIME_MS = 12UL * 3600UL * 1000UL;

  bool login();
  bool fetchDeviceCode();
  bool fetchTelemetry();
  bool sendControl(const char *protocolCode, const char *value);

  const char   *_username;
  const char   *_password;
  String        _token;
  String        _deviceCode;

  bool          _initialized;
  bool          _online;
  unsigned long _lastSuccessMs;
  unsigned long _lastPollMs;
  unsigned long _pollIntervalMs;
  unsigned long _tokenAcquiredMs;

  float         _inletTempC;
  float         _outletTempC;
  float         _ambientTempC;
  uint16_t      _errorCode;
  float         _compressorHz;
  bool          _powerOn;
  float         _targetTempC;
  OperationMode _operationMode;
  SilenceMode   _silenceMode;
};

#endif // HEAT_PUMP_MANAGER_H
