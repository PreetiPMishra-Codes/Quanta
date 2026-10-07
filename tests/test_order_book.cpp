#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>

#include "exchange/order_book.hpp"

using namespace quanta;

namespace {
// Small helper so tests stay short and readable.
std::unique_ptr<LimitOrder> make(OrderId id, Side side, Quantity qty, Price price) {
    return std::make_unique<LimitOrder>(id, /*trader=*/1, side, qty, price);
}
}  // namespace

TEST(OrderBook, EmptyBookHasNoPrices) {
    OrderBook book;
    EXPECT_TRUE(book.empty());
    EXPECT_FALSE(book.best_bid().has_value());
    EXPECT_FALSE(book.best_ask().has_value());
    EXPECT_FALSE(book.spread().has_value());
    EXPECT_EQ(book.front(Side::Buy), nullptr);
    EXPECT_EQ(book.front(Side::Sell), nullptr);
}

TEST(OrderBook, BestBidIsHighestPrice) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    book.add(make(2, Side::Buy, 10, 10010));
    book.add(make(3, Side::Buy, 10, 10005));
    ASSERT_TRUE(book.best_bid().has_value());
    EXPECT_EQ(*book.best_bid(), 10010);
}

TEST(OrderBook, BestAskIsLowestPrice) {
    OrderBook book;
    book.add(make(1, Side::Sell, 10, 10025));
    book.add(make(2, Side::Sell, 10, 10015));
    book.add(make(3, Side::Sell, 10, 10020));
    ASSERT_TRUE(book.best_ask().has_value());
    EXPECT_EQ(*book.best_ask(), 10015);
}

TEST(OrderBook, SpreadIsAskMinusBid) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10010));
    book.add(make(2, Side::Sell, 10, 10015));
    ASSERT_TRUE(book.spread().has_value());
    EXPECT_EQ(*book.spread(), 5);
}

TEST(OrderBook, TimePriorityWithinLevel) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    book.add(make(2, Side::Buy, 10, 10000));
    EXPECT_EQ(book.front(Side::Buy)->id(), 1u);  // first in, first served
    book.cancel(1);
    EXPECT_EQ(book.front(Side::Buy)->id(), 2u);
}

TEST(OrderBook, CancelRemovesAndReturnsOrder) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    auto cancelled = book.cancel(1);
    ASSERT_NE(cancelled, nullptr);
    EXPECT_EQ(cancelled->id(), 1u);
    EXPECT_EQ(book.find(1), nullptr);
    EXPECT_TRUE(book.empty());
}

TEST(OrderBook, CancelLastOrderRemovesTheLevel) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10010));
    book.add(make(2, Side::Buy, 10, 10000));
    book.cancel(1);
    EXPECT_EQ(book.level_count(Side::Buy), 1u);
    EXPECT_EQ(*book.best_bid(), 10000);  // best bid moved down
}

TEST(OrderBook, CancelUnknownIdReturnsNull) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    EXPECT_EQ(book.cancel(999), nullptr);
    EXPECT_EQ(book.order_count(), 1u);  // nothing changed
}

TEST(OrderBook, CancelTwiceSecondFails) {
    OrderBook book;
    book.add(make(1, Side::Sell, 10, 10000));
    EXPECT_NE(book.cancel(1), nullptr);
    EXPECT_EQ(book.cancel(1), nullptr);
}

TEST(OrderBook, CancelMiddleKeepsQueueOrder) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    book.add(make(2, Side::Buy, 10, 10000));
    book.add(make(3, Side::Buy, 10, 10000));
    book.cancel(2);
    EXPECT_EQ(book.front(Side::Buy)->id(), 1u);
    book.cancel(1);
    EXPECT_EQ(book.front(Side::Buy)->id(), 3u);
}

TEST(OrderBook, DuplicateIdThrows) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    EXPECT_THROW(book.add(make(1, Side::Sell, 10, 10100)), std::invalid_argument);
    EXPECT_EQ(book.order_count(), 1u);
}

TEST(OrderBook, NullOrderThrows) {
    OrderBook book;
    EXPECT_THROW(book.add(nullptr), std::invalid_argument);
}

TEST(OrderBook, QuantityAtSumsRemainingOnly) {
    OrderBook book;
    book.add(make(1, Side::Buy, 30, 10000));
    book.add(make(2, Side::Buy, 70, 10000));
    book.add(make(3, Side::Buy, 5, 10005));
    EXPECT_EQ(book.quantity_at(Side::Buy, 10000), 100u);
    EXPECT_EQ(book.quantity_at(Side::Buy, 10005), 5u);
    EXPECT_EQ(book.quantity_at(Side::Buy, 99999), 0u);

    book.front(Side::Buy)->fill(0 + 5);  // best level (10005) is filled completely
    EXPECT_EQ(book.quantity_at(Side::Buy, 10005), 0u);
}

TEST(OrderBook, DepthIsSortedBestFirst) {
    OrderBook book;
    book.add(make(1, Side::Buy, 10, 10000));
    book.add(make(2, Side::Buy, 20, 10010));
    book.add(make(3, Side::Buy, 30, 10005));
    book.add(make(4, Side::Sell, 5, 10030));
    book.add(make(5, Side::Sell, 6, 10020));

    auto bids = book.depth(Side::Buy, 10);
    ASSERT_EQ(bids.size(), 3u);
    EXPECT_EQ(bids[0].first, 10010);  // highest first
    EXPECT_EQ(bids[1].first, 10005);
    EXPECT_EQ(bids[2].first, 10000);

    auto asks = book.depth(Side::Sell, 1);  // only the top level
    ASSERT_EQ(asks.size(), 1u);
    EXPECT_EQ(asks[0].first, 10020);  // lowest first
    EXPECT_EQ(asks[0].second, 6u);
}

TEST(OrderBook, FindReturnsRestingOrder) {
    OrderBook book;
    book.add(make(7, Side::Buy, 10, 10000));
    const LimitOrder* o = book.find(7);
    ASSERT_NE(o, nullptr);
    EXPECT_EQ(o->price(), 10000);
    EXPECT_EQ(book.find(8), nullptr);
}

TEST(OrderBook, ManyAddsAndCancelsLeaveBookEmpty) {
    OrderBook book;
    for (OrderId id = 1; id <= 1000; ++id) {
        const Side  side  = (id % 2 == 0) ? Side::Buy : Side::Sell;
        const Price price = 10000 + static_cast<Price>(id % 50);
        book.add(make(id, side, 10, price));
    }
    EXPECT_EQ(book.order_count(), 1000u);

    for (OrderId id = 1; id <= 1000; ++id) {
        EXPECT_NE(book.cancel(id), nullptr);
    }
    EXPECT_TRUE(book.empty());
    EXPECT_EQ(book.level_count(Side::Buy), 0u);   // no leftover empty levels
    EXPECT_EQ(book.level_count(Side::Sell), 0u);
}