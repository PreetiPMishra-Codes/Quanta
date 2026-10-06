#pragma once

#include <cstdint>
#include <string>

#include "core/types.hpp"

namespace quanta {

class Trader {
public:
    Trader(TraderId id, std::string name, Price starting_cash)
        : id_(id), name_(std::move(name)), cash_(starting_cash) {}

    virtual ~Trader() = default;

    TraderId           id() const noexcept       { return id_; }
    const std::string& name() const noexcept     { return name_; }
    Price              cash() const noexcept     { return cash_; }       // in ticks
    std::int64_t       position() const noexcept { return position_; }   // shares held

    // Each trader type has different limits (used by the risk checks on Day 5).
    virtual std::string  kind() const = 0;
    virtual Quantity     max_order_size() const = 0;
    virtual std::int64_t max_position() const = 0;

    // Update cash and position after a trade.
    void apply_fill(Side side, Quantity qty, Price price);

private:
    TraderId     id_;
    std::string  name_;
    Price        cash_;
    std::int64_t position_ = 0;
};

class RetailTrader final : public Trader {
public:
    RetailTrader(TraderId id, std::string name,
                 Price cash = rupees_to_ticks(100'000))
        : Trader(id, std::move(name), cash) {}

    std::string  kind() const override           { return "Retail"; }
    Quantity     max_order_size() const override { return 100; }
    std::int64_t max_position() const override   { return 1'000; }
};

class InstitutionalTrader final : public Trader {
public:
    InstitutionalTrader(TraderId id, std::string name,
                        Price cash = rupees_to_ticks(100'000'000))
        : Trader(id, std::move(name), cash) {}

    std::string  kind() const override           { return "Institutional"; }
    Quantity     max_order_size() const override { return 100'000; }
    std::int64_t max_position() const override   { return 1'000'000; }
};

class AlgorithmicTrader final : public Trader {
public:
    AlgorithmicTrader(TraderId id, std::string name,
                      Price cash = rupees_to_ticks(10'000'000))
        : Trader(id, std::move(name), cash) {}

    std::string  kind() const override           { return "Algorithmic"; }
    Quantity     max_order_size() const override { return 1'000; }
    std::int64_t max_position() const override   { return 50'000; }
};

}  // namespace quanta