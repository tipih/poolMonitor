#include "HeatPumpManager.h"

HeatPumpManager *HeatPumpManager::_activeInstance = nullptr;

HeatPumpManager::HeatPumpManager()
    : _serial(nullptr),
      _dePin(0),
      _rePin(0),
      _slaveId(1),
      _pollIntervalMs(5000),
      _lastPollMs(0),
      _lastSuccessMs(0),
      _lastResult(ModbusMaster::ku8MBSuccess),
      _online(false),
      _initialized(false),
      _inletTempC(NAN),
      _outletTempC(NAN),
      _ambientTempC(NAN),
      _errorCode(0),
      _powerOn(false),
      _targetTempC(NAN),
      _operationMode(OP_AUTO),
      _silenceMode(SILENCE_SMART)
{
}

HeatPumpManager::~HeatPumpManager()
{
  // Allocated once in begin(). In practice this object lives for the
  // entire firmware lifetime, so the destructor never actually runs on
  // the ESP32 — but defining it keeps ownership symmetric with
  // TemperatureSensor and avoids a leak if the instance is ever scoped.
  if (_activeInstance == this)
    _activeInstance = nullptr;
  delete _serial;
  _serial = nullptr;
}

void HeatPumpManager::begin(uint8_t rxPin, uint8_t txPin, uint8_t dePin, uint8_t rePin,
                            uint8_t slaveId, uint32_t baud, uint8_t uartNum)
{
  if (_initialized) {
    Serial.println("[HeatPump] begin() called twice; ignoring");
    return;
  }

  _dePin = dePin;
  _rePin = rePin;
  _slaveId = slaveId;

  pinMode(_dePin, OUTPUT);
  pinMode(_rePin, OUTPUT);
  // Idle in receive mode (DE low, !RE low).
  digitalWrite(_dePin, LOW);
  digitalWrite(_rePin, LOW);

  _serial = new HardwareSerial(uartNum);
  _serial->begin(baud, SERIAL_8N1, rxPin, txPin);

  _activeInstance = this;
  _node.begin(_slaveId, *_serial);
  _node.preTransmission(HeatPumpManager::preTransmissionStatic);
  _node.postTransmission(HeatPumpManager::postTransmissionStatic);

  _initialized = true;
  Serial.printf("[HeatPump] Modbus initialized (UART%u rx=%u tx=%u de=%u re=%u id=%u %lu baud)\n",
                uartNum, rxPin, txPin, dePin, rePin, slaveId, (unsigned long)baud);
}

void HeatPumpManager::preTransmissionStatic()
{
  if (_activeInstance)
    _activeInstance->preTransmission();
}

void HeatPumpManager::postTransmissionStatic()
{
  if (_activeInstance)
    _activeInstance->postTransmission();
}

void HeatPumpManager::preTransmission()
{
  digitalWrite(_rePin, HIGH); // disable receiver
  digitalWrite(_dePin, HIGH); // enable driver
}

void HeatPumpManager::postTransmission()
{
  digitalWrite(_dePin, LOW); // disable driver
  digitalWrite(_rePin, LOW); // enable receiver
}

