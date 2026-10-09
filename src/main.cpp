#include <iomanip>
#include <iostream>
#include <string>

#include "simulation/simulator.hpp"

using namespace quanta;

int main(int argc, char** argv) {
    std::size_t   events = 50000;
    std::uint32_t seed   = 42;
    if (argc > 1) events = std::stoull(argv[1]);
    if (argc > 2) seed   = static_cast<std::uint32_t>(std::stoul(argv[2]));

    SimulationConfig cfg;
    cfg.seed = seed;
    Simulator sim(cfg);
    sim.run(events);

    const SimulationStats& s = sim.stats();
    std::cout << "QUANTA simulation  (seed " << seed << ", " << cfg.num_traders
              << " traders, " << events << " events)\n\n";

    std::cout << "Orders\n"
              << "  limit orders    " << s.limit_orders << "\n"
              << "  market orders   " << s.market_orders << "\n"
              << "  cancels         " << s.cancels_ok << " ok of " << s.cancels_sent << " sent\n"
              << "  rejected        " << s.rejected << "\n";
    for (int r = 1; r <= 4; ++r) {
        std::cout << "    " << std::left << std::setw(20)
                  << to_string(static_cast<RejectReason>(r)) << std::right
                  << s.rejected_by_reason[r] << "\n";
    }

    std::cout << "\nTrading\n"
              << "  trades          " << s.trades << "\n"
              << "  volume          " << s.volume << " shares\n"
              << "  self-trades     " << s.self_trades << " (prevented: "
              << s.self_trades_prevented << ")\n"
              << "  last price      Rs " << std::fixed << std::setprecision(2)
              << ticks_to_rupees(sim.last_price()) << "\n";

    std::cout << "\nChecks\n"
              << "  net position    " << sim.net_position() << "  (must be 0)\n"
              << "  cash conserved  "
              << (sim.total_cash() == sim.initial_total_cash() ? "yes" : "NO") << "\n";

    std::cout << "\nFinal book (top 8 levels)\n" << sim.exchange().engine().book().render(8);
    return 0;
}