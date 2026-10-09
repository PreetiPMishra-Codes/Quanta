#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "strategies/strategies.hpp"

using namespace quanta;

namespace {
// A typical view: mid and last price both at `last`.
MarketView view(Price last = 10000) {
    MarketView v;
    v.best_bid       = last - 5;
    v.best_ask       = last + 5;
    v.last_price     = last;
    v.mid            = last;
    v.position       = 0;
    v.cash           = rupees_to_ticks(100000);
    v.max_order_size = 100;
    v.max_position   = 1000;
    return v;
}
}  // namespace

TEST(Strategies, NamesAreDistinct) {
    std::set<std::string> names{RandomStrategy().name(), MomentumStrategy().name(),
                                MeanReversionStrategy().name(),
                                MarketMakingStrategy().name()};
    EXPECT_EQ(names.size(), 4u);
}

// ---------- Random ----------

TEST(Strategies, RandomProducesValidActions) {
    RandomStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    for (int i = 0; i < 1000; ++i) s.decide(view(), rng, out);
    ASSERT_EQ(out.size(), 1000u);

    bool market = false, limit = false, buy = false, sell = false;
    for (const Action& a : out) {
        EXPECT_GE(a.quantity, 1u);
        EXPECT_LE(a.quantity, 50u);
        (a.side == Side::Buy ? buy : sell) = true;
        if (a.type == OrderType::Market) {
            market = true;
        } else {
            limit = true;
            EXPECT_GE(a.price, 9990);
            EXPECT_LE(a.price, 10010);
        }
    }
    EXPECT_TRUE(market && limit && buy && sell);   // all four kinds were seen
}

TEST(Strategies, RandomSameSeedSameDecisions) {
    RandomStrategy a, b;
    std::mt19937 r1(7), r2(7);
    std::vector<Action> o1, o2;
    for (int i = 0; i < 100; ++i) {
        a.decide(view(), r1, o1);
        b.decide(view(), r2, o2);
    }
    ASSERT_EQ(o1.size(), o2.size());
    for (std::size_t i = 0; i < o1.size(); ++i) {
        EXPECT_EQ(o1[i].type, o2[i].type);
        EXPECT_EQ(o1[i].side, o2[i].side);
        EXPECT_EQ(o1[i].quantity, o2[i].quantity);
        EXPECT_EQ(o1[i].price, o2[i].price);
    }
}

// ---------- Momentum ----------

TEST(Strategies, MomentumBuysAfterRise) {
    MomentumStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);
    EXPECT_TRUE(out.empty());              // first look only remembers
    s.decide(view(10010), rng, out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].side, Side::Buy);
}

TEST(Strategies, MomentumSellsAfterFall) {
    MomentumStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);
    s.decide(view(9990), rng, out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].side, Side::Sell);
}

TEST(Strategies, MomentumIdleWhenPriceUnchanged) {
    MomentumStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);
    s.decide(view(10000), rng, out);
    EXPECT_TRUE(out.empty());
}

// ---------- Mean reversion ----------

TEST(Strategies, MeanReversionSellsAboveAverage) {
    MeanReversionStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);       // average starts at 10000
    s.decide(view(10100), rng, out);       // far above it
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].side, Side::Sell);
}

TEST(Strategies, MeanReversionBuysBelowAverage) {
    MeanReversionStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);
    s.decide(view(9900), rng, out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].side, Side::Buy);
}

TEST(Strategies, MeanReversionIdleNearAverage) {
    MeanReversionStrategy s;
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);
    s.decide(view(10003), rng, out);       // within the 5-tick threshold
    EXPECT_TRUE(out.empty());
}

// ---------- Market making ----------

TEST(Strategies, MarketMakerQuotesBothSides) {
    MarketMakingStrategy s(5);
    std::mt19937 rng(1);
    std::vector<Action> out;
    s.decide(view(10000), rng, out);
    ASSERT_EQ(out.size(), 2u);
    EXPECT_EQ(out[0].side, Side::Buy);
    EXPECT_EQ(out[0].price, 9995);
    EXPECT_EQ(out[1].side, Side::Sell);
    EXPECT_EQ(out[1].price, 10005);
    EXPECT_LT(out[0].price, out[1].price);
}

TEST(Strategies, MarketMakerStopsBuyingAtInventoryCap) {
    MarketMakingStrategy s(5, 0.8);        // cap = 800 of max 1000
    std::mt19937 rng(1);
    std::vector<Action> out;
    MarketView v = view();
    v.position = 900;                      // already long
    s.decide(v, rng, out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].side, Side::Sell);
}

TEST(Strategies, MarketMakerStopsSellingAtShortCap) {
    MarketMakingStrategy s(5, 0.8);
    std::mt19937 rng(1);
    std::vector<Action> out;
    MarketView v = view();
    v.position = -900;                     // already short
    s.decide(v, rng, out);
    ASSERT_EQ(out.size(), 1u);
    EXPECT_EQ(out[0].side, Side::Buy);
}

// ---------- Polymorphism ----------

TEST(Strategies, PolymorphicContainer) {
    std::vector<std::unique_ptr<TradingStrategy>> all;
    all.push_back(std::make_unique<RandomStrategy>());
    all.push_back(std::make_unique<MomentumStrategy>());
    all.push_back(std::make_unique<MeanReversionStrategy>());
    all.push_back(std::make_unique<MarketMakingStrategy>());

    std::mt19937 rng(3);
    for (auto& s : all) {
        EXPECT_FALSE(s->name().empty());
        std::vector<Action> out;
        for (int i = 0; i < 20; ++i) s->decide(view(10000 + i), rng, out);
        for (const Action& a : out) EXPECT_GE(a.quantity, 1u);
    }
}