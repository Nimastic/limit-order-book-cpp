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

/**
 * @brief Lifecycle state of an order in the book.
 */
enum class OrderStatus { // scoped enumerations, strongly-typed, creating a named list of constants that belong to their own namespace
    Open,
    PartiallyFilled,
    Filled,
    Cancelled
}; // why enum class > plain enum? solves name collisions (leaking name into scope) / solves implicit conversion to int 

enum class OrderType {
    Limit,  // rest remainder
    ELO,    // enhanced limit order: sweep up to 10 levels, rest remainder
    SLO,    // special limit order: sweep up to 10 levels, cancel remainder
    AON,    // all or nothing: fill fully now or reject entirely
    Market  // sweep everything (sentinel price)
};

/**
 * @brief Single limit order: side, price, remaining quantity, identity, and status.
 * @note Price is stored in fixed-point (e.g. cents); display code may divide by 100.0.
 */
struct Order {
    int id;
    bool isBuy;
    int price;
    int quantity;
    long long timestamp; // for time priority
    OrderStatus status = OrderStatus::Open;
    OrderType type = OrderType::Limit;

};

/**
 * @brief Monotonic timestamp in nanoseconds from std::steady_clock.
 * @return Nanoseconds since an unspecified epoch; suitable for ordering, not wall-clock dates.
 */
inline long long now_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

/**
 * @brief One executed trade: both order ids, execution price, and size.
 */
struct Trade {
    int buyOrderId;
    int sellOrderId;
    int price;
    int quantity;
};

/**
 * @brief Limit order book: bid/ask price levels, match-on-insert, cancel, and trade log.
 */
class OrderBook {

/**
 * @brief Debug/test hook: asserts book invariants (implementation in stress_test.cpp).
 * @param book Book to validate.
 * @relates OrderBook
 */
friend void checkInvariants(const OrderBook&);

public:
    /**
     * @brief Submit an order: match against the opposite side, then rest any remainder.
     * @param o Incoming order (by value); modified during matching before optional rest.
     */
    void addOrder(Order o);
    /**
     * @brief Cancel an open or partially filled order by id.
     * @param orderId Order identifier.
     * @return true if marked cancelled; false if missing, filled, or already cancelled.
     * @note The slot may remain in its queue until the front; purgeCancelledFront removes it.
     */
    [[nodiscard]] bool cancelOrder(int orderId); // returns false if not found
    // [[nodiscard]] compiler hint that the return value of a function must not be silently ignored.
    /**
     * @brief Print aggregated bid/ask levels to stdout (skips display sentinels).
     */
    void printBook() const;
    /**
     * @brief All trades recorded by recordTrade, in execution order.
     * @return Reference valid for the lifetime of this OrderBook.
     */
    const std::vector<Trade>& getTrades() const { return trades; }


// private:
// change to public temporarily for stress testing

// NOTE: ordering within a price level relies on std::queue insertion order (FIFO).
// This works correctly only if orders arrive in time order.
// True timestamp-based sorting is not enforced here — if two orders arrive
// at the same price level out of network order, the later-arriving order
// could jump the queue.
// Fix: Stage 2a upgrade replaces std::queue with std::list sorted by timestamp
// on insert. Until then, insertion order == time order is our assumption.

    // bids: highest price first
    // for this bid price, queue of Orders at this price (FIFO)
    /**
     * @brief Bid side: map from limit price to FIFO queue at that price (best bid = largest key).
     */
    std::map<int, std::deque<Order>, std::greater<int>> bids; // changed from queue to deque
    // asks: lowest price first
    // for this ask price, queue of Orders at this price (FIF0)
    /**
     * @brief Ask side: map from limit price to FIFO queue at that price (best ask = smallest key).
     */
    std::map<int, std::deque<Order>> asks; // change from queue to deque
    // queue copies the entire queue at every price level during iterating
    
    // O(1) lookup by order id, needed for cancel later
    /**
     * @brief Order id to pointer to the live Order stored inside a price-level queue.
     */
    std::unordered_map<int, Order*> index; // id and pointer to order

    /**
     * @brief Append-only trade history populated during matching.
     */
    std::vector<Trade> trades;

    /**
     * @brief Match an aggressive buy against resting asks until filled or limit does not cross.
     * @param buy Buy order; quantities and status updated in place.
     */
    void matchBuy(Order& buy);
    /**
     * @brief Match an aggressive sell against resting bids until filled or limit does not cross.
     * @param sell Sell order; quantities and status updated in place.
     */
    void matchSell(Order& sell);
    /**
     * @brief Append one trade to trades and log it.
     * @param qty Size traded.
     * @param price Execution price (same units as Order::price).
     * @param buyId Buy order id.
     * @param sellId Sell order id.
     */
    void recordTrade(int qty, int price, int buyId, int sellId);
    /**
     * @brief Remove cancelled orders from the front of a price-level queue and drop them from index.
     * @param q Queue at one price level on bid or ask side.
     */
    void purgeCancelledFront(std::deque<Order>& q);
};
