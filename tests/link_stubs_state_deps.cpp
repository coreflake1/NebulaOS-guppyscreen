// Link-time-only stubs for the handful of Config/KUtils symbols that state.cpp's
// get_display_sensors()/get_display_fans()/get_display_leds() reference. Those three
// methods are unrelated to the subscription-baseline-ordering property under test in
// tests/test_subscription_baseline_ordering.cpp (which only exercises State::reset()/
// set_data()/get_data()/consume()) and are never called there - these definitions exist
// solely so the linker is satisfied without pulling in config.cpp/utils.cpp's much larger
// dependency chain (websocket_client, wpa_supplicant, LVGL runtime calls). If any real
// test ever needs get_display_*() to behave correctly, delete this file and link the real
// config.cpp/utils.cpp instead.
//
// This file may be distributed under the terms of the GNU GPLv3 license.
#include "../src/config.h"
#include "../src/utils.h"

#include <cstdlib>

Config *Config::instance = nullptr;

Config::Config() {}

Config *Config::get_instance() {
  std::abort();  // never called by the baseline-ordering test
}

json &Config::get_json(const std::string &) {
  std::abort();
}

std::string &Config::df() {
  std::abort();
}

namespace KUtils {
std::string to_title(std::string) {
  std::abort();
}

std::string fan_display_name(const std::string &) {
  std::abort();
}
}  // namespace KUtils
