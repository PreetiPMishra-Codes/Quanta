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

## 📌 Current Status

Phases 1 to 5 are **built, tested and measured**. Phases 6 to 10 are planned.

| Phase | Topic | Status |
|---|---|---|
| 1 | Basic OOP architecture | ✅ Built and tested |
| 2 | Order book | ✅ Built and tested |
| 3 | Matching engine | ✅ Built and tested |
| 4 | Trading strategies and workload simulator | ✅ Built and tested |
| 5 | Benchmarking | ✅ Built, tested and measured |
| 6 | Concurrent processing | ⏳ Planned |
| 7 | Market data pipeline | ⏳ Planned |
| 8 | Performance optimization | ⏳ Planned |
| 9 | Scaling experiments | ⏳ Planned |
| 10 | Failure simulation | ⏳ Planned |

### Verification

- **146 automated tests** (GoogleTest), including property tests with fixed seeds.
- Clean under **AddressSanitizer** and **UndefinedBehaviorSanitizer**.
- Clean build with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` (0 warnings).
- **Conservation checks** after simulated runs, including a 1,000,000-event run: net position is 0 and total cash is unchanged.
- The same seed produces an identical run.
- The order book is never left crossed (best bid is always below best ask).

### Build and run

```bash
sudo apt install -y build-essential cmake git
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
./build/quanta 50000 42
```

Arguments:

```text
./build/quanta <number_of_events> <random_seed>
```

Use `Debug` while developing and `Release` for timing measurements.

---

## 📈 Observed Simulation Behaviour

Seed 42, 50 traders, 1,000,000 events:

| Observation | Result |
|---|---|
| Orders rejected by risk checks | 29,519 |
| Rejection reasons | 28,733 position limit, 786 insufficient funds |
| Trades executed | 375,714 |
| Shares traded | 4,896,633 |
| Self-trades | 0 occurred, 3,972 prevented |
| Successful cancels | 151,412 of 299,759 |
| Shares conserved | Yes |
| Cash conserved | Yes |

Short runs such as 20,000 events may produce no risk rejections, while the longer run exercises the risk-management limits.

---

## 🧠 Design Decisions

- **Prices are integer ticks** (1 tick = 1 paisa), never `double`, so matching comparisons are exact.
- **The order book stores, the matching engine decides.** Each can change independently.
- **Trades execute at the resting order's price**, so the newcomer receives price improvement when applicable.
- **Rejections are result values.** Exceptions are reserved for programmer errors.
- **Self-trades are prevented** by cancelling the trader's own resting order.
- **The `Exchange` uses composition** for the risk manager, matching engine and traders instead of inheriting from them.
- **Strategies see a read-only `MarketView`** and return `Action` data. They cannot directly modify the exchange.

---

# 📊 Performance & Benchmarking

All benchmark measurements below were obtained from the **Release build** using seed `42`.

These measurements describe this simulator on the development machine. They are **not claims about production stock exchanges or real-world HFT systems**.

---

## Baseline Benchmark

Configuration:

```text
Build: Release
Seed: 42
Warm-up events: 50,000
Measured events: 200,000
Runs: 5
```

### Results

| Metric | Median of 5 runs |
|---|---:|
| Mean latency | **520 ns** |
| P50 | **358 ns** |
| P95 | **1,110 ns** |
| P99 | **1,980 ns** |
| P99.9 | **22,762 ns** |
| Engine throughput | **1,922,788 orders/s** |

Timer overhead was measured separately at **22 ns**.

The results show why tail latency matters: although the P50 is only 358 ns, the P99.9 reaches 22.762 µs.

---

## Order-Book Microbenchmark

This benchmark isolates the `OrderBook`.

It does **not** include:

- Matching engine
- Trading strategies
- Risk checks

The benchmark measures individual operations in nanoseconds per operation.

Representative median measurements:

| Workload | Cancel | Add | Best Price | Consume |
|---|---:|---:|---:|---:|
| 1,000 orders / 50 levels | 68.7 ns | 98.0 ns | 9.3 ns | 180.6 ns |
| 10,000 orders / 1,000 levels | 90.8 ns | 171.5 ns | 7.0 ns | 101.8 ns |
| 100,000 orders / 1,000 levels | 344.3 ns | 254.8 ns | 8.7 ns | 144.7 ns |

The benchmark also reports best-batch values and identifies several workloads as noisy. Therefore these numbers should be treated as microbenchmark measurements rather than precise production-performance predictions.

---

## Profile Benchmark

The profile benchmark pooled 5 runs after discarding the first run.

Total latency samples:

```text
929,905
```

### Latency by Work Category

| Category | Share | P50 | P95 | P99 |
|---|---:|---:|---:|---:|
| Limit-rested | 45.3% | 279 ns | 742 ns | 1,166 ns |
| Limit-traded | 20.3% | 572 ns | 1,777 ns | 2,970 ns |
| Market | 2.0% | 636 ns | 1,900 ns | 3,110 ns |
| Cancel-ok | 17.3% | 428 ns | 1,082 ns | 1,702 ns |
| Cancel-failed | 14.9% | 168 ns | 610 ns | 924 ns |
| Rejected | 0.2% | 169 ns | 363 ns | 538 ns |
| **All** | **100.0%** | **329 ns** | **1,132 ns** | **2,038 ns** |

### Important Finding

`limit-traded` operations represent only **20.3% of all samples**, but account for **65.8% of the overall P99 tail**.

This suggests that matching and trade-related processing is a major contributor to high-latency events.

---

## Rehash / Tail-Latency Investigation

The cancellation index uses:

```cpp
std::unordered_map
```

The profile benchmark recorded:

```text
Total operations:             929,905
Operations triggering rehash: 10
Rehashes among 20 slowest:     5
```

The slowest observed operation took:

```text
1,804,692 ns
```

By comparison, limit-rested operations without rehashing had:

```text
P50: 279 ns
P99.9: 8,082 ns
```

This identifies hash-table reallocation as a possible contributor to extreme latency outliers.

A potential future optimization is to investigate reserving capacity for the cancellation index and then **benchmark the change rather than assuming it improves performance**.

---

# 📈 Load-Level Scaling Experiment

The load-level benchmark changes the workload size and therefore the resulting book size.

It does **not** represent a constant arrival-rate test.

| Events | Orders | Book at End | P50 | P99 | Median Throughput |
|---:|---:|---:|---:|---:|---:|
| 1,000 | 989 | 210 | 290 ns | 1,244 ns | 2.30M orders/s |
| 10,000 | 9,438 | 1,538 | 203 ns | 967 ns | 3.49M orders/s |
| 100,000 | 92,496 | 14,960 | 187 ns | 1,447 ns | 2.56M orders/s |
| 1,000,000 | 896,887 | 66,154 | 351 ns | 2,290 ns | 1.75M orders/s |

Several measurements were explicitly marked as noisy by the benchmark.

At the largest tested workload, the resulting book contained **66,154 resting orders**, while median throughput remained approximately **1.75 million orders/s** on the development machine.

P99.9 values at very small workloads should not be over-interpreted because there are too few samples for a reliable percentile estimate.

---

# 🔬 Performance Conclusions

The benchmarks establish a reproducible baseline for future optimization.

The current measurements suggest two interesting optimization directions:

### 1. Matching-related latency

Traded limit orders contribute disproportionately to the latency tail.

Future work can investigate:

- Matching-loop efficiency
- Memory access patterns
- Allocation behavior
- Cache locality
- Trade creation and settlement overhead

### 2. Cancellation-index rehashing

Rare `std::unordered_map` rehashes produced some of the largest latency outliers.

Future experiments can investigate:

- `reserve()`
- Load-factor tuning
- Alternative indexing structures
- Allocation behavior

The optimization methodology is:

```text
1. Establish baseline
          |
          v
