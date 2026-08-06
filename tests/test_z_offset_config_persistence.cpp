// Offline tests for ZOffsetConfigPersistence - plain file I/O against real temp files, no
// LVGL, no WebSocket, no real printer.cfg touched. Build/run: make test-z-offset-persistence
//
// This file may be distributed under the terms of the GNU GPLv3 license.
#include "minitest.h"
#include "../src/z_offset_config_persistence.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string make_tmpdir() {
  char tmpl[] = "/tmp/guppyscreen_zoffset_test_XXXXXX";
  char *dir = mkdtemp(tmpl);
  ASSERT_TRUE(dir != nullptr);
  return std::string(dir);
}

void write_file(const std::string &path, const std::string &content) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  f << content;
}

std::string read_file(const std::string &path) {
  std::ifstream f(path, std::ios::binary);
  std::ostringstream oss;
  oss << f.rdbuf();
  return oss.str();
}

bool file_exists(const std::string &path) {
  struct stat st;
  return ::stat(path.c_str(), &st) == 0;
}

}  // namespace

static const std::string kRealisticConfig =
    "[bltouch]\n"
    "x_offset: 0\n"
    "y_offset: 27\n"
    "\n"
    "#*# <---------------------- SAVE_CONFIG ---------------------->\n"
    "#*# DO NOT EDIT THIS BLOCK OR BELOW\n"
    "#*#\n"
    "#*# [bltouch]\n"
    "#*# z_offset = -0.200\n"
    "#*#\n"
    "#*# [bed_mesh default]\n"
    "#*# version = 1\n";

TEST(Persistence, marker_exists_patch_succeeds) {
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";
  write_file(cfg, kRealisticConfig);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_TRUE(p.patch_z_offset(-0.12734));
  auto content = read_file(cfg);
  ASSERT_TRUE(content.find("#*# z_offset = -0.127") != std::string::npos);
  // everything else in the file must be untouched
  ASSERT_TRUE(content.find("[bltouch]\nx_offset: 0") != std::string::npos);
  ASSERT_TRUE(content.find("[bed_mesh default]") != std::string::npos);
}

TEST(Persistence, marker_missing_fails_without_modifying_file) {
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";
  std::string no_marker = "[bltouch]\nx_offset: 0\ny_offset: 27\n";
  write_file(cfg, no_marker);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_FALSE(p.patch_z_offset(-0.1));
  ASSERT_EQ(read_file(cfg), no_marker);  // untouched - refuse, don't guess/append
}

TEST(Persistence, malformed_marker_missing_trailing_space_is_not_matched) {
  // the marker is the exact literal "#*# z_offset = " (with trailing space) - a
  // near-miss like "#*#z_offset=" must not be treated as a match.
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";
  std::string malformed = "[bltouch]\n#*#z_offset=-0.2\n";
  write_file(cfg, malformed);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_FALSE(p.patch_z_offset(-0.1));
  ASSERT_EQ(read_file(cfg), malformed);
}

TEST(Persistence, multiple_markers_only_first_occurrence_is_patched) {
  // std::string::find() always returns the first occurrence - documenting that as the
  // real, current behavior rather than assuming it. A real printer.cfg should never
  // legitimately have two SAVE_CONFIG blocks, but a hand-edited or corrupted file might.
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";
  std::string dup = "#*# z_offset = -0.100\nsome other line\n#*# z_offset = -0.200\n";
  write_file(cfg, dup);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_TRUE(p.patch_z_offset(0.5));
  auto content = read_file(cfg);
  ASSERT_TRUE(content.find("#*# z_offset = 0.500") != std::string::npos);
  ASSERT_TRUE(content.find("#*# z_offset = -0.200") != std::string::npos);  // 2nd untouched
}

TEST(Persistence, write_failure_when_config_path_is_a_directory) {
  auto dir = make_tmpdir();
  std::string cfg = dir;  // a directory, not a file - ofstream open must fail
  std::string bak = dir + "/printer.cfg.bak";

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_FALSE(p.patch_z_offset(-0.1));
}

TEST(Persistence, backup_then_restore_round_trips_exactly) {
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";
  write_file(cfg, kRealisticConfig);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_TRUE(p.backup());
  ASSERT_TRUE(p.has_backup());
  ASSERT_TRUE(file_exists(bak));

  ASSERT_TRUE(p.patch_z_offset(9.999));
  ASSERT_TRUE(read_file(cfg).find("9.999") != std::string::npos);

  ASSERT_TRUE(p.restore_backup());
  ASSERT_EQ(read_file(cfg), kRealisticConfig);
}

TEST(Persistence, restore_without_prior_backup_fails_and_does_not_touch_config) {
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";  // never created
  write_file(cfg, kRealisticConfig);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_FALSE(p.has_backup());
  ASSERT_FALSE(p.restore_backup());
  ASSERT_EQ(read_file(cfg), kRealisticConfig);
}

TEST(Persistence, backup_of_missing_config_file_fails_cleanly) {
  auto dir = make_tmpdir();
  std::string cfg = dir + "/does_not_exist.cfg";
  std::string bak = dir + "/printer.cfg.bak";

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_FALSE(p.backup());
  ASSERT_FALSE(p.has_backup());
}

TEST(Persistence, restore_after_backup_source_deleted_fails_safely) {
  auto dir = make_tmpdir();
  std::string cfg = dir + "/printer.cfg";
  std::string bak = dir + "/printer.cfg.bak";
  write_file(cfg, kRealisticConfig);

  ZOffsetConfigPersistence p(cfg, bak);
  ASSERT_TRUE(p.backup());
  std::remove(bak.c_str());  // simulate the backup file vanishing between backup() and restore
  ASSERT_FALSE(p.restore_backup());
}

int main() {
  return minitest::run_all("z_offset_config_persistence") > 0 ? 1 : 0;
}
