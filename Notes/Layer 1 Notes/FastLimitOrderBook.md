# How to Build a Fast Limit Order Book

## Context & Scale
- Target: the Nasdaq TotalView ITCH feed — every event on every Nasdaq instrument
- Data rates: **20+ GB/day**, spikes of **3 MB/sec**
- Average message size: ~20 bytes → **100,000–200,000 messages/sec** at peak

---

## Three Core Operations
| Operation | Description |
|-----------|-------------|
| **Add** | Places an order at the end of the list at a given limit price |
| **Cancel** | Removes an order from anywhere in the book |
| **Execute** | Removes an order from the *inside* of the book |

> **Note:** Adds and cancels dominate activity (market makers jockeying for position). Executions are a distant third.

---

## Key Data Structures

### Order
```
Order
  int idNumber
  bool buyOrSell
  int shares
  int limit
  int entryTime
  int eventTime
  Order *nextOrder
  Order *prevOrder
  Limit *parentLimit
```

### Limit *(represents a single price level)*
```
Limit
  int limitPrice
  int size
  int totalVolume
  Limit *parent
  Limit *leftChild
  Limit *rightChild
  Order *headOrder
  Order *tailOrder
```

### Book
```
Book
  Limit *buyTree
  Limit *sellTree
  Limit *lowestSell
  Limit *highestBuy
```

---

## Core Design Idea
- **Binary tree** of `Limit` objects sorted by `limitPrice`
- Each `Limit` is a **doubly linked list** of `Order` objects
- Buy and sell limits live in **separate trees**
- Each `Order` is also stored in a **hash map** keyed by `idNumber`
- Each `Limit` is also stored in a **hash map** keyed by `limitPrice`

---

## Performance Targets
| Operation | Complexity |
|-----------|------------|
| Add (first order at a limit) | O(log M) |
| Add (subsequent orders) | O(1) |
| Cancel | O(1) |
| Execute | O(1) |
| GetVolumeAtLimit | O(1) |
| GetBestBid/Offer | O(1) |

*M = number of price limits (generally much less than N, the number of orders)*

---

## Implementation Variants

### 1. Binary Tree (default)
- Add: O(log M) first order, O(1) after
- Delete/Execute inside limit: O(1) via `Limit *parent` pointer
- **Keep tree balanced** — markets naturally add to one side while removing from the other

### 2. Sparse Array (no linked list)
- Add: O(1) always
- Delete/Execute at inside limit: O(M) — must scan to update `lowestSell`/`highestBuy`
- Best for **dense books** where you get much better than O(M) in practice

### 3. Sparse Array + Linked List
- Add: O(log M)
- Delete/Execute: O(1)
- Good middle ground

> **Rule of thumb:** Best choice depends on *book sparsity* — the average gap in cents between limits that have volume (positively correlated with instrument price).

---

## Additional Tips
- Use **batch allocations** or **object pools** (especially in Java) to avoid GC pauses
- Java can be fast enough for HFT *as long as the garbage collector isn't allowed to run*
- `Book.lowestSell` / `Book.highestBuy` must be updatable in **O(1)** to keep `GetBestBid/Offer` fast — this is why each `Limit` holds a `*parent` pointer
- Multi-threaded access strategies are a separate topic