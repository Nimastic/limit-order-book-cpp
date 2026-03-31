## Market Data Feed — How the Exchange Talks to the World

---

### What is a market data feed?

After the matching engine executes a trade or the book changes,
that information needs to leave the exchange and reach every
firm that's subscribed. The market data feed is that broadcast pipe.

It carries two kinds of events:
- **Book events** — order added, order cancelled, order modified, price level changed
- **Trade events** — a match occurred: symbol, quantity, price

The feed is one-way and one-to-many. The exchange pushes.
Subscribers only receive, never send back on the same pipe.

![View of Exchange](/assets/Figure_1_Conceptual_View_of_Exchange.png)
Trade Clients = brokers submitting orders via OCG-C (order ingestion)
Exchange = OTP-C matching engine, one OrderBook per instrument
Feed Clients = trading firms running internal book replicas via OMD-C
The exchange sits in the middle — it both accepts orders AND broadcasts state.

---

![Exchange Data Flow](/assets/Figure_2_Example_Exchange_Data_Flow.png)

Dashed arrows = event callbacks (engine fires, listener reacts)
Solid arrows = direct data flow
Multiple DepthOrderBook stacks = one book per instrument (your Exchange class)
Encoder → Decoder = the FAST protocol compression layer
Each Subscriber independently decodes and maintains its own replica

### The two sides of the feed (publisher / subscriber)
```
PUBLISHER side (exchange)              SUBSCRIBER side (trading firm)
─────────────────────────              ──────────────────────────────
Matching engine fires event            Receives encoded bytes over TCP
        ↓                                      ↓
Listener callback triggered            Decode message (QuickFAST/binary)
        ↓                                      ↓
Build message (symbol, qty,            Validate sequence number
price, depth levels changed)                   ↓
        ↓                              Apply event to internal book replica
Encode + compress (FAST protocol)              ↓
        ↓                              Internal book is now current
Send over TCP to all subscribers       Strategy reads from local memory
```

![feed_pipeline](/assets/feed_pipeline.svg)


---

### Feed message types

| Message type | When it fires | What it contains |
|---|---|---|
| **Trade** | Every time a match occurs | Symbol, quantity, cost (price × qty) |
| **Depth (incremental)** | When any price level changes | Only the changed levels (e.g. just level 1 bid) |
| **Depth (full snapshot)** | When subscriber first connects | All 5 levels, both sides |
| **BBO** | When best bid or best ask changes | Just the top of book |

**Why incremental?** Most depth changes happen at the top 1–2 levels.
Sending all 5 levels every time wastes bandwidth. Incremental sends
only what changed — typically 1 level, not 5.

**Why full snapshot on connect?** A new subscriber has no prior state.
They can't apply "level 1 changed to $30.02" if they don't know what
level 1 was before. Full snapshot bootstraps them, then incrementals
keep them in sync from there.

---

### Sequence numbers — the most important field

Every message carries a monotonically increasing sequence number.
The subscriber tracks the expected next sequence number and validates
every message against it.
```
Subscriber receives: seq 1, 2, 3, 4, 6  ← gap detected (5 missing)
Result: book state is corrupt — a cancel or add was lost
Action: request full snapshot to resync from scratch
```

A missing sequence number means a missed event.
A missed cancel = ghost order stays in your replica forever.
A missed add = order that exists in real book is invisible to you.
Either corrupts every strategy decision made from that point on.

This is why sequence numbers are non-negotiable. No sequence number
validation = no reliable replica.

---

### Feed encoding — why not just send JSON?

JSON is human readable but wasteful. A single depth update in JSON:
```json
{"symbol":"0700.HK","level":1,"side":"bid","price":30.02,"qty":50000}
```
That's ~65 bytes. A binary-encoded equivalent is 8–12 bytes.

At 1 million events per second across 14,000 instruments, the
difference is ~53MB/s vs ~8MB/s. At scale, encoding format directly
affects whether your feed is even physically deliverable.

**FAST protocol (used in the article)**
- Field-level delta compression — if a field hasn't changed from the
  last message, it's omitted entirely
- Integer encoding for prices (no floats, no strings)
- Used by CME, NASDAQ, and others historically

**Modern real exchange feeds**
- HKEX OMD-C uses its own binary protocol
- CME uses MDP 3.0 (SBE — Simple Binary Encoding)
- All the same principle: compact binary, integers only, no parsing overhead

---

### Depth levels — what "5-level depth" means

