#include "traders/trader.hpp"

namespace quanta {

void Trader::apply_fill(Side side, Quantity qty, Price price) {
    const Price value = static_cast<Price>(qty) * price;
    if (side == Side::Buy) {
        cash_ -= value;
        position_ += static_cast<std::int64_t>(qty);
    } else {
        cash_ += value;
        position_ -= static_cast<std::int64_t>(qty);
    }
}

}  // namespace quanta