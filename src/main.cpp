#include <iomanip>
#include <iostream>
#include <memory>

#include "exchange/matching_engine.hpp"

using namespace quanta;

static void print(const ExecutionResult& r) {
    std::cout << "  -> " << to_string(r.outcome) << ", filled " << r.filled
              << ", remaining " << r.remaining << "\n";
    for (const auto& t : r.trades) {
        std::cout << "     TRADE #" << t.id << ": " << t.quantity << " @ "
                  << std::fixed << std::setprecision(2) << ticks_to_rupees(t.price)
                  << "  (buyer " << t.buyer << ", seller " << t.seller << ")\n";
    }
}

int main() {
    MatchingEngine engine;
    OrderId next_id = 1;

    auto limit = [&](TraderId who, Side side, Quantity qty, double rupees) {
        return engine.submit_limit(std::make_unique<LimitOrder>(
            next_id++, who, side, qty, rupees_to_ticks(rupees)));
    };

    // Trader 1 provides liquidity on both sides.
    limit(1, Side::Buy, 100, 100.00);
    limit(1, Side::Buy, 120, 100.10);
    limit(1, Side::Buy, 500, 100.05);
    limit(1, Side::Sell, 200, 100.15);
    limit(1, Side::Sell, 150, 100.20);
    limit(1, Side::Sell, 300, 100.25);

    std::cout << "Starting book:\n" << engine.book().render() << "\n";

    std::cout << "Trader 2: BUY 250 limit @ 100.20 (walks two ask levels)\n";
    print(limit(2, Side::Buy, 250, 100.20));
    std::cout << engine.book().render() << "\n";

    std::cout << "Trader 3: MARKET SELL 600 (eats the bids)\n";
    MarketOrder m(next_id++, 3, Side::Sell, 600);
    print(engine.submit_market(m));
    std::cout << engine.book().render();

    std::cout << "\nTotal trades: " << engine.trade_count() << "\n";
    return 0;
}