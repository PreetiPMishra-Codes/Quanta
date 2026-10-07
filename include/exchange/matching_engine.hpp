#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "core/types.hpp"
#include "exchange/order_book.hpp"
#include "exchange/trade.hpp"
#include "orders/order.hpp"

namespace quanta {

// What happened to the order we were given.
enum class Outcome : std::uint8_t {
    Filled,          // completely executed
    Resting,         // limit order: leftover now waits in the book
    Expired,         // market order: leftover discarded (no liquidity)
    Cancelled,       // cancel succeeded
    CancelRejected   // cancel failed: unknown id, already filled, or not yours
};

const char* to_string(Outcome o);

struct ExecutionResult {
    OrderId            order_id  = 0;  // for a cancel, this is the TARGET order id
    Outcome            outcome   = Outcome::Filled;
    Quantity           filled    = 0;
    Quantity           remaining = 0;
    std::vector<Trade> trades;
};

class MatchingEngine {
public:
    MatchingEngine() = default;

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    // Limit order: match what crosses, rest the leftover. The book takes ownership.
    ExecutionResult submit_limit(std::unique_ptr<LimitOrder> order);

    // Market order: match what's available, discard the leftover.
    ExecutionResult submit_market(MarketOrder& order);

    // Cancel a resting order (only its owner may do this).
    ExecutionResult cancel(const CancelOrder& request);

    // Accept ANY kind of order and route it to the right handler.
    ExecutionResult process(std::unique_ptr<Order> order);

    const OrderBook& book() const noexcept { return book_; }
    std::uint64_t    trade_count() const noexcept { return next_trade_id_ - 1; }

private:
    // The core loop shared by limit and market orders.
    // limit == nullopt means "no price limit" (market order).
    void match(TradeOrder& incoming, std::optional<Price> limit,
               std::vector<Trade>& out);

    OrderBook     book_;
    std::uint64_t next_trade_id_ = 1;
};

}  // namespace quanta