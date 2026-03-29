#include "order_book.h"
#include <thread>
#include <chrono>

int main() {

    // ── Test 1: market buy sweeps all asks ────────────────────────────────────
    std::cout << "=== Test 1: market buy sweeps everything ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()}); // ask $103
        book.addOrder({2, false, 10400, 100, now_ns()}); // ask $104
        book.addOrder({3, false, 10500, 100, now_ns()}); // ask $105

        // market buy — INT_MAX price, sweeps all three asks
        book.addOrder({4, true, INT_MAX, 300, now_ns()});
        book.printBook();
        // expected: 3 trades fire at $103, $104, $105
        // book empty after
    }

    // ── Test 2: market sell sweeps all bids ───────────────────────────────────
    std::cout << "=== Test 2: market sell sweeps everything ===\n";
    {
        OrderBook book;
        book.addOrder({1, true, 10300, 100, now_ns()}); // bid $103
        book.addOrder({2, true, 10200, 100, now_ns()}); // bid $102
        book.addOrder({3, true, 10100, 100, now_ns()}); // bid $101

        // market sell — price 0, sweeps all three bids
        book.addOrder({4, false, 0, 300, now_ns()});
        book.printBook();
        // expected: 3 trades fire at $103, $102, $101 (best price first)
        // book empty after
    }

    // ── Test 3: market buy partial — remainder rests but doesn't display ──────
    std::cout << "=== Test 3: market buy partial fill ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()}); // ask $103 qty=100

        // market buy for 200 — only 100 available
        book.addOrder({2, true, INT_MAX, 200, now_ns()});
        book.printBook();
        // expected: TRADE 100 @ $103
        // remaining 100 of market buy rests but does NOT show in printBook
        // book should appear empty (sentinel price filtered out)
    }

    // ── Test 4: market order with no liquidity ────────────────────────────────
    std::cout << "=== Test 4: market buy into empty book ===\n";
    {
        OrderBook book;
        // no asks in the book
        book.addOrder({1, true, INT_MAX, 100, now_ns()});
        book.printBook();
        // expected: no trades, book appears empty (sentinel filtered)
    }

    return 0;
}