2. Identify bottleneck
          |
          v
3. Change one thing
          |
          v
4. Benchmark again
          |
          v
5. Compare results
          |
          v
6. Explain the trade-off
```

> **Measure first. Optimize second.**

---

# 🏗️ Architecture

| Layer | Contents |
|---|---|
| **Simulation** | `Simulator`, `TradingStrategy` implementations |
| **Exchange** | `Exchange`, `RiskManager`, `MatchingEngine`, traders |
| **Core Engine** | `MatchingEngine`, `OrderBook`, `Trade` |
| **Domain** | Order hierarchy, Trader hierarchy, core types |

---

## Class Hierarchies

```text
Order
|
|-- TradeOrder
|   |
|   |-- MarketOrder
|   |
|   `-- LimitOrder
|
`-- CancelOrder
```

```text
Trader
|
|-- RetailTrader
|-- InstitutionalTrader
`-- AlgorithmicTrader
```

```text
TradingStrategy
|
|-- RandomStrategy
|-- MomentumStrategy
|-- MeanReversionStrategy
`-- MarketMakingStrategy
```

`CancelOrder` is deliberately separate from `TradeOrder` because a cancellation does not have a side, price or quantity.

---

# 📖 Order Book

The order book follows **price-time priority**.

```text
   SELLERS / ASKS
     100.25  x 300
     100.20  x 150
     100.15  x 200   <- best ask

   ------ spread ------

     100.10  x 120   <- best bid
     100.05  x 500
     100.00  x 100
   BUYERS / BIDS
