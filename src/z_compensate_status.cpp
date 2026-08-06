#include "z_compensate_status.h"

#include <cmath>
#include <set>

ZCompensateStatus parse_z_compensate_status(const json &snapshot) {
  ZCompensateStatus s;
  if (!snapshot.is_object()) {
    return s;
  }

  auto id_it = snapshot.find("calibration_id");
  if (id_it == snapshot.end() || !id_it->is_number_integer()) {
    return s;
  }
  s.id = id_it->template get<long>();

  auto state_it = snapshot.find("calibration_state");
  if (state_it == snapshot.end() || !state_it->is_string()) {
    return s;
  }
  s.state = state_it->template get<std::string>();
  static const std::set<std::string> known_states = {"idle", "running", "complete", "error"};
  if (known_states.find(s.state) == known_states.end()) {
    return s;
  }

  bool offset_present_and_valid = false;
  auto offset_it = snapshot.find("calibration_z_offset");
  if (offset_it != snapshot.end() && !offset_it->is_null() && offset_it->is_number()) {
    double v = offset_it->template get<double>();
    if (std::isfinite(v)) {
      s.has_offset = true;
      s.offset = v;
      offset_present_and_valid = true;
    }
  }

  auto error_it = snapshot.find("calibration_error");
  if (error_it != snapshot.end() && !error_it->is_null() && error_it->is_string()) {
    s.has_error = true;
    s.error = error_it->template get<std::string>();
  }

  if (s.state == "complete" && !offset_present_and_valid) {
    // "complete" with a missing/malformed/NaN/infinite offset is not a well-formed
    // snapshot - the task contract explicitly disallows accepting this as success, and
    // there's no other honest way to interpret it (see header comment).
    return s;
  }

  s.well_formed = true;
  return s;
}

void ZCompensateStatusTracker::begin(long baseline_id) {
  baseline_id_ = baseline_id;
  active_ = true;
  terminal_ = false;
}

void ZCompensateStatusTracker::reset() {
  active_ = false;
  terminal_ = false;
  baseline_id_ = -1;
}

ZCalibrationOutcome ZCompensateStatusTracker::on_status(const ZCompensateStatus &status) {
  ZCalibrationOutcome out;
  if (!active_ || terminal_) {
    return out;  // Ignore: no active invocation, or already reached a terminal result
  }
  if (!status.well_formed) {
    return out;  // Ignore: can't correlate a malformed update - keep waiting, timeout is
                 // the real backstop for "nothing sensible ever arrives"
  }
  if (status.id <= baseline_id_) {
    return out;  // Ignore: stale - same or older id than the baseline captured in begin()
  }

  if (status.state == "error") {
    terminal_ = true;
    out.decision = ZCalibrationDecision::Failed;
    out.error = status.has_error ? status.error : std::string("calibration failed");
    return out;
  }
  if (status.state == "complete") {
    // parse_z_compensate_status() already guarantees has_offset is true here - "complete"
    // with no valid offset was rejected at parse time, never reaches this branch.
    terminal_ = true;
    out.decision = ZCalibrationDecision::Complete;
    out.offset = status.offset;
    return out;
  }
  // "running" (the expected case) or "idle" (shouldn't occur for a newer id in practice,
  // but defensively treated the same way rather than as anything terminal) - both mean
  // "still busy, keep waiting."
  out.decision = ZCalibrationDecision::Busy;
  return out;
}

bool ZCompensateStatusTracker::fail_command(const std::string &error, ZCalibrationOutcome &out) {
  if (!active_ || terminal_) {
    return false;
  }
  terminal_ = true;
  out.decision = ZCalibrationDecision::Failed;
  out.error = error;
  return true;
}
