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
#include "MQTTManager.h"

void setUp(void)
{
  // Reset programmable mock state between tests so order of execution
  // can never cause cross-test contamination.
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

void test_pump_begin_does_not_change_speed(void)
{
  // begin() wires GPIO only — speed must remain NO_SPEED afterwards.
  PumpController pc;
  pc.begin(1, 2, 3, 4);
  TEST_ASSERT_EQUAL(PumpController::NO_SPEED, pc.getCurrentSpeed());
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

void test_pump_repeated_speed_change_updates_state(void)
{
  // Changing speed multiple times must always reflect the last command.
  PumpController pc;
  pc.begin(1, 2, 3, 4);

  pc.setHighSpeed();
  pc.setLowSpeed();
  pc.setHighSpeed();
  TEST_ASSERT_EQUAL(PumpController::HIGH_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("High", pc.getSpeedString());
}

void test_pump_stop_from_running(void)
{
  PumpController pc;
  pc.begin(1, 2, 3, 4);
  pc.setMedSpeed();
  TEST_ASSERT_EQUAL(PumpController::MED_SPEED, pc.getCurrentSpeed());
  pc.setStop();
  TEST_ASSERT_EQUAL(PumpController::STOP, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL_STRING("Stopped", pc.getSpeedString());
}

// ---------------------------------------------------------------------------
// ScheduleManager
// ---------------------------------------------------------------------------

void test_schedule_defaults_are_6_and_18(void)
{
  // Fresh instance must expose 6:00 ON / 18:00 OFF before begin() is called.
  ScheduleManager sm;
  TEST_ASSERT_EQUAL_UINT8(6,  sm.getOnHour());
  TEST_ASSERT_EQUAL_UINT8(18, sm.getOffHour());
}

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

void test_schedule_midnight_wrap_is_valid(void)
{
  // on=23 / off=0 is a legitimate "overnight" schedule.
  ScheduleManager sm;
  TEST_ASSERT_TRUE(sm.isValidSchedule(23, 0));
  TEST_ASSERT_TRUE(sm.isValidSchedule(0, 1));
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

void test_schedule_set_accepts_midnight_off(void)
{
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  sm.begin(pc, mqtt, "test_ns_midnight");
  sm.setSchedule(23, 0);
  TEST_ASSERT_EQUAL_UINT8(23, sm.getOnHour());
  TEST_ASSERT_EQUAL_UINT8(0,  sm.getOffHour());
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

void test_schedule_check_no_op_at_unrelated_hour(void)
{
  // An hour that matches neither boundary must not change pump state or publish.
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  pc.begin(1, 2, 3, 4);
  sm.begin(pc, mqtt, "test_ns_c");
  sm.setSchedule(8, 20);

  sm.checkAndExecute(10); // unrelated hour
  TEST_ASSERT_EQUAL(PumpController::NO_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL(0, MQTTManager::publishCount);
}

void test_schedule_check_on_hour_from_stopped_state(void)
{
  // STOP != MED_SPEED, so on-hour must fire even when pump was explicitly stopped.
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  pc.begin(1, 2, 3, 4);
  pc.setStop();
  sm.begin(pc, mqtt, "test_ns_d");
  sm.setSchedule(9, 21);

  sm.checkAndExecute(9);
  TEST_ASSERT_EQUAL(PumpController::MED_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL(1, MQTTManager::publishCount);
}

void test_schedule_check_off_hour_idempotent(void)
{
  // If the pump is already at LOW when the off-hour fires, nothing should publish.
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  pc.begin(1, 2, 3, 4);
  pc.setLowSpeed(); // already at the off-hour target
  sm.begin(pc, mqtt, "test_ns_e");
  sm.setSchedule(8, 20);

  MQTTManager::publishCount = 0; // reset after setLowSpeed side-effects above
  sm.checkAndExecute(20);
  TEST_ASSERT_EQUAL(PumpController::LOW_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL(0, MQTTManager::publishCount); // no redundant publish
}

void test_schedule_check_before_begin_is_safe(void)
{
  // checkAndExecute() without begin() must not crash (null-ptr guard).
  ScheduleManager sm;
  sm.checkAndExecute(6); // should return safely
  // Reaching here without a crash/fault is the assertion.
  TEST_ASSERT_TRUE(true);
}

void test_schedule_check_at_midnight_on_hour(void)
{
  // Schedule with on=0 must trigger at hour 0 (midnight).
  PumpController pc;
  MQTTManager mqtt;
  ScheduleManager sm;
  pc.begin(1, 2, 3, 4);
  sm.begin(pc, mqtt, "test_ns_f");
  sm.setSchedule(0, 12);

  sm.checkAndExecute(0);
  TEST_ASSERT_EQUAL(PumpController::MED_SPEED, pc.getCurrentSpeed());
  TEST_ASSERT_EQUAL(1, MQTTManager::publishCount);
}

// ---------------------------------------------------------------------------
// Runner
// ---------------------------------------------------------------------------

int main(int /*argc*/, char** /*argv*/)
{
  UNITY_BEGIN();

  RUN_TEST(test_pump_initial_speed_is_no_speed);
  RUN_TEST(test_pump_begin_does_not_change_speed);
  RUN_TEST(test_pump_speed_string_mapping);
  RUN_TEST(test_pump_repeated_speed_change_updates_state);
  RUN_TEST(test_pump_stop_from_running);

  RUN_TEST(test_schedule_defaults_are_6_and_18);
  RUN_TEST(test_schedule_validation_accepts_normal_hours);
  RUN_TEST(test_schedule_validation_rejects_out_of_range);
  RUN_TEST(test_schedule_validation_rejects_equal_hours);
  RUN_TEST(test_schedule_midnight_wrap_is_valid);
  RUN_TEST(test_schedule_set_rejects_invalid_and_keeps_previous);
  RUN_TEST(test_schedule_set_accepts_midnight_off);
  RUN_TEST(test_schedule_check_and_execute_drives_pump);
  RUN_TEST(test_schedule_check_no_op_at_unrelated_hour);
  RUN_TEST(test_schedule_check_on_hour_from_stopped_state);
  RUN_TEST(test_schedule_check_off_hour_idempotent);
  RUN_TEST(test_schedule_check_before_begin_is_safe);
  RUN_TEST(test_schedule_check_at_midnight_on_hour);

  return UNITY_END();
}
