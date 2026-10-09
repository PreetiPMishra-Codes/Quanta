#pragma once

#include "strategies/strategy.hpp"

namespace quanta {

// Random side/size, price near mid. Pure noise.
class RandomStrategy final : public TradingStrategy {
public:
    explicit RandomStrategy(double market_order_probability = 0.10)
        : market_prob_(market_order_probability) {}

    std::string name() const override { return "Random"; }
    void decide(const MarketView& view, std::mt19937& rng,
                std::vector<Action>& out) override;

private:
    double market_prob_;
};

// Price rose since I last looked: buy. Fell: sell.
class MomentumStrategy final : public TradingStrategy {
public:
    std::string name() const override { return "Momentum"; }
    void decide(const MarketView& view, std::mt19937& rng,
                std::vector<Action>& out) override;

private:
    bool  has_prev_ = false;
    Price prev_ = 0;
};

// Price far above its moving average: sell. Far below: buy.
class MeanReversionStrategy final : public TradingStrategy {
public:
    MeanReversionStrategy(double alpha = 0.05, double threshold_ticks = 5.0)
        : alpha_(alpha), threshold_(threshold_ticks) {}

    std::string name() const override { return "MeanReversion"; }
    void decide(const MarketView& view, std::mt19937& rng,
                std::vector<Action>& out) override;

private:
    double alpha_;       // how fast the average follows the price
    double threshold_;   // how far from average counts as "far"
    bool   has_avg_ = false;
    double avg_ = 0.0;
};

// Quote a bid and an ask around mid. Stops quoting a side near the inventory cap.
class MarketMakingStrategy final : public TradingStrategy {
public:
    explicit MarketMakingStrategy(Price half_spread_ticks = 5,
                                  double inventory_fraction = 0.8)
        : half_spread_(half_spread_ticks), inventory_fraction_(inventory_fraction) {}

    std::string name() const override { return "MarketMaking"; }
    void decide(const MarketView& view, std::mt19937& rng,
                std::vector<Action>& out) override;

private:
    Price  half_spread_;
    double inventory_fraction_;
};

}  // namespace quanta