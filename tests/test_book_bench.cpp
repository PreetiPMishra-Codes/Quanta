#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <vector>

#include "benchmark/book_fixture.hpp"
#include "benchmark/stats_util.hpp"

using namespace quanta;

TEST(BookBench, MedianOddAndEven) {
    EXPECT_DOUBLE_EQ(median_value({3.0, 1.0, 2.0}), 2.0);
    EXPECT_DOUBLE_EQ(median_value({4.0, 1.0, 3.0, 2.0}), 2.5);
    EXPECT_DOUBLE_EQ(median_value({7.0}), 7.0);
}

TEST(BookBench, MedianOfEmptyIsZero) {
    EXPECT_DOUBLE_EQ(median_value({}), 0.0);
    EXPECT_DOUBLE_EQ(spread_percent({}), 0.0);
}

TEST(BookBench, SpreadPercent) {
    EXPECT_DOUBLE_EQ(spread_percent({90.0, 100.0, 110.0}), 20.0);
    EXPECT_DOUBLE_EQ(spread_percent({5.0}), 0.0);
    EXPECT_DOUBLE_EQ(spread_percent({100.0, 100.0, 100.0}), 0.0);
}

TEST(BookBench, FixtureFillsRequestedOrders) {
    OrderBook book;
    fill_book(book, 1000, 50);
    EXPECT_EQ(book.order_count(), 1000u);
    EXPECT_EQ(book.level_count(Side::Buy), 50u);
    EXPECT_EQ(book.level_count(Side::Sell), 50u);
    EXPECT_EQ(*book.best_bid(), 9999);
    EXPECT_EQ(*book.best_ask(), 10001);
    EXPECT_EQ(*book.spread(), 2);
}

TEST(BookBench, FixtureNeverCrosses) {
    const std::size_t sizes[]  = {2, 10, 999, 1000, 5000};
    const std::size_t levels[] = {1, 7, 50, 1000};
    for (std::size_t n : sizes) {
        for (std::size_t l : levels) {
            OrderBook book;
            fill_book(book, n, l);
            ASSERT_EQ(book.order_count(), n);
            ASSERT_TRUE(book.best_bid().has_value());
            ASSERT_TRUE(book.best_ask().has_value());
            EXPECT_LT(*book.best_bid(), *book.best_ask()) << "n=" << n << " levels=" << l;
        }
    }
}

TEST(BookBench, FixtureWithFewOrdersHasFewerLevels) {
    OrderBook book;
    fill_book(book, 10, 50);               // only 5 orders per side
    EXPECT_EQ(book.order_count(), 10u);
    EXPECT_EQ(book.level_count(Side::Buy), 5u);
    EXPECT_EQ(book.level_count(Side::Sell), 5u);
}

TEST(BookBench, FixtureRejectsBadLevels) {
    OrderBook book;
    EXPECT_THROW(fill_book(book, 10, 0), std::invalid_argument);
    EXPECT_THROW(fill_book(book, 10, 10000), std::invalid_argument);
}

// The benchmark cancels orders and puts the SAME objects back. This proves that works.
TEST(BookBench, CancelledOrdersCanBeAddedBack) {
    OrderBook book;
    fill_book(book, 1000, 50);
    std::vector<std::unique_ptr<LimitOrder>> held;
    for (OrderId id : {17u, 500u, 1000u, 2u}) {
        auto p = book.cancel(id);
        ASSERT_NE(p, nullptr);
        held.push_back(std::move(p));
    }
    EXPECT_EQ(book.order_count(), 996u);
    for (auto& p : held) book.add(std::move(p));
    EXPECT_EQ(book.order_count(), 1000u);
    EXPECT_NE(book.find(17), nullptr);
}

// "consume best": pop the best asks one by one (what matching does), then restore.
TEST(BookBench, ConsumeBestPattern) {
    OrderBook book;
    fill_book(book, 1000, 50);             // 10 orders at each ask price
    std::vector<std::unique_ptr<LimitOrder>> held;
    for (int i = 0; i < 10; ++i) {
        LimitOrder* f = book.front(Side::Sell);
        ASSERT_NE(f, nullptr);
        held.push_back(book.cancel(f->id()));
    }
    EXPECT_EQ(*book.best_ask(), 10002);    // the whole best level is gone
    for (auto& p : held) book.add(std::move(p));
    EXPECT_EQ(*book.best_ask(), 10001);
    EXPECT_EQ(book.order_count(), 1000u);
}