Exchanges don't broadcast every individual order in the book.
They aggregate by price level and broadcast the top N levels.
```
5-level depth feed (what subscribers see):
BID                         ASK
Level 1: $30.00  100,000    Level 1: $30.02   80,000
Level 2: $29.98   90,000    Level 2: $30.04   70,000
Level 3: $29.96   80,000    Level 3: $30.06  160,000
Level 4: $29.94   60,000    Level 4: $30.08   50,000
Level 5: $29.92  180,000    Level 5: $30.10   60,000

Each level shows: price, aggregate qty, number of orders
```

The firm's internal replica maintains this same structure.
When a depth message arrives saying "level 2 ask changed",
the firm applies that single update to their local copy.

---

### Incremental vs full snapshot — how depth updates are sent

![Snapshot vs Increment](/assets/snapshot_vs_incremental.svg)

### Buffer pools — why they matter for latency

Naive code: allocate a new buffer for every message → heap malloc → slow.

The article uses a buffer pool: pre-allocate a set of buffers at
startup, hand them out when needed, return them when done.
No heap allocation in the hot path. This is a standard low-latency
pattern you'll see everywhere in HFT systems.

Same principle applies to your order structs — pre-allocate a pool
of Order objects at startup rather than `new Order()` per incoming
order. (This is part of the intrusive list upgrade in Layer 2.)

---

### The listener/callback pattern

The matching engine doesn't "push" to the feed directly.
Instead it fires callbacks to registered listeners:
```cpp
// You implement this interface
class TradeListener {
    virtual void on_trade(book, qty, cost) = 0;
};

class DepthListener {
    virtual void on_depth_change(book, depth_tracker) = 0;
};

![Order Book Class Diagram](/assets/Figure_3_Order_Book_Class_Diagram.png)
book::OrderBook<T>       — matching engine only, no depth tracking
                           (this is what you are building in Layer 1)
book::DepthOrderBook<T>  — adds Depth<5> member, tracks top N levels
                           aggregated by price, fires DepthListener callbacks
book::Depth              — the actual aggregated depth structure
                           (5 price levels, each with price/qty/order count)
ExampleOrderBook         — adds symbol string, maps to one instrument
                           (this is your future Exchange class per instrument)
T = C++ template parameter — lets you plug in any Order type

// Engine calls these automatically after each match or book change
// You decide what to do: log it, send it, update a replica, etc.
```

This decouples the matching engine from whatever consumes its output.
The same engine can feed: a market data publisher, a risk system,
a strategy, a logger — all simultaneously, just register more listeners.

**In your build**: your `recordTrade()` call inside `matchBuy()` is
the embryonic version of this. In Layer 1 it just prints. Eventually
it becomes a proper callback that any consumer can subscribe to.

---

### What the Liquibook article is and isn't

**Is:** a complete worked example of the full pipeline —
matching engine → listener callbacks → feed encoding → TCP broadcast
→ subscriber decoding → internal book replica update.

**Isn't:** production-grade. Specific gaps:

| What it does | Production reality |
|---|---|
| Generates random orders internally | Orders arrive via FIX protocol from brokers |
| `double` price, converts to int for Liquibook | Integer throughout, never double |
| `sleep(1)` in order loop | Microsecond-level event processing |
| No sequence gap recovery | Must request full snapshot on gap |
| No colocation | Servers physically inside exchange data centre |
| Single-threaded callbacks | Lock-free multi-threaded pipelines |

---

### Where this fits in your build roadmap
```
Layer 1 (now):
  Your matching engine = what Liquibook already is in the article
  Your recordTrade()   = embryonic version of TradeListener

Layer 1 extension (after stress test passes):
  Add DepthTracker: maintain top-N aggregated levels after each match
  Add listener callbacks: on_trade(), on_depth_change()
  These are the hooks that everything else plugs into

Roadmap — feed publisher:
  Serialise listener callbacks into binary messages
  Add sequence numbers to every message
  Broadcast over TCP to subscribers (Boost ASIO or equivalent)

Roadmap — feed subscriber (firm side):
  Connect to feed, validate sequence numbers
  Bootstrap with full snapshot, then apply incrementals
  This IS the internal book replica — the thing trading firms build
```

---

### Key terms quick reference

| Term | Meaning |
|---|---|
| **BBO** | Best Bid and Offer — just the top level, nothing deeper |
| **Depth feed** | Top N aggregated price levels (N = 5 or 10 typically) |
| **Full order book feed** | Every individual order (rare, very high bandwidth) |
| **Incremental update** | Only the changed levels since last message |
| **Full snapshot** | All levels, sent fresh — used to bootstrap or resync |
| **Sequence number** | Monotonic counter per message — gap = corrupted replica |
| **FAST protocol** | Binary compression standard for market data feeds |
| **SBE** | Simple Binary Encoding — CME's modern feed format |
| **FIX protocol** | Standard for order submission (not market data) |
| **ChangeId** | Liquibook's internal counter tracking which levels changed |