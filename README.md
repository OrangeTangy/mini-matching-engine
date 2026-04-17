# Mini Matching Engine

A simplified electronic trading engine written in pure C for Linux.  It accepts
buy and sell limit orders from stdin or a file, maintains an in-memory order
book, and matches orders using **price-time priority** — the same fundamental
algorithm used in every major electronic exchange (CME, NASDAQ, NYSE, ICE).

---

## Why This Project Matters for Electronic Trading

| Concept demonstrated | Real-world relevance |
|---|---|
| Price-time priority matching | Core algorithm of every lit exchange |
| Integer fixed-point prices | Industry standard to avoid FP rounding errors |
| Sorted order-book maintenance | Foundation of market-making and SOR systems |
| Maker/taker price model | How exchange fee tiers and trade reporting work |
| Partial fills and residual resting orders | Essential for realistic order lifecycle management |
| O(1) match at the top of book | The hot path that HFT firms optimise to nanoseconds |

HFT and electronic market-making firms (Jane Street, Citadel Securities, Jump
Trading, Optiver, etc.) build production versions of exactly this engine.
Understanding the internals — even at this scale — demonstrates that you can
reason about correctness, latency, and data structures in the context of
financial infrastructure.

---

## Project Structure

```
Engine/
├── main.c              Entry point; reads commands from stdin or a file
├── order_book.h/.c     Order struct, sorted linked-list book, add/cancel/print
├── matching_engine.h/.c  Price-time priority matching loop
├── utils.h/.c          Command parser and usage text
├── Makefile            Build targets: all / run / run_file / clean
├── sample_input.txt    Five annotated scenarios
└── README.md           This file
```

---

## Architecture

```
stdin / file
     │
     ▼
  main.c  ──────────────────────────────────────────────────────────
     │  parse_command()            (utils.c)
     │
     ├─ CMD_BUY / CMD_SELL
     │       │
     │       ├─ order_create()     (order_book.c)
     │       ├─ orderbook_add()    inserts into sorted list
     │       └─ match_orders()     (matching_engine.c) ← hot path
     │
     ├─ CMD_CANCEL  ──►  orderbook_cancel()
     └─ CMD_PRINT   ──►  orderbook_print()
```

---

## Data Structures

### Order

```c
typedef struct Order {
    uint64_t id;
    int      side;      /* SIDE_BUY or SIDE_SELL */
    uint64_t price;     /* integer ticks — no floating point */
    uint64_t quantity;
    uint64_t seq;       /* global sequence number → time priority */
    struct Order *next;
} Order;
```

### OrderBook — two sorted singly-linked lists

```
buy_head  → [id=9, price=102, qty=10, seq=9]
          → [id=10, price=101, qty=15, seq=10]
          → [id=11, price=100, qty=25, seq=11]
          → NULL

sell_head → [id=3, price=100, qty=50, seq=3]
          → [id=4, price=102, qty=100, seq=4]
          → NULL
```

**Buy side** is sorted descending by price, then ascending by seq.
The head is always the **best bid** (highest price, earliest arrival at that
price).

**Sell side** is sorted ascending by price, then ascending by seq.
The head is always the **best ask** (lowest price, earliest arrival).

#### Why a sorted linked list?

| Choice | Insert | Best-price access | Cancel | Memory |
|---|---|---|---|---|
| Sorted linked list (this project) | O(n) | O(1) | O(n) | minimal |
| Price-level map + per-level FIFO (production) | O(log P) | O(1) | O(1) | moderate |
| Heap | O(log n) | O(1) | O(n) | minimal |

A sorted list is the simplest correct structure and straightforward to reason
about.  Production engines use a `std::map<price, std::deque<Order*>>` (or a
lock-free equivalent) to get O(log P) inserts and O(1) cancel via an ID→pointer
hash map.  This is called a **price-level book** or **ladder**.

---

## How Price-Time Priority Works

Price-time priority (also called FIFO priority) is a two-key sort:

1. **Price** — a better-priced order always executes first.
   - For buys:  higher price is better.
   - For sells: lower price is better.

2. **Time** — among orders at the *same* price, the one that arrived earlier
   executes first.  This engine uses a monotonically increasing `seq` counter
   as a proxy for wall-clock time, avoiding any OS time-call overhead.

### Matching loop (matching_engine.c)

```
while best_bid.price >= best_ask.price:
    trade_price  = maker's price   (lower seq = arrived first = maker)
    trade_qty    = min(bid_qty, ask_qty)
    print trade log
    deduct trade_qty from both sides
    remove any fully filled order
```

Trade price convention: the **maker** (the resting order, lower seq) sets the
price.  The **taker** (the incoming aggressor) accepts it.  This matches
standard exchange matching engine behaviour.

