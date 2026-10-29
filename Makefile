CC      = gcc
<<<<<<< Updated upstream
CFLAGS  = -Wall -Wextra -std=c99 $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS  = $(shell pkg-config --libs sdl2 SDL2_image)
=======
CFLAGS  = -Wall -Werror -Wextra -std=c99 -fsanitize=address
>>>>>>> Stashed changes
SRC     = $(wildcard src/*.c)
OBJ     = $(SRC:.c=.o)
BIN     = solver, render

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean
