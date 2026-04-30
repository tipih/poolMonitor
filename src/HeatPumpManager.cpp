#include "HeatPumpManager.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

static constexpr const char *BASE_URL =
    "https://cloud.linked-go.com/cloudservice/api/app";

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

HeatPumpManager::HeatPumpManager()
    : _username(nullptr),
      _password(nullptr),
      _initialized(false),
      _online(false),
      _lastSuccessMs(0),
      _lastPollMs(0),
      _pollIntervalMs(90000),
      _tokenAcquiredMs(0),
      _inletTempC(NAN),
      _outletTempC(NAN),
      _ambientTempC(NAN),
      _errorCode(0),
      _powerOn(false),
      _compressorHz(0.0f),
      _targetTempC(NAN),
      _operationMode(OP_AUTO),
      _silenceMode(SILENCE_SMART)
{
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void HeatPumpManager::begin(const char *username, const char *password)
{
  _username    = username;
  _password    = password;
  _initialized = true;
  Serial.println("[HeatPump] Cloud REST client initialized");

  if (login()) {
    fetchDeviceCode();
  }
}

bool HeatPumpManager::poll()
{
  if (!_initialized)
    return false;

  unsigned long now = millis();
  // Rate-limit: enforce the 30 s interval counted from the END of the
  // previous poll (stamp is written at the bottom of this function).
  if (_lastPollMs != 0 && (now - _lastPollMs) < _pollIntervalMs)
    return false;

  Serial.printf("[HeatPump] Poll start (uptime %lu s, last success %lu s ago)\n",
                now / 1000,
                _lastSuccessMs ? (now - _lastSuccessMs) / 1000 : 0);

  // Re-authenticate only when the token is absent or has exceeded its
  // lifetime. The token acquired in begin() is reused for every normal
  // poll cycle until TOKEN_LIFETIME_MS elapses.
  bool needsAuth = _token.isEmpty()
                || (now - _tokenAcquiredMs) >= TOKEN_LIFETIME_MS;

  // Also re-fetch the device code if it was never successfully obtained
  // (e.g. begin()'s fetchDeviceCode() failed transiently).
  bool needsDevice = needsAuth || _deviceCode.isEmpty();

  if (needsAuth) {
    Serial.println("[HeatPump] Token absent/expired – re-authenticating");
    if (!login()) {
      _online = false;
      _lastPollMs = millis(); // keep 30 s back-off on failure
      return true;
    }
  }

  if (needsDevice) {
    if (!fetchDeviceCode()) {
      _online = false;
      _lastPollMs = millis();
      return true;
    }
  }

  _online = fetchTelemetry();
  if (_online) {
    _lastSuccessMs = millis();
    Serial.printf("[HeatPump] Poll OK in %lu ms\n", millis() - now);
  } else {
    Serial.printf("[HeatPump] Telemetry fetch failed (took %lu ms)\n", millis() - now);
  }

  // Stamp AFTER all HTTPS work so the full 30 s always elapses between
  // the end of one poll and the start of the next.
  _lastPollMs = millis();
  return true;
}

// ---------------------------------------------------------------------------
// Authentication
// ---------------------------------------------------------------------------

bool HeatPumpManager::login()
{
  Serial.printf("[HeatPump] Logging in as %s ...\n", _username);
  WiFiClientSecure client;
  client.setInsecure(); // Certificate not validated — no CA bundle on device

  HTTPClient http;
  String url = String(BASE_URL) + "/user/login.json";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json; charset=utf-8");

  JsonDocument reqDoc;
  reqDoc["user_name"] = _username;
  reqDoc["password"]  = _password;
  reqDoc["type"]      = "2";
  String reqBody;
  serializeJson(reqDoc, reqBody);

  unsigned long t0 = millis();
  int code = http.POST(reqBody);
  if (code != 200) {
    Serial.printf("[HeatPump] Login HTTP %d (took %lu ms)\n", code, millis() - t0);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[HeatPump] Login response %d bytes in %lu ms\n",
                payload.length(), millis() - t0);

  JsonDocument resDoc;
  DeserializationError err = deserializeJson(resDoc, payload);
  if (err) {
    Serial.printf("[HeatPump] Login JSON error: %s\n", err.c_str());
    Serial.printf("[HeatPump] Raw: %s\n", payload.c_str());
    return false;
  }

  const char *token = resDoc["object_result"]["x-token"];
  if (!token || token[0] == '\0') {
    Serial.println("[HeatPump] Login: x-token not found in response");
    Serial.printf("[HeatPump] Raw: %s\n", payload.c_str());
    return false;
  }

  _token           = String(token);
  _tokenAcquiredMs = millis();
  // Show only the first 8 chars of the token as a sanity-check fingerprint.
  Serial.printf("[HeatPump] Login OK – token: %.8s...\n", _token.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// Device discovery
// ---------------------------------------------------------------------------

bool HeatPumpManager::fetchDeviceCode()
{
  Serial.println("[HeatPump] Fetching device list ...");
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(BASE_URL) + "/device/deviceList.json";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json; charset=utf-8");
  http.addHeader("x-token", _token);

  unsigned long t0 = millis();
  int code = http.POST("{}");
  if (code != 200) {
    Serial.printf("[HeatPump] DeviceList HTTP %d (took %lu ms)\n", code, millis() - t0);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[HeatPump] DeviceList response %d bytes in %lu ms\n",
                payload.length(), millis() - t0);

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[HeatPump] DeviceList JSON error: %s\n", err.c_str());
    Serial.printf("[HeatPump] Raw: %s\n", payload.c_str());
    return false;
  }

  const char *dc = doc["object_result"][0]["device_code"];
  if (!dc || dc[0] == '\0') {
    Serial.println("[HeatPump] DeviceList: device_code not found");
    Serial.printf("[HeatPump] Raw: %s\n", payload.c_str());
    return false;
  }

  _deviceCode = String(dc);
  Serial.printf("[HeatPump] Device code: %s\n", _deviceCode.c_str());
  return true;
}

// ---------------------------------------------------------------------------
// Telemetry polling
// ---------------------------------------------------------------------------

bool HeatPumpManager::fetchTelemetry()
{
  if (_deviceCode.isEmpty()) return false;
  Serial.println("[HeatPump] Fetching telemetry ...");

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(BASE_URL) + "/device/getDataByCode.json";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json; charset=utf-8");
  http.addHeader("x-token", _token);

  JsonDocument reqDoc;
  reqDoc["device_code"] = _deviceCode;
  JsonArray codes = reqDoc["protocal_codes"].to<JsonArray>(); // typo is in the API
  codes.add("Power");
  codes.add("T02");
  codes.add("T03");
  codes.add("T05");
  codes.add("R02");
  codes.add("Mode");
  codes.add("Manual-mute");
  codes.add("Err");
  codes.add("O07");

  String reqBody;
  serializeJson(reqDoc, reqBody);

  unsigned long t0 = millis();
  int code = http.POST(reqBody);
  if (code != 200) {
    Serial.printf("[HeatPump] Telemetry HTTP %d (took %lu ms)\n", code, millis() - t0);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[HeatPump] Telemetry response %d bytes in %lu ms\n",
                payload.length(), millis() - t0);

  JsonDocument resDoc;
  DeserializationError err = deserializeJson(resDoc, payload);
  if (err) {
    Serial.printf("[HeatPump] Telemetry JSON error: %s\n", err.c_str());
    Serial.printf("[HeatPump] Raw: %s\n", payload.c_str());
    return false;
  }

  JsonArray results = resDoc["object_result"].as<JsonArray>();
  if (results.isNull()) {
    Serial.println("[HeatPump] Telemetry: missing object_result array");
    Serial.printf("[HeatPump] Raw: %s\n", payload.c_str());
    return false;
  }

  for (JsonObject item : results) {
    const char *k = item["code"];
    const char *v = item["value"];
    if (!k || !v) continue;

    if      (strcmp(k, "Power")       == 0) { _powerOn       = (atoi(v) != 0); }
    else if (strcmp(k, "T02")         == 0) { _inletTempC    = atof(v);        }
    else if (strcmp(k, "T03")         == 0) { _outletTempC   = atof(v);        }
    else if (strcmp(k, "T05")         == 0) { _ambientTempC  = atof(v);        }
    else if (strcmp(k, "R02")         == 0) { _targetTempC   = atof(v);        }
    else if (strcmp(k, "Mode")        == 0) { _operationMode = (OperationMode)atoi(v); }
    else if (strcmp(k, "Manual-mute") == 0) { _silenceMode   = (SilenceMode)atoi(v);  }
    else if (strcmp(k, "Err")         == 0) { _errorCode     = (uint16_t)atoi(v);     }
    else if (strcmp(k, "O07")         == 0) { _compressorHz  = atof(v);               }
  }

  Serial.printf("[HeatPump] Telemetry: power=%s inlet=%.1f outlet=%.1f "
                "ambient=%.1f target=%.1f mode=%u silence=%u err=%u "
                "compressor=%.1f Hz (%.0f%%)\n",
                _powerOn ? "ON" : "OFF",
                _inletTempC, _outletTempC, _ambientTempC, _targetTempC,
                (unsigned)_operationMode, (unsigned)_silenceMode, _errorCode,
                _compressorHz, (_compressorHz / 85.0f) * 100.0f);

  return true;
}

// ---------------------------------------------------------------------------
// Cloud control helper
// ---------------------------------------------------------------------------

bool HeatPumpManager::sendControl(const char *protocolCode, const char *value)
{
  if (!_initialized || _deviceCode.isEmpty()) return false;

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  String url = String(BASE_URL) + "/device/control.json";
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json; charset=utf-8");
  http.addHeader("x-token", _token);

  JsonDocument reqDoc;
  JsonArray param   = reqDoc["param"].to<JsonArray>();
  JsonObject entry  = param.add<JsonObject>();
  entry["device_code"]   = _deviceCode;
  entry["protocol_code"] = protocolCode;
  entry["value"]         = value;

  String reqBody;
  serializeJson(reqDoc, reqBody);

  unsigned long t0 = millis();
  int code = http.POST(reqBody);
  http.end();

  if (code != 200) {
    Serial.printf("[HeatPump] Control (%s=%s) HTTP %d (took %lu ms)\n",
                  protocolCode, value, code, millis() - t0);
    return false;
  }
  Serial.printf("[HeatPump] Control (%s=%s) OK (%lu ms)\n",
                protocolCode, value, millis() - t0);
  return true;
}

// ---------------------------------------------------------------------------
// Control methods
// ---------------------------------------------------------------------------

bool HeatPumpManager::setPower(bool on)
{
  bool ok = sendControl("power", on ? "1" : "0");
  if (ok) _powerOn = on;
  return ok;
}

bool HeatPumpManager::setTargetTempC(float celsius)
{
  if (celsius < 5.0f)  celsius = 5.0f;
  if (celsius > 40.0f) celsius = 40.0f;
  char buf[8];
  snprintf(buf, sizeof(buf), "%.0f", celsius);
  bool ok = sendControl("set_temp", buf);
  if (ok) _targetTempC = celsius;
  return ok;
}

bool HeatPumpManager::setOperationMode(OperationMode mode)
{
  char buf[4];
  snprintf(buf, sizeof(buf), "%u", (unsigned)mode);
  bool ok = sendControl("mode", buf);
  if (ok) _operationMode = mode;
  return ok;
}

bool HeatPumpManager::setSilenceMode(SilenceMode mode)
{
  char buf[4];
  snprintf(buf, sizeof(buf), "%u", (unsigned)mode);
  bool ok = sendControl("manual-mute", buf);
  if (ok) _silenceMode = mode;
  return ok;
}
