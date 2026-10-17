#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>

#include "core/types.hpp"
#include "exchange/order_book.hpp"
#include "orders/order.hpp"

namespace quanta {

inline constexpr Price kFixtureMid = 10000;   // 100.00 rupees, in ticks

// Fill `book` with n resting limit orders that never cross:
//   ids are 1..n; even index = buy, odd index = sell
//   bids sit at 9999, 9998, ... and asks at 10001, 10002, ...
//   at most `levels` different prices per side
inline void fill_book(OrderBook& book, std::size_t n, std::size_t levels) {
    if (levels == 0 || levels >= static_cast<std::size_t>(kFixtureMid)) {
        throw std::invalid_argument("levels must be between 1 and 9999");
    }
    for (std::size_t i = 0; i < n; ++i) {
        const bool  buy    = (i % 2 == 0);
        const Price offset = static_cast<Price>((i / 2) % levels) + 1;
        const Price price  = buy ? kFixtureMid - offset : kFixtureMid + offset;
        book.add(std::make_unique<LimitOrder>(static_cast<OrderId>(i + 1), 1,
                                              buy ? Side::Buy : Side::Sell, 10, price));
    }
}

}  // namespace quanta