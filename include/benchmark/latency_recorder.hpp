#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/types.hpp"

namespace quanta {

struct LatencyStats {
    std::size_t   count    = 0;
    std::uint64_t min_ns   = 0;
    double        mean_ns  = 0.0;
    std::uint64_t p50_ns   = 0;
    std::uint64_t p95_ns   = 0;
    std::uint64_t p99_ns   = 0;
    std::uint64_t p999_ns  = 0;
    std::uint64_t max_ns   = 0;
    std::uint64_t total_ns = 0;   // sum of all samples
};

// Nearest-rank percentile on SORTED data.
// permille: 500 = P50, 950 = P95, 990 = P99, 999 = P99.9.
// Integer maths on purpose: no floating-point rounding surprises.
std::uint64_t percentile_permille(const std::vector<std::uint64_t>& sorted,
                                  unsigned permille);

// Statistics of raw values. Takes a copy because it sorts.
LatencyStats summarize_values(std::vector<std::uint64_t> values);

class LatencyRecorder {
public:
    // Pass the expected number of samples so nothing is allocated while measuring.
    explicit LatencyRecorder(std::size_t expected_samples = 0) {
        samples_.reserve(expected_samples);
    }

    void record(Clock::duration d) {
        samples_.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(d).count()));
    }
    void record_ns(std::uint64_t ns) { samples_.push_back(ns); }

    std::size_t size() const noexcept { return samples_.size(); }

    // Sorts a COPY, so the recorder itself is never modified.
    LatencyStats summarize() const;

private:
    std::vector<std::uint64_t> samples_;
};

// ---------- Day 9: samples that remember WHAT was timed ----------

// What kind of work one submit() call did.
enum class Category : std::uint8_t {
    LimitRested,   // limit order, no trade, just went into the book
    LimitTraded,   // limit order that traded at least once
    Market,        // market order
    CancelOk,      // cancel that removed an order
    CancelFailed,  // cancel of an order that was already gone
    Rejected       // refused by the risk checks
};
inline constexpr std::size_t kCategoryCount = 6;

const char* to_string(Category c);

struct DetailedSample {
    std::uint64_t ns          = 0;
    std::uint32_t book_orders = 0;      // resting orders BEFORE this submit
    Category      category    = Category::LimitRested;
    bool          rehashed    = false;  // did the cancel index rehash during this submit?
};

struct BookGroup {
    std::uint32_t min_book = 0;
    std::uint32_t max_book = 0;
    LatencyStats  stats;
};

class DetailedRecorder {
public:
    explicit DetailedRecorder(std::size_t expected_samples = 0) {
        samples_.reserve(expected_samples);
    }

    void record(Clock::duration d, Category c, std::uint32_t book_orders,
                bool rehashed) {
        record_ns(static_cast<std::uint64_t>(
                      std::chrono::duration_cast<std::chrono::nanoseconds>(d).count()),
                  c, book_orders, rehashed);
    }
    void record_ns(std::uint64_t ns, Category c, std::uint32_t book_orders,
                   bool rehashed) {
        samples_.push_back(DetailedSample{ns, book_orders, c, rehashed});
    }

    std::size_t size() const noexcept { return samples_.size(); }
    const std::vector<DetailedSample>& samples() const noexcept { return samples_; }

    // Statistics of every sample for which pred(sample) is true.
    template <typename Pred>
    LatencyStats summarize_if(Pred pred) const {
        std::vector<std::uint64_t> values;
        for (const DetailedSample& s : samples_) {
            if (pred(s)) values.push_back(s.ns);
        }
        return summarize_values(std::move(values));
    }

    LatencyStats summarize_all() const;

    // Sort by book size, cut into `groups` equal-count parts, summarize each.
    std::vector<BookGroup> by_book_size(std::size_t groups) const;

    // The n slowest samples, slowest first.
    std::vector<DetailedSample> slowest(std::size_t n) const;

    // How many samples of each category took at least min_ns.
    std::array<std::size_t, kCategoryCount> count_by_category_at_least(
        std::uint64_t min_ns) const;

private:
    std::vector<DetailedSample> samples_;
};

}  // namespace quanta