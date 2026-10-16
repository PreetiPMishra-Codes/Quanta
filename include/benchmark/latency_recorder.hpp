#pragma once

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

}  // namespace quanta