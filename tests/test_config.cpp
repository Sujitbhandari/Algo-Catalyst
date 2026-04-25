#include "runner.h"
#include "ConfigLoader.h"
#include <fstream>
#include <cstdio>

using namespace AlgoCatalyst;
using namespace TestRunner;

static std::string writeTempConfig(const std::string& json) {
    const char* path = "/tmp/algo_test_config.json";
    std::ofstream f(path);
    f << json;
    return path;
}

TEST(config_read_double) {
    std::string p = writeTempConfig(R"({"stop_loss": 2.5, "take_profit": 6.0})");
    ConfigLoader cfg(R"({"stop_loss": 2.5, "take_profit": 6.0})");
    checkClose(cfg.getDouble("stop_loss"), 2.5, 1e-9, "stop_loss == 2.5");
    checkClose(cfg.getDouble("take_profit"), 6.0, 1e-9, "take_profit == 6.0");
}

TEST(config_read_int) {
    ConfigLoader cfg(R"({"latency": 200, "windows": 5})");
    check(cfg.getInt("latency") == 200, "latency == 200");
    check(cfg.getInt("windows") == 5, "windows == 5");
}

TEST(config_read_string) {
    ConfigLoader cfg(R"({"symbol": "AAPL", "strategy": "breakout"})");
    check(cfg.getString("symbol") == "AAPL", "symbol == AAPL");
    check(cfg.getString("strategy") == "breakout", "strategy == breakout");
}

TEST(config_read_bool) {
    ConfigLoader cfg(R"({"use_filter": true, "debug": false})");
    check(cfg.getBool("use_filter") == true, "use_filter == true");
    check(cfg.getBool("debug") == false, "debug == false");
}

TEST(config_missing_key_returns_default) {
    ConfigLoader cfg(R"({"a": 1.0})");
    checkClose(cfg.getDouble("missing_key", 99.0), 99.0, 1e-9, "default double");
    check(cfg.getString("missing_str", "default") == "default", "default string");
}

TEST(config_has_key) {
    ConfigLoader cfg(R"({"x": 1.0})");
    check(cfg.has("x"), "has('x') == true");
    check(!cfg.has("y"), "has('y') == false");
}

TEST(config_from_file_roundtrip) {
    std::string path = writeTempConfig(R"({"slippage": 5, "symbol": "TEST"})");
    ConfigLoader cfg = ConfigLoader::fromFile(path);
    check(cfg.getInt("slippage") == 5, "slippage from file == 5");
    check(cfg.getString("symbol") == "TEST", "symbol from file == TEST");
    std::remove(path.c_str());
}
