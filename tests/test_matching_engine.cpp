#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <random>
#include <stdexcept>

#include "exchange/matching_engine.hpp"

using namespace quanta;

namespace {
std::unique_ptr<LimitOrder> lim(OrderId id, TraderId trader, Side side,
                                Quantity qty, Price price) {
    return std::make_unique<LimitOrder>(id, trader, side, qty, price);
}
}  // namespace

// ---------- basic crossing ----------

TEST(Matching, NonCrossingOrdersJustRest) {
    MatchingEngine e;
    auto r1 = e.submit_limit(lim(1, 1, Side::Sell, 10, 10015));
    auto r2 = e.submit_limit(lim(2, 2, Side::Buy, 10, 10010));
    EXPECT_EQ(r1.outcome, Outcome::Resting);
    EXPECT_EQ(r2.outcome, Outcome::Resting);
    EXPECT_TRUE(r2.trades.empty());
    EXPECT_EQ(e.book().order_count(), 2u);
    EXPECT_EQ(*e.book().spread(), 5);
}

TEST(Matching, ExactMatchEmptiesTheBook) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    auto r = e.submit_limit(lim(2, 2, Side::Buy, 10, 10000));
    EXPECT_EQ(r.outcome, Outcome::Filled);
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].quantity, 10u);
    EXPECT_EQ(r.trades[0].price, 10000);
    EXPECT_TRUE(e.book().empty());
}

TEST(Matching, TradeHappensAtRestingPrice) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));              // waits at 100.00
    auto r = e.submit_limit(lim(2, 2, Side::Buy, 10, 10010));      // willing to pay 100.10
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].price, 10000);  // buyer gets price improvement
}

TEST(Matching, SellingIntoBidsTradesAtBidPrice) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Buy, 10, 10010));
    auto r = e.submit_limit(lim(2, 2, Side::Sell, 10, 10000));
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].price, 10010);  // resting bid sets the price
    EXPECT_EQ(r.trades[0].aggressor, Side::Sell);
}

// ---------- partial fills ----------

TEST(Matching, IncomingLeftoverRests) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 30, 10000));
    auto r = e.submit_limit(lim(2, 2, Side::Buy, 100, 10000));
    EXPECT_EQ(r.outcome, Outcome::Resting);
    EXPECT_EQ(r.filled, 30u);
    EXPECT_EQ(r.remaining, 70u);
    EXPECT_EQ(*e.book().best_bid(), 10000);
    EXPECT_EQ(e.book().quantity_at(Side::Buy, 10000), 70u);
    EXPECT_FALSE(e.book().best_ask().has_value());
}

TEST(Matching, RestingOrderKeepsItsLeftover) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 100, 10000));
    auto r = e.submit_limit(lim(2, 2, Side::Buy, 30, 10000));
    EXPECT_EQ(r.outcome, Outcome::Filled);
    ASSERT_NE(e.book().find(1), nullptr);
    EXPECT_EQ(e.book().find(1)->remaining(), 70u);
    EXPECT_EQ(e.book().quantity_at(Side::Sell, 10000), 70u);
}

TEST(Matching, BigOrderWalksSeveralLevels) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    e.submit_limit(lim(2, 1, Side::Sell, 10, 10010));
    e.submit_limit(lim(3, 1, Side::Sell, 10, 10020));

    auto r = e.submit_limit(lim(4, 2, Side::Buy, 25, 10015));
    ASSERT_EQ(r.trades.size(), 2u);
    EXPECT_EQ(r.trades[0].price, 10000);
    EXPECT_EQ(r.trades[1].price, 10010);
    EXPECT_EQ(r.filled, 20u);
    EXPECT_EQ(r.remaining, 5u);
    EXPECT_EQ(r.outcome, Outcome::Resting);
    EXPECT_EQ(*e.book().best_bid(), 10015);  // leftover rests at its own limit
    EXPECT_EQ(*e.book().best_ask(), 10020);  // 10020 was too expensive, untouched
}

// ---------- priority ----------

TEST(Matching, TimePriorityAtSamePrice) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    e.submit_limit(lim(2, 1, Side::Sell, 10, 10000));
    auto r = e.submit_limit(lim(3, 2, Side::Buy, 10, 10000));
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].sell_order_id, 1u);       // oldest first
    EXPECT_NE(e.book().find(2), nullptr);           // younger still waiting
    EXPECT_EQ(e.book().find(1), nullptr);
}

