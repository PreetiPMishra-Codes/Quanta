#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <set>
#include <string>

#include "benchmark/latency_recorder.hpp"
#include "exchange/order_book.hpp"
#include "simulation/simulator.hpp"

using namespace quanta;

namespace {
SimulationConfig config(std::uint32_t seed = 42) {
    SimulationConfig c;
    c.seed = seed;
    c.num_traders = 30;
    return c;
}
}  // namespace

// ---------- recorder (no simulator needed) ----------

TEST(Detailed, CategoryNamesAreDistinct) {
    std::set<std::string> names;
    for (std::size_t i = 0; i < kCategoryCount; ++i) {
        names.insert(to_string(static_cast<Category>(i)));
    }
    EXPECT_EQ(names.size(), kCategoryCount);
    EXPECT_STREQ(to_string(Category::Market), "market");
}

TEST(Detailed, RecorderStoresEveryField) {
    DetailedRecorder rec;
    rec.record_ns(123, Category::CancelOk, 77, true);
    ASSERT_EQ(rec.size(), 1u);
    const DetailedSample& s = rec.samples()[0];
    EXPECT_EQ(s.ns, 123u);
    EXPECT_EQ(s.book_orders, 77u);
    EXPECT_EQ(s.category, Category::CancelOk);
    EXPECT_TRUE(s.rehashed);
}

TEST(Detailed, DurationOverloadConvertsToNanoseconds) {
    DetailedRecorder rec;
    rec.record(std::chrono::microseconds(2), Category::Market, 0, false);
    EXPECT_EQ(rec.samples()[0].ns, 2000u);
}

TEST(Detailed, SummarizeIfFiltersByCategory) {
    DetailedRecorder rec;
    for (std::uint64_t i = 1; i <= 10; ++i) rec.record_ns(i, Category::LimitRested, 0, false);
    for (std::uint64_t i = 1; i <= 4; ++i) rec.record_ns(i * 100, Category::Market, 0, false);

    const LatencyStats market = rec.summarize_if(
        [](const DetailedSample& s) { return s.category == Category::Market; });
    EXPECT_EQ(market.count, 4u);
    EXPECT_EQ(market.max_ns, 400u);

    const LatencyStats none = rec.summarize_if(
        [](const DetailedSample& s) { return s.category == Category::Rejected; });
    EXPECT_EQ(none.count, 0u);

    EXPECT_EQ(rec.summarize_all().count, 14u);
}

TEST(Detailed, BookGroupsSplitEvenly) {
    DetailedRecorder rec;
    for (std::uint32_t i = 1; i <= 100; ++i) {
        rec.record_ns(i, Category::LimitRested, i, false);
    }
    const auto groups = rec.by_book_size(4);
    ASSERT_EQ(groups.size(), 4u);
    for (const BookGroup& g : groups) EXPECT_EQ(g.stats.count, 25u);
    EXPECT_EQ(groups[0].min_book, 1u);
    EXPECT_EQ(groups[0].max_book, 25u);
    EXPECT_EQ(groups[3].min_book, 76u);
    EXPECT_EQ(groups[3].max_book, 100u);
    EXPECT_EQ(groups[0].stats.p50_ns, 13u);   // values 1..25
    EXPECT_EQ(groups[3].stats.p50_ns, 88u);   // values 76..100
}

TEST(Detailed, BookGroupsIgnoreInsertionOrder) {
    DetailedRecorder rec;
    for (std::uint32_t i = 100; i >= 1; --i) {
        rec.record_ns(i, Category::LimitRested, i, false);
    }
    const auto groups = rec.by_book_size(4);
    ASSERT_EQ(groups.size(), 4u);
    EXPECT_EQ(groups[0].min_book, 1u);
    EXPECT_EQ(groups[3].max_book, 100u);
}

TEST(Detailed, BookGroupsWithFewSamples) {
    DetailedRecorder rec;
    EXPECT_TRUE(rec.by_book_size(4).empty());
    rec.record_ns(1, Category::Market, 5, false);
    rec.record_ns(2, Category::Market, 6, false);
    rec.record_ns(3, Category::Market, 7, false);
    const auto groups = rec.by_book_size(4);
    EXPECT_EQ(groups.size(), 3u);             // never more groups than samples
    EXPECT_TRUE(rec.by_book_size(0).empty());
}

