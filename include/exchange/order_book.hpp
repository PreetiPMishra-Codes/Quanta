#pragma once

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/types.hpp"
#include "orders/order.hpp"

namespace quanta {

class OrderBook {
public:
    OrderBook() = default;

    // The book owns its orders and keeps internal pointers, so no copying.
    OrderBook(const OrderBook&) = delete;
    OrderBook& operator=(const OrderBook&) = delete;

    // ---- Changing the book ----
    // Rest an order in the book. Throws on null or duplicate id.
    void add(std::unique_ptr<LimitOrder> order);

    // Remove an order by id and hand it back. Returns nullptr if not found.
    std::unique_ptr<LimitOrder> cancel(OrderId id);

    // ---- Looking at the book ----
    const LimitOrder* find(OrderId id) const;

    // Oldest order at the best price on that side (nullptr if side is empty).
    // The matching engine (Day 4) will use this.
    LimitOrder* front(Side side);

    std::optional<Price> best_bid() const;
    std::optional<Price> best_ask() const;
    std::optional<Price> spread() const;

    // Total remaining quantity at one price level.
    Quantity quantity_at(Side side, Price price) const;

    // Top N levels as (price, total quantity), best price first.
    std::vector<std::pair<Price, Quantity>> depth(Side side,
                                                  std::size_t levels) const;

    std::size_t order_count() const noexcept { return index_.size(); }
    std::size_t level_count(Side side) const noexcept;
    bool        empty() const noexcept { return index_.empty(); }

    // Printable ladder like the diagram above.
    std::string render(std::size_t levels = 5) const;

private:
    using Level  = std::list<std::unique_ptr<LimitOrder>>;
    using BidMap = std::map<Price, Level, std::greater<Price>>;
    using AskMap = std::map<Price, Level>;

    // Where an order lives, so cancel can jump straight to it.
    struct Locator {
        Side            side;
        Price           price;
        Level*          level;  // pointer to the price level (map nodes never move)
        Level::iterator it;     // position inside that level's queue
    };

    BidMap bids_;
    AskMap asks_;
    std::unordered_map<OrderId, Locator> index_;
};

}  // namespace quanta