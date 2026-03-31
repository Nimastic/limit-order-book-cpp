# HKEX Trading Mechanism — Notes for Order Book Builders

> These notes are written assuming you are building a limit order book engine and want to understand how a real exchange actually uses one.

---

## 0. Key vocabulary (read this first)

| Term | Plain English |
|---|---|
| **Exchange** | The venue that runs the matching engine. HKEX is the exchange. Moomoo, Saxo, Tiger are *brokers* that connect to the exchange. |
| **Order book** | The live list of all outstanding buy and sell orders. Your job is to build this. |
| **Bid** | A buy order. "I want to buy at this price or cheaper." |
| **Ask / Offer** | A sell order. "I want to sell at this price or more." |
| **Best bid** | The highest price any buyer is currently willing to pay. |
| **Best ask** | The lowest price any seller is currently willing to accept. |
| **Spread** | The gap between best bid and best ask. If best bid = $30.00, best ask = $30.02, spread = $0.02. This is the cost of trading immediately. |
| **Spread (tick unit)** | HKEX also uses the word "spread" to mean one *tick* — the minimum price increment for a given stock. Context tells you which meaning applies. |
| **Tick / Tick size** | The smallest allowed price step. A stock priced around $30 might have a tick size of $0.02, so valid prices are $30.00, $30.02, $30.04... |
| **Price queue** | All orders resting at the *same price level*, sorted by arrival time (FIFO). |
| **Board lot** | The standard trading unit. Like buying eggs by the dozen. If board lot = 500 shares, you trade in multiples of 500. |
| **Automatch** | Orders matched automatically by the engine (as opposed to manually reported trades). Your order book implements automatch. |
| **Outstanding orders** | Orders in the book that haven't been filled or cancelled yet. |
| **Fill** | A completed trade. A full fill means the whole order quantity was matched. |
| **Partial fill** | Only some of the order quantity was matched. The rest stays in the book. |
| **IEP (Indicative Equilibrium Price)** | During auctions, the price at which the most volume can be traded. Think of it as the auction clearing price. |
| **Nominal price** | HKEX's internal reference price used for validation. It tracks the most recent meaningful price (last trade, or best bid/ask if they've moved beyond the last trade). |

---

## 1. The platform: OTP-C

**OTP-C** = Orion Trading Platform – Securities Market. This is HKEX's matching engine, introduced in 2018. It replaced a legacy system called AMS/3.

It is an **order-driven system**. This means:
- There are no designated market makers required to always post prices.
- Prices come entirely from orders submitted by participants.
- The engine matches buy and sell orders automatically when prices cross.

Compare this to a *quote-driven* system (like older FX markets) where dealers post firm bid/ask quotes and you trade against them.

**Your order book is implementing the order-driven model.**

The system currently handles 14,000+ stock counters (instruments) including stocks, ETFs, warrants, and bonds.

---

## 2. The three trading sessions

The trading day is split into three sessions, each with different rules about which order types are allowed.

```
Pre-Opening Session  →  Continuous Trading Session  →  Closing Auction Session
   (auction mode)           (continuous matching)           (auction mode)
```

### 2a. Pre-Opening Session (POS)

- Orders accumulate over a collection period.
- **No matching happens during collection** — orders just queue up.
- At the end, the system finds the IEP (the price that matches the most volume) and executes all matches at that single price simultaneously.
- This is a **batch auction**, not continuous matching.
- Only accepts: **at-auction orders** and **at-auction limit orders**.
- After POS ends, unfilled at-auction limit orders carry forward to continuous trading as regular limit orders (if they have a valid price).

### 2b. Continuous Trading Session (CTS)

- **This is what your order book implements.**
- Orders match immediately and continuously whenever prices cross.
- Price-time priority: best price wins, and among same-price orders, earliest arrival wins.
- Accepts: **limit**, **enhanced limit**, **special limit** orders.

## 📉 Closing Price Calculation

### For non-CAS stocks:
Takes **5 snapshots** of the nominal price in the last minute (every 15 seconds from 3:59:00 PM), then uses the **median** (middle value).

```
Snapshot 1 (3:59:00): $39.42
Snapshot 2 (3:59:15): $39.42
Snapshot 3 (3:59:30): $39.40
Snapshot 4 (3:59:45): $39.40
Snapshot 5 (4:00:00): $39.38

Sorted: $39.38, $39.40, $39.40, $39.42, $39.42
Median (3rd value) = $39.40 ← Closing Price
```

Using median of 5 prevents a single weird trade from distorting the close.

### For CAS stocks:
The final IEP at the end of the Closing Auction Session becomes the closing price.

### 2c. Closing Auction Session (CAS)

