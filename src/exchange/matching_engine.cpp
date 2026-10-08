#include "exchange/matching_engine.hpp"

#include <algorithm>
#include <stdexcept>

namespace quanta {

const char* to_string(Outcome o) {
    switch (o) {
        case Outcome::Filled:         return "FILLED";
        case Outcome::Resting:        return "RESTING";
        case Outcome::Expired:        return "EXPIRED";
        case Outcome::Cancelled:      return "CANCELLED";
        case Outcome::CancelRejected: return "CANCEL_REJECTED";
        case Outcome::Rejected:       return "REJECTED";
    }
    return "UNKNOWN";
}

namespace {

bool crosses(Side incoming_side, Price resting_price, std::optional<Price> limit) {
    if (!limit) return true;  // market order accepts any price
    return incoming_side == Side::Buy ? resting_price <= *limit
                                      : resting_price >= *limit;
}

}  // namespace

// ---------- the core loop ----------
void MatchingEngine::match(TradeOrder& incoming, std::optional<Price> limit,
                           ExecutionResult& result) {
    const Side resting_side = opposite(incoming.side());

    while (!incoming.is_filled()) {
        LimitOrder* resting = book_.front(resting_side);
        if (resting == nullptr) break;
        if (!crosses(incoming.side(), resting->price(), limit)) break;

        // Self-trade prevention: remove our own resting order, then look again.
        if (policy_ == SelfTradePolicy::CancelResting &&
            resting->trader_id() == incoming.trader_id()) {
            book_.cancel(resting->id());   // 'resting' is dangling after this
            ++result.self_trades_prevented;
            continue;                      // re-fetch the new best order
        }

        const Quantity qty   = std::min(incoming.remaining(), resting->remaining());
        const Price    price = resting->price();

        incoming.fill(qty);
        resting->fill(qty);

        const bool buy_is_incoming = (incoming.side() == Side::Buy);
        result.trades.push_back(Trade{
            next_trade_id_++,
            buy_is_incoming ? incoming.id()        : resting->id(),
            buy_is_incoming ? resting->id()        : incoming.id(),
            buy_is_incoming ? incoming.trader_id() : resting->trader_id(),
            buy_is_incoming ? resting->trader_id() : incoming.trader_id(),
            price,
            qty,
            incoming.side(),
            now()});

        if (resting->is_filled()) {
            book_.cancel(resting->id());
        }
    }
}

// ---------- limit ----------
ExecutionResult MatchingEngine::submit_limit(std::unique_ptr<LimitOrder> order) {
    if (!order) {
        throw std::invalid_argument("cannot submit a null order");
    }
    if (book_.find(order->id()) != nullptr) {
        throw std::invalid_argument("duplicate order id");
    }

    ExecutionResult result;
    result.order_id = order->id();

    match(*order, order->price(), result);

    result.filled    = order->filled();
    result.remaining = order->remaining();

    if (order->is_filled()) {
        result.outcome = Outcome::Filled;
    } else {
        book_.add(std::move(order));
        result.outcome = Outcome::Resting;
    }
    return result;
}

// ---------- market ----------
ExecutionResult MatchingEngine::submit_market(MarketOrder& order) {
    if (book_.find(order.id()) != nullptr) {
        throw std::invalid_argument("duplicate order id");
    }

    ExecutionResult result;
    result.order_id = order.id();

    match(order, std::nullopt, result);

    result.filled    = order.filled();
    result.remaining = order.remaining();
    result.outcome   = order.is_filled() ? Outcome::Filled : Outcome::Expired;
    return result;
}

// ---------- cancel ----------
ExecutionResult MatchingEngine::cancel(const CancelOrder& request) {
    ExecutionResult result;
    result.order_id = request.target_id();

    const LimitOrder* target = book_.find(request.target_id());
    if (target == nullptr || target->trader_id() != request.trader_id()) {
        result.outcome = Outcome::CancelRejected;
        return result;
    }

    auto removed     = book_.cancel(request.target_id());
    result.outcome   = Outcome::Cancelled;
    result.filled    = removed->filled();
    result.remaining = removed->remaining();
    return result;
}

// ---------- dispatch ----------
ExecutionResult MatchingEngine::process(std::unique_ptr<Order> order) {
    if (!order) {
        throw std::invalid_argument("cannot process a null order");
    }
    switch (order->type()) {
        case OrderType::Limit:
            return submit_limit(std::unique_ptr<LimitOrder>(
                static_cast<LimitOrder*>(order.release())));
        case OrderType::Market:
            return submit_market(static_cast<MarketOrder&>(*order));
        case OrderType::Cancel:
            return cancel(static_cast<CancelOrder&>(*order));
    }
    throw std::logic_error("unknown order type");
}

}  // namespace quanta