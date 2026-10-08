#include "exchange/exchange.hpp"

#include <stdexcept>

namespace quanta {

Trader& Exchange::add_trader(std::unique_ptr<Trader> trader) {
    if (!trader) {
        throw std::invalid_argument("cannot add a null trader");
    }
    const TraderId id = trader->id();
    if (traders_.count(id) != 0) {
        throw std::invalid_argument("duplicate trader id");
    }
    Trader& ref = *trader;              // the object lives on the heap, so ref stays valid
    traders_.emplace(id, std::move(trader));
    return ref;
}

Trader* Exchange::find_trader(TraderId id) {
    auto it = traders_.find(id);
    return it == traders_.end() ? nullptr : it->second.get();
}

const Trader* Exchange::find_trader(TraderId id) const {
    auto it = traders_.find(id);
    return it == traders_.end() ? nullptr : it->second.get();
}

ExchangeResult Exchange::reject(OrderId id, Quantity qty, RejectReason reason) {
    ++rejected_count_;
    ExchangeResult out;
    out.execution.order_id  = id;
    out.execution.outcome   = Outcome::Rejected;
    out.execution.remaining = qty;
    out.reject_reason       = reason;
    return out;
}

ExchangeResult Exchange::submit(std::unique_ptr<Order> order) {
    if (!order) {
        throw std::invalid_argument("cannot submit a null order");
    }

    const Trader* trader = find_trader(order->trader_id());
    if (trader == nullptr) {
        return reject(order->id(), 0, RejectReason::UnknownTrader);
    }

    // Cancels are not risk-checked: they only reduce exposure.
    if (order->type() != OrderType::Cancel) {
        const auto* trade_order = dynamic_cast<const TradeOrder*>(order.get());
        if (trade_order == nullptr) {
            throw std::logic_error("non-cancel order is not a TradeOrder");
        }

        std::optional<Price> reference;
        if (order->type() == OrderType::Limit) {
            reference = static_cast<const LimitOrder&>(*trade_order).price();
        } else {  // market: estimate with the best price on the other side
            reference = trade_order->side() == Side::Buy
                            ? engine_.book().best_ask()
                            : engine_.book().best_bid();
        }

        const RejectReason reason = risk_.check(*trader, *trade_order, reference);
        if (reason != RejectReason::None) {
            return reject(order->id(), trade_order->quantity(), reason);
        }
    }

    ExchangeResult out;
    out.execution = engine_.process(std::move(order));   // 'order' is empty after this
    settle(out.execution.trades);
    return out;
}

void Exchange::settle(const std::vector<Trade>& trades) {
    for (const Trade& t : trades) {
        if (Trader* buyer = find_trader(t.buyer)) {
            buyer->apply_fill(Side::Buy, t.quantity, t.price);
        }
        if (Trader* seller = find_trader(t.seller)) {
            seller->apply_fill(Side::Sell, t.quantity, t.price);
        }
    }
}

}  // namespace quanta