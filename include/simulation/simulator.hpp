#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

#include "core/types.hpp"
#include "exchange/exchange.hpp"
#include "benchmark/latency_recorder.hpp"
#include "strategies/strategy.hpp"

namespace quanta {

struct SimulationConfig {
    std::uint32_t   seed               = 42;
    std::size_t     num_traders        = 50;
    Price           initial_price      = rupees_to_ticks(100.00);
    double          cancel_probability = 0.30;
    SelfTradePolicy self_trade_policy  = SelfTradePolicy::CancelResting;
};

struct SimulationStats {
    std::uint64_t steps          = 0;
    std::uint64_t limit_orders   = 0;
    std::uint64_t market_orders  = 0;
    std::uint64_t cancels_sent   = 0;
    std::uint64_t cancels_ok     = 0;
    std::uint64_t rejected       = 0;
    std::array<std::uint64_t, 5> rejected_by_reason{};   // indexed by RejectReason
    std::uint64_t trades         = 0;
    std::uint64_t volume         = 0;                     // total shares traded
    std::uint64_t self_trades    = 0;
    std::uint64_t self_trades_prevented = 0;
};

class Simulator {
public:
    explicit Simulator(const SimulationConfig& cfg);

    Simulator(const Simulator&) = delete;
    Simulator& operator=(const Simulator&) = delete;

    void step();                       // one event
    void run(std::size_t steps);       // many events

    const SimulationStats& stats() const noexcept { return stats_; }
    const Exchange& exchange() const noexcept { return exchange_; }
    Price last_price() const noexcept { return last_price_; }
    // Optional: when set, the time spent inside Exchange::submit is recorded
    // for every order. The simulator does not own the recorder.
    void set_latency_recorder(LatencyRecorder* recorder) noexcept {
        recorder_ = recorder;
    }
        // Like the recorder above, but each sample also remembers the kind of order,
    // the book size, and whether the cancel index rehashed during the submit.
    void set_detail_recorder(DetailedRecorder* recorder) noexcept {
        detail_ = recorder;
    }
    // Conservation checks (see tests).
    std::int64_t net_position() const;
    Price        total_cash() const;
    Price        initial_total_cash() const noexcept { return initial_total_cash_; }

private:
    struct LiveOrder {
        OrderId  id;
        TraderId trader;
    };

    MarketView view_for(const Trader& trader) const;
    void send(TraderId trader, const Action& action);
    void cancel_random();
    void record(const ExchangeResult& result);
    ExchangeResult timed_submit(std::unique_ptr<Order> order);

    SimulationConfig cfg_;
    std::mt19937     rng_;
    Exchange         exchange_;
    std::vector<std::unique_ptr<TradingStrategy>> strategies_;   // index = trader id - 1
    std::vector<LiveOrder> live_;        // orders that rested (candidates for cancel)
    std::vector<Action>    scratch_;     // reused every step, no per-step allocation
    OrderId          next_order_id_ = 1;
    Price            last_price_;
    Price            initial_total_cash_ = 0;
    SimulationStats  stats_;
    LatencyRecorder* recorder_ = nullptr;   // not owned
    DetailedRecorder* detail_ = nullptr;    // not owned
};

}  // namespace quanta