```

### Matching Rules

| Rule | Meaning |
|---|---|
| Price priority | Best price trades first |
| Time priority | Earlier order wins at the same price |
| Resting price | Resting order determines trade price |
| Market orders | Consume available liquidity |
| Limit orders | Unfilled quantity rests in the book |
| Cancel | Only the owner can cancel an order |

### Data Structures

The order book uses:

- `std::map` for sorted price levels
- `std::list` for stable order iterators at each price level
- `std::unordered_map` for direct cancellation lookup

This avoids scanning the entire book when cancelling an order.

---

# 🧠 OOP Design

| OOP / C++ Concept | Application in QUANTA |
|---|---|
| **Encapsulation** | Order-book internals and trader state are private |
| **Abstraction** | `Order` and `TradingStrategy` provide abstract interfaces |
| **Inheritance** | `TradeOrder` is the parent of `MarketOrder` and `LimitOrder` |
| **Polymorphism** | Strategy and order interfaces allow different implementations |
| **Composition** | `Exchange` contains a risk manager and matching engine |
| **RAII / Smart pointers** | `unique_ptr` provides clear ownership |
| **STL** | `map`, `list`, `unordered_map`, `vector`, `optional` |
| **Testing** | Unit, property and conservation tests |

> Inheritance is not used merely to demonstrate OOP. Composition is preferred where it produces a cleaner design.

---

# 🛡️ Risk Management

Risk checks occur before an order reaches the order book.

The current risk manager checks:

- Maximum order size
- Position limits
- Sufficient funds for purchases

Cancels do not require risk approval because cancelling an order reduces exposure.

Orders that fail validation return a reason code rather than using exceptions for normal control flow.

---

# 🧪 Testing & Correctness

QUANTA contains **146 automated GoogleTest tests**.

The test suite covers:

- Order creation
- Order validation
- Order-book insertion
- Price-time priority
- Matching
- Market orders
- Limit orders
- Cancellations
- Risk checks
- Trader behavior
- Strategies
- Simulation invariants
- Conservation properties
- Deterministic behavior

The project also uses:

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

Correctness checks include:

```text
Net position = 0
Cash conserved = yes
Best bid < best ask
Self-trades prevented
Same seed -> deterministic output
```

---

# ⚡ Planned Work

## Concurrency

Two possible architectures can be experimentally compared:

```text
A: Shared State

