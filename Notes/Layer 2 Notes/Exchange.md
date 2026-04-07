## This is a limit order book for one "stock", how do we implement it for multiple stock? 

Using a map

```cpp
// One extra layer of indirection
std::unordered_map<std::string, OrderBook> exchange;

exchange["AAPL"].addOrder(...);
exchange["TSLA"].addOrder(...);
```

Each `OrderBook` is self-contained — its own bids, asks, index, trades. You get isolation for free. The real complexity comes from:

- **Order IDs** — must be globally unique across all instruments, not per-book
- **Cross-instrument orders** — spread trades, pairs trading (out of scope for now)
- **Throughput** — one thread per instrument is the classic HFT approach so books don't contend

## The OrderBook itself is standalone. Distributed systems enter the moment you need:
- Within one machine (what you're building)
```
Exchange (your unordered_map wrapper)
│
├── OrderBook "AAPL"   ← has its own matchBuy/matchSell inside thread 1
├── OrderBook "TSLA"   ← has its own matchBuy/matchSell inside thread 2
└── OrderBook "MSFT"   ← has its own matchBuy/matchSell inside thread 3

```
![Exchange](exchange.png)


Your matchBuy / matchSell functions are the matching engine. They live inside OrderBook. There is no separate matching engine class.

Your code             Industry term
──────────────────────────────────
OrderBook             = Matching Engine
matchBuy/matchSell    = the matching logic inside it
Exchange class        = the process/server hosting all books

- Across Machines (Distributed Systems)
```
Client → Gateway Server → [Order Router] → Matching Engine (AAPL)
                                         → Matching Engine (TSLA)
                        → Risk Engine
                        → Market Data Feed
```
![system architecture diagram](sad.png)


The dashed arrows represent async paths — risk checks and trade publication happen off the hot path so they don't add latency to the match. That's a real production pattern.

The "active + standby" label on each matching engine is the NASDAQ failover model you mentioned — two instances, one hot, one mirroring state, with Raft/Paxos managing leader election if the active one dies.

Now you have network partitions, clock skew, consensus problems. 
This is where things like Kafka (trade event log), FIX protocol (order entry), and RAFT/Paxos (leader election for the matching engine) live. 
A real exchange like NASDAQ runs matching engines in active-passive pairs in different datacenters for failover.

Key Insight: The OrderBook is the innermost hot loop. Everything around it is infrastructure to feed it orders and distribute its results.