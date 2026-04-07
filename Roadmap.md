# Order Book — Full Build Roadmap

---

## The three layers (always build in this order)
```
Layer 1 — Correctness     "does it match orders right?"
Layer 2 — Performance     "does it match orders FAST?"
Layer 3 — Distribution    "does it survive a server dying?"
```

Never optimise code that isn't correct first.
Never distribute a system that isn't fast enough to be worth replicating.

---

## Layer 1 — Core matching engine

### Data structure: std::map + std::queue
Simple, readable, correct. Not fast. That's fine for now.
```
bids: std::map<int, std::queue<Order>, std::greater<int>>
asks: std::map<int, std::queue<Order>>
index: std::unordered_map<int, Order*>   ← for cancel lookups
```

### Build steps (do these in order, test each before moving on)

**Step 1 — Order struct**
```cpp
struct Order {
    int       id;
    bool      isBuy;
    int       price;      // ALWAYS integers. $10.25 → 1025
    int       quantity;
    long long timestamp;  // nanoseconds, for time priority
};
```
Why integers: floating-point comparisons silently corrupt matching logic.
0.1 + 0.2 != 0.3 in IEEE 754. Never use double for prices.

**Step 2 — OrderBook skeleton, inserts only**
```cpp
class OrderBook {
    std::map<int, std::queue<Order>, std::greater<int>> bids;
    std::map<int, std::queue<Order>> asks;
    std::unordered_map<int, Order*> index;
public:
    void addOrder(Order o);   // inserts only, no matching yet
    void printBook();
};
```
Get printBook() working first. You need to SEE the state to
trust that inserts are correct before adding matching logic.

**Step 3 — printBook()**
Print asks top-down (highest first), a separator, then bids.
Verify manually: add a few orders, does the book look right?

**Step 4 — matchBuy() and matchSell()**
The greedy loop: keep consuming the best opposite side until
price no longer crosses or quantity runs out.
```cpp
void matchBuy(Order& buy) {
    while (buy.quantity > 0 && !asks.empty()) {
        auto best = asks.begin();
        if (buy.price < best->first) break;  // no cross

        Order& sell = best->second.front();
        int fill = std::min(buy.quantity, sell.quantity);
        recordTrade(fill, best->first, buy.id, sell.id);
        buy.quantity  -= fill;
        sell.quantity -= fill;

        if (sell.quantity == 0) {
            index.erase(sell.id);
            best->second.pop();
            if (best->second.empty()) asks.erase(best);
        }
    }
}
```

**Step 5 — addOrder() with matching**
Try to match first, then rest any remainder in the book.
```cpp
void addOrder(Order o) {
    if (o.isBuy) { matchBuy(o);  if (o.quantity > 0) bids[o.price].push(o); }
    else         { matchSell(o); if (o.quantity > 0) asks[o.price].push(o); }
}
```

**Step 6 — cancelOrder() v1 (O(n) within level, fine for now)**
Use the index map for O(1) price-level lookup.
Within the price queue, scan linearly to find and remove.
Acceptable at this stage — correctness first.
```cpp
void cancelOrder(int id) {
    // find order via index, mark cancelled, remove from queue
}
```

**Step 7 — Order status enum**
```cpp
enum class Status { Open, PartiallyFilled, Filled, Cancelled };
```
A cancelled order must NEVER fill later.
A filled order must never be cancelled.
Add this before any concurrency work.

**Step 8 — Timestamp-based time priority**
Populate timestamp on order creation (nanoseconds since epoch).
Within a price level, orders must fill in arrival order.
Write a specific test: two orders at the same price, different
timestamps — verify the earlier one always fills first.

**Step 9 — Market orders**
No price specified — sweep everything available.
Implement by using sentinel prices:
- Buy market order  → price = INT_MAX  (matches any ask)
- Sell market order → price = 0        (matches any bid)
Same matching loop, no special cases needed.

**Step 10 — Stress test (do not skip this)**
Feed 10,000 random orders. Assert ALL of:
- Every fill has a matching counterpart (no phantom trades)
- Total filled qty on buy side == total filled qty on sell side
- No crossed book remains (no bid >= any ask) after each step
- Cancelled orders never appear in fill records
- Partial fills: order.quantity decremented correctly each time

If any assert fails, your matching logic has a bug.
Fix it before moving to Layer 2.

---

