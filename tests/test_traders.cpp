#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "traders/trader.hpp"

using namespace quanta;

TEST(Traders, BuyReducesCashAndIncreasesPosition) {
    RetailTrader t(1, "Asha", rupees_to_ticks(10000));
    t.apply_fill(Side::Buy, 10, rupees_to_ticks(100.50));
    EXPECT_EQ(t.position(), 10);
    EXPECT_EQ(t.cash(), rupees_to_ticks(10000) - 10 * 10050);
}

TEST(Traders, SellIncreasesCashAndReducesPosition) {
    RetailTrader t(1, "Asha", rupees_to_ticks(10000));
    t.apply_fill(Side::Buy, 10, 10000);
    t.apply_fill(Side::Sell, 4, 10100);
    EXPECT_EQ(t.position(), 6);
}

TEST(Traders, SellingWithoutSharesGoesShort) {
    RetailTrader t(1, "Asha");
    t.apply_fill(Side::Sell, 5, 10000);
    EXPECT_EQ(t.position(), -5);
}

TEST(Traders, DifferentKindsHaveDifferentLimits) {
    RetailTrader r(1, "R");
    InstitutionalTrader i(2, "I");
    AlgorithmicTrader a(3, "A");
    EXPECT_LT(r.max_order_size(), a.max_order_size());
    EXPECT_LT(a.max_order_size(), i.max_order_size());
    EXPECT_EQ(r.kind(), "Retail");
    EXPECT_EQ(i.kind(), "Institutional");
    EXPECT_EQ(a.kind(), "Algorithmic");
}

TEST(Traders, PolymorphicContainer) {
    std::vector<std::unique_ptr<Trader>> traders;
    traders.push_back(std::make_unique<RetailTrader>(1, "Asha"));
    traders.push_back(std::make_unique<InstitutionalTrader>(2, "BigFund"));
    traders.push_back(std::make_unique<AlgorithmicTrader>(3, "Bot-1"));

    for (const auto& t : traders) {
        EXPECT_GT(t->max_order_size(), 0u);
        EXPECT_GT(t->cash(), 0);
    }
    EXPECT_EQ(traders[1]->kind(), "Institutional");
}