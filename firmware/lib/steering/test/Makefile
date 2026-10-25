CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -g
LDLIBS  := -lm

SRCS    := test_steering.c  ../steering.c
HDRS    := ../steering.h
BIN     := test_steering

.PHONY: all test clean

all: $(BIN)

$(BIN): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDLIBS)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.o
