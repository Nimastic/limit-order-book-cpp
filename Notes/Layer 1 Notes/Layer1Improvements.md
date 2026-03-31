# Layer 1 Improvements Notes

## 1) Move semantics: `push(o)` vs `push(std::move(o))`

- A named variable like `o` is an **lvalue**.
- `push(o)` chooses the copy overload (`const T&`) when available.
- `push(std::move(o))` casts to rvalue (`T&&`), enabling move overload.
- `std::move` itself is mostly a cast; the real work is done by move ctor/assignment.
- For current `Order` (ints/bool/enums), move vs copy is nearly same cost.
- Still good style to use `move` when object is no longer needed (future-proof if `Order` gets heavy fields like `std::string`).

### Safety note after move
- Moved-from object is valid but unspecified for non-trivial members.
- Prefer:
  - save `id` before move
  - move object into container
  - use saved `id` for index mapping

---

## 2) “Copy path” vs “Move path”

- **Copy path**: calls copy constructor/assignment (`T(const T&)` / `operator=(const T&)`).
- **Move path**: calls move constructor/assignment (`T(T&&)` / `operator=(T&&)`).
- For heavy types (heap-backed), move is usually cheaper (pointer transfer) than copy (allocation + duplication).

---

## 3) `emplace_back` vs `push_back`

For `std::vector<T>`:

- `push_back(x)` takes one `T` object (`const T&` or `T&&`).
- `emplace_back(args...)` forwards constructor args and constructs `T` in place.

### With aggregate `Trade`
If `Trade` has no custom constructor (aggregate):

- `push_back({a,b,c,d})` works (creates a temporary `Trade` via list-init).
- `emplace_back(a,b,c,d)` may fail (no matching constructor).
- Safe forms:
  - `emplace_back(Trade{a,b,c,d})`
  - or `push_back({a,b,c,d})`

### Common compile error cause
- `trades.std::emplace_back(...)` is invalid syntax.
- Correct is `trades.emplace_back(...)`.

---

## 4) Aggregates (why it matters)

An aggregate (like plain struct of public fields, no user-defined ctor) supports brace initialization:
- `Trade t{buyId, sellId, price, qty};`

But aggregate does not imply constructor overloads for variadic emplacement:
- hence `emplace_back(a,b,c,d)` may not work unless ctor exists.

---

## 5) `queue<Order>` vs `deque<Order>`

### Current issue with `std::queue`
- `std::queue` is an adapter: no iterators.
- To inspect all orders at a price level, code often copies queue then pops through copy:
  - extra O(n) copy work per level.

### Why `std::deque<Order>` helps
- Keeps FIFO semantics (`push_back`, `pop_front`, `front`).
- Adds iterators for direct scanning without copies.
- AON checks and `printBook` can iterate in place:
  - avoids repeated full queue copies
  - biggest practical performance win among discussed topics.

### Mapping operations
- `queue.push(...)` -> `deque.push_back(...)`
- `queue.pop()` -> `deque.pop_front()`
- `queue.front()` -> `deque.front()`
- purge cancelled front becomes repeated `pop_front()` while front is cancelled.

---

## 6) Practical recommendations (Layer 1)

1. Keep `std::move(o)` when resting remainder, but avoid using moved-from `o` except saved primitive id.
2. For `Trade` insertion, prefer:
   - `trades.push_back({buyId, sellId, price, qty});`
   - or `trades.emplace_back(Trade{buyId, sellId, price, qty});`
3. If optimizing matching/inspection paths, migrate price levels from `queue<Order>` to `deque<Order>` to remove copy-and-pop scans.
4. Clarify ELO rule in docs/tests:
   - “10 ticks from touch” vs “10 book levels” are different behaviors.