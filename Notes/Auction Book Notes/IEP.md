The IEP algorithm is the procedure for finding the **single auction clearing price**.

## Core idea

For each candidate price `p`:

* `buyQty(p)` = total buy volume with limit `>= p`
* `sellQty(p)` = total sell volume with limit `<= p`
* `matchQty(p)` = `min(buyQty(p), sellQty(p))`
* `imbalance(p)` = `abs(buyQty(p) - sellQty(p))`

Then choose the price that:

1. **maximises** `matchQty`
2. if tied, **minimises** `imbalance`
3. if still tied, use a deterministic tie-break, such as:

   * closest to reference price
   * lower price
   * higher price depending on exchange rule

## Candidate prices

Usually use the **distinct prices already present in the auction book**:

* all bid prices
* all ask prices

You do not need to scan every integer price.

## Pseudocode

```cpp
for each candidate price p:
    buy  = sum of buy orders with price >= p
    sell = sum of sell orders with price <= p
    match = min(buy, sell)
    imbalance = abs(buy - sell)

pick best p by:
    highest match
    then lowest imbalance
    then tie-break rule
```

## Example

Buys:

* 105 x 100
* 104 x 200
* 103 x 150

Sells:

* 102 x 120
* 103 x 180
* 104 x 170

Check prices:

### at 103

* buys `>= 103` = 450
* sells `<= 103` = 300
* match = 300
* imbalance = 150

### at 104

* buys `>= 104` = 300
* sells `<= 104` = 470
* match = 300
* imbalance = 170

So both give match 300, but 103 has smaller imbalance, so **IEP = 103**.

## After IEP is chosen

Run batch uncrossing:

* eligible buys: `price >= IEP`
* eligible sells: `price <= IEP`

Allocate fills by:

* better price first
* FIFO within same price

But all matched trades execute at the **same price: the IEP**.

## One-line intuition

IEP = **the price that lets the auction match the most volume at one common execution price**.
