#pragma once // tells compilers to include once per CU
#include <map>
#include <queue>
#include <unordered_map>
#include <functional>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <climits> // for INT_MAX

// namespace using std;
// bad to use this in header files

enum class OrderStatus { // scoped enumerations, strongly-typed, creating a named list of constants that belong to their own namespace
    Open,
    PartiallyFilled,
    Filled,
    Cancelled
}; // why enum class > plain enum? solves name collisions (leaking name into scope) / solves implicit conversion to int 

struct Order {
    int id;
    bool isBuy;
    int price;
    int quantity;
    long long timestamp; // for time priority
    OrderStatus status = OrderStatus::Open;

};

inline long long now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

struct Trade {
    int buyOrderId;
    int sellOrderId;
    int price;
    int quantity;
};

class OrderBook {

public:
    void addOrder(Order o);
    bool cancelOrder(int orderId); // returns false if not found
    void printBook() const;
    const std::vector<Trade>& getTrades() const { return trades; }


private:
// NOTE: ordering within a price level relies on std::queue insertion order (FIFO).
// This works correctly only if orders arrive in time order.
// True timestamp-based sorting is not enforced here — if two orders arrive
// at the same price level out of network order, the later-arriving order
// could jump the queue.
// Fix: Stage 2a upgrade replaces std::queue with std::list sorted by timestamp
// on insert. Until then, insertion order == time order is our assumption.

    // bids: highest price first
    // for this bid price, queue of Orders at this price (FIFO)
    std::map<int, std::queue<Order>, std::greater<int>> bids;
    // asks: lowest price first
    // for this ask price, queue of Orders at this price (FIF0)
    std::map<int, std::queue<Order>> asks;
    
    // O(1) lookup by order id, needed for cancel later
    std::unordered_map<int, Order*> index; // id and pointer to order

    std::vector<Trade> trades;

    void matchBuy(Order& buy);
    void matchSell(Order& sell);
    void recordTrade(int qty, int price, int buyId, int sellId);
    void purgeCancelledFront(std::queue<Order>& q);
};
