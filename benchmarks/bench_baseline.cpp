#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "benchmark/latency_recorder.hpp"
#include "simulation/simulator.hpp"

using namespace quanta;

namespace {

#ifdef NDEBUG
constexpr const char* kBuildType = "Release";
#else
constexpr const char* kBuildType = "Debug";
#endif

struct RunResult {
    LatencyStats  stats;
    std::uint64_t events = 0;
    double        wall_s = 0.0;

    // Orders per second counting ONLY time inside the exchange.
    double engine_orders_per_s() const {
        if (stats.total_ns == 0) return 0.0;
        return static_cast<double>(stats.count) * 1e9 /
               static_cast<double>(stats.total_ns);
    }
};

// Cost of calling the clock twice back to back (median of many tries).
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

RunResult run_once(std::uint32_t seed, std::size_t warmup, std::size_t events) {
    SimulationConfig cfg;
    cfg.seed = seed;
    Simulator sim(cfg);

    sim.run(warmup);                        // unmeasured: warm caches, fill the book

    // One event sends at most 2 orders, so 2x events can never overflow.
    LatencyRecorder rec(events * 2);
    sim.set_latency_recorder(&rec);

    const Timestamp t0 = now();
    sim.run(events);
    const Timestamp t1 = now();
    sim.set_latency_recorder(nullptr);

    RunResult r;
    r.stats  = rec.summarize();
    r.events = events;
    r.wall_s = std::chrono::duration<double>(t1 - t0).count();
    return r;
}

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return (n % 2 == 1) ? v[n / 2] : (v[n / 2 - 1] + v[n / 2]) / 2.0;
}

template <typename F>
double median_of(const std::vector<RunResult>& runs, F field) {
    std::vector<double> v;
    for (const RunResult& r : runs) v.push_back(field(r));
    return median(std::move(v));
}

void print_header() {
    std::cout << std::right << std::setw(5) << "run" << std::setw(10) << "orders"
              << std::setw(9) << "mean" << std::setw(8) << "P50" << std::setw(8)
              << "P95" << std::setw(8) << "P99" << std::setw(9) << "P99.9"
              << std::setw(10) << "max" << std::setw(9) << "wall(s)"
              << std::setw(15) << "engine ord/s" << "\n";
}

}  // namespace

int main(int argc, char** argv) {
    // usage: bench_baseline [label] [events] [runs] [seed]
    const std::string label = argc > 1 ? argv[1] : "baseline";
    const std::size_t events = argc > 2
        ? static_cast<std::size_t>(std::stoull(argv[2])) : std::size_t{200000};
    const int runs = argc > 3 ? std::stoi(argv[3]) : 5;
    const std::uint32_t seed = argc > 4
        ? static_cast<std::uint32_t>(std::stoul(argv[4])) : 42u;
    if (events == 0 || runs <= 0) {
        std::cerr << "events and runs must be positive\n";
        return 1;
    }
    const std::size_t warmup = std::max<std::size_t>(10000, events / 4);
    const bool release = (std::string(kBuildType) == "Release");

    std::cout << "QUANTA baseline benchmark\n"
              << "  label " << label << " | build " << kBuildType << " | seed " << seed
              << " | warm-up " << warmup << " | measured events " << events
              << " | runs " << runs << "\n"
              << "  timer overhead (two back-to-back clock reads, median): "
              << timer_overhead_ns() << " ns\n"
              << "  all latencies in nanoseconds\n";
    if (!release) {
        std::cout << "  WARNING: Debug build. These numbers are NOT meaningful and "
                     "will not be saved.\n";
    }
    std::cout << "\n";

    print_header();
    std::vector<RunResult> results;
    for (int i = 1; i <= runs; ++i) {
        results.push_back(run_once(seed, warmup, events));
        const RunResult& r = results.back();
        std::cout << std::setw(5) << i << std::setw(10) << r.stats.count
                  << std::setw(9) << std::fixed << std::setprecision(0) << r.stats.mean_ns
                  << std::setw(8) << r.stats.p50_ns << std::setw(8) << r.stats.p95_ns
                  << std::setw(8) << r.stats.p99_ns << std::setw(9) << r.stats.p999_ns
                  << std::setw(10) << r.stats.max_ns << std::setw(9)
                  << std::setprecision(3) << r.wall_s << std::setw(15)
                  << std::setprecision(0) << r.engine_orders_per_s() << "\n";
    }

    std::cout << "\nMedian of " << runs << " runs\n"
              << std::fixed << std::setprecision(0)
              << "  mean " << median_of(results, [](const RunResult& r) { return r.stats.mean_ns; })
              << " | P50 " << median_of(results, [](const RunResult& r) { return static_cast<double>(r.stats.p50_ns); })
              << " | P95 " << median_of(results, [](const RunResult& r) { return static_cast<double>(r.stats.p95_ns); })
              << " | P99 " << median_of(results, [](const RunResult& r) { return static_cast<double>(r.stats.p99_ns); })
              << " | P99.9 " << median_of(results, [](const RunResult& r) { return static_cast<double>(r.stats.p999_ns); })
              << " | engine orders/s "
              << median_of(results, [](const RunResult& r) { return r.engine_orders_per_s(); })
              << "\n";

    if (release) {
        namespace fs = std::filesystem;
        const fs::path dir = "benchmarks/results";
        fs::create_directories(dir);
        const fs::path file = dir / "baseline.csv";
        const bool is_new = !fs::exists(file);
        std::ofstream csv(file, std::ios::app);
        if (is_new) {
            csv << "label,build,run,seed,events,orders,mean_ns,p50_ns,p95_ns,p99_ns,"
                   "p999_ns,max_ns,wall_s,engine_orders_per_s\n";
        }
        int run_no = 1;
        for (const RunResult& r : results) {
            csv << label << ',' << kBuildType << ',' << run_no++ << ',' << seed << ','
                << r.events << ',' << r.stats.count << ',' << std::fixed
                << std::setprecision(1) << r.stats.mean_ns << ',' << r.stats.p50_ns
                << ',' << r.stats.p95_ns << ',' << r.stats.p99_ns << ','
                << r.stats.p999_ns << ',' << r.stats.max_ns << ','
                << std::setprecision(4) << r.wall_s << ',' << std::setprecision(0)
                << r.engine_orders_per_s() << '\n';
        }
        std::cout << "\nSaved " << results.size() << " rows to " << file.string() << "\n";
    }
    return 0;
}