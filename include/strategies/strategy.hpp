#pragma once

#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "core/types.hpp"
#include "orders/order.hpp"

namespace quanta {

// Everything a strategy is allowed to know. Read-only snapshot.
struct MarketView {
    std::optional<Price> best_bid;
    std::optional<Price> best_ask;
    Price         last_price = 0;       // price of the most recent trade
    Price         mid = 0;              // reference price (see Simulator::view_for)
    std::int64_t  position = 0;         // this trader's shares
    Price         cash = 0;             // this trader's cash, in ticks
    Quantity      max_order_size = 0;
    std::int64_t  max_position = 0;
};

// A decision, as plain data. The simulator turns it into a real order.
struct Action {
    OrderType type     = OrderType::Limit;   // Limit or Market only
    Side      side     = Side::Buy;
    Quantity  quantity = 0;
    Price     price    = 0;                  // ignored for market orders
};

class TradingStrategy {
public:
    virtual ~TradingStrategy() = default;

    virtual std::string name() const = 0;

    // Append zero or more actions to `out`. Not const: strategies keep state.
    virtual void decide(const MarketView& view, std::mt19937& rng,
                        std::vector<Action>& out) = 0;
};

}  // namespace quanta