- Another batch auction, at end of day.
- Four sub-periods:
  1. **Reference Price fixing** — calculates a reference price (median of 5 snapshots in the last minute of CTS).
  2. **Order input** — participants enter orders within ±5% of reference price.
  3. **No-cancellation** — orders can still come in but must be between the current best bid and best ask. No changes or cancels.
  4. **Random closing** — market closes at a random moment within a 2-minute window, then all orders match at the final IEP.
- The final IEP becomes the day's **closing price**.

> **Why does closing price matter for your order book?** Many validations (price limits, the "9x rule") are calculated relative to the previous closing price. You'd need to store this externally.

---

## 3. Order types (the important ones for your engine)

### Limit Order (the baseline — build this first)

- Buy or sell at a **specific price or better**.
- If it can't match immediately, it rests in the book at its price.
- Constraint: a sell limit order cannot be entered *below* the current best bid. A buy limit order cannot be entered *above* the current best ask.

> **Why this constraint?** It's a sanity check. If you're trying to sell at $29.00 but there are buyers at $30.00, you'd match immediately — you wouldn't enter it as a resting ask at $29.00. The system rejects the input if the price is already crossable, to protect against input errors.

### Enhanced Limit Order (ELO)

- Like a limit order, but it can **sweep across up to 10 price levels** in one shot.
- Still bounded by the limit price — won't trade at worse than what you specified.
- Any unfilled remainder becomes a resting limit order at the specified price.
- Constraint: cannot be entered more than 10 tick sizes away from the current best bid/ask.

**Example from HKEX docs:**
- Best ask is $30.02. You submit ELO to buy 650,000 shares at $30.20.
- Engine sweeps from $30.02 up to $30.20, filling at each level.
- It trades: 80,000 @ 30.02 + 70,000 @ 30.04 + 160,000 @ 30.06 + ... = fully filled.
- If only 620,000 were available, the remaining 30,000 rest in the book as a limit order at $30.20.

### Special Limit Order (SLO)

- Same multi-level sweep as ELO (up to 10 levels).
- **No price constraint on input** — can be placed at any price as long as it immediately crosses the spread.
- Any unfilled remainder is **cancelled** (not rested in the book).
- This makes it behave like a market order with a price floor/ceiling.

### At-Auction Order

- No price specified. Matches at whatever the IEP turns out to be.
- Higher matching priority than at-auction limit orders.
- Auction sessions only. Cancelled if not filled by end of session.

### At-Auction Limit Order

- Has a price. Matches at the IEP only if the IEP is equal to or better than the specified price.
- Auction sessions only. Unfilled ones carry forward to CTS as limit orders (if price is valid).

### All-or-Nothing (AON) Qualifier

- Optional add-on for CTS orders.
- Order either fills in full **immediately** or is **rejected entirely** — never rests in the book.
- Useful when a partial fill is not acceptable.

---

## 4. Matching rules

### Price-time priority (the rule your engine must enforce)

1. **Price priority first**: The best bid (highest price) matches against the best ask (lowest price).
2. **Time priority second**: Among orders at the *same* price, the one that arrived earliest gets filled first.

The exact HKEX rule: *"An order entered into the system at an earlier time must be executed in full before an order at the same price entered at a later time is executed."*

### Auction matching (IEP calculation — different from continuous)

In auctions, matching is: **order type priority → price priority → time priority**.
- At-auction orders (no price) beat at-auction limit orders.
- Within the same type, better price wins, then earlier time.

---

## 5. Limits and constraints you need to know

### Board lots and order size
- **Board lot**: Standard unit of trading. Varies by stock (commonly 100, 500, or 1000 shares).
- Maximum order size per submission: **3,000 board lots** for automatch stocks.
- Maximum orders per price queue: **40,000 orders**.
- There is **no cap** on outstanding orders per broker ID (this limit was removed).

> **For your engine**: You don't need to implement board lot logic initially. Just use `quantity` as an integer. But in a real system, you'd validate that quantity is a multiple of the board lot size.

### The 9x rule (price validation)
- An order price cannot deviate **9 times or more** from the nominal price.
- E.g. if nominal price is $1.00, the order price cannot be below $0.111 (1.00 / 9 = 0.111) or above $9.00 (1.00 × 9 = 9.00).
- This exists to catch fat-finger errors (accidentally entering $100 instead of $10).

### Spread table (tick sizes)
- HKEX publishes a table of minimum price increments (tick sizes) by price range.
- Around $30, the tick size is $0.02 — so valid prices are $30.00, $30.02, $30.04 etc.
- Prices that are not multiples of the tick size are rejected.

> **For your engine**: Implement integer prices (price × 100) and also eventually enforce that prices are multiples of the tick size.

### Volatility Control Mechanism (VCM)
- If a potential trade price deviates more than a threshold from the price 5 minutes ago, trading is paused for 5 minutes.
- Thresholds vary by stock size (large cap = ±5%, smaller stocks up to ±50%).
- Think of it as a circuit breaker that prevents flash crashes.

