# CME Market Data Platform (MDP) 3.0 Services & Features — Quick Reference Notes

> Covers: Futures & Options, BrokerTec, BrokerTec Chicago, and EBS Market on CME Globex.
> For latest updates, check the **Pending Client System Impacts** site.

---

## 1. Futures & Options on CME Globex

### Market Data Services

| Service | Key Attributes |
|---|---|
| **CME MDP UDP** | Lowest-latency real-time; MBOFD + 10-deep MBP; UDP Multicast; SBE |
| **CME Smart Stream GCP SBE — Real-time** | Real-time; MBOFD + 10-deep MBP; Google Cloud Pub/Sub; SBE |
| **CME Smart Stream GCP SBE — Delayed** | 10-minute delay; MBOFD + 10-deep MBP; Google Cloud Pub/Sub; SBE |
| **CME Smart Stream GCP JSON** | Real-time; Top-of-book + 2-deep MBP; OI/Settlement/Volume stats; JSON |

**Statistics available on all SBE feeds:** Opening Price, Session High/Low, Session Stats Reset, Electronic Volume, High Bid/Low Ask, Cleared Volume, Fixing Price, Open Interest, Settlement Price, Time & Sales.

---

### Key Feature Support Matrix

| Feature | MDP UDP | GCP SBE RT | GCP SBE Delayed | GCP JSON |
|---|:---:|:---:|:---:|:---:|
| SBE Encoding | ✅ | ✅ | ✅ | ❌ |
| Event-Based Messaging | ✅ | ✅ | ✅ | ❌ |
| Market by Price (MBP) | ✅ | ✅ | ✅ | ✅ |
| Market by Order Full Depth (MBOFD) | ✅ | ✅ | ✅ | ❌ |
| Implied Matching | ✅ | ✅ | ✅ | ❌ |
| Trade Summary | ✅ | ✅ | ✅ | ✅ |
| Security Status | ✅ | ✅ | ✅ | ✅ |
| Conflation Processing | ❌ | ❌ | ❌ | ❌ |
| Trade/Value Date Processing | ❌ | ❌ | ❌ | ❌ |
| Trade Embargo (OFF-SEF NDF) | ❌ | ❌ | ❌ | ❌ |

> **Note:** Futures & Options do **not** support Trade/Value Dates or Trade Embargos.

---

### Statistics Support
- ✅ Opening Price, Indicative Opening, Session High/Low, Session Stats Reset, Electronic Volume, High Bid/Low Ask, Fixing Price
- ✅ Cleared Volume, Open Interest, Settlement Price — available on GCP JSON & WebSocket too
- ❌ Volume Weighted Average Price — not supported

### Recovery
- **UDP Recovery:** Available on MDP UDP and GCP SBE feeds
- **Smart Stream Recovery:** Available on GCP SBE (RT & Delayed) and GCP JSON
- **WebSocket Recovery:** Available via CME Market Data Over WebSocket API

---

## 2. BrokerTec on CME Globex

### Supported Markets by Service

| Service | US Actives | EGBs | EU Repo | US Repo |
|---|:---:|:---:|:---:|:---:|
| **CME MDP Premium** (real-time, MBOFD + 10-deep MBP, UDP) | ✅ | ✅ | ❌ | ❌ |
| **CME MDP Conflated UDP** (50ms, MBOLD top-10 + 10-deep MBP) | ✅ | ✅ | ❌ | ❌ |
| **CME MDP Conflated TCP** (50ms, MBOLD + 10-deep MBP, bilateral Repo support) | ✅ | ✅ | ✅ | ✅ |

---

### Key Feature Support Matrix

| Feature | MDP Premium | Conflated UDP | Conflated TCP |
|---|:---:|:---:|:---:|
| SBE Encoding | ✅ | ✅ | ✅ |
| Event-Based Messaging | ✅ | ✅ | ✅ |
| Conflation Processing | ❌ | ✅ | ✅ |
| Conflated TCP Group Processing | ❌ | ❌ | ✅ |
| MBOFD | ✅ | ❌ | ❌ |
| MBOLD (top 10 orders) | ❌ | ✅ | ✅ |
| Inverted Price Book | ✅ | ✅ | ✅ |
| Implied Matching | ✅ (ch.490) | ✅ (ch.500) | ✅ (ch.510) |
| Mid-Session Order Qty Update | ❌ | ❌ | ✅ (US & EU Repo) |
| All-Or-None | ❌ | ❌ | ✅ (Repo only) |
| Bilateral Trading | ❌ | ❌ | ✅ |
| Cloud (GCP) Processing | ❌ | ❌ | ❌ |

> **Implied Matching** is limited to U.S. Active channels (490, 500, 510).

---

### Statistics Support

| Stat | Premium | Conflated UDP | Conflated TCP |
|---|:---:|:---:|:---:|
| Opening Price | ✅ | ✅ | ✅ |
| Indicative Opening | ✅ (ch.490) | ✅ (ch.500) | ✅ (ch.510) |
| Session High/Low | ✅ | ✅ | ✅ |
| Electronic Volume | ✅ | ✅ | ✅ |
| High Bid/Low Ask | ❌ | ❌ | ❌ |
| Cleared Volume / OI / Settlement / Fixing | ❌ | ❌ | ❌ |
| VWAP | ❌ | ❌ | ✅ (Repo only) |

> **Tie-breaker rule:** Indicative Opening uses *Last Trade Price from prior session* (not settlement price).

### Recovery
- **UDP Recovery:** Premium (MBOFD only) + Conflated UDP (MBOLD only)
- **TCP Recovery:** Conflated TCP only
- Smart Stream & WebSocket Recovery: ❌ Not supported

