#ifndef HEAT_PUMP_MANAGER_H
#define HEAT_PUMP_MANAGER_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <ModbusMaster.h>

/**
 * Reads and controls a Welldana PMH / Fairland IPHCR family pool heat pump
 * over RS485 (Modbus RTU) using a MAX485 transceiver.
 *
 * Documented register numbers are 1-based; this class converts them to the
 * 0-based addresses Modbus libraries expect (address = register - 1).
 *
 *   Coil   0    : power on/off          (FC01 read, FC05 write)
 *   Input  2001 : error code            (FC04 read)        addr 2000
 *   Input  2010 : inlet  water temp     °C / 10            addr 2009
 *   Input  2011 : outlet water temp     °C / 10            addr 2010
 *   Input  2012 : ambient air temp      °C / 10            addr 2011
 *   Holding 3001: target water temp     °C / 10            addr 3000
 *   Holding 3002: operation mode        0=Auto 1=Heat 2=Cool addr 3001
 *   Holding 3003: silence mode          0=Smart 1=Silence 2=Super addr 3002
 *
 * Polling is non-blocking: poll() returns immediately if the configured
 * interval has not elapsed. Sensor values are cached so callers/MQTT can
 * read them at any rate without triggering bus traffic.
 */
class HeatPumpManager {
public:
  enum OperationMode : uint16_t {
    OP_AUTO = 0,
    OP_HEAT = 1,
    OP_COOL = 2
  };

  enum SilenceMode : uint16_t {
    SILENCE_SMART = 0,        // ~100% (full power)
    SILENCE_SILENCE = 1,      // ~85%
    SILENCE_SUPER_SILENCE = 2 // ~24%
  };

  HeatPumpManager();

  /**
   * Initialize the manager.
   *  rxPin/txPin : ESP UART pins wired to MAX485 RO/DI
   *  dePin       : MAX485 DE (driver enable, active high)
   *  rePin       : MAX485 !RE (receiver enable, active low). Pass the same
   *                value as dePin if your module ties DE and !RE together.
   *  slaveId     : Modbus slave ID of the heat pump (default 1)
   *  baud        : Modbus baud rate (default 9600)
   *  uartNum     : ESP32 hardware UART index (default 2)
   */
  void begin(uint8_t rxPin, uint8_t txPin, uint8_t dePin, uint8_t rePin,
             uint8_t slaveId = 1, uint32_t baud = 9600, uint8_t uartNum = 2);

  /**
   * Service the Modbus connection. Call from loop() frequently; refresh
   * happens at most once per pollIntervalMs (default 5 s).
   * Returns true if a fresh poll cycle ran this call (success or failure).
   */
  bool poll();

  /** Override the minimum interval between bus poll cycles (ms). */
  void setPollInterval(unsigned long ms) { _pollIntervalMs = ms; }

  /** True if the most recent poll cycle completed without Modbus errors. */
  bool isOnline() const { return _online; }

  /** millis() timestamp of the most recent successful poll, 0 if never. */
  unsigned long getLastUpdate() const { return _lastSuccessMs; }

  // --- Cached sensor readings (input registers) ---
  float getInletTempC() const { return _inletTempC; }
  float getOutletTempC() const { return _outletTempC; }
  float getAmbientTempC() const { return _ambientTempC; }
  uint16_t getErrorCode() const { return _errorCode; }

  // --- Cached settings (coil + holding registers) ---
  bool isPowerOn() const { return _powerOn; }
  float getTargetTempC() const { return _targetTempC; }
  OperationMode getOperationMode() const { return _operationMode; }
  SilenceMode getSilenceMode() const { return _silenceMode; }

  // --- Control commands (each performs a synchronous Modbus write) ---
  bool setPower(bool on);
  bool setTargetTempC(float celsius);
  bool setOperationMode(OperationMode mode);
  bool setSilenceMode(SilenceMode mode);

  /** Human-readable name for the last Modbus error result (or "OK"). */
  static const char *resultToString(uint8_t result);

private:
  // Documented register -> Modbus address (0-based)
  static constexpr uint16_t COIL_POWER          = 0;
  static constexpr uint16_t IR_ERROR_CODE       = 2000; // 2001 - 1
  static constexpr uint16_t IR_INLET_TEMP       = 2009; // 2010 - 1
  static constexpr uint16_t IR_OUTLET_TEMP      = 2010; // 2011 - 1
  static constexpr uint16_t IR_AMBIENT_TEMP     = 2011; // 2012 - 1
  static constexpr uint16_t HR_TARGET_TEMP      = 3000; // 3001 - 1
  static constexpr uint16_t HR_OPERATION_MODE   = 3001; // 3002 - 1
  static constexpr uint16_t HR_SILENCE_MODE     = 3002; // 3003 - 1

  HardwareSerial *_serial;
  ModbusMaster _node;
  uint8_t _dePin;
  uint8_t _rePin;
  uint8_t _slaveId;
  unsigned long _pollIntervalMs;
  unsigned long _lastPollMs;
  unsigned long _lastSuccessMs;
  uint8_t _lastResult;
  bool _online;
  bool _initialized;

  // Cached values
  float _inletTempC;
  float _outletTempC;
  float _ambientTempC;
  uint16_t _errorCode;
  bool _powerOn;
  float _targetTempC;
  OperationMode _operationMode;
  SilenceMode _silenceMode;

  // Static trampolines required by the ModbusMaster callback signature.
  // Only one HeatPumpManager instance is supported at a time.
  static HeatPumpManager *_activeInstance;
  static void preTransmissionStatic();
  static void postTransmissionStatic();
  void preTransmission();
  void postTransmission();
};

#endif // HEAT_PUMP_MANAGER_H
