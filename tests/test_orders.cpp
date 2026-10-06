#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "orders/order.hpp"

using namespace quanta;

TEST(Orders, LimitOrderStoresItsFields) {
    LimitOrder o(1, 7, Side::Buy, 100, rupees_to_ticks(100.10));
    EXPECT_EQ(o.id(), 1u);
    EXPECT_EQ(o.trader_id(), 7u);
    EXPECT_EQ(o.side(), Side::Buy);
    EXPECT_EQ(o.quantity(), 100u);
    EXPECT_EQ(o.remaining(), 100u);
    EXPECT_EQ(o.price(), 10010);
    EXPECT_EQ(o.type(), OrderType::Limit);
}

TEST(Orders, MarketOrderHasMarketType) {
    MarketOrder o(2, 7, Side::Sell, 50);
    EXPECT_EQ(o.type(), OrderType::Market);
    EXPECT_EQ(o.side(), Side::Sell);
}

TEST(Orders, CancelOrderPointsAtTarget) {
    CancelOrder c(3, 7, 1);
    EXPECT_EQ(c.type(), OrderType::Cancel);
    EXPECT_EQ(c.target_id(), 1u);
}

TEST(Orders, PartialAndFullFill) {
    LimitOrder o(1, 7, Side::Buy, 100, 10000);
    o.fill(30);
    EXPECT_EQ(o.remaining(), 70u);
    EXPECT_EQ(o.filled(), 30u);
    EXPECT_FALSE(o.is_filled());

    o.fill(70);
    EXPECT_EQ(o.remaining(), 0u);
    EXPECT_TRUE(o.is_filled());
}

TEST(Orders, OverfillThrows) {
    LimitOrder o(1, 7, Side::Buy, 100, 10000);
    EXPECT_THROW(o.fill(101), std::invalid_argument);
    EXPECT_THROW(o.fill(0), std::invalid_argument);
    EXPECT_EQ(o.remaining(), 100u);  // unchanged after failed fills
}

TEST(Orders, InvalidOrdersCannotBeCreated) {
    EXPECT_THROW(LimitOrder(1, 7, Side::Buy, 0, 10000), std::invalid_argument);
    EXPECT_THROW(LimitOrder(1, 7, Side::Buy, 10, 0), std::invalid_argument);
    EXPECT_THROW(LimitOrder(1, 7, Side::Buy, 10, -5), std::invalid_argument);
    EXPECT_THROW(MarketOrder(1, 7, Side::Buy, 0), std::invalid_argument);
}

TEST(Orders, DescribeMentionsKindAndId) {
    LimitOrder o(42, 7, Side::Buy, 100, rupees_to_ticks(100.10));
    const std::string d = o.describe();
    EXPECT_NE(d.find("LIMIT"), std::string::npos);
    EXPECT_NE(d.find("#42"), std::string::npos);
    EXPECT_NE(d.find("100.10"), std::string::npos);
}

// Polymorphism: one container, three different kinds of order.
TEST(Orders, PolymorphicContainer) {
    std::vector<std::unique_ptr<Order>> orders;
    orders.push_back(std::make_unique<LimitOrder>(1, 7, Side::Buy, 100, 10000));
    orders.push_back(std::make_unique<MarketOrder>(2, 8, Side::Sell, 50));
    orders.push_back(std::make_unique<CancelOrder>(3, 7, 1));

    EXPECT_EQ(orders[0]->type(), OrderType::Limit);
    EXPECT_EQ(orders[1]->type(), OrderType::Market);
    EXPECT_EQ(orders[2]->type(), OrderType::Cancel);

    // Same call, different behavior depending on the real object type.
    EXPECT_NE(orders[0]->describe().find("LIMIT"), std::string::npos);
    EXPECT_NE(orders[1]->describe().find("MARKET"), std::string::npos);
    EXPECT_NE(orders[2]->describe().find("CANCEL"), std::string::npos);
}