---

## 3. BrokerTec Chicago on CME Globex

### Supported Markets
- **US Actives only** — EGBs, EU Repo, and US Repo are **not** supported.

### Services
| Service | Description |
|---|---|
| **CME MDP Premium UDP** | Real-time, MBOFD + 10-deep MBP, UDP |
| **CME MDP Conflated UDP** | 50ms, MBOLD top-10 + 10-deep MBP, UDP |
| **CME MDP Conflated TCP** | 50ms, MBOLD + 10-deep MBP, TCP |

---

### Key Differences vs BrokerTec (non-Chicago)
- Implied Matching on channels **504, 505, 506** (U.S. Actives only)
- All-Or-None (Conflated TCP): ✅ listed but note — *BrokerTec Chicago does not support Repo markets*
- VWAP (Conflated TCP): ✅ listed as N/A in description
- Statistics, Book Management, Recovery: functionally identical to BrokerTec

---

## 4. EBS Market on CME Globex

### Market Locations & Services

#### EBS Market New York
| Service | Conflation | Depth | Screened? | Protocol |
|---|---|---|---|---|
| CME MDP Conflated UDP — EBS Ultra NY | 5ms | 10-deep MBP | ❌ | UDP/SBE |
| CME MDP 1ms Conflated UDP — EBS Ultra NY | 1ms | 10-deep MBP | ❌ | UDP/SBE |
| CME MDP Conflated TCP — Credit Screened NY | 20ms | 5-deep MBP | ✅ | TCP/SBE |
| CME MDP EBS Ultra 20ms Conflated TCP — NY | 20ms | 10-deep MBP | ❌ | TCP/SBE |

#### EBS Market London
| Service | Conflation | Depth | Screened? | Protocol |
|---|---|---|---|---|
| CME MDP Conflated UDP — EBS Ultra London | 5ms | 10-deep MBP | ❌ | UDP/SBE |
| CME MDP 1ms Conflated UDP — EBS Ultra London | 1ms | 10-deep MBP | ❌ | UDP/SBE |
| CME MDP EBS Ultra 20ms Conflated TCP — London | 20ms | 10-deep MBP | ❌ | TCP/SBE |
| CME MDP Conflated TCP — Credit Screened London | 20ms | 5-deep MBP | ✅ | TCP/SBE |
| CME MDP Conflated TCP — EBS Ultra Russian Ruble | 5ms | 10-deep MBP | ❌ | TCP/SBE |

#### EBS Market OFF-SEF NDF London
| Service | Conflation | Depth | Notes |
|---|---|---|---|
| CME MDP Conflated UDP — EBS Ultra OFF SEF NDF | 100ms or 5ms | 5-deep MBP | Embargo rule applies |
| CME MDP Conflated TCP — Credit Screened OFF SEF NDF | 50ms | 3-deep MBP | Credit screened + Embargo rule |

> **Embargo Rule:** Trade info is delayed a minimum of **400ms** after the trade occurs for OFF-SEF NDF instruments.

---

### Key Feature Support Matrix

| Feature | Conflated UDP | Conflated TCP | EBS Ultra 20ms TCP |
|---|:---:|:---:|:---:|
| SBE Encoding | ✅ | ✅ | ✅ |
| Event-Based Messaging | ✅ | ✅ | ✅ |
| Conflation Processing | ✅ | ✅ | ✅ |
| Conflated TCP Group Processing | ❌ | ✅ | ✅ |
| Market by Price (MBP) | ✅ | ✅ | ✅ |
| MBOFD / MBOLD | ❌ | ❌ | ❌ |
| Implied Matching | ❌ | ❌ | ❌ |
| Inverted Price Book | ❌ | ❌ | ❌ |
| Bilateral Trading | ❌ | ❌ | ❌ |
| Credit Screened Market Best | ❌ | ✅ | ❌ |
| Trade & Value Date Processing | ✅ | ✅ | ✅ |
| Trade Embargo (OFF-SEF NDF) | ✅ | ✅ | ❌ |
| Security Status | ✅ | ✅ | ❌ |
| Cloud (GCP) Processing | ❌ | ❌ | ❌ |

> **EBS Ultra Russian Ruble TCP** channel is **not** credit screened.

---

### Statistics
- ❌ **All standard statistics are N/A for EBS** — no Opening Price, Session High/Low, Electronic Volume, OI, Settlement, VWAP, etc.

### Recovery
- **UDP Recovery:** Conflated UDP only (MBP books only — MBO recovery not available for EBS)
- **TCP Recovery:** Conflated TCP + EBS Ultra 20ms TCP
- Smart Stream & WebSocket Recovery: ❌ Not supported

---

## Quick Cross-Market Comparison

| Capability | Futures & Options | BrokerTec | BrokerTec Chicago | EBS |
|---|:---:|:---:|:---:|:---:|
| GCP Smart Stream | ✅ | ❌ | ❌ | ❌ |
| MBOFD | ✅ | ✅ (Premium) | ✅ (Premium) | ❌ |
| MBOLD | ❌ | ✅ | ✅ | ❌ |
| Inverted Price Book | ❌ | ✅ | ✅ | ❌ |
| Repo / Bilateral Support | ❌ | ✅ | ❌ | ❌ |
| Credit Screened | ❌ | ❌ | ❌ | ✅ (TCP) |
| Trade Embargo (NDF) | ❌ | ❌ | ❌ | ✅ |
| Full Statistics Suite | ✅ | Partial | Partial | ❌ |
| VWAP | ❌ | ✅ (Repo TCP) | ✅ (TCP) | ❌ |