// Native unit tests for the pool-monitor managers.
//
// Built only by [env:native]. Exercises pure logic and string mappings
// that don't require ESP32 hardware. See README in this folder for which
// behaviours are intentionally NOT covered (anything touching real GPIO,
// NVS, MQTT or RS485 traffic).

#include <unity.h>
#include <cmath>

#include "PumpController.h"
#include "ScheduleManager.h"
#include "HeatPumpManager.h"
#include "ModbusMaster.h"
#include "MQTTManager.h"

void setUp(void)
{
  // Reset programmable mock state between tests so order of execution
  // can never cause cross-test contamination.
  ModbusMaster::nextResult = ModbusMaster::ku8MBSuccess;
  for (int i = 0; i < 8; ++i) ModbusMaster::responseBuffer[i] = 0;
  MQTTManager::lastSub.clear();
  MQTTManager::lastVal.clear();
  MQTTManager::publishCount = 0;
}

void tearDown(void) {}

// ---------------------------------------------------------------------------
// PumpController
// ---------------------------------------------------------------------------

void test_pump_initial_speed_is_no_speed(void)
{
  PumpController pc;
  TEST_ASSERT_EQUAL(PumpController::NO_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("None", pc.getSpeedString());
}

void test_pump_speed_string_mapping(void)
{
  // The string table is the public contract used by both the web UI and
  // MQTT, so each enum value must map to a stable label.
  PumpController pc;
  pc.begin(1, 2, 3, 4); // pin numbers irrelevant under the mock

  pc.setLowSpeed();
  TEST_ASSERT_EQUAL(PumpController::LOW_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("Low", pc.getSpeedString());

  pc.setHighSpeed();
  TEST_ASSERT_EQUAL(PumpController::HIGH_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("High", pc.getSpeedString());

  pc.setMedSpeed();
  TEST_ASSERT_EQUAL(PumpController::MED_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("Medium", pc.getSpeedString());

  pc.setStop();
  TEST_ASSERT_EQUAL(PumpController::STOP, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("Stopped", pc.getSpeedString());
}

// ---------------------------------------------------------------------------
// ScheduleManager
// ---------------------------------------------------------------------------

void test_schedule_validation_accepts_normal_hours(void)
{
  ScheduleManager sm;
  TEST_ASSERT_TRUE(sm.isValidSchedule(6, 18));
  TEST_ASSERT_TRUE(sm.isValidSchedule(0, 23));
  TEST_ASSERT_TRUE(sm.isValidSchedule(23, 0));
}

void test_schedule_validation_rejects_out_of_range(void)
{
  ScheduleManager sm;
  TEST_ASSERT_FALSE(sm.isValidSchedule(24, 18));
  TEST_ASSERT_FALSE(sm.isValidSchedule(6, 24));
  TEST_ASSERT_FALSE(sm.isValidSchedule(255, 0));
}

void test_schedule_validation_rejects_equal_hours(void)
{
  // Equal on/off would make checkAndExecute() bounce between MED and LOW
  // every tick on that hour. Validation must block it.
  ScheduleManager sm;
  TEST_ASSERT_FALSE(sm.isValidSchedule(6, 6));
  TEST_ASSERT_FALSE(sm.isValidSchedule(0, 0));
}

void test_schedule_set_rejects_invalid_and_keeps_previous(void)
{
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  sm.begin(pc, mqtt, "test_ns_a");
  sm.setSchedule(7, 19);
  TEST_ASSERT_EQUAL_UINT8(7, sm.getOnHour());
  TEST_ASSERT_EQUAL_UINT8(19, sm.getOffHour());

  sm.setSchedule(25, 5); // out of range, must be ignored
  TEST_ASSERT_EQUAL_UINT8(7, sm.getOnHour());
  TEST_ASSERT_EQUAL_UINT8(19, sm.getOffHour());

  sm.setSchedule(8, 8);  // equal, must be ignored
  TEST_ASSERT_EQUAL_UINT8(7, sm.getOnHour());
  TEST_ASSERT_EQUAL_UINT8(19, sm.getOffHour());
}

void test_schedule_check_and_execute_drives_pump(void)
{
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  pc.begin(1, 2, 3, 4);
  sm.begin(pc, mqtt, "test_ns_b");
  sm.setSchedule(8, 20);

  // At ON-hour, must transition to MED and publish.
  sm.checkAndExecute(8);
  TEST_ASSERT_EQUAL(PumpController::MED_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("pump_speed", MQTTManager::lastSub.c_str());
  TEST_ASSERT_EQUAL_STRING("Medium",     MQTTManager::lastVal.c_str());

  int countAfterFirst = MQTTManager::publishCount;

  // Same hour again: already MED, must not re-trigger.
  sm.checkAndExecute(8);
  TEST_ASSERT_EQUAL(countAfterFirst, MQTTManager::publishCount);

  // At OFF-hour, must transition to LOW and publish again.
  sm.checkAndExecute(20);
  TEST_ASSERT_EQUAL(PumpController::LOW_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("Low", MQTTManager::lastVal.c_str());
}

// ---------------------------------------------------------------------------
// HeatPumpManager
// ---------------------------------------------------------------------------

void test_heatpump_initial_state_is_offline(void)
{
  HeatPumpManager hp;
  TEST_ASSERT_FALSE(hp.isOnline());
  TEST_ASSERT_EQUAL_UINT32(0u, hp.getLastUpdate());
  TEST_ASSERT_FALSE(hp.isPowerOn());
  TEST_ASSERT_EQUAL_UINT16(0, hp.getErrorCode());
  TEST_ASSERT_TRUE(std::isnan(hp.getInletTempC()));
  TEST_ASSERT_TRUE(std::isnan(hp.getOutletTempC()));
  TEST_ASSERT_TRUE(std::isnan(hp.getAmbientTempC()));
  TEST_ASSERT_TRUE(std::isnan(hp.getTargetTempC()));
  TEST_ASSERT_EQUAL(HeatPumpManager::OP_AUTO,      hp.getOperationMode());
  TEST_ASSERT_EQUAL(HeatPumpManager::SILENCE_SMART, hp.getSilenceMode());
}

void test_heatpump_result_to_string_known_codes(void)
{
  // The strings are surfaced in error-path log lines; keep them stable.
  TEST_ASSERT_EQUAL_STRING("OK",
    HeatPumpManager::resultToString(ModbusMaster::ku8MBSuccess));
  TEST_ASSERT_EQUAL_STRING("Illegal function",
    HeatPumpManager::resultToString(ModbusMaster::ku8MBIllegalFunction));
  TEST_ASSERT_EQUAL_STRING("Illegal data address",
    HeatPumpManager::resultToString(ModbusMaster::ku8MBIllegalDataAddress));
  TEST_ASSERT_EQUAL_STRING("Response timed out",
    HeatPumpManager::resultToString(ModbusMaster::ku8MBResponseTimedOut));
  TEST_ASSERT_EQUAL_STRING("Invalid CRC",
    HeatPumpManager::resultToString(ModbusMaster::ku8MBInvalidCRC));
}

void test_heatpump_result_to_string_unknown(void)
{
  TEST_ASSERT_EQUAL_STRING("Unknown error",
    HeatPumpManager::resultToString(0x7F));
}

void test_heatpump_poll_no_op_before_begin(void)
{
  // Without begin() poll() must short-circuit; otherwise we'd call into
  // an uninitialised HardwareSerial pointer.
  HeatPumpManager hp;
  TEST_ASSERT_FALSE(hp.poll());
  TEST_ASSERT_FALSE(hp.isOnline());
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(int /*argc*/, char** /*argv*/)
{
  UNITY_BEGIN();

  RUN_TEST(test_pump_initial_speed_is_no_speed);
  RUN_TEST(test_pump_speed_string_mapping);

  RUN_TEST(test_schedule_validation_accepts_normal_hours);
  RUN_TEST(test_schedule_validation_rejects_out_of_range);
  RUN_TEST(test_schedule_validation_rejects_equal_hours);
  RUN_TEST(test_schedule_set_rejects_invalid_and_keeps_previous);
  RUN_TEST(test_schedule_check_and_execute_drives_pump);

  RUN_TEST(test_heatpump_initial_state_is_offline);
  RUN_TEST(test_heatpump_result_to_string_known_codes);
  RUN_TEST(test_heatpump_result_to_string_unknown);
  RUN_TEST(test_heatpump_poll_no_op_before_begin);

  return UNITY_END();
}