TEST(Detailed, SlowestReturnsLargestFirst) {
    DetailedRecorder rec;
    for (std::uint64_t v : {5u, 1u, 9u, 3u, 10u, 2u, 8u, 4u, 7u, 6u}) {
        rec.record_ns(v, Category::LimitRested, 0, false);
    }
    const auto top = rec.slowest(3);
    ASSERT_EQ(top.size(), 3u);
    EXPECT_EQ(top[0].ns, 10u);
    EXPECT_EQ(top[1].ns, 9u);
    EXPECT_EQ(top[2].ns, 8u);
    EXPECT_EQ(rec.slowest(100).size(), 10u);
    EXPECT_EQ(rec.size(), 10u);               // the recorder itself is untouched
}

TEST(Detailed, CountByCategoryAtLeast) {
    DetailedRecorder rec;
    rec.record_ns(10, Category::Market, 0, false);
    rec.record_ns(50, Category::Market, 0, false);
    rec.record_ns(60, Category::LimitTraded, 0, false);
    rec.record_ns(5, Category::CancelOk, 0, false);

    const auto all = rec.count_by_category_at_least(0);
    EXPECT_EQ(all[static_cast<std::size_t>(Category::Market)], 2u);
    EXPECT_EQ(all[static_cast<std::size_t>(Category::CancelOk)], 1u);

    const auto slow = rec.count_by_category_at_least(50);
    EXPECT_EQ(slow[static_cast<std::size_t>(Category::Market)], 1u);
    EXPECT_EQ(slow[static_cast<std::size_t>(Category::LimitTraded)], 1u);
    EXPECT_EQ(slow[static_cast<std::size_t>(Category::CancelOk)], 0u);
}

TEST(Detailed, SummarizeValuesMatchesRecorder) {
    LatencyRecorder rec;
    std::vector<std::uint64_t> raw;
    for (std::uint64_t i = 100; i >= 1; --i) { rec.record_ns(i); raw.push_back(i); }
    const LatencyStats a = rec.summarize();
    const LatencyStats b = summarize_values(raw);
    EXPECT_EQ(a.p50_ns, b.p50_ns);
    EXPECT_EQ(a.p99_ns, b.p99_ns);
    EXPECT_EQ(a.max_ns, b.max_ns);
    EXPECT_EQ(a.total_ns, b.total_ns);
}

// ---------- order book and simulator hooks ----------

TEST(Detailed, OrderBookBucketCountGrows) {
    OrderBook book;
    for (OrderId id = 1; id <= 1000; ++id) {
        book.add(std::make_unique<LimitOrder>(
            id, 1, Side::Buy, 10, static_cast<Price>(10000) - static_cast<Price>(id % 50)));
    }
    // The standard guarantees load factor <= max load factor (1.0 by default).
    EXPECT_GE(book.index_bucket_count(), book.order_count());
}

TEST(Detailed, SimulatorRecordsEverySubmit) {
    Simulator sim(config());
    DetailedRecorder rec(20000);
    sim.set_detail_recorder(&rec);
    sim.run(5000);
    const auto& s = sim.stats();
    EXPECT_EQ(rec.size(), s.limit_orders + s.market_orders + s.cancels_sent);
    EXPECT_GT(rec.size(), 0u);
}

TEST(Detailed, SimulatorCategoriesMatchStats) {
    Simulator sim(config());
    DetailedRecorder rec(20000);
    sim.set_detail_recorder(&rec);
    sim.run(5000);
    const auto& s = sim.stats();

    const auto counts = rec.count_by_category_at_least(0);
    auto n = [&](Category c) { return counts[static_cast<std::size_t>(c)]; };

    EXPECT_EQ(n(Category::CancelOk), s.cancels_ok);
    EXPECT_EQ(n(Category::CancelOk) + n(Category::CancelFailed), s.cancels_sent);
    EXPECT_EQ(n(Category::Rejected), s.rejected);
    EXPECT_EQ(n(Category::LimitRested) + n(Category::LimitTraded) +
                  n(Category::Market) + n(Category::Rejected),
              s.limit_orders + s.market_orders);
}

TEST(Detailed, SimulatorSeesRehash) {
    Simulator sim(config());
    DetailedRecorder rec(30000);
    sim.set_detail_recorder(&rec);
    sim.run(10000);
    ASSERT_GT(rec.size(), 0u);
    EXPECT_EQ(rec.samples().front().book_orders, 0u);   // first order meets an empty book
    const LatencyStats re = rec.summarize_if(
        [](const DetailedSample& s) { return s.rehashed; });
    EXPECT_GT(re.count, 0u);                            // the index grew, so it rehashed
}

TEST(Detailed, DetailRecorderDoesNotChangeResults) {
    Simulator plain(config(7));
    Simulator timed(config(7));
    DetailedRecorder rec(20000);
    timed.set_detail_recorder(&rec);
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