TEST(Matching, PricePriorityBeatsArrivalOrder) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10010));  // arrived first, worse price
    e.submit_limit(lim(2, 1, Side::Sell, 10, 10000));  // arrived later, better price
    auto r = e.submit_limit(lim(3, 2, Side::Buy, 10, 10020));
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].sell_order_id, 2u);
    EXPECT_EQ(r.trades[0].price, 10000);
}

// ---------- market orders ----------

TEST(Matching, MarketBuyConsumesSeveralLevels) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    e.submit_limit(lim(2, 1, Side::Sell, 10, 10010));

    MarketOrder m(3, 2, Side::Buy, 15);
    auto r = e.submit_market(m);
    EXPECT_EQ(r.outcome, Outcome::Filled);
    ASSERT_EQ(r.trades.size(), 2u);
    EXPECT_EQ(r.trades[0].quantity, 10u);
    EXPECT_EQ(r.trades[0].price, 10000);
    EXPECT_EQ(r.trades[1].quantity, 5u);
    EXPECT_EQ(r.trades[1].price, 10010);
    EXPECT_EQ(e.book().quantity_at(Side::Sell, 10010), 5u);
}

TEST(Matching, MarketOrderOnEmptyBookExpires) {
    MatchingEngine e;
    MarketOrder m(1, 1, Side::Buy, 10);
    auto r = e.submit_market(m);
    EXPECT_EQ(r.outcome, Outcome::Expired);
    EXPECT_TRUE(r.trades.empty());
    EXPECT_EQ(r.remaining, 10u);
    EXPECT_TRUE(e.book().empty());
}

TEST(Matching, MarketOrderLeftoverIsDiscardedNotRested) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    MarketOrder m(2, 2, Side::Buy, 30);
    auto r = e.submit_market(m);
    EXPECT_EQ(r.filled, 10u);
    EXPECT_EQ(r.remaining, 20u);
    EXPECT_EQ(r.outcome, Outcome::Expired);
    EXPECT_TRUE(e.book().empty());                    // nothing rested
    EXPECT_FALSE(e.book().best_bid().has_value());
}

TEST(Matching, MarketSellHitsBestBidsFirst) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Buy, 10, 10000));
    e.submit_limit(lim(2, 1, Side::Buy, 10, 9990));
    MarketOrder m(3, 2, Side::Sell, 15);
    auto r = e.submit_market(m);
    ASSERT_EQ(r.trades.size(), 2u);
    EXPECT_EQ(r.trades[0].price, 10000);
    EXPECT_EQ(r.trades[1].price, 9990);
    EXPECT_EQ(r.trades[1].quantity, 5u);
}

// ---------- cancel ----------

TEST(Matching, CancelRemovesRestingOrder) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Buy, 10, 10000));
    CancelOrder c(2, /*trader=*/1, /*target=*/1);
    auto r = e.cancel(c);
    EXPECT_EQ(r.outcome, Outcome::Cancelled);
    EXPECT_EQ(r.remaining, 10u);
    EXPECT_TRUE(e.book().empty());
}

TEST(Matching, CancelUnknownOrderIsRejected) {
    MatchingEngine e;
    CancelOrder c(1, 1, 999);
    EXPECT_EQ(e.cancel(c).outcome, Outcome::CancelRejected);
}

TEST(Matching, CancelByAnotherTraderIsRejected) {
    MatchingEngine e;
    e.submit_limit(lim(1, /*owner=*/1, Side::Buy, 10, 10000));
    CancelOrder c(2, /*not the owner=*/2, 1);
    EXPECT_EQ(e.cancel(c).outcome, Outcome::CancelRejected);
    EXPECT_NE(e.book().find(1), nullptr);  // still there
}

TEST(Matching, CancelAfterFullFillIsRejected) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    e.submit_limit(lim(2, 2, Side::Buy, 10, 10000));   // order 1 fully filled
    CancelOrder c(3, 1, 1);
    EXPECT_EQ(e.cancel(c).outcome, Outcome::CancelRejected);
}

// ---------- validation ----------

TEST(Matching, DuplicateRestingIdThrows) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Buy, 10, 10000));
    EXPECT_THROW(e.submit_limit(lim(1, 2, Side::Sell, 10, 10100)),
                 std::invalid_argument);
}

// ---------- trade records ----------

