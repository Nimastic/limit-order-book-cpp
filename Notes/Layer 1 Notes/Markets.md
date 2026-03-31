## Markets — the basics

### The four types of financial securities
- **Equity** — you own a slice of the company (stocks/shares). If the company
  does well, your slice is worth more. If it goes bust, you lose it.
- **Debt** — you lend money to a company or government (bonds). They promise
  to pay you back with interest. You don't own anything — you're a creditor.
- **Derivatives** — a contract whose value is *derived from* something else
  (options, futures, warrants). You're not buying the asset, you're buying
  a right or obligation related to it.
- **Hybrid** — has features of both equity and debt. Example: convertible bonds
  start as debt but can convert into shares under certain conditions.

---

### What is a market?
A market is just any mechanism where buyers and sellers meet to agree on a price.
Markets differ by:
- **What's being traded** — stocks, bonds, currencies, commodities, derivatives
- **How prices are set** — order-driven (buyers/sellers post orders, engine matches
  them) vs quote-driven (dealers post firm buy/sell prices, you trade against them)
- **When it's open** — exchanges have fixed hours; FX and bond markets trade 24/5

HKEX is an **order-driven** exchange. Your order book is implementing exactly this.

---

### Participants and what they want

| Who | What they're doing |
|---|---|
| **Retail investors** | Buy and hold for long-term growth or income |
| **Institutional investors** | Funds (pension, mutual, hedge) trading large sizes |
| **Market makers** | Post both bid and ask continuously, profit from spread |
| **Proprietary traders** | Trade firm's own capital using directional strategies |
| **HFT firms** | Extremely high speed, high volume, tiny per-trade edge |
| **Brokers** | Route your orders to the exchange — they don't take positions |

---

### Price discovery — why markets exist
The "real" price of anything is unknowable. Markets aggregate millions of
opinions (buy/sell orders) into a single number everyone can trade at.
When bad news hits, sellers flood in, price drops — the book reflects
new consensus fast. This is price discovery.

Your order book is the mechanism that makes price discovery happen.
Every match is two people agreeing on fair value at that moment.

---

### Key market states your order book needs to handle

| State | What it means |
|---|---|
| **Lit market** | Orders visible to everyone (standard exchange book) |
| **Dark pool** | Orders hidden until matched (institutional, large blocks) |
| **Crossed market** | Best bid ≥ best ask — should immediately match, never rest |
| **Locked market** | Best bid = best ask — same thing, match immediately |
| **One-sided book** | Only bids or only asks — no matches possible yet |
| **Empty book** | No orders at all — first order always rests |

A crossed or locked book should be impossible in a correctly implemented
matching engine. If you ever see it in testing, your matching loop has a bug.