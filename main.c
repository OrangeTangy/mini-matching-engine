#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "order_book.h"
#include "matching_engine.h"
#include "utils.h"

#define MAX_LINE 256

int main(int argc, char *argv[])
{
    FILE *input     = stdin;
    int   from_file = 0;

    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (!input) {
            perror("[ERROR] Cannot open input file");
            return EXIT_FAILURE;
        }
        from_file = 1;
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [input_file]\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!from_file)
        print_usage();

    OrderBook book;
    orderbook_init(&book);

    char line[MAX_LINE];

    while (fgets(line, sizeof(line), input)) {
        /* Strip \r\n (handles both Unix and Windows line endings) */
        line[strcspn(line, "\r\n")] = '\0';

        /* Echo non-empty, non-comment lines when replaying a file */
        if (from_file) {
            const char *p = line;
            while (*p == ' ' || *p == '\t') p++;
            if (*p != '\0' && *p != '#')
                printf(">> %s\n", line);
        }

        Command cmd = parse_command(line);

        switch (cmd.type) {

        case CMD_BUY:
        case CMD_SELL: {
            if (cmd.price == 0 || cmd.quantity == 0) {
                fprintf(stderr,
                        "[ERROR] Price and quantity must be greater than 0\n");
                break;
            }
            if (orderbook_id_exists(&book, cmd.id)) {
                fprintf(stderr,
                        "[ERROR] Duplicate order ID %" PRIu64 " — rejected\n", cmd.id);
                break;
            }

            book.seq_counter++;
            Order *order = order_create(
                cmd.id,
                cmd.type == CMD_BUY ? SIDE_BUY : SIDE_SELL,
                cmd.price,
                cmd.quantity,
                book.seq_counter
            );
            if (!order) {
                fprintf(stderr, "[ERROR] Out of memory\n");
                break;
            }

            printf("[ORDER] %-4s  #%-6" PRIu64 "  Price: %-8" PRIu64 "  Qty: %" PRIu64 "\n",
                   cmd.type == CMD_BUY ? "BUY" : "SELL",
                   cmd.id, cmd.price, cmd.quantity);

            orderbook_add(&book, order);
            match_orders(&book);
            break;
        }

        case CMD_CANCEL:
            if (orderbook_cancel(&book, cmd.id) == 0)
                printf("[CANCEL] Order #%" PRIu64 " removed from book\n", cmd.id);
            else
                fprintf(stderr,
                        "[ERROR] Order #%" PRIu64 " not found in book\n", cmd.id);
            break;

        case CMD_PRINT:
            orderbook_print(&book);
            break;

        case CMD_EMPTY:
            break;

        case CMD_UNKNOWN:
            /* Error message already printed by parse_command */
            break;
        }
    }

    if (from_file)
        fclose(input);

    orderbook_destroy(&book);
    return EXIT_SUCCESS;
}
