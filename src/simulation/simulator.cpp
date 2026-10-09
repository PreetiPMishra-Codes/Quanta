#include "simulation/simulator.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

#include "strategies/strategies.hpp"

namespace quanta {

Simulator::Simulator(const SimulationConfig& cfg)
    : cfg_(cfg),
      rng_(cfg.seed),
      exchange_(cfg.self_trade_policy),
      last_price_(cfg.initial_price) {
    if (cfg.num_traders == 0) {
        throw std::invalid_argument("simulation needs at least one trader");
    }
    if (cfg.initial_price <= 0) {
        throw std::invalid_argument("initial price must be positive");
    }

    for (std::size_t i = 0; i < cfg.num_traders; ++i) {
        const TraderId id = static_cast<TraderId>(i + 1);
        const std::string name = "T" + std::to_string(id);

        // Trader kinds: 3 of 5 retail, 1 algorithmic, 1 institutional.
        switch (i % 5) {
            case 3:  exchange_.add_trader(std::make_unique<AlgorithmicTrader>(id, name)); break;
            case 4:  exchange_.add_trader(std::make_unique<InstitutionalTrader>(id, name)); break;
            default: exchange_.add_trader(std::make_unique<RetailTrader>(id, name)); break;
        }

        // Strategies rotate through the four kinds.
        switch (i % 4) {
            case 0:  strategies_.push_back(std::make_unique<RandomStrategy>()); break;
            case 1:  strategies_.push_back(std::make_unique<MomentumStrategy>()); break;
            case 2:  strategies_.push_back(std::make_unique<MeanReversionStrategy>()); break;
            default: strategies_.push_back(std::make_unique<MarketMakingStrategy>()); break;
        }

        initial_total_cash_ += exchange_.find_trader(id)->cash();
    }
}

MarketView Simulator::view_for(const Trader& t) const {
    const OrderBook& book = exchange_.engine().book();

    MarketView v;
    v.best_bid   = book.best_bid();
    v.best_ask   = book.best_ask();
    v.last_price = last_price_;
    // Reference price: middle of the spread if both sides exist, else the last trade.
    v.mid = (v.best_bid && v.best_ask) ? (*v.best_bid + *v.best_ask) / 2
                                       : last_price_;
    v.position       = t.position();
    v.cash           = t.cash();
    v.max_order_size = t.max_order_size();
    v.max_position   = t.max_position();
    return v;
}

void Simulator::step() {
    ++stats_.steps;

    // Some events are cancels of orders that are still resting.
    std::bernoulli_distribution do_cancel(cfg_.cancel_probability);
    if (!live_.empty() && do_cancel(rng_)) {
        cancel_random();
        return;
    }

    std::uniform_int_distribution<std::size_t> pick(0, strategies_.size() - 1);
    const std::size_t idx = pick(rng_);
    const TraderId id = static_cast<TraderId>(idx + 1);
    const Trader* trader = exchange_.find_trader(id);

    scratch_.clear();
    strategies_[idx]->decide(view_for(*trader), rng_, scratch_);
    for (const Action& action : scratch_) {
        send(id, action);
    }
}

void Simulator::run(std::size_t steps) {
    for (std::size_t i = 0; i < steps; ++i) {
        step();
    }
}

void Simulator::send(TraderId trader, const Action& a) {
    if (a.quantity == 0 || a.type == OrderType::Cancel) return;   // ignore bad actions

    const OrderId id = next_order_id_++;
    std::unique_ptr<Order> order;
    if (a.type == OrderType::Market) {
        order = std::make_unique<MarketOrder>(id, trader, a.side, a.quantity);
        ++stats_.market_orders;
    } else {
        order = std::make_unique<LimitOrder>(id, trader, a.side, a.quantity,
                                             std::max<Price>(a.price, 1));
        ++stats_.limit_orders;
    }

    const ExchangeResult result = exchange_.submit(std::move(order));
    if (result.execution.outcome == Outcome::Resting) {
        live_.push_back(LiveOrder{id, trader});
    }
    record(result);
}

void Simulator::cancel_random() {
    std::uniform_int_distribution<std::size_t> pick(0, live_.size() - 1);
    const std::size_t i = pick(rng_);
    const LiveOrder target = live_[i];

    live_[i] = live_.back();          // O(1) removal: swap with last, then pop
    live_.pop_back();

    ++stats_.cancels_sent;
    const ExchangeResult result = exchange_.submit(
        std::make_unique<CancelOrder>(next_order_id_++, target.trader, target.id));
    if (result.execution.outcome == Outcome::Cancelled) {
        ++stats_.cancels_ok;          // otherwise it was already filled
    }
    record(result);
}

void Simulator::record(const ExchangeResult& r) {
    stats_.self_trades_prevented += r.execution.self_trades_prevented;

    if (!r.accepted()) {
        ++stats_.rejected;
        ++stats_.rejected_by_reason[static_cast<std::size_t>(r.reject_reason)];
        return;
    }
    for (const Trade& t : r.execution.trades) {
        ++stats_.trades;
        stats_.volume += t.quantity;
        last_price_ = t.price;
        if (t.buyer == t.seller) ++stats_.self_trades;
    }
}

std::int64_t Simulator::net_position() const {
    std::int64_t sum = 0;
    for (std::size_t i = 1; i <= strategies_.size(); ++i) {
        sum += exchange_.find_trader(static_cast<TraderId>(i))->position();
    }
    return sum;
}

Price Simulator::total_cash() const {
    Price sum = 0;
    for (std::size_t i = 1; i <= strategies_.size(); ++i) {
        sum += exchange_.find_trader(static_cast<TraderId>(i))->cash();
    }
    return sum;
}

}  // namespace quanta