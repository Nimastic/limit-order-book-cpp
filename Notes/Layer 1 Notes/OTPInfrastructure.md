## OTP-C infrastructure — what matters and what doesn't

### What it is
OTP-C (Orion Trading Platform) is HKEX's matching engine, introduced
2018 to replace the legacy AMS/3 system. Built on open-systems
technology, designed to be scalable and modular.

### Two interfaces — worth knowing the names
- **OCG-C** (Orion Central Gateway) — the ORDER submission interface.
  Brokers connect here to send orders and receive confirmations.
  Uses a throttle mechanism — firms can buy higher throughput capacity.
  Supports binary and FIX protocol variants.

- **OMD-C** (Orion Market Data Platform) — the MARKET DATA interface.
  Streams book events to participants and vendors.
  This is what trading firms subscribe to for their internal book replica.
  OCG-C does NOT carry market data — they are completely separate pipes.

### Four markets on one platform
OTP-C runs four markets simultaneously, each with its own schedule:
main board, GEM, NASDAQ-listed securities, and ETS
(Extended Trading Securities). One engine, multiple books.

### Three matching methods supported
- Continuous price-time priority matching (your order book = this)
- Single-price auction (POS and CAS)
- Semi-automatic matching for odd/special lots

### Disaster recovery
Warm backup system. If primary fails, backup activates after
reconfiguration and recovery. Not instant failover — this is relevant
context for why the distributed systems concepts (state machine
replication, consensus) exist in production-grade systems.

### What you don't need yet
- FIX protocol specs
- Binary protocol details
- Throttle purchasing
- Drop-copy session setup
- Broker Supplied System (BSS) vendor list
```

---
