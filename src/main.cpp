#include <iomanip>
#include <iostream>
#include <memory>

#include "exchange/exchange.hpp"

using namespace quanta;

static void show(const char* what, const ExchangeResult& r) {
    std::cout << what << "\n  -> ";
    if (r.accepted()) {
        std::cout << to_string(r.execution.outcome) << ", filled "
                  << r.execution.filled << ", remaining " << r.execution.remaining;
    } else {
        std::cout << "REJECTED (" << to_string(r.reject_reason) << ")";
    }
    std::cout << "\n";
}

static void account(const Exchange& ex, TraderId id) {
    const Trader* t = ex.find_trader(id);
    std::cout << "  " << t->name() << " [" << t->kind() << "]: position "
              << t->position() << ", cash Rs " << std::fixed << std::setprecision(2)
              << ticks_to_rupees(t->cash()) << "\n";
}

int main() {
    Exchange ex;
    ex.add_trader(std::make_unique<RetailTrader>(1, "Asha"));
    ex.add_trader(std::make_unique<InstitutionalTrader>(2, "BigFund"));
    ex.add_trader(std::make_unique<RetailTrader>(3, "Broke", rupees_to_ticks(500)));

    OrderId id = 1;
    auto limit = [&](TraderId who, Side side, Quantity qty, double rupees) {
        return std::make_unique<LimitOrder>(id++, who, side, qty,
                                            rupees_to_ticks(rupees));
    };

    show("BigFund: SELL 1000 @ 100.00", ex.submit(limit(2, Side::Sell, 1000, 100.00)));
    show("Asha: BUY 50 @ 100.00", ex.submit(limit(1, Side::Buy, 50, 100.00)));
    show("Asha: BUY 500 @ 100.00 (retail max is 100)",
         ex.submit(limit(1, Side::Buy, 500, 100.00)));
    show("Broke: BUY 10 @ 100.00 (costs Rs 1000, has Rs 500)",
         ex.submit(limit(3, Side::Buy, 10, 100.00)));

    std::cout << "\nAccounts:\n";
    account(ex, 1);
    account(ex, 2);
    account(ex, 3);

    std::cout << "\nBook:\n" << ex.engine().book().render()
              << "\nRejected orders: " << ex.rejected_count() << "\n";
    return 0;
}