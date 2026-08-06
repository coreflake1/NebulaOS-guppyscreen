#ifndef __Z_OFFSET_CONFIG_PERSISTENCE_H__
#define __Z_OFFSET_CONFIG_PERSISTENCE_H__

#include <string>

// Pure file-I/O logic for GuppyScreen's own printer.cfg z_offset persistence path
// (backup / patch / restore) - extracted verbatim (same marker string, same file-open
// error handling, same "{:.3f}" value formatting) from the original
// RecalibrationWizardPanel::backup_printer_cfg/restore_printer_cfg_backup/
// patch_z_offset_value, purely so it's unit-testable without an LVGL display. Behavior is
// unchanged from before this extraction.
//
// This is GuppyScreen's own persistence mechanism - it patches the real, on-disk
// "#*# z_offset = " line Klipper's own SAVE_CONFIG machinery writes, and is entirely
// independent of the Klipper-side z_compensate persist_offset config option (see
// docs/z_compensate_status_api.md in the ke-mainline-klipper repo for why those two are
// deliberately decoupled).
//
// This file may be distributed under the terms of the GNU GPLv3 license.

class ZOffsetConfigPersistence {
 public:
  ZOffsetConfigPersistence(std::string config_path, std::string backup_path);

  // Copies config_path -> backup_path. Must succeed (and be called) before restore_backup()
  // can do anything - matches the original's config_backed_up guard.
  bool backup();

  // Copies backup_path -> config_path. Returns false without touching config_path at all if
  // backup() was never called or did not succeed this run.
  bool restore_backup() const;

  // Finds the marker line "#*# z_offset = " in config_path and rewrites the numeric value
  // on that line (up to the next newline) to `value`, formatted with 3 decimal places -
  // matches the original's fmt::format("{:.3f}", value) exactly. Returns false, leaving
  // config_path untouched, if the file can't be opened or the marker isn't found (e.g. no
  // SAVE_CONFIG has ever run on this printer) - this is a deliberate refusal, not a
  // best-effort append: silently inventing an unverified marker format/location would risk
  // corrupting the file's real SAVE_CONFIG block.
  bool patch_z_offset(double value) const;

  bool has_backup() const { return has_backup_; }

  static const char *marker() { return "#*# z_offset = "; }

 private:
  std::string config_path_;
  std::string backup_path_;
  bool has_backup_ = false;
};

#endif  // __Z_OFFSET_CONFIG_PERSISTENCE_H__
