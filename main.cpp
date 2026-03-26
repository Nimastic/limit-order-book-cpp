#include "order_book.h"

int main() {

    // ── Test 1: cancel resting order ─────────────────────────────────────────
    std::cout << "=== Test 1: cancel resting order ===\n";
    {
        OrderBook book;
        book.addOrder({1, true,  10200, 100, 1000}); // bid rests
        book.addOrder({2, true,  10100, 200, 1001}); // bid rests
        book.printBook();                             // two bids visible

        book.cancelOrder(1);                          // cancel order #1
        book.printBook();                             // only order #2 visible
    }

    // ── Test 2: cancelled order must never fill ───────────────────────────────
    std::cout << "=== Test 2: cancelled order never fills ===\n";
    {
        OrderBook book;
        book.addOrder({1, true, 10300, 100, 1000});  // bid rests
        book.cancelOrder(1);                          // cancel it
        book.addOrder({2, false, 10300, 100, 1001}); // sell arrives at same price
        book.printBook();
        // no trade should fire — order #1 is cancelled
        // sell #2 should rest on ask side
    }

    // ── Test 3: cancel non-existent order ────────────────────────────────────
    std::cout << "=== Test 3: cancel non-existent ===\n";
    {
        OrderBook book;
        book.cancelOrder(99); // should print CANCEL FAILED, not crash
    }

    // ── Test 4: cancel one of many orders at same level ──────────────────────
    std::cout << "=== Test 4: cancel middle order at price level ===\n";
    {
        OrderBook book;
        book.addOrder({1, true, 10200, 100, 1000});
        book.addOrder({2, true, 10200, 200, 1001}); // same price level
        book.addOrder({3, true, 10200, 150, 1002}); // same price level
        book.printBook();                            // total qty=450, orders=3

        book.cancelOrder(2);                         // cancel middle one
        book.printBook();                            // total qty=250, orders=2

        // now send a sell that sweeps — should fill #1 and #3, skip #2
        book.addOrder({4, false, 10000, 400, 1003});
        book.printBook();
    }

    return 0;
}