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

LatencyStats LatencyRecorder::summarize() const {
    LatencyStats s;
    if (samples_.empty()) return s;

    std::vector<std::uint64_t> sorted = samples_;
    std::sort(sorted.begin(), sorted.end());

    s.count    = sorted.size();
    s.min_ns   = sorted.front();
    s.max_ns   = sorted.back();
    s.total_ns = std::accumulate(sorted.begin(), sorted.end(), std::uint64_t{0});
    s.mean_ns  = static_cast<double>(s.total_ns) / static_cast<double>(s.count);
    s.p50_ns   = percentile_permille(sorted, 500);
    s.p95_ns   = percentile_permille(sorted, 950);
    s.p99_ns   = percentile_permille(sorted, 990);
    s.p999_ns  = percentile_permille(sorted, 999);
    return s;
}

}  // namespace quanta