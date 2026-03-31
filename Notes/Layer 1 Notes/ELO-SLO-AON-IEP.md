## ELO, SLO, AON

These are all variations of the same question: **what do we do with the remainder after matching?**

Your current limit order always rests the remainder in the book. These three types change that behaviour.

---

**Regular limit order (what you have now)**
```
Buy 300 @ $104, book has 100 available @ $103
→ fills 100 @ $103
→ remaining 200 rests in book @ $104
→ waits for more sellers
```

---

**ELO — Enhanced Limit Order**

Same as limit order but it can sweep across multiple price levels in one shot, up to 10 levels away from the best price. Remainder still rests.

```
Buy ELO 300 @ $106, book has:
  100 @ $103
  100 @ $104
  100 @ $105

→ sweeps all three levels: fills 100+100+100 = 300
→ nothing left, fully filled

Buy ELO 400 @ $106, same book:
→ sweeps all three: fills 300
→ remaining 100 rests in book @ $106
```

The key constraint: the sweep can only go up to 10 price levels (10 tick sizes) beyond the best price. If you specify a price 11+ levels away, the order is rejected.

Remainder behaviour: **rests in book** at your specified price, same as a regular limit order.

Your current matching loop already sweeps multiple levels — `matchBuy()` keeps looping until quantity runs out or price doesn't cross. So ELO is almost free. The only thing you need to add is the 10-level input validation.

---

**SLO — Special Limit Order**

Same multi-level sweep as ELO, but remainder is **cancelled**, never rests.

```
Buy SLO 400 @ $106, book has 300 available across 3 levels:
→ sweeps all three: fills 300
→ remaining 100 is CANCELLED, does not rest in book
```

Also has no restriction on input price (unlike ELO which rejects if price is 10+ levels away) — as long as the order immediately crosses the spread.

Remainder behaviour: **cancelled immediately**.

In code: run the same matching loop, but after matching, if `o.quantity > 0` don't push to the book — just discard.

---

**AON — All or Nothing**

Completely different. Don't match at all unless you can fill the entire quantity right now.

```
Buy AON 300 @ $104, book has 200 available:
→ can't fill 300 fully → entire order rejected, nothing happens

Buy AON 300 @ $104, book has 400 available:
→ fills 300 fully → done
```

AON never rests in the book. It either fills completely right now or is thrown away entirely.

In code: before running the matching loop, check if enough quantity exists at crossable prices. If yes, match. If no, discard without touching the book.

---

**Side by side**

```
Order type    Sweeps levels    Remainder
──────────────────────────────────────────────────
Limit         1 level only     rests in book
ELO           up to 10         rests in book
SLO           up to 10         cancelled
AON           all or nothing   never rests, rejected if can't fill fully
Market        unlimited        rests (sentinel price) — you built this
```

---

## Auction mode and IEP

Continuous trading (what you built) matches orders immediately as they arrive. Auction mode is completely different — it's a two-phase process.

**Phase 1 — collection**
Orders come in but nothing matches. Everyone is just submitting their interest. No trades fire.

**Phase 2 — uncrossing**
At a fixed point in time, the exchange finds a single price that maximises the volume traded, then executes all matches at that one price simultaneously. This is the IEP.

---

**What is the IEP (Indicative Equilibrium Price)?**

The price at which the maximum number of shares can be traded if the auction ended right now.

Here's how to find it. Build cumulative volume curves:

```
Orders in the book during auction:

BID side (buyers)          ASK side (sellers)
Price  Qty                 Price  Qty
$103   200                 $101   150
$102   300                 $102   200
$101   100                 $103   250
$100   400                 $104   100

Cumulative bids (buyers willing to pay AT LEAST this price):
$103 → 200          (only buyers at $103+)
$102 → 500          (buyers at $102 and $103)
$101 → 600          (buyers at $101, $102, $103)
$100 → 1000         (all buyers)

Cumulative asks (sellers willing to sell AT MOST this price):
$101 → 150          (only sellers at $101 or below)
$102 → 350          (sellers at $101 and $102)
$103 → 600          (sellers at $101, $102, $103)
$104 → 700          (all sellers)

Matchable volume at each price = min(cumBid, cumAsk):
$101 → min(600, 150) = 150
$102 → min(500, 350) = 350   ← highest
$103 → min(200, 600) = 200
$104 → min(0,   700) = 0

IEP = $102 — maximises matched volume at 350 shares
```

All orders that can execute at $102 do so simultaneously. Buyers who bid $102 or higher get filled. Sellers who asked $102 or lower get filled. Everyone trades at the same price — $102.

This is fundamentally different from continuous matching where each match happens at the resting order's price. In an auction, everyone gets the same clearing price regardless of what they individually specified.

---

**Why auctions exist**

Three reasons exchanges use opening and closing auctions:

Price discovery — at the open, there's been overnight news. Many participants have orders queued up. Running an auction lets all that information aggregate into one fair price rather than having the first order of the day set the tone for everyone.

Reduces speed advantage — in continuous trading, being 1 microsecond faster than everyone else gives you an edge. In an auction, it doesn't matter if your order arrives at 9:00:00.000001 or 9:00:00.999999 — both participate equally in the auction.

Closing price integrity — the closing price is used for fund valuations, derivatives settlement, index calculations. A single manipulated trade in continuous trading could distort the close. The auction uses aggregate supply and demand so one participant can't easily move the price.

---

**In code — auction is a separate algorithm**

```
Continuous matching:
  order arrives → matchBuy/matchSell → rest remainder
  real-time, event-driven

Auction:
  phase 1: collect orders into separate auction book, no matching
  phase 2: build cumulative bid/ask curves
           find price maximising min(cumBid, cumAsk)
           execute all matches at that price
  batch, time-driven
```

You do not modify your existing `matchBuy()`/`matchSell()`. Auction mode is a completely separate class or function that operates on its own order collection and runs the IEP algorithm at the end.

```
Project: Exchange Matching Engine
├── Continuous matching (LOB)     ← done, Layer 1 complete
│     ├── Limit orders            ← done
│     ├── Market orders           ← done
│     ├── ELO                     ← building next
│     ├── SLO                     ← building next
│     └── AON                     ← building next
│
└── Auction engine                ← after ELO/SLO/AON
      ├── Order collection phase
      ├── IEP calculation
      └── Batch uncrossing
```