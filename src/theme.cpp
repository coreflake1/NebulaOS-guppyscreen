#include "theme.h"
#include "platform.h"

#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <experimental/filesystem>

namespace fs = std::experimental::filesystem;

ThemeConfig *ThemeConfig::instance{NULL};

ThemeConfig::ThemeConfig() {
}

ThemeConfig *ThemeConfig::get_instance() {
  if (instance == NULL) {
    instance = new ThemeConfig();
  }
  return instance;
}

void ThemeConfig::init(const std::string config_path) {
  path = config_path;
  struct stat buffer;

  bool parsed = false;
  if (stat(config_path.c_str(), &buffer) == 0) {
    // See Config::init()'s own identical guard for why this is
    // exception-safe rather than letting a parse_error crash the whole
    // process on startup - a stat()-existing file can still fail to
    // actually open/read (e.g. a read-only mount).
    try {
      data = json::parse(std::fstream(config_path));
      parsed = true;
    } catch (const json::parse_error &) {
      parsed = false;
    }
  }
  if (!parsed) {
    data = {
        {"primary_color", "0x2196F3"}, //blue
        {"secondary_color", "0xF44336"} // red
    };
  }

  // Best-effort only: on a read-only mount this fails to open and silently
  // no-ops (std::ofstream does not throw on a failed open by default) -
  // fine, this write-back is just a normalize/persist convenience, not
  // load-bearing for startup.
  std::ofstream o(config_path);
  o << std::setw(2) << data << std::endl;
}

json &ThemeConfig::get_json(const std::string &json_path) {
  return data[json::json_pointer(json_path)];
}

void ThemeConfig::save() {
  std::ofstream o(path);
  o << std::setw(2) << data << std::endl;
}
