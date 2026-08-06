#ifndef __Z_COMPENSATE_STATUS_H__
#define __Z_COMPENSATE_STATUS_H__

#include "hv/json.hpp"

#include <string>

using json = nlohmann::json;

// Pure, LVGL-free parsing + correlation logic for the z_compensate structured status
// contract (see docs/z_compensate_status_api.md in the ke-mainline-klipper repo:
// calibration_id/calibration_state/calibration_z_offset/calibration_error via
// printer.objects.subscribe). Deliberately factored out of RecalibrationWizardPanel so the
// part that actually *decides* what an update means - keep waiting, you're done, you
// failed - is unit-testable as a plain host binary: no LVGL display, no WebSocket, no
// Moonraker. RecalibrationWizardPanel owns one of these and translates its decisions into
// UI actions; it should not reimplement any of this logic itself.
//
// This file may be distributed under the terms of the GNU GPLv3 license.

// One fully-merged z_compensate status snapshot. "Fully merged" matters: Moonraker
// subscription notifications may carry only the fields that changed since the last push
// (see docs/z_compensate_status_api.md's "partial update handling" section) - the caller
// is responsible for merging incremental patches into a complete view before calling
// parse_z_compensate_status() (this project's existing State class, which already does
// exactly this via nlohmann::json::merge_patch for every other subscribed object, is the
// intended source for that merged view - see recalibration_wizard_panel.cpp).
struct ZCompensateStatus {
  bool well_formed = false;
  long id = -1;
  std::string state;
  bool has_offset = false;
  double offset = 0.0;
  bool has_error = false;
  std::string error;
};

// Validates one merged snapshot against the v1 contract. well_formed is false for: a
// missing/non-integer id; a missing/non-string/unrecognized state; or state=="complete"
// paired with a missing, non-numeric, NaN, or infinite offset. A malformed
// calibration_error field is tolerated (treated as absent, not as invalidating the whole
// snapshot) - a real "error" state signal should not be discarded just because its
// accompanying message happens to be the wrong JSON type; the state itself is what matters
// most for correctness, and losing a badly-typed message string is a far smaller problem
// than silently ignoring a genuine backend failure.
ZCompensateStatus parse_z_compensate_status(const json &snapshot);

enum class ZCalibrationDecision { Ignore, Busy, Complete, Failed };

struct ZCalibrationOutcome {
  ZCalibrationDecision decision = ZCalibrationDecision::Ignore;
  double offset = 0.0;
  std::string error;
};

// Correlates a sequence of status snapshots (or a direct JSON-RPC command failure) against
// one active Z_OFFSET_CALIBRATION invocation. One instance per wizard run: call begin()
// with the calibration_id observed *before* sending the command, then feed every
// subsequent snapshot through on_status(). Once a terminal outcome (Complete or Failed) has
// been produced, every later call is ignored - "the wizard must have a terminal state and
// ignore later contradictory events" (docs/z_compensate_status_api.md).
class ZCompensateStatusTracker {
 public:
  void begin(long baseline_id);
  void reset();
  bool active() const { return active_; }
  bool has_terminal_result() const { return terminal_; }

  // status.id > baseline_id_ is required to be considered "about this invocation" at all -
  // anything else (stale, malformed, not yet active, already terminal) decision-defaults to
  // Ignore. A newer id in "idle" or "running" both mean Busy (see .cpp for why "idle" is
  // included defensively even though it shouldn't occur in practice for a newer id).
  ZCalibrationOutcome on_status(const ZCompensateStatus &status);

  // A JSON-RPC-level command failure (e.g. printer.gcode.script's own response carrying an
  // "error" key) - not correlated to any calibration_id, since the command never ran far
  // enough to publish one. Returns true the first time this is called for an active,
  // non-terminal invocation (the caller should act on `out`); false otherwise (a duplicate
  // or out-of-context signal - the caller must not act on `out`, its contents are
  // unspecified). Implements the "first terminal failure wins" precedence rule.
  bool fail_command(const std::string &error, ZCalibrationOutcome &out);

 private:
  bool active_ = false;
  bool terminal_ = false;
  long baseline_id_ = -1;
};

#endif  // __Z_COMPENSATE_STATUS_H__