TEST(Matching, TradeRecordsEveryField) {
    MatchingEngine e;
    e.submit_limit(lim(1, /*seller=*/5, Side::Sell, 10, 10000));
    auto r = e.submit_limit(lim(2, /*buyer=*/6, Side::Buy, 10, 10010));
    ASSERT_EQ(r.trades.size(), 1u);
    const Trade& t = r.trades[0];
    EXPECT_EQ(t.buy_order_id, 2u);
    EXPECT_EQ(t.sell_order_id, 1u);
    EXPECT_EQ(t.buyer, 6u);
    EXPECT_EQ(t.seller, 5u);
    EXPECT_EQ(t.price, 10000);
    EXPECT_EQ(t.quantity, 10u);
    EXPECT_EQ(t.aggressor, Side::Buy);
}

TEST(Matching, TradeIdsIncreaseByOne) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 5, 10000));
    e.submit_limit(lim(2, 1, Side::Sell, 5, 10000));
    auto r = e.submit_limit(lim(3, 2, Side::Buy, 10, 10000));
    ASSERT_EQ(r.trades.size(), 2u);
    EXPECT_EQ(r.trades[0].id, 1u);
    EXPECT_EQ(r.trades[1].id, 2u);
    EXPECT_EQ(e.trade_count(), 2u);
}

// ---------- polymorphic entry point ----------

TEST(Matching, ProcessRoutesEachOrderKind) {
    MatchingEngine e;
    auto a = e.process(std::make_unique<LimitOrder>(1, 1, Side::Sell, 10, 10000));
    EXPECT_EQ(a.outcome, Outcome::Resting);

    auto b = e.process(std::make_unique<MarketOrder>(2, 2, Side::Buy, 4));
    EXPECT_EQ(b.outcome, Outcome::Filled);
    EXPECT_EQ(b.filled, 4u);

    auto c = e.process(std::make_unique<CancelOrder>(3, 1, 1));
    EXPECT_EQ(c.outcome, Outcome::Cancelled);
    EXPECT_EQ(c.remaining, 6u);
}

// ---------- property test: invariants under random load ----------

TEST(Matching, RandomFlowNeverLeavesTheBookCrossed) {
    std::mt19937 rng(12345);  // fixed seed: same "random" run every time
    std::uniform_int_distribution<int> side_d(0, 1);
    std::uniform_int_distribution<int> price_d(9990, 10010);
    std::uniform_int_distribution<int> qty_d(1, 50);

    MatchingEngine e;
    std::uint64_t traded = 0, filled_sum = 0;

    for (OrderId id = 1; id <= 5000; ++id) {
        const Side side = side_d(rng) == 0 ? Side::Buy : Side::Sell;
        auto r = e.submit_limit(lim(id, static_cast<TraderId>(id % 7 + 1), side,
                                    static_cast<Quantity>(qty_d(rng)),
                                    static_cast<Price>(price_d(rng))));
        filled_sum += r.filled;
        for (const auto& t : r.trades) traded += t.quantity;

        const auto bid = e.book().best_bid();
        const auto ask = e.book().best_ask();
        if (bid && ask) {
            ASSERT_LT(*bid, *ask) << "book crossed after order " << id;
        }
    }
    EXPECT_GT(traded, 0u);              // the test really did trade
    EXPECT_EQ(traded, filled_sum);      // no quantity created or lost
}
// ---------- Day 5: edge cases ----------

TEST(Matching, SweepWholeBook) {
    MatchingEngine e;
    for (int i = 0; i < 100; ++i) {
        e.submit_limit(lim(static_cast<OrderId>(i + 1), 1, Side::Sell, 10, 10000 + i));
    }
    auto r = e.submit_limit(lim(1000, 2, Side::Buy, 1000, 20000));
    EXPECT_EQ(r.outcome, Outcome::Filled);
    EXPECT_EQ(r.filled, 1000u);
    ASSERT_EQ(r.trades.size(), 100u);
    EXPECT_EQ(r.trades.front().price, 10000);
    EXPECT_EQ(r.trades.back().price, 10099);
    EXPECT_TRUE(e.book().empty());
    EXPECT_EQ(e.book().level_count(Side::Sell), 0u);
}

TEST(Matching, CancelStormLeavesBookEmpty) {
    MatchingEngine e;
    for (OrderId id = 1; id <= 1000; ++id) {
        const bool buy = (id % 2 == 0);
        // Bids and asks are far apart, so nothing trades.
        const Price price = buy ? 9000 + static_cast<Price>(id % 50)
                                : 11000 + static_cast<Price>(id % 50);
        e.submit_limit(lim(id, 1, buy ? Side::Buy : Side::Sell, 5, price));
    }
    ASSERT_EQ(e.book().order_count(), 1000u);

    for (OrderId id = 1; id <= 1000; ++id) {
        CancelOrder c(100000 + id, 1, id);
        EXPECT_EQ(e.cancel(c).outcome, Outcome::Cancelled);
    }
    EXPECT_TRUE(e.book().empty());
    EXPECT_EQ(e.book().level_count(Side::Buy), 0u);
    EXPECT_EQ(e.book().level_count(Side::Sell), 0u);
}

