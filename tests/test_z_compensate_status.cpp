// Offline tests for the pure z_compensate status parsing + correlation logic - no LVGL, no
// WebSocket, no Moonraker, no physical printer. Build/run:
//   make test-z-compensate-status
//
// This file may be distributed under the terms of the GNU GPLv3 license.
#include "minitest.h"
#include "../src/z_compensate_status.h"

#include <cmath>
#include <limits>

using json = nlohmann::json;

// ---------------------------------------------------------------------------------------
// parse_z_compensate_status: well-formed contract payloads
// ---------------------------------------------------------------------------------------

TEST(Parse, idle_is_well_formed) {
  json j = {{"calibration_id", 0}, {"calibration_state", "idle"},
            {"calibration_z_offset", nullptr}, {"calibration_error", nullptr}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ASSERT_EQ(s.id, 0);
  ASSERT_EQ(s.state, std::string("idle"));
  ASSERT_FALSE(s.has_offset);
  ASSERT_FALSE(s.has_error);
}

TEST(Parse, running_is_well_formed) {
  json j = {{"calibration_id", 1}, {"calibration_state", "running"},
            {"calibration_z_offset", nullptr}, {"calibration_error", nullptr}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ASSERT_EQ(s.state, std::string("running"));
}

TEST(Parse, complete_with_valid_offset_is_well_formed) {
  json j = {{"calibration_id", 1}, {"calibration_state", "complete"},
            {"calibration_z_offset", -0.12734}, {"calibration_error", nullptr}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ASSERT_TRUE(s.has_offset);
  ASSERT_NEAR(s.offset, -0.12734, 1e-9);
}

TEST(Parse, error_with_string_message_is_well_formed) {
  json j = {{"calibration_id", 1}, {"calibration_state", "error"},
            {"calibration_z_offset", nullptr},
            {"calibration_error", "Load-cell trigger not detected"}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ASSERT_TRUE(s.has_error);
  ASSERT_EQ(s.error, std::string("Load-cell trigger not detected"));
}

TEST(Parse, partial_update_missing_optional_fields_still_well_formed) {
  // Moonraker subscription notifications may omit unchanged fields - a running update
  // that only reports id+state (offset/error genuinely absent, not merged in by the
  // caller) must still parse as well-formed.
  json j = {{"calibration_id", 5}, {"calibration_state", "running"}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ASSERT_FALSE(s.has_offset);
  ASSERT_FALSE(s.has_error);
}

// ---------------------------------------------------------------------------------------
// parse_z_compensate_status: malformed payloads
// ---------------------------------------------------------------------------------------

TEST(Parse, missing_id_is_malformed) {
  json j = {{"calibration_state", "running"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, string_id_is_malformed) {
  json j = {{"calibration_id", "5"}, {"calibration_state", "running"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, missing_state_is_malformed) {
  json j = {{"calibration_id", 5}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, unknown_state_is_malformed) {
  json j = {{"calibration_id", 5}, {"calibration_state", "frobnicating"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, complete_with_null_offset_is_malformed) {
  json j = {{"calibration_id", 5}, {"calibration_state", "complete"},
            {"calibration_z_offset", nullptr}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, complete_with_missing_offset_field_is_malformed) {
  json j = {{"calibration_id", 5}, {"calibration_state", "complete"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, complete_with_string_offset_is_malformed) {
  json j = {{"calibration_id", 5}, {"calibration_state", "complete"},
            {"calibration_z_offset", "-0.2"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, complete_with_nan_offset_is_malformed) {
  json j = {{"calibration_id", 5}, {"calibration_state", "complete"}};
  j["calibration_z_offset"] = std::numeric_limits<double>::quiet_NaN();
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, complete_with_infinite_offset_is_malformed) {
  json j = {{"calibration_id", 5}, {"calibration_state", "complete"}};
  j["calibration_z_offset"] = std::numeric_limits<double>::infinity();
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);
}

TEST(Parse, error_with_numeric_error_value_keeps_state_drops_message) {
  // A real "error" state signal must not be discarded just because the message field
  // happens to be the wrong JSON type - see header comment for the rationale.
  json j = {{"calibration_id", 5}, {"calibration_state", "error"},
            {"calibration_error", 123}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ASSERT_EQ(s.state, std::string("error"));
  ASSERT_FALSE(s.has_error);  // malformed message tolerated as "absent", not fatal
}

TEST(Parse, non_object_input_is_malformed) {
  json arr = json::array({1, 2, 3});
  ASSERT_FALSE(parse_z_compensate_status(arr).well_formed);
  json empty;
  ASSERT_FALSE(parse_z_compensate_status(empty).well_formed);
}

// ---------------------------------------------------------------------------------------
// ZCompensateStatusTracker: correlation / decision logic
// ---------------------------------------------------------------------------------------

static ZCompensateStatus make_status(long id, const std::string &state,
                                      bool has_offset = false, double offset = 0.0,
                                      bool has_error = false, const std::string &error = "") {
  ZCompensateStatus s;
  s.well_formed = true;
  s.id = id;
  s.state = state;
  s.has_offset = has_offset;
  s.offset = offset;
  s.has_error = has_error;
  s.error = error;
  return s;
}

TEST(Tracker, inactive_tracker_ignores_everything) {
  ZCompensateStatusTracker t;
  auto out = t.on_status(make_status(1, "complete", true, -0.1));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
}

TEST(Tracker, stale_same_id_is_ignored) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(4, "complete", true, -0.1));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
}

TEST(Tracker, stale_older_id_is_ignored) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(2, "complete", true, -0.1));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
}

TEST(Tracker, malformed_status_is_ignored_not_failed) {
  ZCompensateStatusTracker t;
  t.begin(4);
  ZCompensateStatus bad;  // well_formed defaults to false
  auto out = t.on_status(bad);
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
  ASSERT_FALSE(t.has_terminal_result());
}

TEST(Tracker, newer_running_is_busy) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(5, "running"));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Busy);
  ASSERT_FALSE(t.has_terminal_result());
}

TEST(Tracker, newer_idle_is_treated_as_busy_defensively) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(5, "idle"));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Busy);
}

TEST(Tracker, newer_complete_with_finite_offset_succeeds_exactly) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(5, "complete", true, -0.12734));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Complete);
  ASSERT_NEAR(out.offset, -0.12734, 1e-9);
  ASSERT_TRUE(t.has_terminal_result());
}

TEST(Tracker, newer_error_fails_with_message) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(5, "error", false, 0.0, true, "Load-cell trigger not detected"));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Failed);
  ASSERT_EQ(out.error, std::string("Load-cell trigger not detected"));
  ASSERT_TRUE(t.has_terminal_result());
}

TEST(Tracker, error_without_message_gets_generic_text) {
  ZCompensateStatusTracker t;
  t.begin(4);
  auto out = t.on_status(make_status(5, "error"));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Failed);
  ASSERT_FALSE(out.error.empty());
}