bool HeatPumpManager::poll()
{
  if (!_initialized)
    return false;

  unsigned long now = millis();
  if (_lastPollMs != 0 && (now - _lastPollMs) < _pollIntervalMs)
    return false;
  _lastPollMs = now;

  bool ok = true;
  uint8_t r;

  // 1) Input registers: error + 3 temperatures.
  // Error code is at 2000; temps are at 2009..2011. Read them as two blocks
  // to avoid pulling ~10 unused registers in between.
  r = _node.readInputRegisters(IR_ERROR_CODE, 1);
  if (r == ModbusMaster::ku8MBSuccess) {
    _errorCode = _node.getResponseBuffer(0);
  } else {
    ok = false;
    _lastResult = r;
  }

  r = _node.readInputRegisters(IR_INLET_TEMP, 3);
  if (r == ModbusMaster::ku8MBSuccess) {
    _inletTempC   = (int16_t)_node.getResponseBuffer(0) / 10.0f;
    _outletTempC  = (int16_t)_node.getResponseBuffer(1) / 10.0f;
    _ambientTempC = (int16_t)_node.getResponseBuffer(2) / 10.0f;
  } else {
    ok = false;
    _lastResult = r;
  }

  // 2) Holding registers: target temp, op mode, silence mode (3 contiguous).
  r = _node.readHoldingRegisters(HR_TARGET_TEMP, 3);
  if (r == ModbusMaster::ku8MBSuccess) {
    _targetTempC    = (int16_t)_node.getResponseBuffer(0) / 10.0f;
    _operationMode  = (OperationMode)_node.getResponseBuffer(1);
    _silenceMode    = (SilenceMode)_node.getResponseBuffer(2);
  } else {
    ok = false;
    _lastResult = r;
  }

  // 3) Coil 0: power state.
  r = _node.readCoils(COIL_POWER, 1);
  if (r == ModbusMaster::ku8MBSuccess) {
    _powerOn = (_node.getResponseBuffer(0) & 0x0001) != 0;
  } else {
    ok = false;
    _lastResult = r;
  }

  _online = ok;
  if (ok) {
    _lastSuccessMs = now;
    _lastResult = ModbusMaster::ku8MBSuccess;
  } else {
    Serial.printf("[HeatPump] poll error: %s (0x%02X)\n", resultToString(_lastResult), _lastResult);
  }
  return true;
}

bool HeatPumpManager::setPower(bool on)
{
  if (!_initialized) return false;
  uint8_t r = _node.writeSingleCoil(COIL_POWER, on ? 1 : 0);
  if (r == ModbusMaster::ku8MBSuccess) {
    _powerOn = on;
    return true;
  }
  Serial.printf("[HeatPump] setPower error: %s\n", resultToString(r));
  return false;
}

bool HeatPumpManager::setTargetTempC(float celsius)
{
  if (!_initialized) return false;
  // Clamp to a sane range before scaling. The hardware will also enforce
  // its own limits, but this avoids accidentally writing absurd values.
  if (celsius < 5.0f) celsius = 5.0f;
  if (celsius > 40.0f) celsius = 40.0f;
  uint16_t raw = (uint16_t)lroundf(celsius * 10.0f);
  uint8_t r = _node.writeSingleRegister(HR_TARGET_TEMP, raw);
  if (r == ModbusMaster::ku8MBSuccess) {
    _targetTempC = raw / 10.0f;
    return true;
  }
  Serial.printf("[HeatPump] setTargetTempC error: %s\n", resultToString(r));
  return false;
}

bool HeatPumpManager::setOperationMode(OperationMode mode)
{
  if (!_initialized) return false;
  uint8_t r = _node.writeSingleRegister(HR_OPERATION_MODE, (uint16_t)mode);
  if (r == ModbusMaster::ku8MBSuccess) {
    _operationMode = mode;
    return true;
  }
  Serial.printf("[HeatPump] setOperationMode error: %s\n", resultToString(r));
  return false;
}

bool HeatPumpManager::setSilenceMode(SilenceMode mode)
{
  if (!_initialized) return false;
  uint8_t r = _node.writeSingleRegister(HR_SILENCE_MODE, (uint16_t)mode);
  if (r == ModbusMaster::ku8MBSuccess) {
    _silenceMode = mode;
    return true;
  }
  Serial.printf("[HeatPump] setSilenceMode error: %s\n", resultToString(r));
  return false;
}

const char *HeatPumpManager::resultToString(uint8_t result)
{
  switch (result) {
    case ModbusMaster::ku8MBSuccess:             return "OK";
    case ModbusMaster::ku8MBIllegalFunction:     return "Illegal function";
    case ModbusMaster::ku8MBIllegalDataAddress:  return "Illegal data address";
    case ModbusMaster::ku8MBIllegalDataValue:    return "Illegal data value";
    case ModbusMaster::ku8MBSlaveDeviceFailure:  return "Slave device failure";
    case ModbusMaster::ku8MBInvalidSlaveID:      return "Invalid slave ID";
    case ModbusMaster::ku8MBInvalidFunction:     return "Invalid function";
    case ModbusMaster::ku8MBResponseTimedOut:    return "Response timed out";
    case ModbusMaster::ku8MBInvalidCRC:          return "Invalid CRC";
    default:                                     return "Unknown error";
  }
}
