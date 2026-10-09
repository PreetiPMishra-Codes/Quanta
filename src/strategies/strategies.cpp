#include "strategies/strategies.hpp"

#include <algorithm>
#include <cmath>

namespace quanta {

namespace {

constexpr Quantity kMaxClip = 50;   // keep order sizes modest

// Random size in [1, min(trader's max, 50)].
Quantity random_qty(std::mt19937& rng, Quantity max_order_size) {
    const Quantity hi = std::max<Quantity>(1, std::min(max_order_size, kMaxClip));
    std::uniform_int_distribution<Quantity> d(1, hi);
    return d(rng);
}

}  // namespace

// ---------- Random ----------
void RandomStrategy::decide(const MarketView& view, std::mt19937& rng,
                            std::vector<Action>& out) {
    Action a;
    a.side     = std::bernoulli_distribution(0.5)(rng) ? Side::Buy : Side::Sell;
    a.quantity = random_qty(rng, view.max_order_size);

    if (std::bernoulli_distribution(market_prob_)(rng)) {
        a.type = OrderType::Market;
    } else {
        std::uniform_int_distribution<int> offset(-10, 10);   // +/- 0.10 rupee
        a.type  = OrderType::Limit;
        a.price = view.mid + offset(rng);
    }
    out.push_back(a);
}

// ---------- Momentum ----------
void MomentumStrategy::decide(const MarketView& view, std::mt19937& rng,
                              std::vector<Action>& out) {
    if (!has_prev_) {                 // first look: just remember the price
        prev_ = view.last_price;
        has_prev_ = true;
        return;
    }
    const Price change = view.last_price - prev_;
    prev_ = view.last_price;
    if (change == 0) return;          // no trend, no trade

    Action a;
    a.type     = OrderType::Limit;
    a.side     = change > 0 ? Side::Buy : Side::Sell;
    a.quantity = random_qty(rng, view.max_order_size);
    // Slightly aggressive: price through the middle so it is likely to trade.
    a.price    = a.side == Side::Buy ? view.mid + 3 : view.mid - 3;
    out.push_back(a);
}

// ---------- Mean reversion ----------
void MeanReversionStrategy::decide(const MarketView& view, std::mt19937& rng,
                                   std::vector<Action>& out) {
    const double last = static_cast<double>(view.last_price);
    if (!has_avg_) {
        avg_ = last;
        has_avg_ = true;
    }
    const double deviation = last - avg_;     // compare BEFORE updating the average
    avg_ += alpha_ * (last - avg_);           // exponential moving average

    if (std::fabs(deviation) <= threshold_) return;

    Action a;
    a.type     = OrderType::Limit;
    a.side     = deviation > 0 ? Side::Sell : Side::Buy;   // bet on a return to average
    a.quantity = random_qty(rng, view.max_order_size);
    a.price    = a.side == Side::Sell ? view.mid - 1 : view.mid + 1;
    out.push_back(a);
}

// ---------- Market making ----------
void MarketMakingStrategy::decide(const MarketView& view, std::mt19937& rng,
                                  std::vector<Action>& out) {
    const std::int64_t cap = static_cast<std::int64_t>(
        inventory_fraction_ * static_cast<double>(view.max_position));
    const Quantity qty = random_qty(rng, view.max_order_size);
    const Price bid = view.mid - half_spread_;
    const Price ask = view.mid + half_spread_;

    // Don't keep buying when already long near the cap, or selling when short.
    if (view.position < cap && bid > 0) {
        out.push_back(Action{OrderType::Limit, Side::Buy, qty, bid});
    }
    if (view.position > -cap && ask > 0) {
        out.push_back(Action{OrderType::Limit, Side::Sell, qty, ask});
    }
}

}  // namespace quanta