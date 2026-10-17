#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "benchmark/book_fixture.hpp"
#include "benchmark/stats_util.hpp"
#include "exchange/order_book.hpp"

using namespace quanta;
namespace fs = std::filesystem;

namespace {

#ifdef NDEBUG
constexpr bool kRelease = true;
constexpr const char* kBuildType = "Release";
#else
constexpr bool kRelease = false;
constexpr const char* kBuildType = "Debug";
#endif

volatile Price g_sink = 0;   // stops the compiler from deleting the best-price loop

double ns_between(Timestamp a, Timestamp b) {
    return std::chrono::duration<double, std::nano>(b - a).count();
}

struct Op {
    std::vector<double> ns_per_op;   // one entry per batch
    double median() const { return median_value(ns_per_op); }
    double best() const {
        return ns_per_op.empty() ? 0.0
                                 : *std::min_element(ns_per_op.begin(), ns_per_op.end());
    }
        // How much slower the typical (median) batch is than the best batch, in percent.
    // We do not use largest-minus-smallest here: one OS pause would dominate it.
    double spread() const {
        const double med = median();
        return med > 0.0 ? (med - best()) / med * 100.0 : 0.0;
    }
};

struct Cell {
    std::size_t n = 0, levels = 0, batch = 0;
    Op cancel, add, best_price, consume;
};

// One book size, one price-level count. Every operation is timed in BATCHES, so the
// cost of reading the clock (tens of ns) is spread over hundreds of operations.
Cell run_cell(std::size_t n, std::size_t levels, int batches, std::uint32_t seed) {
    OrderBook book;
    fill_book(book, n, levels);

    const std::size_t k = std::min<std::size_t>(1000, n / 2);   // operations per batch
    constexpr int kLookups = 100000;

    std::vector<OrderId> ids(n);
    std::iota(ids.begin(), ids.end(), OrderId{1});
    std::mt19937 rng(seed);
    std::shuffle(ids.begin(), ids.end(), rng);   // random positions in the book

    std::vector<std::unique_ptr<LimitOrder>> held;
    held.reserve(k);

    Cell cell;
    cell.n = n;
    cell.levels = levels;
    cell.batch = k;

    constexpr int kWarmup = 3;       // unrecorded batches
    std::size_t cursor = 0;

    for (int b = -kWarmup; b < batches; ++b) {
        const bool record = (b >= 0);

        // 1. cancel k random resting orders
        const Timestamp c0 = now();
        for (std::size_t i = 0; i < k; ++i) {
            held.push_back(book.cancel(ids[(cursor + i) % n]));
        }
        const Timestamp c1 = now();
        cursor = (cursor + k) % n;
        for (const auto& p : held) {
            if (!p) throw std::logic_error("cancel returned null");
        }

        // 2. put the same orders back (same prices, so same positions in the book)
        const Timestamp a0 = now();
        for (auto& p : held) book.add(std::move(p));
        const Timestamp a1 = now();
        held.clear();

        // 3. best bid and best ask lookups
        Price acc = 0;
        const Timestamp p0 = now();
        for (int i = 0; i < kLookups; ++i) acc += *book.best_bid() + *book.best_ask();
        const Timestamp p1 = now();
        g_sink = acc;

        // 4. consume the k best asks (what matching does when resting orders fill)
        const Timestamp m0 = now();
        for (std::size_t i = 0; i < k; ++i) {
            LimitOrder* front = book.front(Side::Sell);
            held.push_back(book.cancel(front->id()));
        }
        const Timestamp m1 = now();
        for (auto& p : held) book.add(std::move(p));
        held.clear();

        if (book.order_count() != n) throw std::logic_error("book size changed");

        if (record) {
            const double dk = static_cast<double>(k);
            cell.cancel.ns_per_op.push_back(ns_between(c0, c1) / dk);
            cell.add.ns_per_op.push_back(ns_between(a0, a1) / dk);
            cell.best_price.ns_per_op.push_back(ns_between(p0, p1) / (2.0 * kLookups));
            cell.consume.ns_per_op.push_back(ns_between(m0, m1) / dk);
        }
    }
    return cell;
}

std::string fmt(const Op& op) {
    std::ostringstream s;
    s << std::fixed << std::setprecision(1) << op.median() << "/" << op.best();
    return s.str();
}

}  // namespace

int main(int argc, char** argv) {
    // usage: bench_book [label=book] [batches=40] [seed=42] [big]
    const std::string label = argc > 1 ? argv[1] : "book";
    const int batches = argc > 2 ? std::stoi(argv[2]) : 40;
    const std::uint32_t seed = argc > 3 ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 42u;
    const bool big = argc > 4 && std::string(argv[4]) == "big";
    if (batches <= 0) {
        std::cerr << "batches must be positive\n";
        return 1;
    }

    std::vector<std::size_t> sizes{1000, 10000, 100000};
    if (big) sizes.push_back(1000000);
    const std::vector<std::size_t> level_counts{50, 1000};

    std::cout << "QUANTA order-book micro-benchmark | label " << label << " | build "
              << kBuildType << " | batches " << batches << " (+3 warm-up) | seed " << seed
              << "\nOnly the OrderBook is measured: no engine, no strategies, no risk.\n"
              << "Each cell is nanoseconds PER OPERATION as median/best over the batches.\n"
              << "cancel/add: ~1000 random orders per batch. consume: pop the 1000 best asks.\n";
    if (!kRelease) {
        std::cout << "WARNING: Debug build. Numbers are NOT meaningful and will not be saved.\n";
    }
    std::cout << "\n" << std::right << std::setw(9) << "orders" << std::setw(8)
              << "levels" << std::setw(17) << "cancel" << std::setw(17) << "add"
              << std::setw(17) << "best-price" << std::setw(17) << "consume"
              << std::setw(11) << "spread" << "\n";

    std::ofstream csv;
    if (kRelease) {
        fs::create_directories("benchmarks/results");
        const fs::path file = fs::path("benchmarks/results") / "book.csv";
        const bool is_new = !fs::exists(file);
        csv.open(file, std::ios::app);
        if (is_new) {
            csv << "label,orders,levels,ops_per_batch,batches,op,median_ns,best_ns,spread_pct\n";
        }
    }

    for (std::size_t n : sizes) {
        for (std::size_t levels : level_counts) {
            const Cell c = run_cell(n, levels, batches, seed);
            const double worst = std::max({c.cancel.spread(), c.add.spread(),
                                           c.best_price.spread(), c.consume.spread()});
            std::cout << std::right << std::setw(9) << c.n << std::setw(8) << c.levels
                      << std::setw(17) << fmt(c.cancel) << std::setw(17) << fmt(c.add)
                      << std::setw(17) << fmt(c.best_price) << std::setw(17)
                      << fmt(c.consume) << std::setw(9) << std::fixed
                      << std::setprecision(0) << worst << "%"
                      << (worst > 25.0 ? " noisy" : "") << "\n";
            std::cout.flush();

            if (kRelease) {
                auto row = [&](const char* op, const Op& o) {
                    csv << label << ',' << c.n << ',' << c.levels << ',' << c.batch << ','
                        << batches << ',' << op << ',' << std::fixed
                        << std::setprecision(2) << o.median() << ',' << o.best() << ','
                        << std::setprecision(1) << o.spread() << '\n';
                };
                row("cancel", c.cancel);
                row("add", c.add);
                row("best_price", c.best_price);
                row("consume", c.consume);
            }
        }
    }
        std::cout << "\nspread = how much slower the median batch is than the best batch (%), worst of the four operations.\n";
    if (kRelease) std::cout << "Saved to benchmarks/results/book.csv\n";
    return 0;
}