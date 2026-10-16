#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "benchmark/latency_recorder.hpp"
#include "simulation/simulator.hpp"

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

// ---------- small helpers ----------

std::uint64_t timer_overhead_ns() {
    constexpr int kTries = 100000;
    std::vector<std::uint64_t> d;
    d.reserve(kTries);
    for (int i = 0; i < kTries; ++i) {
        const Timestamp a = now();
        const Timestamp b = now();
        d.push_back(static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count()));
    }
    std::sort(d.begin(), d.end());
    return percentile_permille(d, 500);
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return (n % 2 == 1) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

// (largest - smallest) / median, in percent. Tells you how noisy the runs were.
double spread_pct(const std::vector<double>& v) {
    if (v.empty()) return 0.0;
    const double lo  = *std::min_element(v.begin(), v.end());
    const double hi  = *std::max_element(v.begin(), v.end());
    const double med = median(v);
    return med > 0.0 ? (hi - lo) / med * 100.0 : 0.0;
}

double pct(std::size_t part, std::size_t whole) {
    return whole == 0 ? 0.0
                      : 100.0 * static_cast<double>(part) / static_cast<double>(whole);
}

std::ofstream open_csv(const std::string& name, const std::string& header) {
    fs::create_directories("benchmarks/results");
    const fs::path file = fs::path("benchmarks/results") / name;
    const bool is_new = !fs::exists(file);
    std::ofstream out(file, std::ios::app);
    if (is_new) out << header << '\n';
    return out;
}

void print_stats_header(const char* first) {
    std::cout << std::left << std::setw(16) << first << std::right << std::setw(10)
              << "count" << std::setw(8) << "share" << std::setw(8) << "P50"
              << std::setw(8) << "P95" << std::setw(9) << "P99" << std::setw(10)
              << "P99.9" << std::setw(11) << "max" << "\n";
}

void print_stats_row(const std::string& name, const LatencyStats& s,
                     std::size_t total) {
    std::cout << std::left << std::setw(16) << name << std::right << std::setw(10)
              << s.count << std::setw(7) << std::fixed << std::setprecision(1)
              << pct(s.count, total) << "%" << std::setw(8) << s.p50_ns
              << std::setw(8) << s.p95_ns << std::setw(9) << s.p99_ns << std::setw(10)
              << s.p999_ns << std::setw(11) << s.max_ns << "\n";
}

// ---------- mode 1: profile ----------

