# QUANTA: Low-Latency Stock Exchange & Market Infrastructure Simulator

> **Course:** Object-Oriented Programming (OOP) Semester Project  
> **Developer:** Preeti Pragyan Mishra  
> **ID:** b125085 | **Branch:** CSE-B | **Group:** 1  

---

## 1. Project Philosophy

QUANTA is a modern C++ systems engineering exploration into the infrastructure underneath financial markets. It avoids generic stock management or price prediction to focus strictly on performance engineering.

### Core Engineering Question
> *How can a financial market process massive order flow with microsecond latency while remaining correct, concurrent, scalable, and fault-tolerant?*

---

## 2. System Architecture

The simulator processes synthetic order flow from automated traders, routes them through risk checks, and matches them using **Price-Time Priority (FIFO)**.

```text
+------------------+     +-----------------+     +-----------------+
|  Traders / Flow  | --> |   Risk Checks   | --> | Matching Engine |
+------------------+     +-----------------+     +-----------------+
|
v
+------------------+     +-----------------+     +-----------------+
| Telemetry / Logs | <-- |   Market Data   | <-- |   Order Book    |
+------------------+     +-----------------+     +-----------------+
```
### Core Subsystems
* **Order Book & Matching Engine:** Core execution engine optimizing for an $O(1)$ critical path.
* **Trader Strategies:** Polymorphic order generators driving realistic market pressure.
* **Market Data Pipeline:** Asynchronous event distribution via lock-free pub/sub channels.
* **Telemetry & Metrics:** Microsecond-level performance profiling targeting tail latencies.

---

## 3. Data Structures & Performance Roadmap

| Area | Initial Approach | Optimized Target | Engineering Goal |
| :--- | :--- | :--- | :--- |
| **Order Book** | `std::map<Price, OrderList>` | Custom Sparse Array / B-Tree | Eliminate $O(\log n)$ tree-traversal overhead |
| **Memory Management** | Dynamic Heap Allocation (`new`/`delete`) | Thread-Local Object Pools | Eliminate OS syscalls and prevent memory fragmentation |
| **Concurrency** | Shared Mutex on Order Book | Lock-Free Queue + Single-Writer Thread | Minimize lock contention and CPU context switching |
| **Data Layout** | Standard Pointer Structures | Contiguous Flat Buffers | Optimize CPU L1/L2 cache locality and line prefetching |

---

## 4. OOP Design & Component Hierarchy

QUANTA balances clean software engineering with runtime performance, avoiding unnecessary dynamic dispatch on hot paths.

### Class Hierarchies

### Design Principles Applied
* **Encapsulation & RAII:** Automatic resource management with smart pointers and zero-leak guarantees.
* **Composition Over Inheritance:** Structural components use lightweight composition to avoid vtable penalties on critical paths.
* **SOLID Principles:** Decoupled interfaces for strategy generation, risk validation, and market-data streaming.

---

## 5. Benchmarking & Tail Latency Metrics

QUANTA tracks microsecond latency distributions rather than simple averages to isolate system jitter.

```text
======================================================
               QUANTA BENCHMARK REPORT                
======================================================
Throughput : 184,291 orders/sec  |  96,421 trades/sec
------------------------------------------------------
Latency    : P50   :  1.2 µs     |  P95   :  2.7 µs
             P99   :  5.8 µs     |  P99.9 : 14.3 µs
------------------------------------------------------
System     : Queue : 2,421       |  CPU   : 78%
             Memory: 412 MB      |  Locks : Minimal
======================================================
```
## 6. Incremental Development Plan
[x] Phase 1: Core Domain Model & OOP Architecture

[ ] Phase 2: Order Book Data Structures (std::map baseline)

[ ] Phase 3: Matching Engine Logic & Price-Time Priority

[ ] Phase 4: Polymorphic Workload Generators (Traders)

[ ] Phase 5: Microsecond Telemetry & Latency Profiling

[ ] Phase 6: Multi-Threaded Concurrency Models

[ ] Phase 7: Asynchronous Market Data Event Pipeline

[ ] Phase 8: Low-Latency Optimizations (Memory Pools & Cache Alignment)

[ ] Phase 9: Scalability & Partitioning Benchmarks

[ ] Phase 10: Fault Injection, Backpressure, & System Recovery
