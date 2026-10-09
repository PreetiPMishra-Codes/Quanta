#include <gtest/gtest.h>

#include <numeric>
#include <stdexcept>

#include "simulation/simulator.hpp"

using namespace quanta;

namespace {
SimulationConfig config(std::uint32_t seed = 42, std::size_t traders = 30) {
    SimulationConfig c;
    c.seed = seed;
    c.num_traders = traders;
    return c;
}
}  // namespace

TEST(Simulator, InvalidConfigThrows) {
    SimulationConfig c = config();
    c.num_traders = 0;
    EXPECT_THROW(Simulator sim(c), std::invalid_argument);

    c = config();
    c.initial_price = 0;
    EXPECT_THROW(Simulator sim(c), std::invalid_argument);
}

TEST(Simulator, RunsRequestedNumberOfSteps) {
    Simulator sim(config());
    sim.run(2000);
    EXPECT_EQ(sim.stats().steps, 2000u);
}

TEST(Simulator, ProducesTradesAndVolume) {
    Simulator sim(config());
    sim.run(5000);
    EXPECT_GT(sim.stats().limit_orders, 0u);
    EXPECT_GT(sim.stats().trades, 0u);
    EXPECT_GE(sim.stats().volume, sim.stats().trades);   // each trade is >= 1 share
}

TEST(Simulator, BookNeverCrossedDuringRun) {
    Simulator sim(config());
    for (int i = 0; i < 3000; ++i) {
        sim.step();
        const auto& book = sim.exchange().engine().book();
        const auto bid = book.best_bid();
        const auto ask = book.best_ask();
        if (bid && ask) {
            ASSERT_LT(*bid, *ask) << "book crossed at step " << i;
        }
    }
}

TEST(Simulator, SameSeedGivesIdenticalRuns) {
    Simulator a(config(99));
    Simulator b(config(99));
    a.run(5000);
    b.run(5000);

    const auto& x = a.stats();
    const auto& y = b.stats();
    EXPECT_EQ(x.limit_orders, y.limit_orders);
    EXPECT_EQ(x.market_orders, y.market_orders);
    EXPECT_EQ(x.cancels_sent, y.cancels_sent);
    EXPECT_EQ(x.cancels_ok, y.cancels_ok);
    EXPECT_EQ(x.rejected, y.rejected);
    EXPECT_EQ(x.trades, y.trades);
    EXPECT_EQ(x.volume, y.volume);
    EXPECT_EQ(a.last_price(), b.last_price());
    EXPECT_EQ(a.exchange().engine().book().render(10),
              b.exchange().engine().book().render(10));
}

TEST(Simulator, DifferentSeedsGiveDifferentRuns) {
    Simulator a(config(1));
    Simulator b(config(2));
    a.run(5000);
    b.run(5000);
    EXPECT_TRUE(a.stats().volume != b.stats().volume ||
                a.last_price() != b.last_price());
}

TEST(Simulator, SharesAreConserved) {
    Simulator sim(config());
    sim.run(5000);
    EXPECT_EQ(sim.net_position(), 0);   // every trade has one buyer and one seller
}

TEST(Simulator, CashIsConserved) {
    Simulator sim(config());
    sim.run(5000);
    EXPECT_EQ(sim.total_cash(), sim.initial_total_cash());
}

TEST(Simulator, NoSelfTradesWhenPolicyCancelsResting) {
    SimulationConfig c = config(5, 10);   // few traders, so collisions are likely
    c.self_trade_policy = SelfTradePolicy::CancelResting;
    Simulator sim(c);
    sim.run(20000);
    EXPECT_EQ(sim.stats().self_trades, 0u);
}

TEST(Simulator, RejectCountersAddUp) {
    Simulator sim(config());
    sim.run(10000);
    const auto& s = sim.stats();
    const std::uint64_t by_reason = std::accumulate(
        s.rejected_by_reason.begin(), s.rejected_by_reason.end(), std::uint64_t{0});
    EXPECT_EQ(by_reason, s.rejected);
    EXPECT_LE(s.rejected, s.limit_orders + s.market_orders);
    EXPECT_LE(s.cancels_ok, s.cancels_sent);
}