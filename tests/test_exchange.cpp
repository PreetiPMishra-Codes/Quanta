#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "exchange/exchange.hpp"

using namespace quanta;

namespace {
std::unique_ptr<Order> lim(OrderId id, TraderId trader, Side side,
                           Quantity qty, Price price) {
    return std::make_unique<LimitOrder>(id, trader, side, qty, price);
}
}  // namespace

TEST(Exchange, UnknownTraderIsRejected) {
    Exchange ex;
    auto r = ex.submit(lim(1, 99, Side::Buy, 10, 10000));
    EXPECT_FALSE(r.accepted());
    EXPECT_EQ(r.reject_reason, RejectReason::UnknownTrader);
    EXPECT_EQ(r.execution.outcome, Outcome::Rejected);
    EXPECT_TRUE(ex.engine().book().empty());
}

TEST(Exchange, DuplicateTraderThrows) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "A"));
    EXPECT_THROW(ex.add_trader(std::make_unique<RetailTrader>(1, "B")),
                 std::invalid_argument);
}

TEST(Exchange, RejectedOrderNeverTouchesTheBook) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "A"));
    auto r = ex.submit(lim(1, 1, Side::Buy, 500, 10000));   // retail max is 100
    EXPECT_EQ(r.reject_reason, RejectReason::OrderTooLarge);
    EXPECT_EQ(r.execution.outcome, Outcome::Rejected);
    EXPECT_TRUE(ex.engine().book().empty());
    EXPECT_EQ(ex.rejected_count(), 1u);
}

TEST(Exchange, AcceptedOrderRestsInBook) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "A"));
    auto r = ex.submit(lim(1, 1, Side::Buy, 10, 10000));
    EXPECT_TRUE(r.accepted());
    EXPECT_EQ(r.execution.outcome, Outcome::Resting);
    EXPECT_EQ(ex.engine().book().order_count(), 1u);
}

TEST(Exchange, FillsUpdateBothTradersAccounts) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "Seller"));
    ex.add_trader(std::make_unique<RetailTrader>(2, "Buyer"));
    ex.submit(lim(1, 1, Side::Sell, 10, 10000));
    auto r = ex.submit(lim(2, 2, Side::Buy, 10, 10000));
    EXPECT_EQ(r.execution.outcome, Outcome::Filled);

    const Price start = rupees_to_ticks(100'000);
    const Trader* buyer  = ex.find_trader(2);
    const Trader* seller = ex.find_trader(1);
    EXPECT_EQ(buyer->position(), 10);
    EXPECT_EQ(buyer->cash(), start - 100'000);
    EXPECT_EQ(seller->position(), -10);
    EXPECT_EQ(seller->cash(), start + 100'000);
}

TEST(Exchange, MarketOrderFillsUpdateAccounts) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "Seller"));
    ex.add_trader(std::make_unique<RetailTrader>(2, "Buyer"));
    ex.submit(lim(1, 1, Side::Sell, 10, 10000));
    auto r = ex.submit(std::make_unique<MarketOrder>(2, 2, Side::Buy, 4));
    EXPECT_EQ(r.execution.outcome, Outcome::Filled);
    EXPECT_EQ(ex.find_trader(2)->position(), 4);
    EXPECT_EQ(ex.find_trader(1)->position(), -4);   // only the traded part
}

TEST(Exchange, CancelGoesThroughWithoutRiskChecks) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "A"));
    ex.submit(lim(1, 1, Side::Buy, 10, 10000));
    auto r = ex.submit(std::make_unique<CancelOrder>(2, 1, 1));
    EXPECT_TRUE(r.accepted());
    EXPECT_EQ(r.execution.outcome, Outcome::Cancelled);
    EXPECT_TRUE(ex.engine().book().empty());
}

TEST(Exchange, RejectedCounterCounts) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "A"));
    ex.submit(lim(1, 99, Side::Buy, 10, 10000));    // unknown trader
    ex.submit(lim(2, 1, Side::Buy, 500, 10000));    // too large
    ex.submit(lim(3, 1, Side::Buy, 10, 10000));     // fine
    EXPECT_EQ(ex.rejected_count(), 2u);
}

TEST(Exchange, SelfTradePolicyIsPassedToEngine) {
    Exchange ex(SelfTradePolicy::CancelResting);
    ex.add_trader(std::make_unique<RetailTrader>(1, "A"));
    ex.submit(lim(1, 1, Side::Sell, 10, 10000));
    auto r = ex.submit(lim(2, 1, Side::Buy, 10, 10000));
    EXPECT_TRUE(r.execution.trades.empty());
    EXPECT_EQ(r.execution.self_trades_prevented, 1u);
    EXPECT_EQ(ex.find_trader(1)->position(), 0);
}

TEST(Exchange, PositionLimitStopsRepeatedBuying) {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "Buyer"));          // limit 1000
    ex.add_trader(std::make_unique<InstitutionalTrader>(2, "Seller"));
    ex.submit(lim(1000, 2, Side::Sell, 5000, 1000));

    for (OrderId id = 1; id <= 10; ++id) {
        auto r = ex.submit(lim(id, 1, Side::Buy, 100, 1000));
        ASSERT_TRUE(r.accepted()) << "buy #" << id;
    }
    EXPECT_EQ(ex.find_trader(1)->position(), 1000);

    auto r = ex.submit(lim(11, 1, Side::Buy, 100, 1000));
    EXPECT_EQ(r.reject_reason, RejectReason::PositionLimit);
    EXPECT_EQ(ex.find_trader(1)->position(), 1000);   // unchanged
}