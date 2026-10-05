# ⚡ QUANTA

### Low-Latency Stock Exchange & Market Infrastructure Simulator

<p align="center">

**A modern C++ simulation of the infrastructure behind financial markets.**

<br>

[![C++](https://img.shields.io/badge/C%2B%2B-17%2B-00599C?style=for-the-badge\&logo=cplusplus\&logoColor=white)](#)
[![OOP](https://img.shields.io/badge/OOP-Coursework-6A5ACD?style=for-the-badge)](#)
[![Systems](https://img.shields.io/badge/Systems-Engineering-2E8B57?style=for-the-badge)](#)
[![Performance](https://img.shields.io/badge/Performance-Low%20Latency-FF8C00?style=for-the-badge)](#)

</p>

---

## 👨‍💻 Project Information

|                  |                                                        |
| ---------------- | ------------------------------------------------------ |
| **Project**      | QUANTA                                                 |
| **Student ID**   | ID-B125085                                             |
| **Name**         | **Preeti Pragyan Mishra**                              |
| **Course**       | Object-Oriented Programming                            |
| **Language**     | Modern C++                                             |
| **Project Type** | Systems Engineering / Market Infrastructure Simulation |

---

# 🚀 What is QUANTA?

**QUANTA** is a simplified **stock exchange and market infrastructure simulator** built in modern C++.

Instead of building a conventional stock-management, portfolio-management, or CRUD application, QUANTA focuses on the **infrastructure underneath financial markets**.

The central question is:

> **How can a financial exchange process a huge number of orders and market-data events with extremely low latency while remaining correct, concurrent, scalable, and fault tolerant?**

The simulator models the major components of an exchange:

```text
                 ┌─────────────────────┐
                 │   Simulated Traders  │
                 └──────────┬──────────┘
                            │
                            ▼
                    ┌───────────────┐
                    │     Orders    │
                    └───────┬───────┘
                            │
                            ▼
                 ┌─────────────────────┐
                 │    Risk Checks      │
                 └──────────┬──────────┘
                            │
                            ▼
              ┌──────────────────────────┐
              │       ORDER BOOK         │
              │                          │
              │  BUY             SELL   │
              │  ───             ────   │
              │ ₹100.10          ₹100.15│
              │ ₹100.05          ₹100.20│
              │ ₹100.00          ₹100.25│
              └────────────┬─────────────┘
                           │
                           ▼
                 ┌──────────────────┐
                 │ Matching Engine  │
                 └────────┬─────────┘
                          │
                          ▼
                   ┌─────────────┐
                   │   Trades    │
                   └──────┬──────┘
                          │
                          ▼
              ┌─────────────────────────┐
              │    Market Data Feed     │
              └────────────┬────────────┘
                           │
                ┌──────────┼──────────┐
                ▼          ▼          ▼
             Traders      Risk     Analytics
```

---

# 🎯 Project Goals

QUANTA is designed to explore:

* 🧩 Object-Oriented Programming
* ⚡ Low-latency system design
* 🚀 High-throughput processing
* 🧵 Concurrency
* 🧠 Efficient data structures
* 💾 Memory efficiency
* 📊 Performance engineering
* 📡 Event-driven architecture
* 📈 Scalability
* 🛡️ Fault tolerance

The goal is **not** to build a profitable trading bot.

The goal is to understand **how the infrastructure that processes trades can be designed and optimized**.

---

# 🏗️ Core Architecture

QUANTA is divided into several logical components:

```text
                         QUANTA
                            │
          ┌─────────────────┼─────────────────┐
          │                 │                 │
          ▼                 ▼                 ▼
     DOMAIN LAYER       EXCHANGE LAYER    SIMULATION
          │                 │                 │
          │                 │                 │
      ┌───┴───┐       ┌─────┴──────┐      ┌───┴────────┐
      │       │       │            │      │            │
    Order   Trader  Order Book  Matching  Strategies  Workload
      │       │       │            │      │            │
      └───┬───┘       └─────┬──────┘      └────────────┘
          │                 │
          │                 ▼
          │          Trade Execution
          │                 │
          │                 ▼
          │          Market Data
          │                 │
          └─────────────────┼────────────────┐
                            ▼                ▼
                       Benchmarking       Monitoring
```

---

# 🧱 Major Components

### 👤 Traders

Simulated participants that generate orders.

Possible trader types:

```text
Trader
├── RetailTrader
├── InstitutionalTrader
└── AlgorithmicTrader
```

---

### 📋 Orders

Orders represent trading instructions submitted to the exchange.

```text
Order
├── MarketOrder
├── LimitOrder
└── CancelOrder
```

The design allows new order types to be introduced without rewriting the entire matching engine.

---

### 📖 Order Book

The order book maintains:

* Bid prices
* Ask prices
* Orders at each price level
* Order insertion
* Order cancellation
* Order matching

The system follows appropriate **price/time priority** rules.

---

### ⚙️ Matching Engine

The matching engine is the heart of QUANTA.

Its responsibility is to:

```text
Receive Order
      │
      ▼
Validate
      │
      ▼
Risk Check
      │
      ▼
Insert into Order Book
      │
      ▼
Find Compatible Orders
      │
      ▼
Match
      │
      ▼
Generate Trade
      │
      ▼
Publish Market Event
```

---

# 🧠 OOP Design

Since QUANTA is an **Object-Oriented Programming coursework project**, OOP is an important part of the architecture.

The project demonstrates:

| OOP / C++ Concept    | Application in QUANTA                                 |
| -------------------- | ----------------------------------------------------- |
| **Encapsulation**    | Protecting order-book and exchange state              |
| **Abstraction**      | Interfaces for strategies and exchange components     |
| **Inheritance**      | Different order/trader/strategy types                 |
| **Polymorphism**     | Runtime selection of strategies/order behavior        |
| **Composition**      | Exchange composed of books, engine, risk system, etc. |
| **SOLID Principles** | Modular and extensible architecture                   |
| **Design Patterns**  | Applied where they solve real design problems         |
| **RAII**             | Safe resource and lifetime management                 |
| **Smart Pointers**   | Controlled dynamic ownership                          |
| **STL**              | Efficient containers and algorithms                   |
| **Templates**        | Used where genuinely beneficial                       |

> **Design principle:** inheritance will not be forced merely to demonstrate OOP. Composition and modern C++ techniques will be preferred where they produce a better architecture.

---

# 📊 Data Structures & Performance

One of the key learning objectives is understanding **why certain data structures perform better under latency-sensitive workloads**.

Potential structures include:

```text
std::map
std::unordered_map
priority queues
custom price-level structures
custom order containers
```

Rather than simply choosing one structure, QUANTA will allow different approaches to be benchmarked.

### Example Question

> Why might a particular order-book representation outperform another under a high-frequency synthetic workload?

The answer should come from **measurement and architectural reasoning**, not assumptions.

---

# ⚡ Low-Latency Pipeline

Latency is one of the primary goals of QUANTA.

The simulator measures the processing pipeline:

```text
┌───────────────┐
│ Order Received│
└───────┬───────┘
        ▼
┌───────────────┐
│   Validation  │
└───────┬───────┘
        ▼
┌───────────────┐
│   Risk Check  │
└───────┬───────┘
        ▼
┌───────────────┐
│ Order Insertion│
└───────┬───────┘
        ▼
┌───────────────┐
│    Matching   │
└───────┬───────┘
        ▼
┌───────────────┐
│ Trade Creation│
└───────┬───────┘
        ▼
┌───────────────┐
│ Market Data   │
└───────────────┘
```

### Metrics

QUANTA will measure:

* **Average latency**
* **P50**
* **P95**
* **P99**
* **P99.9**
* **Maximum latency**
* **Throughput**

Tail latency is particularly important because average latency alone can hide occasional but significant slowdowns.

---

# 🧵 Concurrency

QUANTA simulates multiple traders submitting orders concurrently.

Modern C++ concurrency facilities can be explored, including:

```cpp
std::thread
std::mutex
std::shared_mutex
std::atomic
std::condition_variable
```

and thread-pool architectures.

The project investigates:

```text
Race Conditions
       │
       ▼
Data Races
       │
       ▼
Lock Contention
       │
       ▼
Deadlocks
       │
       ▼
Thread Coordination
       │
       ▼
Latency vs Throughput
```

A key experiment is comparing different architectures.

### Architecture A — Shared State

```text
Thread ──┐
Thread ──┼──► Shared Order Book
Thread ──┤          │
Thread ──┘          ▼
                 Heavy Locks
```

### Architecture B — Controlled Critical Path

```text
Multiple Producers
        │
        ▼
     Queue
        │
        ▼
Serialized Matching Path
        │
        ▼
   Order Book
        │
        ▼
Parallel Consumers
```

The objective is to determine experimentally when each architecture makes sense.

---

# 📡 Market Data Pipeline

After trades are executed, QUANTA generates market-data events.

```text
                 Trade Executed
                       │
                       ▼
                Market Data Event
                       │
             ┌─────────┼─────────┐
             ▼         ▼         ▼
          Traders     Risk    Analytics
```

The system explores:

* Producer/consumer architecture
* Queues
* Backpressure
* Batching
* Event processing
* Event prioritization

---

# 📈 Scalability Experiments

The workload generator can simulate increasing order rates:

```text
1,000 orders/sec
        ↓
10,000 orders/sec
        ↓
100,000 orders/sec
        ↓
1,000,000 orders/sec
```

These are **synthetic workloads**, not claims of real-world exchange performance.

Actual performance depends on the hardware and implementation.

Measurements include:

| Metric                  | Purpose               |
| ----------------------- | --------------------- |
| Orders/sec              | Processing throughput |
| Trades/sec              | Matching throughput   |
| Latency                 | Processing delay      |
| Queue depth             | Backlog               |
| CPU usage               | Compute utilization   |
| Memory usage            | Resource consumption  |
| Contention              | Concurrency overhead  |
| Performance degradation | Scalability behavior  |

---

# 🤖 Trading Strategies

Trading strategies are used primarily to create varied and realistic order flow.

```text
TradingStrategy
├── RandomStrategy
├── MomentumStrategy
├── MeanReversionStrategy
└── MarketMakingStrategy
```

They demonstrate **runtime polymorphism** while acting as simulated participants in the exchange.

> The objective is **not** to create a profitable trading algorithm.

---

# 🛡️ Risk Management

QUANTA includes simple exchange-level risk checks:

* Insufficient funds
* Position limits
* Maximum order size
* Trading limits

Risk management remains intentionally lightweight so the project stays focused on **market infrastructure rather than portfolio analytics**.

---

# 💥 Failure Simulation

The system will eventually simulate controlled failures such as:

```text
Matching Engine Failure
Market Data Consumer Failure
Queue Overload
Slow Consumer
Network Delay
Temporary Service Failure
```

The project investigates:

* Retries
* Recovery
* Backpressure
* Fault isolation
* Failover
* Data consistency

---

# 📊 Benchmarking

Performance numbers will be generated from actual experiments.

Example dashboard:

```text
╔══════════════════════════════════════════╗
║              QUANTA BENCHMARK            ║
╠══════════════════════════════════════════╣
║ Orders/sec              [MEASURED]       ║
║ Trades/sec              [MEASURED]       ║
║ P50 latency             [MEASURED]       ║
║ P95 latency             [MEASURED]       ║
║ P99 latency             [MEASURED]       ║
║ P99.9 latency           [MEASURED]       ║
║ Maximum latency         [MEASURED]       ║
║ Queue depth             [MEASURED]       ║
║ CPU utilization         [MEASURED]       ║
║ Memory usage            [MEASURED]       ║
╚══════════════════════════════════════════╝
```

**No benchmark values will be fabricated.**

Every optimization should follow:

```text
1. Establish baseline
        ↓
2. Identify bottleneck
        ↓
3. Change one thing
        ↓
4. Benchmark again
        ↓
5. Compare results
        ↓
6. Explain the trade-off
```

---

# 🗺️ Development Roadmap

```text
Phase 1  ──► Basic OOP Architecture
                  │
Phase 2  ──► Order Book
                  │
Phase 3  ──► Matching Engine
                  │
Phase 4  ──► Trading Strategies
                  │
Phase 5  ──► Benchmarking
                  │
Phase 6  ──► Concurrent Processing
                  │
Phase 7  ──► Market Data Pipeline
                  │
Phase 8  ──► Performance Optimization
                  │
Phase 9  ──► Scaling Experiments
                  │
Phase 10 ──► Failure Simulation
```

Each phase should keep the previous version functional.

---

# 🛠️ Technology Stack

### Core

* **C++ / Modern C++**
* **STL**
* Object-Oriented Design
* Multithreading
* Concurrency primitives
* RAII
* Smart pointers
* Templates

### Systems Concepts

* Order-book architecture
* Matching engines
* Event-driven systems
* Producer/consumer pipelines
* Queues
* Backpressure
* Concurrency
* Memory management
* Cache locality
* Performance profiling
* Benchmarking
* Fault tolerance

### Development Philosophy

> **Measure first. Optimize second.**

The project prioritizes correctness, measurable performance, architectural reasoning, and learning over simply adding more features.

---

# 📁 Planned Project Structure

```text
QUANTA/
│
├── include/
│   ├── core/
│   ├── exchange/
│   ├── orders/
│   ├── traders/
│   ├── strategies/
│   ├── risk/
│   ├── market_data/
│   └── benchmark/
│
├── src/
│   ├── core/
│   ├── exchange/
│   ├── orders/
│   ├── traders/
│   ├── strategies/
│   ├── risk/
│   ├── market_data/
│   └── benchmark/
│
├── tests/
│
├── benchmarks/
│
├── docs/
│
├── CMakeLists.txt
│
└── README.md
```

*The structure can evolve as the architecture develops.*

---

# 🔬 What Makes QUANTA Different?

QUANTA deliberately avoids becoming:

❌ A generic stock-management system
❌ A portfolio-management system
❌ A stock-price prediction system
❌ A machine-learning project
❌ A CRUD application
❌ A database-management application
❌ A fake HFT system making unrealistic performance claims

Instead, it focuses on:

```text
              ┌────────────────────┐
              │       QUANTA       │
              └─────────┬──────────┘
                        │
       ┌────────────────┼────────────────┐
       ▼                ▼                ▼
      OOP             C++         Market Infrastructure
       │                │                │
       └────────────────┼────────────────┘
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
      Low Latency   Concurrency   High Throughput
          │             │             │
          └─────────────┼─────────────┘
                        ▼
                 Performance
                        │
                        ▼
                   Scalability
                        │
                        ▼
                 Fault Tolerance
```

---

# 🎓 Learning Outcomes

By completing QUANTA, the goal is to confidently explain:

> **How does an exchange process enormous numbers of orders?**

> **How does an order book work internally?**

> **Why are some data structures better for latency-sensitive workloads?**

> **Why does memory allocation affect performance?**

> **What is cache locality?**

> **Why can adding threads make a system slower?**

> **How do locks affect latency?**

> **How can an exchange architecture scale?**

> **How do queues and backpressure work?**

> **How is P99 latency measured?**

> **How are system bottlenecks identified?**

> **What happens when a component fails?**

---

# 📌 Project Philosophy

QUANTA is not intended to be an unrealistic replica of a production exchange.

It is a **learning-focused systems-engineering simulator** designed to connect OOP concepts with real-world problems in:

**performance • concurrency • market infrastructure • scalability • reliability**

The emphasis is on understanding **why** an architecture works, measuring its behavior, and learning from its limitations.

---

## 👤 Author

### Preeti Pragyan Mishra

**Student ID:** `ID-B125085`

**Course:** Object-Oriented Programming

**Project:** `QUANTA — Low-Latency Stock Exchange & Market Infrastructure Simulator`

---

<p align="center">

### ⚡ QUANTA

**Simulate the market. Measure the system. Understand the infrastructure.**

</p>
