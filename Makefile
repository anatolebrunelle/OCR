CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS  = $(shell pkg-config --libs sdl2 SDL2_image)
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)
BIN     = solver

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
