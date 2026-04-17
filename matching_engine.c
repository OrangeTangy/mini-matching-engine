#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "matching_engine.h"

void match_orders(OrderBook *book)
{
    while (book->buy_head && book->sell_head) {
        Order *buy  = book->buy_head;
        Order *sell = book->sell_head;

        /* No cross: best bid is strictly below best ask — nothing to do */
        if (buy->price < sell->price)
            break;

        /*
         * Determine the maker (the resting, passive order).
         * The order with the lower sequence number arrived first and was
         * already sitting in the book — it is the maker.  Trades execute
         * at the maker's price, which is the standard exchange convention
         * and protects the passive participant from price slippage.
         */
        uint64_t trade_price = (sell->seq < buy->seq) ? sell->price
                                                       : buy->price;
        uint64_t trade_qty   = (buy->quantity < sell->quantity)
                               ? buy->quantity : sell->quantity;

        printf("[TRADE] Buy  #%-6" PRIu64 "  x  Sell #%-6" PRIu64
               "  |  Price: %-8" PRIu64 "  |  Qty: %" PRIu64 "\n",
               buy->id, sell->id, trade_price, trade_qty);

        buy->quantity  -= trade_qty;
        sell->quantity -= trade_qty;

        if (buy->quantity == 0) {
            book->buy_head = buy->next;
            printf("[FILL ] Buy  #%" PRIu64 " fully filled\n", buy->id);
            free(buy);
        } else {
            printf("[PART ] Buy  #%" PRIu64 " partial fill — %" PRIu64 " remaining\n",
                   buy->id, buy->quantity);
        }

        if (sell->quantity == 0) {
            book->sell_head = sell->next;
            printf("[FILL ] Sell #%" PRIu64 " fully filled\n", sell->id);
            free(sell);
        } else {
            printf("[PART ] Sell #%" PRIu64 " partial fill — %" PRIu64 " remaining\n",
                   sell->id, sell->quantity);
        }
    }
}
