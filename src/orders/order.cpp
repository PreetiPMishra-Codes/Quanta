#include "orders/order.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace quanta {

// ---------- TradeOrder ----------
TradeOrder::TradeOrder(OrderId id, TraderId trader, Timestamp ts,
                       Side side, Quantity qty)
    : Order(id, trader, ts), side_(side), quantity_(qty), remaining_(qty) {
    if (qty == 0) {
        throw std::invalid_argument("order quantity must be greater than 0");
    }
}

void TradeOrder::fill(Quantity qty) {
    if (qty == 0 || qty > remaining_) {
        throw std::invalid_argument("invalid fill quantity");
    }
    remaining_ -= qty;
}

// ---------- MarketOrder ----------
MarketOrder::MarketOrder(OrderId id, TraderId trader, Side side, Quantity qty,
                         Timestamp ts)
    : TradeOrder(id, trader, ts, side, qty) {}

std::string MarketOrder::describe() const {
    std::ostringstream out;
    out << "MARKET #" << id() << " trader=" << trader_id() << " "
        << to_string(side()) << " " << quantity()
        << " (remaining " << remaining() << ")";
    return out.str();
}

// ---------- LimitOrder ----------
LimitOrder::LimitOrder(OrderId id, TraderId trader, Side side, Quantity qty,
                       Price price, Timestamp ts)
    : TradeOrder(id, trader, ts, side, qty), price_(price) {
    if (price <= 0) {
        throw std::invalid_argument("limit price must be greater than 0");
    }
}

std::string LimitOrder::describe() const {
    std::ostringstream out;
    out << "LIMIT #" << id() << " trader=" << trader_id() << " "
        << to_string(side()) << " " << quantity() << " @ "
        << std::fixed << std::setprecision(2) << ticks_to_rupees(price_)
        << " (remaining " << remaining() << ")";
    return out.str();
}

// ---------- CancelOrder ----------
std::string CancelOrder::describe() const {
    std::ostringstream out;
    out << "CANCEL #" << id() << " trader=" << trader_id()
        << " target=#" << target_id();
    return out.str();
}

}  // namespace quanta