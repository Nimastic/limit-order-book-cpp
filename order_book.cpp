#include "order_book.h"

using namespace std; // no overhead to use this

// Helpers

/** @copydoc OrderBook::purgeCancelledFront */
// Remove cancelled order from the front of a queue, called before every match attempt so we dont accidentally fill a dead order
void OrderBook::purgeCancelledFront(queue<Order>& q) {
    while (!q.empty() && q.front().status == OrderStatus::Cancelled) {
        index.erase(q.front().id);
        q.pop();
    }
};

/** @copydoc OrderBook::recordTrade */
// Add trades to vector<Trade> trades;
void OrderBook::recordTrade(int qty, int price, int buyId, int sellId) {
    trades.push_back({buyId, sellId, price, qty});
    cout << "TRADE: " << qty << " @ $" << price / 100.0 << " (buy#" << buyId << " x sell#" << sellId << ")\n";
}

/** @copydoc OrderBook::cancelOrder */
// Cancelling orders
bool OrderBook::cancelOrder(int orderId) {
    auto it = index.find(orderId); // unordered_map<int, Order*>
    if (it == index.end()) { // not in index
        cout << " CANCEL ORDER FAILED: order #" << orderId << " not found\n";
        return false;
    }
    // order exists
    Order* order = it->second; // get pointer of index
    if (order->status == OrderStatus::Filled) {
        cout << "CANCEL FAILED: order #" << orderId << " already filled\n";
        return false;
    }
    if (order->status == OrderStatus::Cancelled) {
        cout << "CANCEL FAILED: order #" << orderId << " already cancelled\n";
        return false;
    }

    // order status is open or partially filled
    order->status = OrderStatus::Cancelled; // set as cancelled
    index.erase(it);        // erase by iterator (slightly faster)
    cout << "CANCELLED: order #" << orderId << "\n";
    // we dont remove from the queue, purge will clean it when it reaches the front
    return true;
}


// -- Core Matching

/** @copydoc OrderBook::matchBuy */
void OrderBook::matchBuy(Order& buy) {
    while (buy.quantity > 0 && !asks.empty()) {
        auto best = asks.begin();          // lowest ask
        purgeCancelledFront(best->second); // skip dead orders and cleanses them
        if (best->second.empty()) { 
            asks.erase(best);
            continue;
        } // if no more Orders, means no more valid orders for the lowest ask

        // (best->second).empty()
        // (*best).second.empty()
        // X best->(second.empty())
        if (buy.price < best->first) break; // price doesn't cross, stop

        Order& sell = best->second.front(); // oldest order at this level
        int fill = min(buy.quantity, sell.quantity);

        recordTrade(fill, best->first, buy.id, sell.id);

        buy.quantity  -= fill;
        sell.quantity -= fill;

        if (sell.quantity == 0) {
            sell.status = OrderStatus::Filled;
            index.erase(sell.id);
            best->second.pop();
            if (best->second.empty()) asks.erase(best);
        } else {
            sell.status = OrderStatus::PartiallyFilled;
        }
    }

    if (buy.quantity == 0) buy.status = OrderStatus::Filled;
}

/** @copydoc OrderBook::matchSell */
void OrderBook::matchSell(Order& sell) {
    while (sell.quantity > 0 && !bids.empty()) {
        auto best = bids.begin();            // highest bid

        purgeCancelledFront(best->second);
        if (best->second.empty()) {
            bids.erase(best);
            continue;
        }

        if (sell.price > best->first) break; // price doesn't cross, stop

        Order& buy = best->second.front();
        int fill = min(sell.quantity, buy.quantity);

        recordTrade(fill, best->first, buy.id, sell.id);

        sell.quantity -= fill;
        buy.quantity  -= fill;

        if (buy.quantity == 0) {
            buy.status = OrderStatus::Filled;
            index.erase(buy.id);
            best->second.pop();
            if (best->second.empty()) bids.erase(best);
        } else {
            buy.status = OrderStatus::PartiallyFilled;
        }
    }
    if (sell.quantity == 0) sell.status = OrderStatus::Filled;
}

/** @copydoc OrderBook::addOrder */
// Public Interface

