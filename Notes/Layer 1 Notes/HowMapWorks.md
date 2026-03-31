## Maps — underlying data structures

### Is a map always a tree across all languages?

No — it depends on the language and what they chose to name things.
This is one of the most confusing naming inconsistencies in programming:

| Language | Name | Underlying structure | Ordered? |
|---|---|---|---|
| C++ | `std::map` | Red-black tree | Yes |
| C++ | `std::unordered_map` | Hash table | No |
| Java | `TreeMap` | Red-black tree | Yes |
| Java | `HashMap` | Hash table | No |
| Python | `dict` | Hash table | Yes (insertion order, Python 3.7+) |
| Go | `map` | Hash table | No |
| JavaScript | `Map` | Hash table | Yes (insertion order) |

In C++, `std::map` = tree, `std::unordered_map` = hash table.
In Python, `dict` = hash table but they call it a dict.
The word "map" just means "key maps to value" — the underlying data
structure varies by language and even by implementation.

For the order book, `std::map` (the tree) is needed specifically because
prices must be sorted. A hash table gives O(1) lookup but no ordering —
you couldn't find "best bid" (highest price) efficiently because prices
would be scattered randomly.

---

### Why is `std::map` O(log n) for both insert and lookup?

Both are O(log n) because of the tree structure.

A binary search tree stores values so that for any node:
everything to the left is smaller, everything to the right is larger.
```
Finding price 104 in this tree:

         103
        /   \
      101    105
     /   \   /  \
   100  102 104  106

Start at 103. Is 104 > 103? Go right.
At 105. Is 104 < 105? Go left.
At 104. Found it. → 3 steps for 7 nodes = log₂(7) ≈ 3
```

Each step eliminates half the remaining nodes — same logic as binary
search. For 1 million price levels, any price is found in at most
20 steps (log₂(1,000,000) ≈ 20).

The "red-black" part is a set of rules that keep the tree balanced.
Without balancing, inserting prices in order (100, 101, 102, 103...)
causes a naive BST to degenerate into a linked list and lookup becomes
O(n). Red-black trees rebalance on every insert/delete to guarantee
the tree stays roughly equal depth on both sides, preserving O(log n)
worst case always.