TEST(Tracker, terminal_state_ignores_all_further_updates) {
  ZCompensateStatusTracker t;
  t.begin(4);
  t.on_status(make_status(5, "complete", true, -0.1));
  ASSERT_TRUE(t.has_terminal_result());

  // duplicate complete
  auto dup = t.on_status(make_status(5, "complete", true, -0.1));
  ASSERT_TRUE(dup.decision == ZCalibrationDecision::Ignore);

  // a "better" complete for a newer id must still be ignored - one terminal result per run
  auto newer = t.on_status(make_status(6, "complete", true, -0.999));
  ASSERT_TRUE(newer.decision == ZCalibrationDecision::Ignore);

  // error arriving after a completed result must not flip anything
  auto err = t.on_status(make_status(6, "error", false, 0.0, true, "late error"));
  ASSERT_TRUE(err.decision == ZCalibrationDecision::Ignore);
}

TEST(Tracker, complete_after_already_failed_is_ignored) {
  ZCompensateStatusTracker t;
  t.begin(4);
  t.on_status(make_status(5, "error", false, 0.0, true, "first failure"));
  ASSERT_TRUE(t.has_terminal_result());
  auto out = t.on_status(make_status(6, "complete", true, -0.1));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
}

TEST(Tracker, fail_command_before_any_status_wins_first) {
  ZCompensateStatusTracker t;
  t.begin(4);
  ZCalibrationOutcome out;
  bool acted = t.fail_command("printer.gcode.script JSON-RPC error", out);
  ASSERT_TRUE(acted);
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Failed);
  ASSERT_EQ(out.error, std::string("printer.gcode.script JSON-RPC error"));
  ASSERT_TRUE(t.has_terminal_result());
}

TEST(Tracker, fail_command_after_terminal_is_ignored_deterministic_precedence) {
  ZCompensateStatusTracker t;
  t.begin(4);
  t.on_status(make_status(5, "complete", true, -0.1));
  ZCalibrationOutcome out;
  bool acted = t.fail_command("late command error", out);
  ASSERT_FALSE(acted);  // first terminal signal (the structured complete) already won
}

