#include "risk/risk_manager.hpp"

namespace quanta {

const char* to_string(RejectReason r) {
    switch (r) {
        case RejectReason::None:              return "NONE";
        case RejectReason::UnknownTrader:     return "UNKNOWN_TRADER";
        case RejectReason::OrderTooLarge:     return "ORDER_TOO_LARGE";
        case RejectReason::PositionLimit:     return "POSITION_LIMIT";
        case RejectReason::InsufficientFunds: return "INSUFFICIENT_FUNDS";
    }
    return "UNKNOWN";
}

RejectReason RiskManager::check(const Trader& trader, const TradeOrder& order,
                                std::optional<Price> reference_price) const {
    const Quantity qty = order.quantity();

    // 1. Maximum order size
    if (qty > trader.max_order_size()) {
        return RejectReason::OrderTooLarge;
    }

    // 2. Position limit (long or short)
    const std::int64_t signed_qty = static_cast<std::int64_t>(qty);
    const std::int64_t after =
        trader.position() + (order.side() == Side::Buy ? signed_qty : -signed_qty);
    const std::int64_t limit = trader.max_position();
    if (after > limit || after < -limit) {
        return RejectReason::PositionLimit;
    }

    // 3. Funds (buys only). Division instead of multiplication avoids overflow.
    if (order.side() == Side::Buy && reference_price.has_value()) {
        if (*reference_price > trader.cash() / static_cast<Price>(qty)) {
            return RejectReason::InsufficientFunds;
        }
    }

    return RejectReason::None;
}

}  // namespace quanta