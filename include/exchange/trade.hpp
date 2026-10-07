#pragma once

#include <cstdint>

#include "core/types.hpp"

namespace quanta {

// One execution: a buyer and a seller agreed on a price and quantity.
struct Trade {
    std::uint64_t id;
    OrderId       buy_order_id;
    OrderId       sell_order_id;
    TraderId      buyer;
    TraderId      seller;
    Price         price;      // always the RESTING order's price
    Quantity      quantity;
    Side          aggressor;  // side of the order that arrived and caused the trade
    Timestamp     timestamp;
};

}  // namespace quanta