TEST(Tracker, structured_error_after_fail_command_is_ignored) {
  ZCompensateStatusTracker t;
  t.begin(4);
  ZCalibrationOutcome out;
  t.fail_command("command-level failure", out);
  auto later = t.on_status(make_status(5, "error", false, 0.0, true, "structured failure"));
  ASSERT_TRUE(later.decision == ZCalibrationDecision::Ignore);  // duplicate signal ignored
}

TEST(Tracker, inactive_after_reset_ignores_status) {
  ZCompensateStatusTracker t;
  t.begin(4);
  t.on_status(make_status(5, "complete", true, -0.1));
  t.reset();
  ASSERT_FALSE(t.active());
  auto out = t.on_status(make_status(6, "running"));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
}

TEST(Tracker, reusable_across_repeated_wizard_runs) {
  ZCompensateStatusTracker t;
  t.begin(0);
  auto first = t.on_status(make_status(1, "complete", true, 0.05));
  ASSERT_TRUE(first.decision == ZCalibrationDecision::Complete);

  // second wizard run: begin() again with the id observed just before re-issuing the
  // command (which is whatever the first run left behind, id=1).
  t.begin(1);
  ASSERT_FALSE(t.has_terminal_result());
  auto second = t.on_status(make_status(2, "complete", true, 0.30));
  ASSERT_TRUE(second.decision == ZCalibrationDecision::Complete);
  ASSERT_NEAR(second.offset, 0.30, 1e-9);
}

TEST(Tracker, out_of_order_complete_for_old_id_after_running_for_new_id) {
  ZCompensateStatusTracker t;
  t.begin(4);
  t.on_status(make_status(5, "running"));
  auto stale_complete = t.on_status(make_status(4, "complete", true, -0.1));
  ASSERT_TRUE(stale_complete.decision == ZCalibrationDecision::Ignore);
  ASSERT_FALSE(t.has_terminal_result());
  // the real (newer-id) completion must still be accepted afterward
  auto real_complete = t.on_status(make_status(5, "complete", true, -0.2));
  ASSERT_TRUE(real_complete.decision == ZCalibrationDecision::Complete);
}

// ---------------------------------------------------------------------------------------
// Legacy text has no effect - the machine interface no longer accepts strings at all.
// ---------------------------------------------------------------------------------------

TEST(LegacyTextIgnored, terminal_shaped_z_offset_line_does_not_parse_as_status) {
  // What the *old* wizard used to scan gcode response lines for. Feeding the literal old
  // text through the *new* JSON-based entry point (as if some caller mistakenly tried to
  // route console text into it) must fail to parse - there is no field named "line" or
  // "message" in the contract, so this can never be mistaken for a real snapshot.
  json j = {{"line", "z_offset:-0.200"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);

  ZCompensateStatusTracker t;
  t.begin(0);
  auto out = t.on_status(parse_z_compensate_status(j));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);
  ASSERT_FALSE(t.has_terminal_result());
}

TEST(LegacyTextIgnored, terminal_shaped_pr_err_code_line_does_not_trigger_failure) {
  json j = {{"message", "PR_ERR_CODE_PRES_LOST_RUN_DATA"}};
  ASSERT_FALSE(parse_z_compensate_status(j).well_formed);

  ZCompensateStatusTracker t;
  t.begin(0);
  auto out = t.on_status(parse_z_compensate_status(j));
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Ignore);  // NOT Failed
  ASSERT_FALSE(t.has_terminal_result());
}

TEST(LegacyTextIgnored, both_legacy_strings_embedded_in_an_otherwise_valid_snapshot_are_inert) {
  // Even if "z_offset:" / "PR_ERR_CODE" text shows up *inside* a real field (e.g. an error
  // message that happens to echo old terminal phrasing), it has no special meaning to the
  // parser or tracker - only the structured calibration_state field decides the outcome.
  json j = {{"calibration_id", 1}, {"calibration_state", "running"},
            {"calibration_error", "previous line was 'z_offset:-0.2 PR_ERR_CODE_5', ignored"}};
  auto s = parse_z_compensate_status(j);
  ASSERT_TRUE(s.well_formed);
  ZCompensateStatusTracker t;
  t.begin(0);
  auto out = t.on_status(s);
  ASSERT_TRUE(out.decision == ZCalibrationDecision::Busy);  // driven by "running", not the text
}

int main() {
  return minitest::run_all("z_compensate_status") > 0 ? 1 : 0;
}
