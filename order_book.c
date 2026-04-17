#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "order_book.h"

/* ------------------------------------------------------------------ */
/*  Lifecycle                                                          */
/* ------------------------------------------------------------------ */

void orderbook_init(OrderBook *book)
{
    book->buy_head   = NULL;
    book->sell_head  = NULL;
    book->seq_counter = 0;
}

void orderbook_destroy(OrderBook *book)
{
    Order *curr, *nxt;

    curr = book->buy_head;
    while (curr) { nxt = curr->next; free(curr); curr = nxt; }

    curr = book->sell_head;
    while (curr) { nxt = curr->next; free(curr); curr = nxt; }

    book->buy_head  = NULL;
    book->sell_head = NULL;
}

/* ------------------------------------------------------------------ */
/*  Order construction                                                 */
/* ------------------------------------------------------------------ */

Order *order_create(uint64_t id, int side,
                    uint64_t price, uint64_t qty, uint64_t seq)
{
    Order *o = malloc(sizeof(Order));
    if (!o) return NULL;
    o->id       = id;
    o->side     = side;
    o->price    = price;
    o->quantity = qty;
    o->seq      = seq;
    o->next     = NULL;
    return o;
}

/* ------------------------------------------------------------------ */
/*  Sorted insertion helpers                                           */
/* ------------------------------------------------------------------ */

/*
 * Buy side: highest price first.  Ties broken by lowest seq (earliest order).
 * Uses a pointer-to-pointer walk so no special-case for the head node.
 */
static void insert_buy(OrderBook *book, Order *order)
{
    Order **slot = &book->buy_head;

    while (*slot) {
        int better_price = order->price > (*slot)->price;
        int same_price_earlier = (order->price == (*slot)->price)
                                 && (order->seq < (*slot)->seq);
        if (better_price || same_price_earlier)
            break;
        slot = &(*slot)->next;
    }

    order->next = *slot;
    *slot = order;
}

/*
 * Sell side: lowest price first.  Ties broken by lowest seq (earliest order).
 */
static void insert_sell(OrderBook *book, Order *order)
{
    Order **slot = &book->sell_head;

    while (*slot) {
        int better_price = order->price < (*slot)->price;
        int same_price_earlier = (order->price == (*slot)->price)
                                 && (order->seq < (*slot)->seq);
        if (better_price || same_price_earlier)
            break;
        slot = &(*slot)->next;
    }

    order->next = *slot;
    *slot = order;
}

/* ------------------------------------------------------------------ */
/*  Book operations                                                    */
/* ------------------------------------------------------------------ */

int orderbook_add(OrderBook *book, Order *order)
{
    if (!order) return -1;
    if (order->side == SIDE_BUY)
        insert_buy(book, order);
    else
        insert_sell(book, order);
    return 0;
}

int orderbook_cancel(OrderBook *book, uint64_t id)
{
    Order **slot;

    slot = &book->buy_head;
    while (*slot) {
        if ((*slot)->id == id) {
            Order *tmp = *slot;
            *slot = tmp->next;
            free(tmp);
            return 0;
        }
        slot = &(*slot)->next;
    }

    slot = &book->sell_head;
    while (*slot) {
        if ((*slot)->id == id) {
            Order *tmp = *slot;
            *slot = tmp->next;
            free(tmp);
            return 0;
        }
        slot = &(*slot)->next;
    }

    return -1; /* not found */
}

int orderbook_id_exists(const OrderBook *book, uint64_t id)
{
    const Order *o;
    for (o = book->buy_head;  o; o = o->next) if (o->id == id) return 1;
    for (o = book->sell_head; o; o = o->next) if (o->id == id) return 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  PRINT                                                              */
/* ------------------------------------------------------------------ */

/*
 * Standard order book display: asks from highest to lowest price above
 * the spread line, bids from highest to lowest below it.  This puts the
 * best ask and best bid closest to the centre, mirroring real terminal
 * depth-of-market views.
 */
void orderbook_print(const OrderBook *book)
{
#define MAX_DISPLAY 512
    Order  *asks[MAX_DISPLAY];
    int     ask_n = 0;
    Order  *o;

    for (o = book->sell_head; o && ask_n < MAX_DISPLAY; o = o->next)
        asks[ask_n++] = o;

    printf("\n");
    printf("  +--------------------------------------------------+\n");
    printf("  |                   ORDER BOOK                     |\n");
    printf("  +----------+------------+----------+---------------+\n");
    printf("  | %-8s | %-10s | %-8s | %-13s |\n",
           "Price", "Qty", "Order ID", "Side");
    printf("  +----------+------------+----------+---------------+\n");

    if (ask_n == 0) {
        printf("  | %-48s |\n", "(no resting asks)");
    } else {
        /* Print highest ask first so best ask sits nearest the spread line */
        for (int i = ask_n - 1; i >= 0; i--)
            printf("  | %-8" PRIu64 " | %-10" PRIu64 " | #%-7" PRIu64 " | %-13s |\n",
                   asks[i]->price, asks[i]->quantity,
                   asks[i]->id, "ASK (SELL)");
    }

    printf("  +----------+--- SPREAD -+----------+---------------+\n");

    if (!book->buy_head) {
        printf("  | %-48s |\n", "(no resting bids)");
    } else {
        for (o = book->buy_head; o; o = o->next)
            printf("  | %-8" PRIu64 " | %-10" PRIu64 " | #%-7" PRIu64 " | %-13s |\n",
                   o->price, o->quantity, o->id, "BID (BUY)");
    }

    printf("  +----------+------------+----------+---------------+\n");
    printf("\n");
#undef MAX_DISPLAY
}
