# Exchanges vs Trading Firms

## Visual Relationship

![Relationship](/assets/trading_firm_architecture.svg)

## The Two Order Books — Exchange vs Firm

### The key insight
There are two completely separate order books in any professional trading setup.
They are NOT the same thing.

| | Exchange book | Firm's internal book |
|---|---|---|
| Who owns it | HKEX (OTP-C) | The trading firm |
| Where it lives | Exchange servers | Firm's own servers |
| Who can modify it | Only the exchange engine | The firm's own code |
| How fast you can read it | Network round-trip (slow) | Local memory (nanoseconds) |
| What it contains | Ground truth | Real-time replica |

---

### Why firms build their own book replica

You can't query the exchange book at will. There's no API where you call
"give me best bid right now" a million times per second — the exchange
won't allow that, and even if it did, each call would cost a network round-trip.

Instead, firms subscribe to the **market data feed** (OMD-C at HKEX).
This is a raw stream of every single book event:
- Order added at price X, qty Y
- Order cancelled (order ID Z)
- Trade occurred: N shares @ price P

The firm receives these events and applies each one to their own
in-memory data structure — essentially replaying the exchange's book
locally. After each event, their internal book is a perfect clone
of the exchange's book as of that moment.

This internal book can be queried in nanoseconds with no network cost.
When the strategy wants to know mid-price, depth at each level, or
whether a large order just appeared — it reads from local memory,
not from the exchange.

**This internal book is what all the data structure obsession is about.**
Intrusive linked lists, bit packing, cache-efficient layouts — all of it
is in service of processing each incoming book event as fast as possible
so the firm's replica stays current with minimum delay.

---

### Where latency actually lives
![Latency Pipeline](/assets/latency_pipeline.svg)

The two network hops are minimised by **colocation** — physically placing
the firm's servers inside the same data centre as the exchange. You can't
eliminate the speed-of-light delay, but you can make the wire as short
as possible.

The middle steps are pure software. That's the engineering battleground.

---

### Colocation

Exchanges sell rack space inside their data centres to trading firms.
Your servers sit metres from the matching engine rather than kilometres away.
The difference: microseconds vs milliseconds of network latency.
At 10 million trades per day, microseconds compound into real edge.

---

### What firms actually control vs not

**Cannot control:**
- The exchange's matching logic
- The exchange's book data structure
- Other participants' order flow
- Network physics (speed of light)

**Can control:**
- How fast they process the incoming market data feed
- How efficiently their internal book is stored and queried
- How fast their strategy reaches a decision
- How fast their order submission code sends bytes over the wire
- Physical server placement (colocation)
- Network card and kernel tuning