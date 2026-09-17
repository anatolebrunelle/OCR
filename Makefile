CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)
BIN     = solver

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
