#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <stdint.h>

#define SIDE_BUY  0
#define SIDE_SELL 1

/*
 * Prices are unsigned 64-bit integers (e.g. ticks or cents).
 * Using integers avoids floating-point comparison pitfalls that are
 * particularly dangerous in financial software.
 */
typedef struct Order {
    uint64_t     id;
    int          side;       /* SIDE_BUY or SIDE_SELL */
    uint64_t     price;
    uint64_t     quantity;
    uint64_t     seq;        /* monotonic sequence number — defines time priority */
    struct Order *next;
} Order;

/*
 * The order book holds two singly-linked sorted lists:
 *
 *   buy_head  — sorted descending by price, then ascending by seq.
 *               The head is always the best bid (highest price, earliest time).
 *
 *   sell_head — sorted ascending by price, then ascending by seq.
 *               The head is always the best ask (lowest price, earliest time).
 *
 * A sorted linked list gives O(n) insertion but trivially correct ordering
 * and zero extra memory overhead.  Production engines use a price-level map
 * (hash/tree from price -> FIFO queue) to achieve O(1) best-price access and
 * O(1) amortised insert at an existing price level.
 */
typedef struct {
    Order   *buy_head;
    Order   *sell_head;
    uint64_t seq_counter;   /* incremented before every new order */
} OrderBook;

/* Lifecycle */
void     orderbook_init(OrderBook *book);
void     orderbook_destroy(OrderBook *book);

/* Order construction */
Order   *order_create(uint64_t id, int side,
                      uint64_t price, uint64_t qty, uint64_t seq);

/* Book operations */
int      orderbook_add(OrderBook *book, Order *order);
int      orderbook_cancel(OrderBook *book, uint64_t id);
int      orderbook_id_exists(const OrderBook *book, uint64_t id);
void     orderbook_print(const OrderBook *book);

#endif /* ORDER_BOOK_H */