Thread --+
Thread --+--> Shared Order Book + Locks
Thread --+


B: Controlled Critical Path

Producers
    |
    v
 Queue
    |
    v
Single Matching Thread
    |
    v
Order Book
```

The purpose would be to study throughput, contention, latency and tail behavior rather than assuming that more threads automatically improve performance.

---

## Market Data Pipeline

Future work could publish trade events through a bounded queue to subscribers, allowing experiments with:

- Backpressure
- Queue capacity
- Consumer lag
- Event delivery
- Throughput

---

## Performance Optimization

The completed benchmarks provide the baseline.

Future experiments may investigate:

- Order-book representations
- Memory allocation
- Hash-table capacity management
- Cache locality
- Matching-loop optimization

Every optimization should be measured against the existing baseline.

---

## Scaling Experiments

Future work can explore larger synthetic workloads and different book sizes.

These experiments would be synthetic and should not be interpreted as claims about real-world exchange capacity.

---

## Failure Simulation

Future experiments may model:

- Slow consumers
- Queue overload
- Temporary component failures
- Recovery
- Backpressure

---

# 🛠️ Technology Stack

- **C++17**
- **STL**
- **CMake**
- **GoogleTest**
- **Git**
- **AddressSanitizer**
- **UndefinedBehaviorSanitizer**
- **MinGW-w64 / WSL** for the Windows build workflow

Planned concurrency technologies:

```text
std::thread
std::mutex
std::atomic
std::condition_variable
```

---

# 📁 Project Structure

```text
QUANTA/
|
|-- include/
|   |-- core/
|   |   `-- types.hpp
|   |
|   |-- orders/
|   |   `-- order.hpp
|   |
|   |-- traders/
|   |   `-- trader.hpp
|   |
|   |-- exchange/
|   |   |-- order_book.hpp
|   |   |-- matching_engine.hpp
|   |   |-- trade.hpp
|   |   `-- exchange.hpp
|   |
|   |-- risk/
|   |   `-- risk_manager.hpp
|   |
|   |-- strategies/
|   |   |-- strategy.hpp
|   |   `-- strategies.hpp
|   |
|   `-- simulation/
|       `-- simulator.hpp
|
|-- src/
|   |-- implementations
|   `-- main.cpp
|
|-- tests/
|   `-- GoogleTest suites
|
|-- benchmarks/
|   |-- bench_baseline
|   |-- bench_book
|   |-- bench_profile
|   `-- measured CSV results
|
|-- docs/
|
|-- CMakeLists.txt
|
`-- README.md
```

---

# 🔬 What Makes QUANTA Different?

QUANTA deliberately avoids being:

- A generic stock-management system
- A portfolio manager
- A CRUD application
- A price prediction model
- A machine-learning project
- A fake HFT system making unrealistic performance claims

Instead, it focuses on the **engineering infrastructure behind order processing**.

---

# 🎓 Learning Outcomes

The project provides practical experience with:

- Object-oriented design
- Inheritance and polymorphism
- Composition
- Encapsulation
- Order-book data structures
- Matching algorithms
- Risk management
- Deterministic simulation
- Property-based testing
- Performance measurement
- Tail latency
- P99/P99.9 analysis
- Memory behavior
- Hash-table behavior
- Systems benchmarking
- Identifying performance bottlenecks

---

# 📌 Project Philosophy

QUANTA is not an unrealistic replica of a production exchange.

It is a **learning-focused systems-engineering simulator** connecting OOP concepts with real problems in:

- Performance
- Market infrastructure
- Concurrency
- Scalability
- Reliability
- Data structures

The emphasis is on understanding **why** an architecture works, measuring its behavior, and learning from its limitations.

> **Simulate the market. Measure the system. Understand the infrastructure.**

---

## 👤 Author

**Preeti Pragyan Mishra**

Student ID: `ID-B125085`

Course: **Object-Oriented Programming**