void OrderBook::addOrder(Order o) {
    // AON: check if fully fillable before touching book
    if (o.type == OrderType::AON) {
        int available = 0;
        if (o.isBuy) {
            for (auto& [price, q] : asks) {
                if (price == 0 || price == INT_MAX ) continue;
                if (o.price < price) break; // price doesnt cross
                auto copy = q;
                while (!copy.empty()) {
                    if (copy.front().status != OrderStatus::Cancelled)
                        available += copy.front().quantity;
                    copy.pop();
                }
                if (available >= o.quantity) break; 
            }
        } else {
            for (auto& [price, q] : bids) {
                if (price == 0 || price == INT_MAX ) continue;
                if (o.price > price) break; // price doesnt cross
                auto copy = q;
                while (!copy.empty()) {
                    if (copy.front().status != OrderStatus::Cancelled)
                        available += copy.front().quantity;
                    copy.pop();
                }
                if (available >= o.quantity) break; 
            }
        }
        if (available < o.quantity) {
            cout << "AON REJECTED: order #" << o.id
                      << " - only " << available
                      << " available, need " << o.quantity << "\n";
            o.status = OrderStatus::Cancelled;
            return; // dont touch the book
        }
        // enough evailable, fall through to normal matching
    }


    // ELO: check if price is within 10 levels of best
    if (o.type == OrderType::ELO) {
        if (o.isBuy && !asks.empty()) {
            int bestAsk = asks.begin() -> first; // get lowest ask rice
            // reject if price is 10+ levels above the best ask
            // using tick size of 1 cent = 1 unit for simplicity
            if (o.price >= bestAsk + 10) {
                cout << "ELO REJECTED: order #" << o.id
                     << " price is too far from the best ask\n";
                return;
            }
        }
        if (!o.isBuy && !bids.empty()) { // is ask, and bids is not empty
            int bestBid = bids.begin()->first; // get lowest bid
            if (o.price <= bestBid - 10) {
                cout << "ELO REJECTED: order #" << o.id
                     << " price too far from best bid\n";
                return;
            }
        }
    }


    if (o.isBuy) { // if this order isbuy
        matchBuy(o);
        if (o.quantity > 0) {
            if (o.type == OrderType::SLO) {

                cout << "SLO REMAINDER CANCELLED: order #" << o.id
                    << " unfilled qty=" << o.quantity << "\n";
                o.status = OrderStatus::Cancelled;

            } else { // Limit, ELO, AON, Market all rest remainder
                bids[o.price].push(o); // rest remainder //  we put into buy map and push order into that price's queue
                index[o.id] = &bids[o.price].back();
            }
        }
    } else {
        matchSell(o);
        if (o.quantity > 0) { 
            if (o.type == OrderType::SLO) {
                cout << "SLO REMAINDER CANCELLED: order #" << o.id
                     << " unfilled qty=" << o.quantity << "\n";
                o.status = OrderStatus::Cancelled;
            } else {
                asks[o.price].push(o);
                index[o.id] = &asks[o.price].back();
            }
        }
    }
}

/** @copydoc OrderBook::printBook */
// Display

void OrderBook::printBook() const {
    cout << "\n=== ASK SIDE (sellers) ===\n";
    // prints highest ask first 
    for (auto it = asks.rbegin(); it != asks.rend(); ++it) { // from the highest bid first we iterate down
        if (it->first == INT_MAX) continue; // market order sentinel, skip display
        auto q = it->second; // get the queue
        int total = 0;
        int count = 0;
        while (!q.empty()) { // while queue is not empty
            if (q.front().status != OrderStatus::Cancelled) {
                total += q.front().quantity;
                count++;
            }
            q.pop();
        }
        if (count > 0) {
            cout << " $" << it->first / 100.0
                << " qty=" << total
                << " orders=" << count << "\n";
        }
    }

    cout << "------=== SPREAD ===------\n";
    // using [price, q] : bids] vs auto it = asks.rbegin(); it != asks.rend(); ++it has no performance difference
    
    for (auto& [price, q] : bids) {
        if (price == 0) continue;  // market order sentinel, skip display
        if (price == INT_MAX) continue; // market buy sentinel
        auto copy = q;
        int total = 0;
        int count = 0;
        while (!copy.empty()) {
            if (copy.front().status != OrderStatus::Cancelled) {
                total += copy.front().quantity;
                count++;
            }
            copy.pop();
        }
        if (count > 0) {
            cout << " $" << price / 100.0
                << " qty=" << total
                << " orders=" << count << "\n";
        }
    }
    cout << "=== BID SIDE (buyers) ===\n\n";
}