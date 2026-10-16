#include "benchmark/latency_recorder.hpp"

#include <algorithm>
#include <numeric>
#include <stdexcept>

namespace quanta {

std::uint64_t percentile_permille(const std::vector<std::uint64_t>& sorted,
                                  unsigned permille) {
    if (sorted.empty()) {
        throw std::invalid_argument("percentile of empty data");
    }
    if (permille > 1000) {
        throw std::invalid_argument("permille must be between 0 and 1000");
    }
    const std::uint64_t n = sorted.size();
    // rank = ceil(permille * n / 1000), using integer arithmetic
    std::uint64_t rank = (static_cast<std::uint64_t>(permille) * n + 999) / 1000;
    if (rank < 1) rank = 1;
    if (rank > n) rank = n;
    return sorted[static_cast<std::size_t>(rank - 1)];
}

LatencyStats summarize_values(std::vector<std::uint64_t> values) {
    LatencyStats s;
    if (values.empty()) return s;

    std::sort(values.begin(), values.end());

    s.count    = values.size();
    s.min_ns   = values.front();
    s.max_ns   = values.back();
    s.total_ns = std::accumulate(values.begin(), values.end(), std::uint64_t{0});
    s.mean_ns  = static_cast<double>(s.total_ns) / static_cast<double>(s.count);
    s.p50_ns   = percentile_permille(values, 500);
    s.p95_ns   = percentile_permille(values, 950);
    s.p99_ns   = percentile_permille(values, 990);
    s.p999_ns  = percentile_permille(values, 999);
    return s;
}

LatencyStats LatencyRecorder::summarize() const {
    return summarize_values(samples_);   // passes a copy, so samples_ stays untouched
}

// ---------- Day 9 ----------

const char* to_string(Category c) {
    switch (c) {
        case Category::LimitRested:  return "limit-rested";
        case Category::LimitTraded:  return "limit-traded";
        case Category::Market:       return "market";
        case Category::CancelOk:     return "cancel-ok";
        case Category::CancelFailed: return "cancel-failed";
        case Category::Rejected:     return "rejected";
    }
    return "unknown";
}

LatencyStats DetailedRecorder::summarize_all() const {
    return summarize_if([](const DetailedSample&) { return true; });
}

std::vector<BookGroup> DetailedRecorder::by_book_size(std::size_t groups) const {
    std::vector<BookGroup> out;
    const std::size_t n = samples_.size();
    if (n == 0 || groups == 0) return out;
    const std::size_t g = std::min(groups, n);

    std::vector<DetailedSample> sorted = samples_;
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const DetailedSample& a, const DetailedSample& b) {
                         return a.book_orders < b.book_orders;
                     });

    for (std::size_t i = 0; i < g; ++i) {
        const std::size_t begin = i * n / g;
        const std::size_t end   = (i + 1) * n / g;
        std::vector<std::uint64_t> values;
        values.reserve(end - begin);
        for (std::size_t k = begin; k < end; ++k) values.push_back(sorted[k].ns);

        BookGroup group;
        group.min_book = sorted[begin].book_orders;
        group.max_book = sorted[end - 1].book_orders;
        group.stats    = summarize_values(std::move(values));
        out.push_back(group);
    }
    return out;
}

std::vector<DetailedSample> DetailedRecorder::slowest(std::size_t n) const {
    std::vector<DetailedSample> copy = samples_;
    n = std::min(n, copy.size());
    std::partial_sort(copy.begin(), copy.begin() + static_cast<std::ptrdiff_t>(n),
                      copy.end(),
                      [](const DetailedSample& a, const DetailedSample& b) {
                          return a.ns > b.ns;
                      });
    copy.resize(n);
    return copy;
}

std::array<std::size_t, kCategoryCount> DetailedRecorder::count_by_category_at_least(
    std::uint64_t min_ns) const {
    std::array<std::size_t, kCategoryCount> counts{};
    for (const DetailedSample& s : samples_) {
        if (s.ns >= min_ns) ++counts[static_cast<std::size_t>(s.category)];
    }
    return counts;
}

}  // namespace quanta