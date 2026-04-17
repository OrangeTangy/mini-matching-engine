#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
#include "utils.h"

Command parse_command(const char *line)
{
    Command cmd = { CMD_UNKNOWN, 0, 0, 0 };

    /* Skip leading whitespace */
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;

    /* Blank line or comment */
    if (*p == '\0' || *p == '#') {
        cmd.type = CMD_EMPTY;
        return cmd;
    }

    char verb[16] = {0};
    if (sscanf(p, "%15s", verb) != 1)
        return cmd;

    /* Case-insensitive verb matching */
    for (int i = 0; verb[i]; i++)
        verb[i] = (char)toupper((unsigned char)verb[i]);

    if (strcmp(verb, "BUY") == 0) {
        if (sscanf(p, "%*s %" SCNu64 " %" SCNu64 " %" SCNu64,
                   &cmd.id, &cmd.price, &cmd.quantity) == 3)
            cmd.type = CMD_BUY;
        else
            fprintf(stderr,
                    "[ERROR] Bad BUY syntax.  Expected: BUY <id> <price> <qty>\n");

    } else if (strcmp(verb, "SELL") == 0) {
        if (sscanf(p, "%*s %" SCNu64 " %" SCNu64 " %" SCNu64,
                   &cmd.id, &cmd.price, &cmd.quantity) == 3)
            cmd.type = CMD_SELL;
        else
            fprintf(stderr,
                    "[ERROR] Bad SELL syntax.  Expected: SELL <id> <price> <qty>\n");

    } else if (strcmp(verb, "CANCEL") == 0) {
        if (sscanf(p, "%*s %" SCNu64, &cmd.id) == 1)
            cmd.type = CMD_CANCEL;
        else
            fprintf(stderr,
                    "[ERROR] Bad CANCEL syntax.  Expected: CANCEL <id>\n");

    } else if (strcmp(verb, "PRINT") == 0) {
        cmd.type = CMD_PRINT;

    } else {
        fprintf(stderr, "[ERROR] Unknown command: '%s'\n", verb);
    }

    return cmd;
}

void print_usage(void)
{
    printf("╔═══════════════════════════════════════════════════╗\n");
    printf("║         Mini Matching Engine  —  Commands         ║\n");
    printf("╠═══════════════════════════════════════════════════╣\n");
    printf("║  BUY    <id> <price> <qty>   submit a bid order   ║\n");
    printf("║  SELL   <id> <price> <qty>   submit an ask order  ║\n");
    printf("║  CANCEL <id>                 cancel resting order ║\n");
    printf("║  PRINT                       display order book   ║\n");
    printf("║  # ...                       comment (ignored)    ║\n");
    printf("║  Ctrl-D / EOF                quit                 ║\n");
    printf("╚═══════════════════════════════════════════════════╝\n\n");
}
