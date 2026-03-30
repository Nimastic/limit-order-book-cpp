#include "order_book.h"
#include <thread>
#include <chrono>

int main() {

    // ── ELO Test 1: sweep multiple price levels within 10 ticks, rest remainder ──
    std::cout << "=== ELO: sweep and rest (within 10 ticks) ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 80,  now_ns()}); // ask 103.00
        book.addOrder({2, false, 10304, 70,  now_ns()}); // ask 103.04
        book.addOrder({3, false, 10308, 160, now_ns()}); // ask 103.08

        // best ask = 10300
        // ELO buy at 10309 is only 9 ticks above best ask -> allowed
        // should sweep all 3 levels, total fill = 310, rest 90 on bid side
        Order elo = {4, true, 10309, 400, now_ns(),
                     OrderStatus::Open, OrderType::ELO};
        book.addOrder(elo);
        book.printBook();
    }

    // ── ELO Test 2: fully filled, nothing rests ───────────────────────────────
    std::cout << "=== ELO: fully filled (within 10 ticks) ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()});
        book.addOrder({2, false, 10305, 100, now_ns()});

        // best ask = 10300
        // ELO buy at 10306 is 6 ticks above best ask -> allowed
        // fills exactly 200, nothing rests
        Order elo = {3, true, 10306, 200, now_ns(),
                     OrderStatus::Open, OrderType::ELO};
        book.addOrder(elo);
        book.printBook();
    }

    // ── ELO Test 3: boundary reject at exactly 10 ticks with current code ─────
    std::cout << "=== ELO: rejected at exactly 10 ticks ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()});

        // current code uses:
        // if (o.price >= bestAsk + 10) reject;
        // so 10310 is rejected
        Order elo = {2, true, 10310, 100, now_ns(),
                     OrderStatus::Open, OrderType::ELO};
        book.addOrder(elo);
        book.printBook();
    }

    // ── SLO Test 1: remainder cancelled, not rested ───────────────────────────
    std::cout << "=== SLO: remainder cancelled ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()}); // ask 103.00

        // SLO is not subject to ELO tick-band validation
        // fills 100, cancels remaining 200
        Order slo = {2, true, 10500, 300, now_ns(),
                     OrderStatus::Open, OrderType::SLO};
        book.addOrder(slo);
        book.printBook();
    }

    // ── SLO vs ELO same scenario, but use valid ELO price ─────────────────────
    std::cout << "=== SLO vs ELO same scenario ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()});

        // ELO buy is 5 ticks above best ask -> allowed
        // fills 100, rests remaining 200
        Order elo = {2, true, 10305, 300, now_ns(),
                     OrderStatus::Open, OrderType::ELO};
        book.addOrder(elo);
        book.printBook();
    }

    // ── AON Test 1: enough liquidity — fills fully ────────────────────────────
    std::cout << "=== AON: enough liquidity ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 200, now_ns()});
        book.addOrder({2, false, 10400, 200, now_ns()});

        // AON buy 300 — 400 available, should fill
        Order aon = {3, true, 10500, 300, now_ns(),
                     OrderStatus::Open, OrderType::AON};
        book.addOrder(aon);
        book.printBook();
    }

    // ── AON Test 2: not enough liquidity — rejected entirely ──────────────────
    std::cout << "=== AON: not enough liquidity ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 100, now_ns()}); // only 100 available

        Order aon = {2, true, 10500, 300, now_ns(),
                     OrderStatus::Open, OrderType::AON};
        book.addOrder(aon);
        book.printBook();
    }

    // ── AON Test 3: book not touched on rejection ─────────────────────────────
    std::cout << "=== AON: book untouched on rejection ===\n";
    {
        OrderBook book;
        book.addOrder({1, false, 10300, 50, now_ns()});

        Order aon = {2, true, 10300, 100, now_ns(),
                     OrderStatus::Open, OrderType::AON};
        book.addOrder(aon);
        book.printBook();
    }

    return 0;
}