> **For your engine**: Not needed for a basic implementation. Add this as a bonus challenge after the core matching loop works.

---

## 6. Trade types (what your engine produces)

When your matching engine executes a match, it produces a **trade record**. HKEX classifies these:

| Trade type | What it is |
|---|---|
| **Automatch trade** | Your engine produces this — two orders matched automatically. The core output. |
| **Auction matching trade** | Same automatic matching, but during a pre-opening or closing auction session. |
| **Manual trade** | Reported by a broker, not matched by the engine. Not your concern. |
| **Odd lot trade** | Quantity less than one board lot. Special handling. |
| **Special lot trade** | Quantity larger than one board lot but not a multiple of it. Special handling. |

---

## 7. The infrastructure layer (context only)

OTP-C has two main interfaces:

- **OCG-C** (Orion Central Gateway) — brokers connect here to submit orders and receive confirmations. This is the order ingress/egress layer.
- **OMD-C** (Orion Market Data Platform) — disseminates market data (the live book state, trade prints) to vendors and participants.

For your project: OCG-C = the "add order" API. OMD-C = the "print book state" output. Your order book sits between them.

---

## 8. What your basic engine needs to implement (summary)

From everything above, here's what matters for a basic limit order book engine:

| Feature | Priority | Notes |
|---|---|---|
| Limit order add | Essential | Core of everything |
| Price-time priority matching | Essential | Best price first, then FIFO within price |
| Partial fills | Essential | Order remains in book with reduced quantity |
| Cancel order | Essential | O(1) cancel requires an index |
| Integer prices | Essential | Never use floats. price × 100 |
| ELO / SLO | Nice to have | Multi-level sweep, same matching loop |
| Auction / IEP | Advanced | Separate algorithm, different from continuous |
| Board lot validation | Nice to have | Validate qty is multiple of lot size |
| Tick size enforcement | Nice to have | Validate price is on a valid grid |
| VCM circuit breaker | Advanced | After core is solid |
| 9x rule | Nice to have | Price sanity check on input |
---


## 🔄 Trade Types Quick Reference

| Type | Code | Description |
|---|---|---|
| Automatch (non-direct) | `" "` (space) | Two brokers, matched by system |
| Automatch (direct) | `Y` | Same broker on both sides, matched by system |
| Manual Trade | `M` / `X` | Not matched by system, reported manually |
| Odd Lot Trade | `D` | Less than one board lot |
| Auction Matching | `U` | Matched during POS or CAS auction |
| Pre-opening Trade | `P` | Trade from before the morning session |

**Non-direct** = two different Exchange Participants (one buyer, one seller)  
**Direct** = same Exchange Participant acts for both buyer and seller

---

## ❓ Your Questions Answered

**Q: What's the "central order book"?**  
It's a single shared data structure (essentially a sorted list of bids and asks) that the exchange maintains. When you submit an order, it may get written into this book to wait for a matching order. AON orders are never written here — they either fill immediately or vanish.

**Q: What's the max 40,000 orders per price queue?**  
Each price level in the order book can hold up to 40,000 pending orders. So if 40,000 people all have limit orders at $30.00, the 40,001st is rejected. In practice this limit is rarely hit.

**Q: What's a "broker ID"?**  
Each Exchange Participant (broker) gets an ID. Previously HKEX limited outstanding orders per broker ID, but that cap has been removed — a broker can have unlimited pending orders in the system.

**"What's automatch?"**
Orders matched automatically by the engine, as opposed to manually reported trades. When you build a matching engine, every match it produces is an automatch.

**"What's a board lot?"**
The minimum trading unit for a stock. Like how you can't buy half an egg carton. If board lot = 500 shares, you trade 500, 1000, 1500... not 300 or 750.

**"Maximum number of outstanding orders per broker ID has been removed — what does that mean?"**
There used to be a cap on how many open (unfilled, uncancelled) orders a single broker could have at once. HKEX removed that cap. From your engine's perspective: no limit on how many orders a single participant can have resting in the book simultaneously.

**"All-or-Nothing — is it like a temporary queue?"**
No. It's simpler: the order arrives, the engine tries to fill it completely right now, and if it can't, the order is thrown away entirely. It never touches the book. No queue involved.

**"What are the spreads in the Enhanced Limit Order examples?"**
The word "spread" in those examples means *tick size* — the minimum price increment. When HKEX says "up to 10 price queues at 9 spreads away", they mean: starting from the best price, the ELO can reach up to 9 ticks further. At $30.02 with tick $0.02: 9 ticks away = $30.02 + (9 × $0.02) = $30.20. That's why the ELO example uses a buy at $30.20 to sweep from $30.02 to $30.20.