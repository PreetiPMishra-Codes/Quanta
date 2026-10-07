#include "exchange/order_book.hpp"

#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>

namespace quanta {

namespace {

using LevelT = std::list<std::unique_ptr<LimitOrder>>;

Quantity total_remaining(const LevelT& level) {
    Quantity sum = 0;
    for (const auto& order : level) {
        sum += order->remaining();
    }
    return sum;
}

// Works for both the bid map and the ask map.
template <typename Map>
std::vector<std::pair<Price, Quantity>> collect(const Map& levels,
                                                std::size_t n) {
    std::vector<std::pair<Price, Quantity>> out;
    for (const auto& entry : levels) {
        if (out.size() >= n) break;
        out.emplace_back(entry.first, total_remaining(entry.second));
    }
    return out;
}

}  // namespace

// ---------- add ----------
void OrderBook::add(std::unique_ptr<LimitOrder> order) {
    if (!order) {
        throw std::invalid_argument("cannot add a null order");
    }
    const OrderId id = order->id();
    if (index_.count(id) != 0) {
        throw std::invalid_argument("duplicate order id");
    }
    if (order->is_filled()) {
        throw std::invalid_argument("cannot rest a fully filled order");
    }

    const Side  side  = order->side();
    const Price price = order->price();

    // operator[] creates the price level if it doesn't exist yet.
    Level* level = nullptr;
    if (side == Side::Buy) {
        level = &bids_[price];
    } else {
        level = &asks_[price];
    }

    level->push_back(std::move(order));  // back of queue = lowest time priority
    index_.emplace(id, Locator{side, price, level, std::prev(level->end())});
}

// ---------- cancel ----------
std::unique_ptr<LimitOrder> OrderBook::cancel(OrderId id) {
    auto found = index_.find(id);
    if (found == index_.end()) {
        return nullptr;
    }

    const Locator loc = found->second;     // copy before we erase anything
    auto order = std::move(*loc.it);       // take ownership out of the list
    loc.level->erase(loc.it);              // O(1) thanks to the stored iterator

    // Never leave an empty price level behind.
    if (loc.level->empty()) {
        if (loc.side == Side::Buy) {
            bids_.erase(loc.price);
        } else {
            asks_.erase(loc.price);
        }
    }

    index_.erase(found);
    return order;
}

// ---------- looking ----------
const LimitOrder* OrderBook::find(OrderId id) const {
    auto found = index_.find(id);
    return found == index_.end() ? nullptr : found->second.it->get();
}

LimitOrder* OrderBook::front(Side side) {
    if (side == Side::Buy) {
        return bids_.empty() ? nullptr : bids_.begin()->second.front().get();
    }
    return asks_.empty() ? nullptr : asks_.begin()->second.front().get();
}

std::optional<Price> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

std::optional<Price> OrderBook::spread() const {
    if (bids_.empty() || asks_.empty()) return std::nullopt;
    return asks_.begin()->first - bids_.begin()->first;
}

Quantity OrderBook::quantity_at(Side side, Price price) const {
    if (side == Side::Buy) {
        auto it = bids_.find(price);
        return it == bids_.end() ? 0 : total_remaining(it->second);
    }
    auto it = asks_.find(price);
    return it == asks_.end() ? 0 : total_remaining(it->second);
}

std::vector<std::pair<Price, Quantity>> OrderBook::depth(
    Side side, std::size_t levels) const {
    return side == Side::Buy ? collect(bids_, levels) : collect(asks_, levels);
}

std::size_t OrderBook::level_count(Side side) const noexcept {
    return side == Side::Buy ? bids_.size() : asks_.size();
}

// ---------- render ----------
std::string OrderBook::render(std::size_t levels) const {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2);

    // Asks printed highest to lowest, so the best ask sits next to the spread.
    const auto asks = depth(Side::Sell, levels);
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) {
        out << "  SELL " << std::setw(10) << ticks_to_rupees(it->first)
            << "  x " << it->second << "\n";
    }

    out << "  ---- spread: ";
    if (auto s = spread()) {
        out << ticks_to_rupees(*s);
    } else {
        out << "n/a";
    }
    out << " ----\n";

    for (const auto& level : depth(Side::Buy, levels)) {
        out << "  BUY  " << std::setw(10) << ticks_to_rupees(level.first)
            << "  x " << level.second << "\n";
    }
    return out.str();
}

}  // namespace quanta