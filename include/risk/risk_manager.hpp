#pragma once

#include <cstdint>
#include <optional>

#include "core/types.hpp"
#include "orders/order.hpp"
#include "traders/trader.hpp"

namespace quanta {

enum class RejectReason : std::uint8_t {
    None,
    UnknownTrader,
    OrderTooLarge,
    PositionLimit,
    InsufficientFunds
};

const char* to_string(RejectReason r);

// Stateless: it only looks at what it is given, so it is easy to test.
class RiskManager {
public:
    // reference_price = the price used to estimate the cost of a buy.
    //   limit order  -> its own limit price
    //   market order -> best price on the other side (nullopt if none)
    RejectReason check(const Trader& trader, const TradeOrder& order,
                       std::optional<Price> reference_price) const;
};

}  // namespace quanta