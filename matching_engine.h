#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include "order_book.h"

/*
 * Attempt to match the best resting bid against the best resting ask.
 * Called immediately after every new order is inserted into the book.
 *
 * Matching rules implemented here:
 *   1. A trade occurs whenever best_bid.price >= best_ask.price (prices cross).
 *   2. The trade executes at the MAKER's price.  The maker is the order with
 *      the lower sequence number (it arrived first and was resting in the book).
 *   3. Traded quantity = min(bid_qty, ask_qty).
 *   4. Partial fills leave a residual quantity resting in the book.
 *   5. Fully filled orders are freed immediately.
 *   6. The loop continues until no crossing orders remain.
 */
void match_orders(OrderBook *book);

#endif /* MATCHING_ENGINE_H */
