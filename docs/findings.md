## F7. Cancelling at random positions gets about 4x more expensive between 10,000 and 100,000 orders (Day 10)
Order-book micro-benchmark: OrderBook only, 40 batches of about 1,000 operations,
nanoseconds per operation, best batch.

| orders  | levels | cancel | add   | best-price | consume |
|---------|--------|--------|-------|------------|---------|
| 1,000   | 50     | 42.8   | 51.6  | 6.5        | 58.5    |
| 10,000  | 50     | 50.6   | 79.1  | 6.5        | 59.8    |
| 100,000 | 50     | 220.5  | 100.3 | 6.0        | 67.5    |
| 10,000  | 1000   | 57.2   | 126.4 | 6.0        | 61.8    |
| 100,000 | 1000   | 237.9  | 159.6 | 6.0        | 54.2    |
| 1,000   | 1000   | 162.1  | 142.4 | 7.3        | 166.3   |

- Sanity check passed: best-price is flat (6.0 to 7.3 ns) at every size.
- Cancel from 10k to 100k orders: 4.4x at 50 levels, 4.2x at 1000 levels.
- Operations at the touch (consume best, best-price) barely change with size.
- Hypothesis: random-position operations touch scattered memory, and the book stops
  fitting in the CPU caches. Cache misses were not measured.
- The 1,000-order / 1000-level row is high. Hypothesis: one order per price makes every
  cancel and add destroy or create a std::map node.
- The simulation's book (about 4,000 to 18,000 orders) is in the range where the effect is
  mild (1.2x from 1k to 10k), so the Day 9 book-size groups (F6) are probably not
  explained by book size alone.
- Replicated in a second run: [fill in yes or no].

## F8. Where the instructions go inside Exchange::submit (Day 10, callgrind)
Callgrind, RelWithDebInfo build, seed 42, 50 traders, 100,000 events.
Whole program: 179,611,861 instructions. Exchange::submit: 104,800,802 (58.4%).
Percentages are approximate (callgrind lists some functions twice, plus or minus 7%).

| part                      | instructions | share of submit |
|---------------------------|--------------|-----------------|
| MatchingEngine::process   | 64.5M        | 62%             |
| MatchingEngine::match     | 25.5M        | 24%             |
| OrderBook::add            | 18.8M        | 18%             |
| OrderBook::cancel         | 12.3M        | 12%             |
| Exchange::settle          | 4.8M         | 4.5%            |
| RiskManager::check        | 3.4M         | 3.2%            |

(These overlap: add and cancel run inside process.)
- Allocator code is 50% of OrderBook::add (9.5M of 18.8M) and 65% of OrderBook::cancel
  (8.0M of 12.3M): about 17% of submit in total.
- dynamic_cast (__dynamic_cast + __do_dyncast) is 9.86M, about 9% of submit. With
  __strcmp_avx2 (3.84M, probably from the cast) it would be about 13%.
- The risk check is cheap. The random number generator is about 9% of the whole
  program but belongs to the workload, not the exchange.
- Limit: instruction counts are not time. Cache misses are not counted.

## F9. Method for the optimisation days
- Wall-clock timings on this machine vary by 20% to 60%, so a 10% improvement is invisible.
- Callgrind instruction counts are exact and repeat identically. Use them as the main
  before/after measure, and confirm with timing where the change is large enough to see.
- Candidates, ranked by evidence (not yet tried): (1) cut per-order memory allocation in
  the book, (2) replace dynamic_cast with static_cast in Exchange::submit.