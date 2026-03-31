## CPU cache and memory — why cache-friendly code matters

### How memory actually works

The CPU runs at ~3-4 GHz. RAM runs much slower. If the CPU had to
wait for RAM every time it needed data, it would spend most of its
time idle. So CPUs have a cache — small, extremely fast memory built
directly into the chip that stores recently accessed data.
```
CPU registers   ~0.3 ns    ~32 bytes     (what the CPU is working on right now)
L1 cache        ~1 ns      ~32 KB        (recently used data, on-chip)
L2 cache        ~5 ns      ~256 KB       (slightly older data, on-chip)
L3 cache        ~20 ns     ~8 MB         (shared between cores, on-chip)
RAM             ~100 ns    ~16-64 GB     (everything else, off-chip)
SSD             ~100 μs    ~1 TB         (persistent storage)
```

The CPU automatically loads data from RAM into cache in chunks called
**cache lines** — typically 64 bytes at a time. When you access one
byte, the surrounding 63 bytes come along for free, because the CPU
assumes you'll need nearby data soon (spatial locality).

---

### Why `std::map` causes cache misses
```
Each tree node is allocated separately on the heap.
Heap allocations go wherever there's free memory.
Three adjacent nodes in the tree might live at:

  node A: address 0x7f3a2b10
  node B: address 0x2c819f40   ← completely different region of RAM
  node C: address 0x5e127c88   ← yet another region

When you walk from A to B to C:
  - Access A → cache miss → CPU stalls ~100ns waiting for RAM
  - Access B → cache miss → CPU stalls ~100ns again
  - Access C → cache miss → CPU stalls ~100ns again
```

The data doesn't fail to retrieve — it always eventually arrives.
The CPU just stalls and waits. It's not re-chasing, it's waiting.
Those ~100ns stalls per node are the bottleneck.

---

### Why contiguous memory is fast
```
Orders stored contiguously in memory (array / memory pool):
  [Order0][Order1][Order2][Order3][Order4]...

Access Order0 → cache miss → CPU loads Order0 through Order7 in one shot
Access Order1 → cache HIT → already loaded, costs ~1ns
Access Order2 → cache HIT → already loaded, costs ~1ns
Access Order3 → cache HIT → already loaded, costs ~1ns
```

One cache miss loads many useful things at once.
This is why arrays beat linked lists and trees in practice even when
the big-O complexity is the same — the constant factor on cache-friendly
code is ~100x better.

At 10 million orders/second, spending 100ns per cache miss means
burning 1 second of stall time per second of processing — throughput
collapses. This is why the intrusive list upgrade in Layer 2 matters:
orders live in a pre-allocated contiguous pool, so walking through
them is cache-friendly.

**For Layer 1 this doesn't matter yet.** `std::map` is completely fine.
The bottleneck right now is correctness, not nanoseconds.
The upgrade path exists for when it does matter.