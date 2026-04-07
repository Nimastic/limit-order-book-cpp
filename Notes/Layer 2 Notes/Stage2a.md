## Stage 2a Plan

The change is surgical — two things:

**1. `deque` → `std::list` in the price levels**

`list` gives you stable iterators. Once an `Order` is in the list, its iterator never invalidates on insert/erase elsewhere in the list. That's the property `deque` doesn't have.

**2. `index` stores `list::iterator` instead of `Order*`**

```cpp
// Before
std::unordered_map<int, Order*> index;

// After
using OrderIter = std::list<Order>::iterator;
std::unordered_map<int, OrderIter> index;
```

Now `cancelOrder` becomes true O(1) — look up iterator, call `list::erase(it)`, done. No lazy purge needed at all, which means you can delete `purgeCancelledFront` entirely.

**3. `matchBuy`/`matchSell` use `list::front()` + `list::pop_front()`**

Same interface as deque, just different type.

---

The tradeoff worth knowing before you start:

| | `deque` (current) | `list` (Stage 2a) |
|---|---|---|
| Cancel | O(n) lazy purge | O(1) true erase |
| Memory | Contiguous chunks | Pointer per node, heap scattered |
| Cache | Better | Worse (pointer chasing) |
| Iterator stability | No | Yes |

So you're trading cache friendliness for O(1) cancel. Stage 2b (intrusive list) then recovers the cache performance. That's why the ordering 2a→2b makes sense as a pair.