## Layer 1 extensions (still single-node, after stress test passes)

**Auction mode (batch matching)**
Used in pre-opening and closing sessions.
Different algorithm from continuous matching:
1. Collect orders over a time window (no matching yet)
2. Build cumulative bid and ask volume curves
3. Find the IEP: the price where cumulative bid qty >= ask qty
   and total matched volume is maximised
4. Execute all matches at that single price simultaneously
Keep this completely separate from your continuous matching code.

**Multiple order types**
- Enhanced limit order: sweeps up to 10 price levels, remainder rests
- Special limit order: same sweep, remainder cancelled (not rested)
- All-or-Nothing: fill fully right now or reject entirely, never rests
These are all variations on the same matching loop with different
"what do we do with the remainder?" logic.

**Integer overflow guard**
price × quantity can overflow int.
Always use int64_t for any price-qty multiplication:
```cpp
int64_t notional = (int64_t)order.price * order.quantity;
```

---

## Layer 2 — Performance engineering

Only start this after Layer 1 stress test passes completely.
Profile first — measure before you optimise. The bottleneck
is almost never where you think it is.

### Data structure progression

**Stage 2a — std::map with iterator hints**
Small change, meaningful insert speedup in sequential patterns.
Same underlying red-black tree, just pass a hint iterator.
Still not cache-friendly. Intermediate step, optional.

**Stage 2b — Replace std::queue with std::list + iterator index**
This is the O(1) cancel upgrade.
```cpp
// index now stores a list iterator, not a raw pointer
std::unordered_map<int, std::list<Order>::iterator> index;
```
Cancel becomes: look up iterator in O(1), splice out in O(1).
No scanning. This is the first real data structure upgrade.

**Stage 2c — Intrusive linked list + flat hashmap**
The production HFT approach.
- Order nodes embed their prev/next pointers directly (intrusive)
- Orders live in a pre-allocated memory pool (no heap alloc per order)
- Price level lookup via flat open-addressing hashmap (not std::unordered_map)
- Result: O(1) everything, cache-friendly traversal, zero malloc per order
This is a significant rewrite. Only do it once you've profiled
and confirmed std::list is your actual bottleneck.

**Stage 2d — Bit packing**
Compress order fields to fit more orders per cache line.
Side (buy/sell), status, and flags can pack into bit fields.
Price stored as uint32_t offset from a base price (saves bytes).
Measure cache miss rate before and after to confirm the gain.

**Stage 2e — Lock-free structures (if going concurrent)**
Replace mutexes with atomic operations + memory ordering.
Use a skip list for the price index if you need concurrent
readers and writers without locks.
This is hard to get right. Read "C++ Concurrency in Action"
(Williams) before attempting.

### Performance targets to aim for (rough benchmarks)
```
std::map version:       ~1–5 million orders/second
std::list + index:      ~5–20 million orders/second  
intrusive list:         ~50–200 million orders/second
```

---

## Layer 3 — Distributed systems

Only start this after Layer 2 is profiled and fast enough to
be worth the operational complexity of running multiple nodes.

### Step 1 — Make the engine deterministic
State machine replication only works if every replica given the
same input produces the same output.
Audit your code for any non-determinism:
- No std::chrono for matching decisions (use sequence numbers)
- No random tie-breaking
- No platform-dependent behaviour
- All operations must be pure functions of the input

### Step 2 — Event log
Every order (add, cancel, modify) gets a sequence number.
The engine replays events in sequence number order, not wall-clock.
This is the foundation everything else builds on.

### Step 3 — State machine replication
Run 3 or 5 identical engine replicas.
Feed all replicas the same event log in the same sequence order.
Every replica produces the same book state independently.
If the leader dies, a replica takes over — book state is identical.

### Step 4 — Consensus (Raft is the readable one)
Raft solves: "which replica decides the sequence number order?"
One leader is elected. All order submissions go to the leader.
Leader assigns sequence numbers and replicates the log.
If leader dies, remaining replicas elect a new one.

Resources:
- "The Raft consensus algorithm" — raft.github.io (readable paper)
- "Designing Data-Intensive Applications" (Kleppmann) — Ch 5, 7, 9

