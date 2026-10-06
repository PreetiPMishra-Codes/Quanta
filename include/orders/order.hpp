#pragma once

#include <cstdint>
#include <string>

#include "core/types.hpp"

namespace quanta {

// ---------- Order kinds ----------
enum class OrderType : std::uint8_t { Market, Limit, Cancel };

constexpr const char* to_string(OrderType t) {
    switch (t) {
        case OrderType::Market: return "MARKET";
        case OrderType::Limit:  return "LIMIT";
        case OrderType::Cancel: return "CANCEL";
    }
    return "UNKNOWN";
}

// ---------- Base class: what EVERY order has ----------
class Order {
public:
    Order(OrderId id, TraderId trader, Timestamp ts)
        : id_(id), trader_id_(trader), timestamp_(ts) {}

    virtual ~Order() = default;

    // No copying: prevents "slicing" a derived order into a base order.
    Order(const Order&) = delete;
    Order& operator=(const Order&) = delete;

    OrderId   id() const noexcept        { return id_; }
    TraderId  trader_id() const noexcept { return trader_id_; }
    Timestamp timestamp() const noexcept { return timestamp_; }

    // Each child class must say what it is and how to print itself.
    virtual OrderType   type() const noexcept = 0;
    virtual std::string describe() const = 0;

private:
    OrderId   id_;
    TraderId  trader_id_;
    Timestamp timestamp_;
};

// ---------- Orders that buy/sell something ----------
class TradeOrder : public Order {
public:
    Side     side() const noexcept      { return side_; }
    Quantity quantity() const noexcept  { return quantity_; }
    Quantity remaining() const noexcept { return remaining_; }
    Quantity filled() const noexcept    { return quantity_ - remaining_; }
    bool     is_filled() const noexcept { return remaining_ == 0; }

    // Called by the matching engine when part/all of the order trades.
    void fill(Quantity qty);

protected:
    // protected: only child classes can construct a TradeOrder.
    TradeOrder(OrderId id, TraderId trader, Timestamp ts, Side side, Quantity qty);

private:
    Side     side_;
    Quantity quantity_;
    Quantity remaining_;
};

// ---------- Market order: trade now at the best available price ----------
class MarketOrder final : public TradeOrder {
public:
    MarketOrder(OrderId id, TraderId trader, Side side, Quantity qty,
                Timestamp ts = now());

    OrderType   type() const noexcept override { return OrderType::Market; }
    std::string describe() const override;
};

// ---------- Limit order: trade only at this price or better ----------
class LimitOrder final : public TradeOrder {
public:
    LimitOrder(OrderId id, TraderId trader, Side side, Quantity qty, Price price,
               Timestamp ts = now());

    Price price() const noexcept { return price_; }

    OrderType   type() const noexcept override { return OrderType::Limit; }
    std::string describe() const override;

private:
    Price price_;
};

// ---------- Cancel: "remove order #target_id" ----------
class CancelOrder final : public Order {
public:
    CancelOrder(OrderId id, TraderId trader, OrderId target_id,
                Timestamp ts = now())
        : Order(id, trader, ts), target_id_(target_id) {}

    OrderId target_id() const noexcept { return target_id_; }

    OrderType   type() const noexcept override { return OrderType::Cancel; }
    std::string describe() const override;

private:
    OrderId target_id_;
};

}  // namespace quanta