---

## Build and Run

### Prerequisites

- GCC (any version supporting C11)
- GNU Make
- Linux (or WSL on Windows)

### Commands

```bash
make              # compile everything → ./matching_engine
make run          # compile + launch interactive mode
make run_file     # compile + replay sample_input.txt
make clean        # remove object files and binary
```

### Interactive mode

```
$ ./matching_engine
> BUY 1 100 50
> SELL 2 100 50
> PRINT
```

### File mode

```
$ ./matching_engine sample_input.txt
```

---

## Example Input and Output

### Full match

```
>> BUY 1 100 50
[ORDER] BUY   #1      Price: 100       Qty: 50
>> SELL 2 100 50
[ORDER] SELL  #2      Price: 100       Qty: 50
[TRADE] Buy  #1      x  Sell #2      |  Price: 100       |  Qty: 50
[FILL ] Buy  #1      fully filled
[FILL ] Sell #2      fully filled
```

### Partial fill

```
>> BUY 3 101 100
[ORDER] BUY   #3      Price: 101       Qty: 100
>> SELL 4 101 30
[ORDER] SELL  #4      Price: 101       Qty: 30
[TRADE] Buy  #3      x  Sell #4      |  Price: 101       |  Qty: 30
[FILL ] Sell #4      fully filled
[PART ] Buy  #3      partial fill — 70 remaining
```

### Cancel

```
>> CANCEL 3
[CANCEL] Order #3 removed from book
```

### Multiple resting orders — time priority demonstrated

```
>> BUY 5 100 20
>> BUY 6 100 30
>> BUY 7 100 10
>> SELL 8 100 40
[TRADE] Buy  #5      x  Sell #8      |  Price: 100       |  Qty: 20
[FILL ] Buy  #5      fully filled
[TRADE] Buy  #6      x  Sell #8      |  Price: 100       |  Qty: 20
[FILL ] Sell #8      fully filled
[PART ] Buy  #6      partial fill — 10 remaining
```

Orders #5 and #6 (earlier seq) fill before #7 even though all three are at the
same price.

### Aggressive order sweeps multiple price levels

```
>> SELL 12 99 50
[TRADE] Buy  #9      x  Sell #12     |  Price: 102       |  Qty: 10
[FILL ] Buy  #9      fully filled
[TRADE] Buy  #10     x  Sell #12     |  Price: 101       |  Qty: 15
[FILL ] Buy  #10     fully filled
[TRADE] Buy  #6      x  Sell #12     |  Price: 100       |  Qty: 10
[FILL ] Buy  #6      fully filled
[TRADE] Buy  #7      x  Sell #12     |  Price: 100       |  Qty: 10
[FILL ] Buy  #7      fully filled
[PART ] Sell #12     partial fill — 5 remaining
```

### PRINT output

```
  +--------------------------------------------------+
  |                   ORDER BOOK                     |
  +----------+------------+----------+---------------+
  | Price    | Qty        | Order ID | Side          |
  +----------+------------+----------+---------------+
  | 101      | 50         | #3       | ASK (SELL)    |
  | 100      | 50         | #4       | ASK (SELL)    |
  +----------+--- SPREAD -+----------+---------------+
  | 102      | 10         | #9       | BID (BUY)     |
  | 101      | 15         | #10      | BID (BUY)     |
  | 100      | 25         | #11      | BID (BUY)     |
  +----------+------------+----------+---------------+
```

---

## Possible Future Improvements

| Improvement | What it adds |
|---|---|
| **Price-level map** (`price → FIFO queue`) | O(log P) insert, O(1) cancel via ID hash map — the real production data structure |
| **TCP socket interface** | Multiple clients can submit orders concurrently, closer to FIX/FAST protocol |
| **Multithreading + lock-free queue** | Separate inbound order parsing from the matching core; demonstrate producer-consumer pattern |
| **Binary protocol** (FIX, ITCH, OUCH) | Real exchanges use compact binary formats; parsing them is a key HFT skill |
| **Nanosecond timestamps** via `clock_gettime(CLOCK_REALTIME)` | Replace sequence numbers with wall-clock time for realistic latency measurement |
| **Persistent order log** | Append-only file or memory-mapped ring buffer for crash recovery |
| **Benchmark harness** | Measure throughput (orders/sec) and match latency (ns) under load |
| **IOC / FOK / Market orders** | Immediate-or-cancel and fill-or-kill order types used by every real venue |
| **Multiple instruments** | One engine, many symbols — demonstrates the `symbol → OrderBook` dispatch layer |

---

## License

MIT — use freely for learning and portfolios.
