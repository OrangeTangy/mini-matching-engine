#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

typedef enum {
    CMD_BUY,
    CMD_SELL,
    CMD_CANCEL,
    CMD_PRINT,
    CMD_EMPTY,    /* blank line or comment */
    CMD_UNKNOWN   /* parse error */
} CommandType;

typedef struct {
    CommandType type;
    uint64_t    id;
    uint64_t    price;
    uint64_t    quantity;
} Command;

/*
 * Parse one line of text into a Command.
 * Errors are printed to stderr; the returned type will be CMD_UNKNOWN.
 */
Command parse_command(const char *line);
void    print_usage(void);

#endif /* UTILS_H */
