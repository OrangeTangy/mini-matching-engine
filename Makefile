CC      = gcc
CFLAGS  = -Wall -Wextra -Wpedantic -std=c11 -O2
TARGET  = matching_engine
SRCS    = main.c order_book.c matching_engine.c utils.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all run run_file clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Interactive mode — type commands at the prompt
run: $(TARGET)
	./$(TARGET)

# Replay the bundled sample input file
run_file: $(TARGET)
	./$(TARGET) sample_input.txt

clean:
	rm -f $(OBJS) $(TARGET)

# Dependency tracking (keeps incremental builds correct)
main.o:            main.c order_book.h matching_engine.h utils.h
order_book.o:      order_book.c order_book.h
matching_engine.o: matching_engine.c matching_engine.h order_book.h
utils.o:           utils.c utils.h
