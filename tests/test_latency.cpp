#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "benchmark/latency_recorder.hpp"
#include "simulation/simulator.hpp"

using namespace quanta;

namespace {
std::vector<std::uint64_t> one_to(std::uint64_t n) {
    std::vector<std::uint64_t> v;
    for (std::uint64_t i = 1; i <= n; ++i) v.push_back(i);
    return v;
}

SimulationConfig config(std::uint32_t seed = 42) {
    SimulationConfig c;
    c.seed = seed;
    c.num_traders = 30;
    return c;
}
}  // namespace

// ---------- percentile maths ----------

TEST(Latency, PercentileOneToHundred) {
    const auto v = one_to(100);
    EXPECT_EQ(percentile_permille(v, 500), 50u);
    EXPECT_EQ(percentile_permille(v, 950), 95u);
    EXPECT_EQ(percentile_permille(v, 990), 99u);
    EXPECT_EQ(percentile_permille(v, 999), 100u);   // only 100 samples: P99.9 is the max
    EXPECT_EQ(percentile_permille(v, 0), 1u);
    EXPECT_EQ(percentile_permille(v, 1000), 100u);
}

TEST(Latency, PercentileOneToThousand) {
    const auto v = one_to(1000);
    EXPECT_EQ(percentile_permille(v, 500), 500u);
    EXPECT_EQ(percentile_permille(v, 990), 990u);
    EXPECT_EQ(percentile_permille(v, 999), 999u);   // exact: no floating-point error
}

TEST(Latency, PercentileSingleSample) {
    const std::vector<std::uint64_t> v{42};
    EXPECT_EQ(percentile_permille(v, 0), 42u);
    EXPECT_EQ(percentile_permille(v, 500), 42u);
    EXPECT_EQ(percentile_permille(v, 999), 42u);
}

TEST(Latency, PercentileRejectsBadInput) {
    EXPECT_THROW(percentile_permille({}, 500), std::invalid_argument);
    EXPECT_THROW(percentile_permille(one_to(10), 1001), std::invalid_argument);
}

// ---------- recorder ----------

TEST(Latency, SummarizeEmptyIsZero) {
    LatencyRecorder rec;
    const LatencyStats s = rec.summarize();
    EXPECT_EQ(s.count, 0u);
    EXPECT_EQ(s.max_ns, 0u);
    EXPECT_EQ(s.total_ns, 0u);
}

TEST(Latency, SummarizeKnownData) {
    LatencyRecorder rec(100);
    for (int i = 1; i <= 100; ++i) rec.record(std::chrono::nanoseconds(i));
    const LatencyStats s = rec.summarize();
    EXPECT_EQ(s.count, 100u);
    EXPECT_EQ(s.min_ns, 1u);
    EXPECT_EQ(s.max_ns, 100u);
    EXPECT_EQ(s.total_ns, 5050u);
    EXPECT_DOUBLE_EQ(s.mean_ns, 50.5);
    EXPECT_EQ(s.p50_ns, 50u);
    EXPECT_EQ(s.p95_ns, 95u);
    EXPECT_EQ(s.p99_ns, 99u);
}

TEST(Latency, SummarizeIgnoresInsertionOrder) {
    LatencyRecorder rec;
    for (int i = 100; i >= 1; --i) rec.record_ns(static_cast<std::uint64_t>(i));
    const LatencyStats s = rec.summarize();
    EXPECT_EQ(s.min_ns, 1u);
    EXPECT_EQ(s.max_ns, 100u);
    EXPECT_EQ(s.p50_ns, 50u);
}

TEST(Latency, SummarizeDoesNotModifyRecorder) {
    LatencyRecorder rec;
    for (int i = 5; i >= 1; --i) rec.record_ns(static_cast<std::uint64_t>(i));
    const LatencyStats a = rec.summarize();
    const LatencyStats b = rec.summarize();
    EXPECT_EQ(rec.size(), 5u);
    EXPECT_EQ(a.p50_ns, b.p50_ns);
    EXPECT_EQ(a.max_ns, b.max_ns);
}

// ---------- simulator hook ----------

TEST(Latency, SimulatorRecordsEverySubmit) {
    Simulator sim(config());
    LatencyRecorder rec(20000);
    sim.set_latency_recorder(&rec);
    sim.run(5000);
    const auto& s = sim.stats();
    // Every limit order, market order and cancel goes through submit exactly once.
    EXPECT_EQ(rec.size(), s.limit_orders + s.market_orders + s.cancels_sent);
    EXPECT_GT(rec.size(), 0u);
}

TEST(Latency, DetachedRecorderStopsRecording) {
    Simulator sim(config());
    LatencyRecorder rec(20000);
    sim.set_latency_recorder(&rec);
    sim.run(1000);
    const std::size_t after_first = rec.size();
    sim.set_latency_recorder(nullptr);
    sim.run(1000);
    EXPECT_EQ(rec.size(), after_first);
}

TEST(Latency, RecorderDoesNotChangeSimulationResults) {
    Simulator plain(config(7));
    Simulator timed(config(7));
    LatencyRecorder rec(20000);
    timed.set_latency_recorder(&rec);
    plain.run(3000);
    timed.run(3000);

    const auto& x = plain.stats();
    const auto& y = timed.stats();
    EXPECT_EQ(x.limit_orders, y.limit_orders);
    EXPECT_EQ(x.market_orders, y.market_orders);
    EXPECT_EQ(x.cancels_sent, y.cancels_sent);
    EXPECT_EQ(x.rejected, y.rejected);
    EXPECT_EQ(x.trades, y.trades);
    EXPECT_EQ(x.volume, y.volume);
    EXPECT_EQ(plain.last_price(), timed.last_price());
    EXPECT_EQ(plain.exchange().engine().book().render(10),
              timed.exchange().engine().book().render(10));
}