int run_profile(std::size_t events, int runs, std::uint32_t seed,
                const std::string& label) {
    const std::size_t warmup = std::max<std::size_t>(10000, events / 4);
    DetailedRecorder pooled(static_cast<std::size_t>(runs) * events);

    // Run 0 is thrown away (extra warm-up). Runs 1..runs are pooled.
    for (int r = 0; r <= runs; ++r) {
        SimulationConfig cfg;
        cfg.seed = seed;
        Simulator sim(cfg);
        sim.run(warmup);

        DetailedRecorder discard(r == 0 ? events : 0);
        sim.set_detail_recorder(r == 0 ? &discard : &pooled);
        sim.run(events);
        sim.set_detail_recorder(nullptr);
    }

    const std::size_t total = pooled.size();
    const LatencyStats all = pooled.summarize_all();

    std::cout << "QUANTA profile | label " << label << " | build " << kBuildType
              << " | seed " << seed << " | warm-up " << warmup << " | events/run "
              << events << " | runs pooled " << runs << " (first run discarded)\n"
              << "timer overhead: " << timer_overhead_ns() << " ns | samples: "
              << total << " | latencies in nanoseconds\n";
    if (!kRelease) {
        std::cout << "WARNING: Debug build. Numbers are NOT meaningful and will not be saved.\n";
    }

    // --- A: by kind of work ---
    std::cout << "\nA. Latency by kind of work\n";
    print_stats_header("category");
    std::vector<std::pair<std::string, LatencyStats>> rows;
    for (std::size_t i = 0; i < kCategoryCount; ++i) {
        const Category c = static_cast<Category>(i);
        const LatencyStats s = pooled.summarize_if(
            [c](const DetailedSample& x) { return x.category == c; });
        if (s.count == 0) continue;
        print_stats_row(to_string(c), s, total);
        rows.emplace_back(to_string(c), s);
    }
    print_stats_row("ALL", all, total);

    std::cout << "\n   Who owns the slowest 1%? (orders at or above the overall P99 = "
              << all.p99_ns << " ns)\n";
    const auto tail = pooled.count_by_category_at_least(all.p99_ns);
    const auto everything = pooled.count_by_category_at_least(0);
    std::size_t tail_total = 0;
    for (std::size_t n : tail) tail_total += n;
    std::cout << std::left << std::setw(16) << "   category" << std::right
              << std::setw(12) << "in tail" << std::setw(16) << "share of tail"
              << std::setw(18) << "share of all" << "\n";
    for (std::size_t i = 0; i < kCategoryCount; ++i) {
        if (everything[i] == 0) continue;
        std::cout << std::left << std::setw(16)
                  << (std::string("   ") + to_string(static_cast<Category>(i)))
                  << std::right << std::setw(12) << tail[i] << std::setw(15)
                  << std::fixed << std::setprecision(1) << pct(tail[i], tail_total)
                  << "%" << std::setw(17) << pct(everything[i], total) << "%\n";
    }

    // --- B: by book size ---
    std::cout << "\nB. Latency by book size (4 equal-count groups, resting orders before the submit)\n";
    std::cout << std::left << std::setw(8) << "group" << std::setw(20)
              << "book orders" << std::right << std::setw(10) << "count"
              << std::setw(8) << "P50" << std::setw(8) << "P95" << std::setw(9)
              << "P99" << std::setw(10) << "P99.9" << std::setw(11) << "max" << "\n";
    const auto groups = pooled.by_book_size(4);
    for (std::size_t i = 0; i < groups.size(); ++i) {
        const BookGroup& g = groups[i];
        std::cout << std::left << std::setw(8) << (i + 1) << std::setw(20)
                  << (std::to_string(g.min_book) + " .. " + std::to_string(g.max_book))
                  << std::right << std::setw(10) << g.stats.count << std::setw(8)
                  << g.stats.p50_ns << std::setw(8) << g.stats.p95_ns << std::setw(9)
                  << g.stats.p99_ns << std::setw(10) << g.stats.p999_ns << std::setw(11)
                  << g.stats.max_ns << "\n";
    }

    // --- C: the slowest orders, and the rehash test ---
    std::cout << "\nC. The 20 slowest orders\n";
    std::cout << std::right << std::setw(10) << "ns" << "  " << std::left
              << std::setw(16) << "category" << std::right << std::setw(12)
              << "book orders" << "  index rehashed?\n";
    const auto slow = pooled.slowest(20);
    std::size_t slow_rehash = 0;
    for (const DetailedSample& s : slow) {
        if (s.rehashed) ++slow_rehash;
        std::cout << std::right << std::setw(10) << s.ns << "  " << std::left
                  << std::setw(16) << to_string(s.category) << std::right
                  << std::setw(12) << s.book_orders << "  "
                  << (s.rehashed ? "YES" : "no") << "\n";
    }

    const LatencyStats re = pooled.summarize_if(
        [](const DetailedSample& s) { return s.rehashed; });
    const LatencyStats no_re = pooled.summarize_if(
        [](const DetailedSample& s) { return !s.rehashed; });
    const LatencyStats rested_re = pooled.summarize_if([](const DetailedSample& s) {
        return s.category == Category::LimitRested && s.rehashed;
    });
    const LatencyStats rested_no = pooled.summarize_if([](const DetailedSample& s) {
        return s.category == Category::LimitRested && !s.rehashed;
    });

    std::cout << "\n   Rehash test (the cancel index is a std::unordered_map)\n"
              << "   orders during which the index rehashed: " << re.count << " of "
              << total << "\n"
              << "   of the 20 slowest orders, " << slow_rehash << " were rehashes\n"
              << "   limit-rested WITH rehash:    count " << rested_re.count << ", P50 "
              << rested_re.p50_ns << ", max " << rested_re.max_ns << "\n"
              << "   limit-rested WITHOUT rehash: count " << rested_no.count
              << ", P50 " << rested_no.p50_ns << ", P99.9 " << rested_no.p999_ns
              << ", max " << rested_no.max_ns << "\n"
              << "   all orders:               P99.9 " << all.p999_ns << ", max "
              << all.max_ns << "\n"
              << "   excluding rehash orders:  P99.9 " << no_re.p999_ns << ", max "
              << no_re.max_ns << "\n";

    if (kRelease) {
        std::ofstream csv = open_csv(
            "profile.csv", "label,section,name,count,p50_ns,p95_ns,p99_ns,p999_ns,max_ns");
        auto row = [&](const std::string& section, const std::string& name,
                       const LatencyStats& s) {
            csv << label << ',' << section << ',' << name << ',' << s.count << ','
                << s.p50_ns << ',' << s.p95_ns << ',' << s.p99_ns << ',' << s.p999_ns
                << ',' << s.max_ns << '\n';
        };
        for (const auto& r : rows) row("category", r.first, r.second);
        row("category", "ALL", all);
        for (std::size_t i = 0; i < groups.size(); ++i) {
            row("book_group", std::to_string(groups[i].min_book) + ".." +
                                  std::to_string(groups[i].max_book),
                groups[i].stats);
        }
        std::cout << "\nSaved to benchmarks/results/profile.csv\n";
    }
    return 0;
}

// ---------- mode 2: load levels ----------

struct LevelRun {
    LatencyStats stats;
    std::size_t  book_end = 0;
    double       engine_orders_per_s = 0.0;
};

