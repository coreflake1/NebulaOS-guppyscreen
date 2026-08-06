// Focused regression test for the subscription-baseline ordering guarantee (deployment
// verification task, 2026-08-06, Part 3): "the current backend calibration_id must have
// been received from Moonraker and stored before Z_OFFSET_CALIBRATION can be sent."
//
// This exercises the REAL State class (src/state.{h,cpp}) exactly as
// RecalibrationWizardPanel::start_sensor_refine() (src/recalibration_wizard_panel.cpp)
// and InitPanel::connected() (src/init_panel.cpp) actually use it - no mock, no
// reimplementation of State's merge semantics. The guard expression in
// baseline_readiness_guard() below is a deliberate literal copy of
// start_sensor_refine()'s own condition (recalibration_wizard_panel.cpp, the
// `z_compensate_status.is_null() || !z_compensate_status.contains("calibration_id")`
// check) - if that guard is ever edited, this copy must be updated to match, or this test
// stops proving anything about the real code path.
//
// State's own methods have zero LVGL runtime calls (verified: state.cpp includes
// lvgl/lvgl.h only for the header-only GUPPY_COLORS palette table, never calls an lv_*
// function) - safe to link and run as a plain host binary, no display needed.
//
// Build/run: make test-subscription-baseline-ordering
//
// This file may be distributed under the terms of the GNU GPLv3 license.
#include "minitest.h"
#include "../src/state.h"

#include <mutex>

namespace {

std::mutex g_lock;

// Literal copy of start_sensor_refine()'s own guard - see file header comment.
bool baseline_readiness_guard(const json &z_compensate_status) {
  return !(z_compensate_status.is_null() || !z_compensate_status.contains("calibration_id"));
}

// The exact shape init_panel.cpp's printer.objects.subscribe callback merges in:
// State::get_instance()->set_data("printer_state", data, "/result/status") where `data`
// is the full JSON-RPC response envelope.
json make_subscribe_response(const json &status_body) {
  return json{{"result", {{"status", status_body}}}};
}

}  // namespace

TEST(BaselineOrdering, fresh_state_has_no_baseline_command_must_not_send) {
  // Mirrors app startup before any printer.objects.subscribe response has arrived, and
  // is also what a mid-session State::reset() (InitPanel::connected(), called on every
  // reconnect) produces immediately afterward.
  State::get_instance()->reset();
  auto status = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(status.is_null());
  ASSERT_FALSE(baseline_readiness_guard(status));
}

TEST(BaselineOrdering, after_subscribe_response_baseline_is_available) {
  State::get_instance()->reset();
  json body = {{"z_compensate", {{"calibration_id", 0}, {"calibration_state", "idle"},
                                  {"calibration_z_offset", nullptr}, {"calibration_error", nullptr}}}};
  json resp = make_subscribe_response(body);
  State::get_instance()->set_data("printer_state", resp, "/result/status");

  auto status = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_FALSE(status.is_null());
  ASSERT_TRUE(baseline_readiness_guard(status));
  ASSERT_TRUE(status["calibration_id"].is_number_integer());
  ASSERT_EQ(status["calibration_id"].get<long>(), 0L);
}

TEST(BaselineOrdering, z_compensate_not_in_subscribed_objects_stays_blocked) {
  // The extra genuinely isn't loaded on this printer (or an older Klipper without the
  // status contract) - the subscribe response arrives, but its status body has no
  // z_compensate key at all, distinct from "hasn't reported yet."
  State::get_instance()->reset();
  json body = {{"toolhead", {{"homed_axes", ""}}}};
  json resp = make_subscribe_response(body);
  State::get_instance()->set_data("printer_state", resp, "/result/status");

  auto status = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(status.is_null());
  ASSERT_FALSE(baseline_readiness_guard(status));
}

TEST(BaselineOrdering, status_object_without_calibration_id_stays_blocked) {
  // A malformed/older backend that publishes the section but not yet the id field -
  // must not be treated as baseline-ready.
  State::get_instance()->reset();
  json body = {{"z_compensate", {{"calibration_state", "idle"}}}};
  json resp = make_subscribe_response(body);
  State::get_instance()->set_data("printer_state", resp, "/result/status");

  auto status = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_FALSE(status.is_null());
  ASSERT_FALSE(baseline_readiness_guard(status));
}

TEST(BaselineOrdering, reconnect_reset_clears_any_prior_baseline_before_resubscribe) {
  // Simulates: connected -> subscribe response (baseline id=3) -> disconnect ->
  // reconnect -> InitPanel::connected() calls State::reset() first, exactly as
  // init_panel.cpp does, BEFORE any new subscribe response can arrive. A wizard read
  // in that window must see "not ready," never the stale id=3 from before the drop.
  State::get_instance()->reset();
  json body1 = {{"z_compensate", {{"calibration_id", 3}, {"calibration_state", "idle"}}}};
  json resp1 = make_subscribe_response(body1);
  State::get_instance()->set_data("printer_state", resp1, "/result/status");
  auto pre_disconnect = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(baseline_readiness_guard(pre_disconnect));
  ASSERT_EQ(pre_disconnect["calibration_id"].get<long>(), 3L);

  // Reconnect: InitPanel::connected()'s first action.
  State::get_instance()->reset();
  auto mid_reconnect = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(mid_reconnect.is_null());
  ASSERT_FALSE(baseline_readiness_guard(mid_reconnect));  // must not fall back to id=3

  // Fresh subscribe response after reconnect - new real baseline, not the old one.
  json body2 = {{"z_compensate", {{"calibration_id", 3}, {"calibration_state", "idle"}}}};
  json resp2 = make_subscribe_response(body2);
  State::get_instance()->set_data("printer_state", resp2, "/result/status");
  auto post_reconnect = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(baseline_readiness_guard(post_reconnect));
}

TEST(BaselineOrdering, duplicate_subscribe_response_is_idempotent) {
  State::get_instance()->reset();
  json body = {{"z_compensate", {{"calibration_id", 0}, {"calibration_state", "idle"}}}};
  json resp = make_subscribe_response(body);
  State::get_instance()->set_data("printer_state", resp, "/result/status");
  State::get_instance()->set_data("printer_state", resp, "/result/status");  // duplicate

  auto status = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(baseline_readiness_guard(status));
  ASSERT_EQ(status["calibration_id"].get<long>(), 0L);
}

TEST(BaselineOrdering, partial_notify_update_after_subscribe_preserves_prior_fields) {
  // consume() (State's NotifyConsumer override) merge_patches "/params/0" into the same
  // "printer_state" key the initial subscribe response used - proving a partial
  // notify_status_update that doesn't even mention z_compensate can't erase a baseline
  // already established by the initial full snapshot.
  State::get_instance()->reset();
  json body = {{"z_compensate", {{"calibration_id", 5}, {"calibration_state", "idle"}}}};
  json resp = make_subscribe_response(body);
  State::get_instance()->set_data("printer_state", resp, "/result/status");

  json partial_notify = {{"params", json::array({{{"toolhead", {{"homed_axes", "xyz"}}}}})}};
  State::get_instance()->consume(partial_notify);

  auto status = State::get_instance()->get_data("/printer_state/z_compensate"_json_pointer);
  ASSERT_TRUE(baseline_readiness_guard(status));
  ASSERT_EQ(status["calibration_id"].get<long>(), 5L);
}

int main() {
  return minitest::run_all("subscription_baseline_ordering") > 0 ? 1 : 0;
}
