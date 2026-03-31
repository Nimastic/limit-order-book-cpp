## Stress test — what we learned

### The bug the stress test found

Order #300 partially filled, stayed in the index as `PartiallyFilled`,
then got cancelled. At the next checkpoint, the invariant check found
a cancelled order still in the index and fired:

```
INVARIANT BROKEN: cancelled order still in index
```

The root cause: `cancelOrder()` was relying entirely on
`purgeCancelledFront()` to clean up the index. But purge only runs
when a cancelled order reaches the **front** of a queue. If the order
sits in the middle and a checkpoint fires first, the index still holds
a pointer to a cancelled order.

---

### The fix — immediate index cleanup on cancel

```cpp
// order status is open or partially filled
order->status = OrderStatus::Cancelled;
index.erase(it);        // revoke access immediately
// order stays in queue — purge handles physical removal lazily
```

The two cleanup steps are intentionally separate:

```
cancelOrder()          → remove from index (immediate)
                         "nobody can look this order up anymore"

purgeCancelledFront()  → remove from queue (lazy, when it reaches front)
                         "physically discard the order object"
```

The index is the access point. The queue is the storage.
Revoke access immediately. Clean up storage lazily.

---

### Mental model — access revocation, not destruction

This is NOT a destructor. The order object still exists in memory
inside the queue. You are removing it from the directory so nothing
can find it, not destroying it.

Real world analogy: when someone leaves a company, you deactivate
their keycard immediately (index erase) so they can't get in. Their
desk sits there for a few days until facilities cleans it out
(queue purge). Access gone immediately. Physical cleanup lazy.

---

### What the three invariants check and why

**Invariant 1 — no crossed book**
After every batch of orders, best bid must be strictly less than
best ask. If bid >= ask, a match should have happened but didn't.
A crossed book means your matching loop has a bug.

**Invariant 2 — equal fill quantities**
Total filled quantity on the buy side must equal total filled quantity
on the sell side across all trades. A phantom trade (fill created
from nothing) would break this. Catches recordTrade() bugs.

**Invariant 3 — no cancelled order in index**
Every order pointer in the index must not be cancelled. This is
the invariant the bug broke. Catches incomplete cleanup logic.

---

### Why stress testing matters

Small unit tests only cover cases you thought of.
A stress test with 10,000 random orders hits combinations you
never would have written manually:
- orders that partially fill then get cancelled
- cancels on orders that were already filled (CANCEL FAILED)
- market orders hitting empty books
- sweeps across many price levels simultaneously

The bug found here (cancelled order in index) would never have
appeared in any of the manual Step 6 tests because those tests
cancelled orders that were cleanly resting, not partially filled ones.

---

### `friend` keyword

Used to give `checkInvariants()` access to private members
(`bids`, `asks`, `index`) without making them public:

```cpp
// inside class declaration in order_book.h
friend void checkInvariants(const OrderBook&);
```

Targeted exception to the private rule — only this specific function
gets access. Right use case: test code that needs to inspect internal
state without exposing that state permanently through public getters.

---

### Checklist after any matching engine change

Before moving to the next feature, always verify:
- No crossed book at any point during 10,000 orders
- Total buy filled == total sell filled
- No cancelled order remains in the index
- Cancelled orders never appear in trade records
- CANCEL FAILED on already-filled and already-cancelled orders
- No crash on cancel of non-existent order id
```
