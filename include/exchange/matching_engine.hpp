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

enum class Outcome : std::uint8_t {
    Filled,          // completely executed
    Resting,         // limit order: leftover now waits in the book
    Expired,         // market order: leftover discarded (no liquidity)
    Cancelled,       // cancel succeeded
    CancelRejected,  // cancel failed: unknown id, already filled, or not yours
    Rejected         // refused by risk checks (set by Exchange, never by the engine)
};

const char* to_string(Outcome o);

enum class SelfTradePolicy : std::uint8_t {
    Allow,          // a trader may trade with their own resting order
    CancelResting   // cancel their own resting order instead of trading with it
};

struct ExecutionResult {
    OrderId            order_id  = 0;  // for a cancel, this is the TARGET order id
    Outcome            outcome   = Outcome::Filled;
    Quantity           filled    = 0;
    Quantity           remaining = 0;
    std::uint32_t      self_trades_prevented = 0;
    std::vector<Trade> trades;
};

class MatchingEngine {
public:
    explicit MatchingEngine(SelfTradePolicy policy = SelfTradePolicy::Allow)
        : policy_(policy) {}

    MatchingEngine(const MatchingEngine&) = delete;
    MatchingEngine& operator=(const MatchingEngine&) = delete;

    ExecutionResult submit_limit(std::unique_ptr<LimitOrder> order);
    ExecutionResult submit_market(MarketOrder& order);
    ExecutionResult cancel(const CancelOrder& request);
    ExecutionResult process(std::unique_ptr<Order> order);

    const OrderBook& book() const noexcept { return book_; }
    std::uint64_t    trade_count() const noexcept { return next_trade_id_ - 1; }
    SelfTradePolicy  policy() const noexcept { return policy_; }

private:
    void match(TradeOrder& incoming, std::optional<Price> limit,
               ExecutionResult& result);

    OrderBook       book_;
    std::uint64_t   next_trade_id_ = 1;
    SelfTradePolicy policy_;
};

}  // namespace quanta