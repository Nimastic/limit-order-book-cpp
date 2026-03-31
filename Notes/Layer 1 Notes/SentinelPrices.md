## Sentinel prices — how market orders work without special cases

A sentinel value is a special value you inject into a normal field to
signal an extreme or edge case — instead of adding a new field or type.

For market orders: no price limit, match whatever is available immediately.
But the matching loop compares prices:
```cpp
if (buy.price < best->first) break;  // stops when price doesn't cross
```

Set a buy market order's price to `INT_MAX` (largest possible integer)
and the condition `buy.price < best->first` can never be true — the loop
never breaks early. It sweeps everything available.

Same logic for sells — set price to `0` and `sell.price > best->first`
is never true. Sweeps everything.
```cpp
// market buy — will match any ask no matter how high
Order marketBuy  = {id, true,  INT_MAX, qty, now_ns()};

// market sell — will match any bid no matter how low
Order marketSell = {id, false, 0,       qty, now_ns()};
```

No new code in the matching loop. No special cases. The existing price
comparison handles it automatically. The sentinel reuses existing logic
instead of adding branching.

One guard needed: printBook() must not display a resting market order
as $0.00 or $21474836.47 if it partially fills and has remainder.