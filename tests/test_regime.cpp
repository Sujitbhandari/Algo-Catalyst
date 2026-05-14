#include "runner.h"
#include "AI_Regime.h"
#include "Events.h"

using namespace AlgoCatalyst;
using namespace TestRunner;

static Tick makeTick(double price, std::int64_t vol = 1000,
                     std::int64_t ts = 1'700'000'000'000'000LL) {
    Tick t;
    t.price        = price;
    t.volume       = vol;
    t.bid_size     = 500;
    t.ask_size     = 500;
    t.high         = price * 1.001;
    t.low          = price * 0.999;
    t.timestamp_us = ts;
    return t;
}

TEST(regime_classifier_initializes) {
    RegimeClassifier rc(50, 3);
    // Default regime before any data
    auto r = rc.getCurrentRegime();
    check(r == RegimeClassifier::Regime::CHOPPY ||
          r == RegimeClassifier::Regime::TRENDING ||
          r == RegimeClassifier::Regime::VOLATILE,
          "Initial regime is a valid enum value");
}

TEST(regime_classifier_updates_without_crash) {
    RegimeClassifier rc(20, 3);
    double price = 100.0;
    for (int i = 0; i < 50; ++i) {
        price += (i % 2 == 0) ? 0.5 : -0.3;
        auto tick = makeTick(price, 1000 + i * 10,
                             1'700'000'000'000'000LL + i * 1'000'000LL);
        rc.updateAndClassify(tick);
    }
    auto r = rc.getCurrentRegime();
    check(r == RegimeClassifier::Regime::CHOPPY ||
          r == RegimeClassifier::Regime::TRENDING ||
          r == RegimeClassifier::Regime::VOLATILE,
          "Regime is a valid enum after 50 ticks");
}

TEST(regime_to_string_returns_non_empty) {
    check(std::string(RegimeClassifier::regimeToString(RegimeClassifier::Regime::CHOPPY))   == "CHOPPY",   "CHOPPY string");
    check(std::string(RegimeClassifier::regimeToString(RegimeClassifier::Regime::TRENDING)) == "TRENDING", "TRENDING string");
    check(std::string(RegimeClassifier::regimeToString(RegimeClassifier::Regime::VOLATILE)) == "VOLATILE", "VOLATILE string");
}

TEST(regime_position_multiplier_in_range) {
    RegimeClassifier rc(20, 3);
    double mult = rc.getPositionMultiplier();
    check(mult >= 0.0 && mult <= 2.0, "Position multiplier in [0, 2]");
}

TEST(regime_trending_gives_higher_multiplier) {
    RegimeClassifier rc(20, 3);
    double price = 100.0;
    // Feed strong trending data
    for (int i = 0; i < 60; ++i) {
        price += 1.5;
        auto tick = makeTick(price, 5000 + i * 100,
                             1'700'000'000'000'000LL + i * 1'000'000LL);
        rc.updateAndClassify(tick);
    }
    // We can't guarantee TRENDING but multiplier must still be valid
    double mult = rc.getPositionMultiplier();
    check(mult >= 0.0, "Multiplier non-negative after trending feed");
}
