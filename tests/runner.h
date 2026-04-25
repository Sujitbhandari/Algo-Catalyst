/**
 * Lightweight test runner — no external framework required.
 *
 * Build:
 *   g++ -std=c++20 -I../include -o test_runner \
 *       test_indicators.cpp test_performance.cpp \
 *       ../src/Indicators.cpp ../src/AI_Regime.cpp runner.cpp
 *
 * Run:
 *   ./test_runner
 */

#pragma once

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <cmath>

namespace TestRunner {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> cases;
    return cases;
}

inline int failures = 0;
inline int total    = 0;

inline void registerTest(const std::string& name, std::function<void()> fn) {
    registry().push_back({name, fn});
}

inline void runAll() {
    for (auto& tc : registry()) {
        ++total;
        try {
            tc.fn();
            std::cout << "  [ OK ] " << tc.name << "\n";
        } catch (const std::exception& e) {
            ++failures;
            std::cout << "  [FAIL] " << tc.name << " — " << e.what() << "\n";
        } catch (...) {
            ++failures;
            std::cout << "  [FAIL] " << tc.name << " — unknown exception\n";
        }
    }
    std::cout << "\n" << (total - failures) << "/" << total << " tests passed.\n";
}

inline void check(bool cond, const std::string& msg) {
    if (!cond) throw std::runtime_error(msg);
}

inline void checkClose(double a, double b, double tol, const std::string& msg) {
    if (std::abs(a - b) > tol) {
        throw std::runtime_error(
            msg + " (got " + std::to_string(a) + ", expected ~" + std::to_string(b) + ")");
    }
}

} // namespace TestRunner

#define TEST(name) \
    static void _test_##name(); \
    static bool _reg_##name = (TestRunner::registerTest(#name, _test_##name), true); \
    static void _test_##name()
