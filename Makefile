CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude \
         $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS = $(shell pkg-config --libs sdl2 SDL2_image)

SOLVER_SRC = src/main.c src/grid.c src/solver.c
PREPROCESS_SRC = src/preprocessing_cli.c src/preprocessing.c
SPLIT_SRC = src/test.c src/split.c

all: solver preprocess-demo

solver: $(SOLVER_SRC)
	$(CC) $(CFLAGS) -o $@ $(SOLVER_SRC) $(LDLIBS)

preprocess-demo: $(PREPROCESS_SRC)
	$(CC) $(CFLAGS) -o $@ $(PREPROCESS_SRC) $(LDLIBS) -lm

split-test: $(SPLIT_SRC)
	$(CC) $(CFLAGS) -o $@ $(SPLIT_SRC) $(LDLIBS)

clean:
	rm -f solver preprocess-demo split-test src/*.o

.PHONY: all clean
