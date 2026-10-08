#include <gtest/gtest.h>

#include <cstring>
#include <limits>

#include "risk/risk_manager.hpp"

using namespace quanta;

namespace {
const RiskManager risk;
}

TEST(Risk, WithinLimitsAccepted) {
    RetailTrader t(1, "A");
    LimitOrder o(1, 1, Side::Buy, 10, 10000);
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::None);
}

TEST(Risk, TooLargeRejected) {
    RetailTrader t(1, "A");  // max order size 100
    LimitOrder o(1, 1, Side::Buy, 101, 10000);
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::OrderTooLarge);
}

TEST(Risk, ExactlyMaxSizeAccepted) {
    RetailTrader t(1, "A");
    LimitOrder o(1, 1, Side::Buy, 100, 10000);
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::None);
}

TEST(Risk, PositionLimitRejectsBuy) {
    RetailTrader t(1, "A");              // max position 1000
    t.apply_fill(Side::Buy, 950, 1);     // now holds 950
    LimitOrder o(1, 1, Side::Buy, 100, 10000);   // would reach 1050
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::PositionLimit);
}

TEST(Risk, PositionLimitRejectsShortSell) {
    RetailTrader t(1, "A");
    t.apply_fill(Side::Sell, 950, 1);    // short 950
    LimitOrder o(1, 1, Side::Sell, 100, 10000);  // would reach -1050
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::PositionLimit);
}

TEST(Risk, ReducingPositionIsAllowed) {
    RetailTrader t(1, "A");
    t.apply_fill(Side::Buy, 950, 1);
    LimitOrder o(1, 1, Side::Sell, 100, 10000);  // 950 -> 850
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::None);
}

TEST(Risk, InsufficientFundsRejectsBuy) {
    RetailTrader t(1, "A", rupees_to_ticks(1000));
    LimitOrder o(1, 1, Side::Buy, 100, 10000);   // costs 10,000 rupees
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::InsufficientFunds);
}

TEST(Risk, ExactCashIsEnough) {
    RetailTrader t(1, "A", rupees_to_ticks(1000));
    LimitOrder o(1, 1, Side::Buy, 10, 10000);    // costs exactly 1,000 rupees
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::None);
}

TEST(Risk, SellNeedsNoCash) {
    RetailTrader t(1, "A", 0);
    LimitOrder o(1, 1, Side::Sell, 10, 10000);
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::None);
}

TEST(Risk, MarketBuyUsesReferencePrice) {
    RetailTrader t(1, "A", rupees_to_ticks(1000));   // 100,000 ticks
    MarketOrder m(1, 1, Side::Buy, 100);
    EXPECT_EQ(risk.check(t, m, 10000), RejectReason::InsufficientFunds);
    EXPECT_EQ(risk.check(t, m, 500), RejectReason::None);
}

TEST(Risk, MarketOrderWithoutReferenceSkipsFundsCheck) {
    RetailTrader t(1, "A", 0);
    MarketOrder m(1, 1, Side::Buy, 100);
    EXPECT_EQ(risk.check(t, m, std::nullopt), RejectReason::None);
}

TEST(Risk, HugePriceDoesNotOverflow) {
    RetailTrader t(1, "A");
    LimitOrder o(1, 1, Side::Buy, 100, std::numeric_limits<Price>::max() / 2);
    EXPECT_EQ(risk.check(t, o, o.price()), RejectReason::InsufficientFunds);
}

TEST(Risk, DifferentTraderTypesHaveDifferentLimits) {
    RetailTrader r(1, "R");
    InstitutionalTrader i(2, "I");
    LimitOrder big(1, 1, Side::Buy, 5000, 10000);
    EXPECT_EQ(risk.check(r, big, big.price()), RejectReason::OrderTooLarge);
    EXPECT_EQ(risk.check(i, big, big.price()), RejectReason::None);
}

TEST(Risk, ReasonsHaveReadableNames) {
    EXPECT_STREQ(to_string(RejectReason::OrderTooLarge), "ORDER_TOO_LARGE");
    EXPECT_STREQ(to_string(RejectReason::PositionLimit), "POSITION_LIMIT");
    EXPECT_STREQ(to_string(RejectReason::InsufficientFunds), "INSUFFICIENT_FUNDS");
    EXPECT_STREQ(to_string(RejectReason::UnknownTrader), "UNKNOWN_TRADER");
}