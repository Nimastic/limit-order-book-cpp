Call auction / batch auction

* orders are collected during an auction window
* no immediate matching on arrival
* at auction time, the engine finds one single clearing price
* all eligible trades execute together at that same price
* this is commonly used for opening and closing auctions 

IEP (Indicative Equilibrium Price)

* IEP is the auction clearing price
* for each candidate price `p`:

  * buy quantity = all buy volume with limit `>= p`
  * sell quantity = all sell volume with limit `<= p`
  * executable quantity = `min(buyQty, sellQty)`
* choose the price that gives the highest executable quantity 

Tie-breaks

* if multiple prices give the same max executable volume:

  * choose the one with the smallest imbalance
  * imbalance = `|buyQty - sellQty|`
  * if still tied, use a deterministic rule such as nearest reference price or lower price 

Batch uncrossing

* once IEP is chosen:

  * buys with `price >= IEP` are eligible
  * sells with `price <= IEP` are eligible
* allocation still uses priority:

  * better price first
  * FIFO within the same price level
* but all matched trades print at the same auction price: the IEP 

Key difference vs continuous matching

* continuous book: “can this order trade now?”
* auction book: “what single price maximises total matched volume?” 
