#include <iostream>
#include <memory>

#include "exchange/order_book.hpp"

using namespace quanta;

int main() {
    OrderBook book;
    OrderId next_id = 1;

    auto place = [&](Side side, Quantity qty, double rupees) {
        book.add(std::make_unique<LimitOrder>(next_id++, /*trader=*/1, side, qty,
                                              rupees_to_ticks(rupees)));
    };

    place(Side::Buy, 100, 100.00);
    place(Side::Buy, 120, 100.10);
    place(Side::Buy, 500, 100.05);
    place(Side::Sell, 200, 100.15);
    place(Side::Sell, 150, 100.20);
    place(Side::Sell, 300, 100.25);

    std::cout << "QUANTA order book\n" << book.render() << "\n";

    std::cout << "Cancelling order #2 (the best bid)...\n";
    book.cancel(2);
    std::cout << book.render();
    return 0;
}