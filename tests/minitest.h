#ifndef __MINITEST_H__
#define __MINITEST_H__

// Minimal, dependency-free host test runner. This repo has no existing C++ test
// framework and no gtest/catch2 vendored - rather than pull in a new dependency for a
// handful of pure-logic test files, this is intentionally small: register-a-function,
// run-them-all, print pass/fail. Not meant to grow into a general framework.
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace minitest {

struct TestCase {
  std::string name;
  std::function<void()> fn;
};

inline std::vector<TestCase> &registry() {
  static std::vector<TestCase> r;
  return r;
}

struct Registrar {
  Registrar(const std::string &name, std::function<void()> fn) {
    registry().push_back({name, std::move(fn)});
  }
};

struct AssertionFailure {
  std::string message;
};

inline int run_all(const std::string &suite_name) {
  int failed = 0;
  std::cout << "=== " << suite_name << " (" << registry().size() << " tests) ===" << std::endl;
  for (auto &tc : registry()) {
    try {
      tc.fn();
      std::cout << "[PASS] " << tc.name << std::endl;
    } catch (const AssertionFailure &e) {
      std::cout << "[FAIL] " << tc.name << ": " << e.message << std::endl;
      failed++;
    } catch (const std::exception &e) {
      std::cout << "[FAIL] " << tc.name << ": unexpected exception: " << e.what() << std::endl;
      failed++;
    }
  }
  std::cout << (registry().size() - failed) << "/" << registry().size()
            << " passed in " << suite_name << std::endl;
  return failed;
}

}  // namespace minitest

#define TEST(group, name)                                                    \
  void test_##group##_##name();                                             \
  static minitest::Registrar reg_##group##_##name(                          \
      #group "::" #name, test_##group##_##name);                            \
  void test_##group##_##name()

#define ASSERT_MSG(cond, extra)                                              \
  do {                                                                       \
    if (!(cond)) {                                                           \
      std::ostringstream _oss;                                              \
      _oss << "assertion failed: " #cond " (" << extra << ") at "           \
           << __FILE__ << ":" << __LINE__;                                  \
      throw minitest::AssertionFailure{_oss.str()};                         \
    }                                                                        \
  } while (0)

#define ASSERT_TRUE(cond) ASSERT_MSG(cond, "expected true")
#define ASSERT_FALSE(cond) ASSERT_MSG(!(cond), "expected false")

#define ASSERT_EQ(a, b)                                                      \
  do {                                                                       \
    auto _a = (a);                                                          \
    auto _b = (b);                                                          \
    if (!(_a == _b)) {                                                      \
      std::ostringstream _oss;                                              \
      _oss << "assertion failed: " #a " == " #b " (got " << _a << " vs "    \
           << _b << ") at " << __FILE__ << ":" << __LINE__;                 \
      throw minitest::AssertionFailure{_oss.str()};                         \
    }                                                                        \
  } while (0)

#define ASSERT_NEAR(a, b, eps)                                               \
  do {                                                                       \
    double _a = (a);                                                        \
    double _b = (b);                                                        \
    double _d = _a > _b ? _a - _b : _b - _a;                                \
    if (_d > (eps)) {                                                       \
      std::ostringstream _oss;                                              \
      _oss << "assertion failed: " #a " ~= " #b " (got " << _a << " vs "    \
           << _b << ", diff " << _d << ") at " << __FILE__ << ":" << __LINE__; \
      throw minitest::AssertionFailure{_oss.str()};                         \
    }                                                                        \
  } while (0)

#endif  // __MINITEST_H__
