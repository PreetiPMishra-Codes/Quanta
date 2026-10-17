# ⚡ QUANTA

### Low-Latency Stock Exchange & Market Infrastructure Simulator

**A modern C++17 simulation of the infrastructure behind financial markets.**

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](#)
[![OOP](https://img.shields.io/badge/OOP-Coursework-6A5ACD?style=for-the-badge)](#)
[![Tests](https://img.shields.io/badge/Tests-146%20passing-2E8B57?style=for-the-badge)](#)
[![Sanitizers](https://img.shields.io/badge/ASan%20%2B%20UBSan-clean-FF8C00?style=for-the-badge)](#)

---

## 👨‍💻 Project Information

|                  |                                                        |
| ---------------- | ------------------------------------------------------ |
| **Project**      | QUANTA                                                 |
| **Student ID**   | ID-B125085                                             |
| **Name**         | **Preeti Pragyan Mishra**                              |
| **Course**       | Object-Oriented Programming                            |
| **Language**     | Modern C++ (C++17)                                     |
| **Project Type** | Systems Engineering / Market Infrastructure Simulation |

---

## 🚀 What is QUANTA?

QUANTA is a simplified **stock exchange and market infrastructure simulator**. It is not a stock-management, portfolio or CRUD application. It focuses on the **infrastructure underneath financial markets**.

> **How can a financial exchange process a huge number of orders and market-data events with extremely low latency while remaining correct, concurrent, scalable and fault tolerant?**

```text
 Simulated Traders  (strategies decide)
         |
         v
       Orders  (Market / Limit / Cancel)
         |
         v
   Risk Checks  --- rejected ---> reason code returned to trader
         |
         v
   Order Book  (price/time priority)
         |
         v
  Matching Engine
         |
         v
      Trades  --->  Settlement (cash and positions)  --->  Market Data (planned)
```

The goal is **not** a profitable trading bot. The goal is to understand how the infrastructure that processes trades can be designed, measured and optimized.

---

## 📌 Current Status (Week 1 complete)

Phases 1 to 4 are **built and tested**. Phases 5 to 10 are planned.

| Phase | Topic | Status |
|---|---|---|
| 1 | Basic OOP architecture | ✅ Built and tested |
| 2 | Order book | ✅ Built and tested |
| 3 | Matching engine | ✅ Built and tested |
| 4 | Trading strategies and workload simulator | ✅ Built and tested |
| 5 | Benchmarking | ✅ Built and tested |
| 6 | Concurrent processing | ⏳ Planned |
| 7 | Market data pipeline | ⏳ Planned |
| 8 | Performance optimization | ⏳ Planned |
| 9 | Scaling experiments | ⏳ Planned |
| 10 | Failure simulation | ⏳ Planned |

### Verification

- **111 automated tests** (GoogleTest), including property tests with fixed seeds.
- Clean under **AddressSanitizer** and **UndefinedBehaviorSanitizer**.
- Clean build with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` (0 warnings).
- **Conservation checks** after simulated runs, including a 1,000,000-event run:
  net position is 0 and total cash is unchanged.
- The same seed produces an identical run.
- The order book is never left crossed (best bid is always below best ask).

### Build and run

```bash
sudo apt install -y build-essential cmake git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/quanta 50000 42      # arguments: number of events, random seed
```

Use `Debug` while developing and `Release` for any timing measurement.

### Observed simulation behaviour

Seed 42, 50 traders, 1,000,000 events (measured on the development machine):

| Observation | Result |
|---|---|
| Orders rejected by risk checks | 29,519 (about 4.8% of orders) |
| Rejection reasons | 28,733 position limit, 786 insufficient funds |
| Trades executed | 375,714 (4,896,633 shares) |
| Self-trades | 0 occurred, 3,972 prevented |
| Cancel requests that succeeded | 151,412 of 299,759 (the rest targeted orders that had already filled) |
| Shares conserved / cash conserved | yes / yes |

Short runs (20,000 events) produce no risk rejections, so the risk checks are verified by unit tests there and by the long run above.

### Design decisions

- **Prices are integer ticks** (1 tick = 1 paisa), never `double`, so matching comparisons are exact.
- **The order book stores, the matching engine decides.** Each can change independently.
- **Trades execute at the resting order's price**, so the newcomer gets price improvement.
- **Rejections are result values.** Exceptions are reserved for programmer errors.
- **Self-trades are prevented** by cancelling the trader's own resting order, which keeps the book uncrossed.
- **The `Exchange` composes** a risk manager, a matching engine and traders instead of inheriting from them.
- **Strategies see a read-only `MarketView`** and return `Action` data. They cannot touch the exchange directly.

### Known limitations

- The funds check ignores cash already promised to resting orders.
- Strategies only observe the market on their own turn.
- Market-maker quotes are never cancelled by the maker, so stale quotes pile up (visible in the final book of long runs).
- Random results are reproducible only with the same compiler and standard library.
- No benchmark has been measured yet.

### Benchmarks

**Not yet measured.** Performance figures will be added only from real runs in Release mode. No value will be fabricated.

---

## 🎯 Project Goals

QUANTA explores:

- 🧩 Object-Oriented Programming
- ⚡ Low-latency system design
- 🚀 High-throughput processing
- 🧵 Concurrency
- 🧠 Efficient data structures
- 💾 Memory efficiency
- 📊 Performance engineering
- 📡 Event-driven architecture
- 📈 Scalability
- 🛡️ Fault tolerance

---

## 🏗️ Architecture

| Layer | Contents |
|---|---|
| **Simulation** | `Simulator`, `TradingStrategy` (Random, Momentum, MeanReversion, MarketMaking) |
| **Exchange** | `Exchange` (composes `RiskManager`, `MatchingEngine`, traders) |
| **Core engine** | `MatchingEngine`, `OrderBook`, `Trade` |
| **Domain** | `Order` hierarchy, `Trader` hierarchy, core types (`Price`, `Quantity`, `Side`) |

### Class hierarchies

```text
Order                     (id, trader, timestamp)
|-- TradeOrder            (side, quantity, remaining)
|   |-- MarketOrder       (trade now at the best available price)
|   `-- LimitOrder        (trade only at this price or better)
`-- CancelOrder           (target order id)

Trader
|-- RetailTrader
|-- InstitutionalTrader
`-- AlgorithmicTrader

TradingStrategy
|-- RandomStrategy
|-- MomentumStrategy
|-- MeanReversionStrategy
`-- MarketMakingStrategy
```

`CancelOrder` is deliberately separate from `TradeOrder`: a cancel has no side, quantity or price, so inheritance models "is a kind of", not just a shared name.

---

## 📖 Order Book and Matching

```text
   SELLERS (asks), lowest price is best
     100.25  x 300
     100.20  x 150
     100.15  x 200   <- best ask
   ------ spread = 0.05 ------
     100.10  x 120   <- best bid
     100.05  x 500
     100.00  x 100
   BUYERS (bids), highest price is best
```

| Rule | Meaning |
|---|---|
| Price priority | The best price trades first |
| Time priority | At equal prices, the earliest order goes first |
| Resting price rule | The order already waiting sets the trade price |
| Market orders | Take what is available. Any leftover is discarded |
| Limit orders | Unfilled quantity rests in the book |
| Cancel | Only the owner of an order may cancel it |

**Data structures:** `std::map` for sorted price levels (`std::greater` for bids), `std::list` per level for stable iterators, and `std::unordered_map` as a cancel index so cancellation finds an order directly instead of scanning the book.

---

## 🧠 OOP Design

| OOP / C++ Concept | Application in QUANTA |
|---|---|
| **Encapsulation** | Order book internals and trader accounts are private |
| **Abstraction** | `Order` and `TradingStrategy` are abstract interfaces |
| **Inheritance** | `TradeOrder` is the parent of `MarketOrder` and `LimitOrder` |
| **Polymorphism** | One loop calls `strategy->decide()`. `process()` routes any order type |
| **Composition** | `Exchange` has a `RiskManager`, `MatchingEngine` and traders |
| **RAII / Smart pointers** | `unique_ptr` gives each order one clear owner |
| **STL** | `map`, `list`, `unordered_map`, `vector`, `optional` |
| **Testing** | Unit tests, property tests and conservation tests |

> Inheritance is not forced to demonstrate OOP. Composition is preferred wherever it gives a better design.

---

## 🛡️ Risk Management

Lightweight checks that run before an order reaches the book:

- Maximum order size (per trader type)
- Position limit (long or short)
- Sufficient funds (buys only, using an overflow-safe comparison)

Cancels are not risk-checked, because they only reduce exposure.

---

## ⚡ Planned Work

### Low-latency measurement (Phase 5)

Average, P50, P95, P99, P99.9 and maximum latency, plus throughput, measured per pipeline stage (validate, risk, insert, match, trade). Tail latency matters because averages can hide occasional large delays.

### Concurrency (Phase 6)

Two architectures will be compared experimentally:

```text
A: Shared state                    B: Controlled critical path

 Thread --+                         Producers
 Thread --+--> shared book + locks     |
 Thread --+                            v
                                     Queue --> single matching thread --> book
```

### Market data pipeline (Phase 7)

Trade events published through a bounded queue to subscribers, with backpressure.

### Optimization and scaling (Phases 8 and 9)

Order-book representations compared by measurement, followed by synthetic load sweeps (1k to 1M orders/sec). These are synthetic workloads, not claims of real exchange performance.

### Failure simulation (Phase 10)

Slow consumers, queue overload and temporary failures, studying retries, recovery and backpressure.

### Optimization method

```text
1. Establish baseline  ->  2. Identify bottleneck  ->  3. Change one thing
4. Benchmark again     ->  5. Compare results     ->  6. Explain the trade-off
```

> **Measure first. Optimize second.**

---

## 🛠️ Technology Stack

C++17, STL, CMake, GoogleTest, Git, AddressSanitizer, UndefinedBehaviorSanitizer. Planned: `std::thread`, `std::mutex`, `std::atomic`, `std::condition_variable`.

---

## 📁 Project Structure

```text
QUANTA/
|-- include/
|   |-- core/          types.hpp
|   |-- orders/        order.hpp
|   |-- traders/       trader.hpp
|   |-- exchange/      order_book.hpp, matching_engine.hpp, trade.hpp, exchange.hpp
|   |-- risk/          risk_manager.hpp
|   |-- strategies/    strategy.hpp, strategies.hpp
|   |-- simulation/    simulator.hpp
|   |-- market_data/   (planned)
|   `-- benchmark/     (planned)
|-- src/               implementations, plus main.cpp
|-- tests/             GoogleTest suites
|-- benchmarks/        (planned)
|-- docs/
|-- CMakeLists.txt
`-- README.md
```

---

## 🔬 What Makes QUANTA Different?

QUANTA deliberately avoids being a generic stock-management system, a portfolio manager, a price predictor, a machine-learning project, a CRUD application, or a fake HFT system making unrealistic performance claims.

---

## 🎓 Learning Outcomes

By completing QUANTA, the goal is to confidently explain:

- How an exchange processes large numbers of orders
- How an order book works internally
- Why some data structures suit latency-sensitive workloads
- Why memory allocation and cache locality affect performance
- Why adding threads can make a system slower
- How locks affect latency
- How queues and backpressure work
- How P99 latency is measured
- How bottlenecks are identified
- What happens when a component fails

---

## 📌 Project Philosophy

QUANTA is not an unrealistic replica of a production exchange. It is a **learning-focused systems-engineering simulator** that connects OOP concepts with real problems in **performance, concurrency, market infrastructure, scalability and reliability**. The emphasis is on understanding **why** an architecture works, measuring its behaviour, and learning from its limitations.

---

## 👤 Author

**Preeti Pragyan Mishra**  |  Student ID: `ID-B125085`  |  Course: Object-Oriented Programming

**Simulate the market. Measure the system. Understand the infrastructure.**
