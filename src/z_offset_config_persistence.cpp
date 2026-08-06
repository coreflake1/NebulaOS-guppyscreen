#include "z_offset_config_persistence.h"

#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <sys/stat.h>

namespace {
// Found during testing (2026-08-06): opening a *directory* via std::ifstream on this
// platform/libstdc++ combination passes the initial `if (!stream)` check but then throws
// an uncaught std::ios_base::failure the moment a read is actually attempted
// (basic_filebuf::underflow) - this is a faithful behavior of the original file-I/O logic
// this class was extracted from, not something introduced here, but it's a real defect:
// an uncaught exception from a wizard's own safety-restore path could crash the whole
// GuppyScreen process. Guarding with an explicit regular-file check up front, rather than a
// broad try/catch around every stream operation, keeps the failure mode a plain `false`
// return - consistent with every other failure path in this class.
bool is_regular_file(const std::string &path) {
  struct stat st;
  if (::stat(path.c_str(), &st) != 0) {
    return false;  // doesn't exist / inaccessible - callers already return false for this
  }
  return S_ISREG(st.st_mode);
}
}  // namespace

ZOffsetConfigPersistence::ZOffsetConfigPersistence(std::string config_path,
                                                     std::string backup_path)
  : config_path_(std::move(config_path)), backup_path_(std::move(backup_path)) {
}

bool ZOffsetConfigPersistence::backup() {
  has_backup_ = false;
  if (!is_regular_file(config_path_)) {
    return false;
  }
  std::ifstream src(config_path_, std::ios::binary);
  if (!src) {
    return false;
  }
  std::ofstream dst(backup_path_, std::ios::binary | std::ios::trunc);
  if (!dst) {
    return false;
  }
  dst << src.rdbuf();
  dst.flush();
  has_backup_ = dst.good();
  return has_backup_;
}

bool ZOffsetConfigPersistence::restore_backup() const {
  if (!has_backup_ || !is_regular_file(backup_path_)) {
    return false;
  }
  std::ifstream src(backup_path_, std::ios::binary);
  if (!src) {
    return false;
  }
  std::ofstream dst(config_path_, std::ios::binary | std::ios::trunc);
  if (!dst) {
    return false;
  }
  dst << src.rdbuf();
  dst.flush();
  return dst.good();
}

bool ZOffsetConfigPersistence::patch_z_offset(double value) const {
  if (!is_regular_file(config_path_)) {
    return false;
  }
  std::ifstream in(config_path_, std::ios::binary);
  if (!in) {
    return false;
  }
  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  in.close();

  const std::string marker_str = marker();
  auto pos = content.find(marker_str);
  if (pos == std::string::npos) {
    // No SAVE_CONFIG has ever run on this printer (or the file was hand-edited) - refuse
    // rather than guess where/how to append an unverified marker format. See header.
    return false;
  }
  auto value_start = pos + marker_str.size();
  auto line_end = content.find('\n', value_start);
  if (line_end == std::string::npos) {
    line_end = content.size();
  }

  std::ostringstream formatted;
  formatted << std::fixed << std::setprecision(3) << value;
  content.replace(value_start, line_end - value_start, formatted.str());

  std::ofstream out(config_path_, std::ios::binary | std::ios::trunc);
  if (!out) {
    return false;
  }
  out << content;
  out.flush();
  return out.good();
}
