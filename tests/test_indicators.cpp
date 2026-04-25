#include "runner.h"
#include "Indicators.h"

using namespace AlgoCatalyst;
using namespace TestRunner;

// ── EMA ──────────────────────────────────────────────────────────────────────
TEST(ema_warm_up_returns_zero_before_period) {
    Indicators ind;
    ind.updateEMA(100.0, 5);
    check(ind.getEMA(5) == 0.0, "EMA should be 0 before warm-up");
}

TEST(ema_converges_on_constant_series) {
    Indicators ind;
    for (int i = 0; i < 50; ++i) ind.updateEMA(100.0, 10);
    checkClose(ind.getEMA(10), 100.0, 0.001, "EMA of constant series");
}

// ── RSI ──────────────────────────────────────────────────────────────────────
TEST(rsi_default_is_neutral_before_warm_up) {
    Indicators ind;
    for (int i = 0; i < 10; ++i) ind.updateRSI(100.0 + i, 14);
    check(ind.getRSI(14) == 50.0, "RSI should be 50 before 14-tick warm-up");
}

TEST(rsi_all_up_moves_approaches_100) {
    Indicators ind;
    double price = 100.0;
    for (int i = 0; i < 50; ++i) {
        ind.updateRSI(price, 14);
        price += 1.0;
    }
    check(ind.getRSI(14) > 90.0, "RSI should be high on all-up series");
}

TEST(rsi_all_down_moves_approaches_0) {
    Indicators ind;
    double price = 200.0;
    for (int i = 0; i < 50; ++i) {
        ind.updateRSI(price, 14);
        price -= 1.0;
    }
    check(ind.getRSI(14) < 10.0, "RSI should be low on all-down series");
}

// ── Bollinger Bands ───────────────────────────────────────────────────────────
TEST(bollinger_upper_greater_than_lower) {
    Indicators ind;
    for (int i = 0; i < 25; ++i) ind.updateBollingerBands(100.0 + (i % 5) * 2.0, 20);
    check(ind.getBollingerUpper() > ind.getBollingerLower(), "Upper > Lower");
}

TEST(bollinger_constant_series_has_zero_bandwidth) {
    Indicators ind;
    for (int i = 0; i < 25; ++i) ind.updateBollingerBands(100.0, 20);
    checkClose(ind.getBollingerBandwidth(), 0.0, 1e-9, "Zero bandwidth on constant");
}

// ── WMA ──────────────────────────────────────────────────────────────────────
TEST(wma_returns_zero_before_period) {
    Indicators ind;
    ind.updateWMA(100.0, 5);
    check(ind.getWMA(5) == 0.0, "WMA returns 0 before warm-up");
}

TEST(wma_constant_series_equals_price) {
    Indicators ind;
    for (int i = 0; i < 10; ++i) ind.updateWMA(50.0, 5);
    checkClose(ind.getWMA(5), 50.0, 0.001, "WMA of constant = price");
}

TEST(wma_weights_recent_price_more) {
    Indicators ind;
    for (int i = 0; i < 4; ++i) ind.updateWMA(10.0, 5);
    ind.updateWMA(100.0, 5);
    check(ind.getWMA(5) > 10.0, "WMA biased toward recent high price");
}

// ── Donchian Channel ──────────────────────────────────────────────────────────
TEST(donchian_upper_is_max_of_period) {
    Indicators ind;
    for (int i = 0; i < 20; ++i) ind.updateDonchian(90.0 + i, 80.0 + i, 20);
    checkClose(ind.getDonchianUpper(), 90.0 + 19, 0.01, "Donchian upper = highest high");
}

TEST(donchian_lower_is_min_of_period) {
    Indicators ind;
    for (int i = 0; i < 20; ++i) ind.updateDonchian(90.0 + i, 80.0 + i, 20);
    checkClose(ind.getDonchianLower(), 80.0, 0.01, "Donchian lower = lowest low");
}

// ── CCI ───────────────────────────────────────────────────────────────────────
TEST(cci_zero_on_constant_series) {
    Indicators ind;
    for (int i = 0; i < 25; ++i) ind.updateCCI(100.0, 100.0, 100.0, 20);
    checkClose(ind.getCCI(), 0.0, 1e-6, "CCI = 0 on constant series");
}

// ── DEMA ──────────────────────────────────────────────────────────────────────
TEST(dema_converges_on_constant_series) {
    Indicators ind;
    for (int i = 0; i < 60; ++i) ind.updateDEMA(100.0, 10);
    checkClose(ind.getDEMA(10), 100.0, 0.1, "DEMA converges to constant value");
}

// ── OBV ───────────────────────────────────────────────────────────────────────
TEST(obv_increases_on_up_tick) {
    Indicators ind;
    ind.updateOBV(100.0, 1000);
    ind.updateOBV(101.0, 500);
    check(ind.getOBV() > 0.0, "OBV positive after up tick");
}

TEST(obv_decreases_on_down_tick) {
    Indicators ind;
    ind.updateOBV(100.0, 1000);
    ind.updateOBV(99.0, 500);
    check(ind.getOBV() < 0.0, "OBV negative after down tick");
}

// ── Williams %R ───────────────────────────────────────────────────────────────
TEST(williams_r_range) {
    Indicators ind;
    for (int i = 0; i < 20; ++i)
        ind.updateWilliamsR(100.0 + i, 90.0 + i, 95.0 + i, 14);
    double wr = ind.getWilliamsR();
    check(wr >= -100.0 && wr <= 0.0, "Williams %R in [-100, 0]");
}

// ── Reset ─────────────────────────────────────────────────────────────────────
TEST(reset_clears_all_state) {
    Indicators ind;
    for (int i = 0; i < 50; ++i) {
        ind.updateEMA(100.0, 10);
        ind.updateRSI(100.0, 14);
    }
    ind.reset();
    check(ind.getEMA(10) == 0.0, "EMA cleared after reset");
    check(ind.getRSI(14) == 50.0, "RSI neutral after reset");
}
