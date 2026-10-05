#pragma once

#include <chrono>
#include <cmath>
#include <cstdint>

namespace quanta {

// ---- Basic type aliases ----
using Price    = std::int64_t;   // price in TICKS (integer), never double
using Quantity = std::uint32_t;  // number of shares
using OrderId  = std::uint64_t;  // unique id per order
using TraderId = std::uint32_t;  // unique id per trader

// 1 rupee = 100 ticks, so 1 tick = 1 paisa
inline constexpr Price TICKS_PER_RUPEE = 100;

// ---- Order side ----
enum class Side : std::uint8_t { Buy, Sell };

constexpr Side opposite(Side s) {
    return s == Side::Buy ? Side::Sell : Side::Buy;
}

constexpr const char* to_string(Side s) {
    return s == Side::Buy ? "BUY" : "SELL";
}

// ---- Time ----
using Clock     = std::chrono::steady_clock;
using Timestamp = Clock::time_point;

inline Timestamp now() { return Clock::now(); }

// ---- Price conversion helpers ----
inline Price rupees_to_ticks(double rupees) {
    return static_cast<Price>(std::llround(rupees * TICKS_PER_RUPEE));
}

inline double ticks_to_rupees(Price ticks) {
    return static_cast<double>(ticks) / TICKS_PER_RUPEE;
}

}  // namespace quanta