TEST(Matching, HugeMarketOrderExpiresCleanly) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    MarketOrder m(2, 2, Side::Buy, 4'000'000'000u);
    auto r = e.submit_market(m);
    EXPECT_EQ(r.outcome, Outcome::Expired);
    EXPECT_EQ(r.filled, 10u);
    EXPECT_EQ(r.remaining, 3'999'999'990u);
    EXPECT_TRUE(e.book().empty());
}

TEST(Matching, CancelAfterPartialFillReportsFilledQuantity) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 100, 10000));
    e.submit_limit(lim(2, 2, Side::Buy, 30, 10000));
    CancelOrder c(3, 1, 1);
    auto r = e.cancel(c);
    EXPECT_EQ(r.outcome, Outcome::Cancelled);
    EXPECT_EQ(r.filled, 30u);
    EXPECT_EQ(r.remaining, 70u);
}

// ---------- Day 5: self-trade prevention ----------

TEST(Matching, SelfTradeAllowedByDefault) {
    MatchingEngine e;
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    auto r = e.submit_limit(lim(2, 1, Side::Buy, 10, 10000));   // same trader
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].buyer, r.trades[0].seller);
    EXPECT_EQ(r.self_trades_prevented, 0u);
}

TEST(Matching, SelfTradeCancelRestingPolicy) {
    MatchingEngine e(SelfTradePolicy::CancelResting);
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));
    auto r = e.submit_limit(lim(2, 1, Side::Buy, 10, 10000));
    EXPECT_TRUE(r.trades.empty());
    EXPECT_EQ(r.self_trades_prevented, 1u);
    EXPECT_EQ(r.outcome, Outcome::Resting);
    EXPECT_EQ(e.book().find(1), nullptr);      // our own old order was removed
    ASSERT_NE(e.book().find(2), nullptr);      // the new order rests
    EXPECT_FALSE(e.book().best_ask().has_value());
}

TEST(Matching, SelfTradeSkipsOwnOrderAndTradesWithOthers) {
    MatchingEngine e(SelfTradePolicy::CancelResting);
    e.submit_limit(lim(1, 1, Side::Sell, 10, 10000));   // trader 1 (oldest)
    e.submit_limit(lim(2, 2, Side::Sell, 10, 10000));   // trader 2
    auto r = e.submit_limit(lim(3, 1, Side::Buy, 10, 10000));
    EXPECT_EQ(r.self_trades_prevented, 1u);
    ASSERT_EQ(r.trades.size(), 1u);
    EXPECT_EQ(r.trades[0].sell_order_id, 2u);
    EXPECT_EQ(r.outcome, Outcome::Filled);
    EXPECT_TRUE(e.book().empty());
}

TEST(Matching, SelfTradePolicyRandomFlowStaysUncrossed) {
    std::mt19937 rng(777);
    std::uniform_int_distribution<int> side_d(0, 1);
    std::uniform_int_distribution<int> price_d(9990, 10010);
    std::uniform_int_distribution<int> qty_d(1, 50);

    MatchingEngine e(SelfTradePolicy::CancelResting);
    std::uint64_t traded = 0, filled_sum = 0, prevented = 0;

    for (OrderId id = 1; id <= 5000; ++id) {
        const Side side = side_d(rng) == 0 ? Side::Buy : Side::Sell;
        // Only 3 traders, so self-trades are frequent.
        auto r = e.submit_limit(lim(id, static_cast<TraderId>(id % 3 + 1), side,
                                    static_cast<Quantity>(qty_d(rng)),
                                    static_cast<Price>(price_d(rng))));
        filled_sum += r.filled;
        prevented  += r.self_trades_prevented;
        for (const auto& t : r.trades) {
            traded += t.quantity;
            ASSERT_NE(t.buyer, t.seller) << "self-trade slipped through";
        }
        const auto bid = e.book().best_bid();
        const auto ask = e.book().best_ask();
        if (bid && ask) ASSERT_LT(*bid, *ask) << "book crossed after order " << id;
    }
    EXPECT_GT(traded, 0u);
    EXPECT_GT(prevented, 0u);        // the test really exercised the policy
    EXPECT_EQ(traded, filled_sum);
}