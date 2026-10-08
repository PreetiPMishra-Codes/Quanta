#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include "core/types.hpp"
#include "exchange/matching_engine.hpp"
#include "orders/order.hpp"
#include "risk/risk_manager.hpp"
#include "traders/trader.hpp"

namespace quanta {

struct ExchangeResult {
    ExecutionResult execution;
    RejectReason    reject_reason = RejectReason::None;

    bool accepted() const noexcept { return reject_reason == RejectReason::None; }
};

// The front door of the exchange. It HAS a risk manager, a matching engine
// and a set of traders (composition), instead of inheriting from any of them.
class Exchange {
public:
    explicit Exchange(SelfTradePolicy policy = SelfTradePolicy::Allow)
        : engine_(policy) {}

    // The exchange takes ownership of each trader.
    Trader& add_trader(std::unique_ptr<Trader> trader);
    Trader*       find_trader(TraderId id);
    const Trader* find_trader(TraderId id) const;

    // Risk check, then match, then update the traders' cash and positions.
    ExchangeResult submit(std::unique_ptr<Order> order);

    const MatchingEngine& engine() const noexcept { return engine_; }
    std::uint64_t rejected_count() const noexcept { return rejected_count_; }

private:
    ExchangeResult reject(OrderId id, Quantity qty, RejectReason reason);
    void settle(const std::vector<Trade>& trades);

    MatchingEngine engine_;
    RiskManager    risk_;
    std::unordered_map<TraderId, std::unique_ptr<Trader>> traders_;
    std::uint64_t  rejected_count_ = 0;
};

}  // namespace quanta