### Step 5 — Leader election (ring or Raft built-in)
Raft includes its own leader election — you likely don't need
a separate ring election algorithm if you're using Raft.
Ring leader election (your friend's mention) is a simpler
standalone algorithm, useful for understanding but less
production-hardened than Raft.

---

## Future roadmap — beyond single book

### Multiple books, one engine (multi-instrument)
HKEX runs 14,000+ instruments on one OTP-C platform.
Your engine needs:
- One OrderBook instance per instrument (stock code)
- A router that maps incoming orders to the right book
- Separate matching loops per book (they don't interact)
- Shared infrastructure: order ID generator, trade recorder

### Cross-instrument features
- **Index arbitrage**: trigger orders in one book based on
  price movement in another (e.g. futures vs underlying stock)
- **Basket orders**: simultaneous execution across multiple books
- **Reference data**: tick sizes, board lots, price limits
  per instrument — stored separately, looked up per order

### Market data dissemination
Replicate the OMD-C feed:
- Publish every book event (add, cancel, trade) to subscribers
- Subscribers maintain their own internal book replica
- Format: binary protocol (FIX or custom) for speed
- Sequence numbers so subscribers can detect gaps

### Risk and position management
Before orders hit the book in production:
- Pre-trade risk checks: position limits, notional limits
- Fat-finger checks: is this price wildly off market?
- Credit checks: does this participant have enough margin?
These sit in front of the matching engine as a gate.

---

## Where you are right now
```
[x] Layer 1 — Step 1: Order struct with integer prices
[x] Layer 1 — Step 2: OrderBook skeleton, inserts only
[x] Layer 1 — Step 3: printBook()
[x] Layer 1 — Step 4: matchBuy() and matchSell()
[x] Layer 1 — Step 5: addOrder() with matching
[x] Layer 1 — Step 6: cancelOrder() v1 (O(n) within level, fine for now)
[x] Layer 1 — Step 7: Order status enum (Open, PartiallyFilled, Filled, Cancelled)
[x] Layer 1 — Step 8: Timestamp time priority — verify FIFO within a price level
[X] Layer 1 — Step 9: Market orders (INT_MAX for buy, 0 for sell)
[X] Layer 1 — Step 10: Stress test (10,000 orders, assert no crossed book)
[X] Layer 1 ext: Multiple order types (ELO sweep, SLO cancel remainder, AON)
[ ] Layer 1 ext: Auction mode / IEP calculation (separate algorithm, not continuous)
[X] Layer 1.5 — Move semantics
      addOrder(Order o) → addOrder(Order&& o)
      std::move when pushing into queues
      benchmark copy vs move with 100,000 orders
      (low real benefit until Order carries heap-allocated fields like std::string)
[ ] Layer 2 — Stage 2a: cancelOrder() v2 — upgrade queue to std::list,
      store iterators in index for true O(1) cancel
  [ ] Layer 2 — Stage 2d: Lock-free structures if going concurrent (skip list, atomics)
[ ] Layer 2 — Stage 2b: Intrusive list + flat hashmap (cache-friendly, no malloc per order)
[ ] Layer 2 — Stage 2c: Bit packing (compress order fields, fit more per cache line)
[ ] Layer 2 — Profile first — measure before optimising, bottleneck is rarely where you think
[ ] Layer 3 — Determinism audit (no wall-clock time in matching, no thread races)
[ ] Layer 3 — Event log + sequence numbers (foundation for replication)
[ ] Layer 3 — State machine replication (identical replicas, same event stream)
[ ] Layer 3 — Raft consensus (leader election, log replication, failover)
[ ] Roadmap — Template the OrderBook matching policy (pro-rata vs FIFO, not per-instrument)
[ ] Roadmap — Multi-instrument router (one Exchange class, map<symbol, OrderBook>)
[ ] Roadmap — Depth tracker (top 5 aggregated levels, fires on every book change)
[ ] Roadmap — Market data feed publisher (listener callbacks → binary encode → TCP broadcast)
[ ] Roadmap — Feed subscriber / internal book replica (sequence validation, snapshot + incrementals)
[ ] Roadmap — Pre-trade risk checks (position limits, fat-finger, credit checks)
[ ] Roadmap — Pre-trade risk checks (position limits, fat-finger, credit checks)
```


```
# Auction IEP Roadmap

## 1. Auction mode / architecture
- [ ] Add an auction mode separate from continuous matching
- [ ] Make `addOrder()` branch by mode:
  - [ ] Continuous mode → match immediately
  - [ ] Auction mode → collect only, no immediate matching
- [ ] Decide whether to use:
  - [ ] Separate `AuctionEngine` class
  - [ ] Or auction state inside `OrderBook`

## 2. Auction order storage
- [ ] Create auction bid-side storage
- [ ] Create auction ask-side storage
- [ ] Store orders by:
  - [ ] price level
  - [ ] FIFO queue within level
- [ ] Reuse existing book structures where possible

## 3. Order collection phase
- [ ] Implement auction collection logic
- [ ] Accept auction buy/sell limit orders
- [ ] Insert into auction book
- [ ] Do **not** call continuous `matchBuy()` / `matchSell()`
- [ ] Decide whether cancel/modify is supported in v1

## 4. Candidate price generation
- [ ] Gather all distinct bid prices
- [ ] Gather all distinct ask prices
- [ ] Merge into one sorted candidate price list
- [ ] Use only these prices for IEP evaluation

## 5. IEP calculation
- [ ] For each candidate price `p`, compute:
  - [ ] `buyQty(p)` = total buy volume with `price >= p`
  - [ ] `sellQty(p)` = total sell volume with `price <= p`
  - [ ] `matchQty(p)` = `min(buyQty, sellQty)`
  - [ ] `imbalance(p)` = `abs(buyQty - sellQty)`
- [ ] Choose the price with:
  - [ ] maximum `matchQty`
  - [ ] then minimum `imbalance`
  - [ ] then deterministic tie-break

## 6. Tie-break rules
- [ ] Define tie-break rules clearly
- [ ] Recommended v1 rule:
  - [ ] maximise executable volume
  - [ ] minimise absolute imbalance
  - [ ] choose price nearest reference price
  - [ ] if still tied, choose lower price
- [ ] Keep the rule deterministic and documented

## 7. Auction result output
- [ ] Store computed IEP
- [ ] Store executable volume
- [ ] Store buy/sell imbalance
- [ ] Print or expose these values for debugging/tests

## 8. Batch uncrossing
- [ ] Implement uncross after IEP is chosen
- [ ] Eligible buy orders: `price >= IEP`
- [ ] Eligible sell orders: `price <= IEP`
- [ ] Allocate fills using:
  - [ ] better price first
  - [ ] FIFO within same price
- [ ] Execute all matched trades at **one common price**:
  - [ ] the IEP

## 9. Partial fills
- [ ] Handle partially filled orders correctly
- [ ] Reduce remaining quantity after execution
- [ ] Mark fully filled orders
- [ ] Leave unfilled remainder where appropriate

## 10. Leftover order policy
- [ ] Decide what happens after the auction:
  - [ ] leftover orders move to continuous book
  - [ ] or remain in auction book
  - [ ] or get cancelled
- [ ] Recommended v1:
  - [ ] leftover limit orders carry into continuous book

## 11. Edge cases
- [ ] No crossing / no trade
- [ ] One-sided book only
- [ ] Multiple prices tie on max volume
- [ ] Tie on max volume and imbalance
- [ ] Partial fills at the clearing price
- [ ] Empty book
- [ ] Reference price tie-break case

## 12. Testing roadmap
- [ ] Test unique IEP case
- [ ] Test tie on executable volume
- [ ] Test tie broken by imbalance
- [ ] Test tie broken by reference price
- [ ] Test no-trade auction
- [ ] Test partial fill at IEP
- [ ] Test leftovers carried into continuous book
- [ ] Test FIFO within same price level
- [ ] Test better-price priority over same-price orders

## 13. Recommended v1 scope
- [ ] Limit orders only
- [ ] No market orders yet
- [ ] No advanced auction order types yet
- [ ] Deterministic tie-break rules
- [ ] Leftovers move to continuous book
- [ ] Focus on correctness before optimisation

## 14. Suggested implementation order
- [ ] Build auction storage
- [ ] Build collection-only mode
- [ ] Build candidate price generation
- [ ] Build `computeIEP()`
- [ ] Build batch uncrossing
- [ ] Build leftover carry-forward logic
- [ ] Add tests
- [ ] Refactor / optimise after correctness

## 15. Core mental model
- [ ] Continuous matching = match on arrival
- [ ] Auction matching = collect first, compute one price, then batch match
- [ ] Priority decides **who gets filled**
- [ ] IEP decides **the price everyone trades at**
```