// Cross-project contract test (Part 7): reads the literal fixture copied from
// ke-mainline-klipper's own generated docs/z_compensate_status_contract_fixture.json
// (see tests/fixtures/README.md for provenance) and drives it through the real C++
// parser/tracker - proving the backend's actual get_status() output and this frontend's
// actual parsing code agree on the wire contract, without linking the two languages
// together in one process (impractical here; the documented serialized-fixture boundary
// is what Part 7 explicitly permits for a contract this small and stable).
//
// Build/run: make test-contract-fixture (invoked from the repo root, so the relative
// fixture path below resolves correctly - matches how `make test` runs every other
// test target).
//
// This file may be distributed under the terms of the GNU GPLv3 license.
#include "minitest.h"
#include "../src/z_compensate_status.h"

#include <fstream>
#include <sstream>
#include <string>

namespace {

const char *kFixturePath = "tests/fixtures/z_compensate_status_contract.json";

json load_fixture() {
  std::ifstream f(kFixturePath, std::ios::binary);
  ASSERT_TRUE(static_cast<bool>(f));
  std::ostringstream oss;
  oss << f.rdbuf();
  return json::parse(oss.str());
}

}  // namespace

TEST(ContractFixture, idle_parses_well_formed_and_tracker_ignores_it) {
  auto fixture = load_fixture();
  auto status = parse_z_compensate_status(fixture.at("idle"));
  ASSERT_TRUE(status.well_formed);
  ASSERT_EQ(status.id, 0L);
  ASSERT_EQ(status.state, std::string("idle"));
  ASSERT_FALSE(status.has_offset);
  ASSERT_FALSE(status.has_error);

  // idle's id (0) is not newer than a tracker baselined at 0 - correctly ignored, this
  // is the pre-invocation snapshot the wizard observes before ever sending a command.
  ZCompensateStatusTracker tracker;
  tracker.begin(0);
  auto outcome = tracker.on_status(status);
  ASSERT_TRUE(outcome.decision == ZCalibrationDecision::Ignore);
}

TEST(ContractFixture, running_parses_well_formed_and_tracker_reports_busy) {
  auto fixture = load_fixture();
  auto status = parse_z_compensate_status(fixture.at("running"));
  ASSERT_TRUE(status.well_formed);
  ASSERT_EQ(status.id, 1L);
  ASSERT_EQ(status.state, std::string("running"));
  ASSERT_FALSE(status.has_offset);
  ASSERT_FALSE(status.has_error);

  ZCompensateStatusTracker tracker;
  tracker.begin(0);
  auto outcome = tracker.on_status(status);
  ASSERT_TRUE(outcome.decision == ZCalibrationDecision::Busy);
  ASSERT_FALSE(tracker.has_terminal_result());
}

TEST(ContractFixture, complete_parses_well_formed_and_tracker_reports_exact_offset) {
  auto fixture = load_fixture();
  auto status = parse_z_compensate_status(fixture.at("complete"));
  ASSERT_TRUE(status.well_formed);
  ASSERT_EQ(status.id, 1L);
  ASSERT_EQ(status.state, std::string("complete"));
  ASSERT_TRUE(status.has_offset);
  ASSERT_NEAR(status.offset, -0.12734, 1e-9);
  ASSERT_FALSE(status.has_error);

  ZCompensateStatusTracker tracker;
  tracker.begin(0);
  auto outcome = tracker.on_status(status);
  ASSERT_TRUE(outcome.decision == ZCalibrationDecision::Complete);
  ASSERT_NEAR(outcome.offset, -0.12734, 1e-9);
  ASSERT_TRUE(tracker.has_terminal_result());
}

TEST(ContractFixture, error_parses_well_formed_and_tracker_reports_exact_message) {
  auto fixture = load_fixture();
  auto status = parse_z_compensate_status(fixture.at("error"));
  ASSERT_TRUE(status.well_formed);
  ASSERT_EQ(status.id, 1L);
  ASSERT_EQ(status.state, std::string("error"));
  ASSERT_FALSE(status.has_offset);
  ASSERT_TRUE(status.has_error);
  ASSERT_EQ(status.error, std::string("Load-cell trigger not detected"));

  ZCompensateStatusTracker tracker;
  tracker.begin(0);
  auto outcome = tracker.on_status(status);
  ASSERT_TRUE(outcome.decision == ZCalibrationDecision::Failed);
  ASSERT_EQ(outcome.error, std::string("Load-cell trigger not detected"));
  ASSERT_TRUE(tracker.has_terminal_result());
}

TEST(ContractFixture, full_lifecycle_replay_in_wire_order_matches_a_real_wizard_run) {
  // idle (pre-invocation) -> running -> complete, fed through one tracker in the exact
  // order a real wizard run would observe them, baselined the way start_sensor_refine()
  // actually baselines: from the id seen in the idle snapshot captured before the
  // Z_OFFSET_CALIBRATION command is ever sent.
  auto fixture = load_fixture();
  auto idle = parse_z_compensate_status(fixture.at("idle"));
  auto running = parse_z_compensate_status(fixture.at("running"));
  auto complete = parse_z_compensate_status(fixture.at("complete"));

  ZCompensateStatusTracker tracker;
  tracker.begin(idle.id);

  auto o1 = tracker.on_status(running);
  ASSERT_TRUE(o1.decision == ZCalibrationDecision::Busy);

  auto o2 = tracker.on_status(complete);
  ASSERT_TRUE(o2.decision == ZCalibrationDecision::Complete);
  ASSERT_NEAR(o2.offset, -0.12734, 1e-9);
}

int main() {
  return minitest::run_all("contract_fixture") > 0 ? 1 : 0;
}