LevelRun run_level(std::size_t n, std::uint32_t seed) {
    SimulationConfig cfg;
    cfg.seed = seed;
    Simulator sim(cfg);
    sim.run(n);                              // n unmeasured events first

    LatencyRecorder rec(n * 2);
    sim.set_latency_recorder(&rec);
    sim.run(n);                              // then n measured events
    sim.set_latency_recorder(nullptr);

    LevelRun r;
    r.stats    = rec.summarize();
    r.book_end = sim.exchange().engine().book().order_count();
    if (r.stats.total_ns > 0) {
        r.engine_orders_per_s = static_cast<double>(r.stats.count) * 1e9 /
                                static_cast<double>(r.stats.total_ns);
    }
    return r;
}

int run_levels(int runs, std::uint32_t seed, const std::string& label) {
    const std::array<std::size_t, 4> levels{1000, 10000, 100000, 1000000};

    std::cout << "QUANTA load levels | label " << label << " | build " << kBuildType
              << " | seed " << seed << " | runs per level " << runs
              << " (first run of each level discarded)\n"
              << "timer overhead: " << timer_overhead_ns() << " ns | latencies in nanoseconds\n"
              << "Each level: n unmeasured events, then n measured events.\n"
              << "This changes the WORKLOAD SIZE (and so the book size), not the arrival rate.\n";
    if (!kRelease) {
        std::cout << "WARNING: Debug build. Numbers are NOT meaningful and will not be saved.\n";
    }
    std::cout << "\n" << std::right << std::setw(9) << "events" << std::setw(9)
              << "orders" << std::setw(11) << "book end" << std::setw(10)
              << "best P50" << std::setw(8) << "med P50" << std::setw(9)
              << "spread" << std::setw(9) << "med P99" << std::setw(10)
              << "med P99.9" << std::setw(13) << "med ord/s" << "\n";

    std::ofstream csv;
    if (kRelease) {
        csv = open_csv("levels.csv",
                       "label,level_events,run,orders,book_orders_end,p50_ns,p95_ns,"
                       "p99_ns,p999_ns,max_ns,engine_orders_per_s");
    }

    for (std::size_t n : levels) {
        const int level_runs = (n >= 1000000) ? std::min(runs, 3) : runs;
        std::vector<double> p50, p99, p999, ops;
        std::size_t orders = 0, book_end = 0;

        for (int r = 0; r <= level_runs; ++r) {
            const LevelRun lr = run_level(n, seed);
            if (r == 0) continue;            // discard the first run
            p50.push_back(static_cast<double>(lr.stats.p50_ns));
            p99.push_back(static_cast<double>(lr.stats.p99_ns));
            p999.push_back(static_cast<double>(lr.stats.p999_ns));
            ops.push_back(lr.engine_orders_per_s);
            orders   = lr.stats.count;
            book_end = lr.book_end;
            if (kRelease) {
                csv << label << ',' << n << ',' << r << ',' << lr.stats.count << ','
                    << lr.book_end << ',' << lr.stats.p50_ns << ',' << lr.stats.p95_ns
                    << ',' << lr.stats.p99_ns << ',' << lr.stats.p999_ns << ','
                    << lr.stats.max_ns << ',' << std::fixed << std::setprecision(0)
                    << lr.engine_orders_per_s << '\n';
            }
        }
        const double sp = spread_pct(p50);
        std::cout << std::right << std::setw(9) << n << std::setw(9) << orders
                  << std::setw(11) << book_end << std::setw(10) << std::fixed
                  << std::setprecision(0) << *std::min_element(p50.begin(), p50.end())
                  << std::setw(8) << median(p50) << std::setw(8) << std::setprecision(0)
                  << sp << "%" << std::setw(9) << median(p99) << std::setw(10)
                  << median(p999) << std::setw(13) << median(ops)
                  << (sp > 20.0 ? "  noisy" : "") << "\n";
        std::cout.flush();
    }
    std::cout << "\nP99.9 from a level with fewer than about 10,000 orders is basically the max. Ignore it.\n";
    if (kRelease) std::cout << "Saved to benchmarks/results/levels.csv\n";
    return 0;
}

void usage() {
    std::cerr << "usage:\n"
              << "  bench_profile profile [events=200000] [runs=5] [seed=42] [label=profile]\n"
              << "  bench_profile levels  [runs=5] [seed=42] [label=levels]\n";
}

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "";
    if (mode == "profile") {
        const std::size_t events = argc > 2 ? static_cast<std::size_t>(std::stoull(argv[2])) : 200000;
        const int runs = argc > 3 ? std::stoi(argv[3]) : 5;
        const std::uint32_t seed = argc > 4 ? static_cast<std::uint32_t>(std::stoul(argv[4])) : 42u;
        const std::string label = argc > 5 ? argv[5] : "profile";
        if (events == 0 || runs <= 0) { usage(); return 1; }
        return run_profile(events, runs, seed, label);
    }
    if (mode == "levels") {
        const int runs = argc > 2 ? std::stoi(argv[2]) : 5;
        const std::uint32_t seed = argc > 3 ? static_cast<std::uint32_t>(std::stoul(argv[3])) : 42u;
        const std::string label = argc > 4 ? argv[4] : "levels";
        if (runs <= 0) { usage(); return 1; }
        return run_levels(runs, seed, label);
    }
    usage();
    return 1;
}