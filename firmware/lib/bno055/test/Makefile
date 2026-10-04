CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Werror -Wno-error=unused-function -g
LDLIBS  := -lm

SRCS    := test_bno055.c fake_i2c.c ../bno055.c
HDRS    := fake_i2c.h ../bno055.h
BIN     := test_bno055

.PHONY: all test clean

all: $(BIN)

$(BIN): $(SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDLIBS)

test: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN) *.o
