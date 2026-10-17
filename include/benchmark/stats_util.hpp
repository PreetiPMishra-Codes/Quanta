#pragma once

#include <algorithm>
#include <cstddef>
#include <vector>

namespace quanta {

// Median of the values (0 for an empty list). Takes a copy because it sorts.
inline double median_value(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return (n % 2 == 1) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

// (largest - smallest) / median, in percent. Tells you how noisy a set of runs was.
inline double spread_percent(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    const double lo  = *std::min_element(v.begin(), v.end());
    const double hi  = *std::max_element(v.begin(), v.end());
    const double med = median_value(v);
    return med > 0.0 ? (hi - lo) / med * 100.0 : 0.0;
}

}  // namespace quanta