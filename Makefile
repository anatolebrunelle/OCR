CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude \
         $(shell pkg-config --cflags sdl2 SDL2_image)
LDLIBS = $(shell pkg-config --libs sdl2 SDL2_image) -lm

HEADERS = $(wildcard include/*.h)

SOLVER_SRC = src/main.c src/grid.c src/solver.c
PREPROCESS_SRC = src/preprocessing_cli.c src/preprocessing.c
SPLIT_SRC = src/test.c src/split.c

OCR_OBJ = src/ocr.o src/preprocessing.o src/split.o src/grid.o src/solver.o \
          src/detect_char_lib.o src/cut_lib.o

all: ocr solver preprocess-demo split-test detect-demo cut-demo

ocr: $(OCR_OBJ)
	$(CC) $(CFLAGS) -o $@ $(OCR_OBJ) $(LDLIBS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

src/detect_char_lib.o: src/detect_char.c $(HEADERS)
	$(CC) $(CFLAGS) -Dmain=detect_char_demo -Dscan=detect_scan \
	    -c -o $@ $<

src/cut_lib.o: src/cut.c $(HEADERS)
	$(CC) $(CFLAGS) -Dmain=cut_demo -Dscan=cut_scan -c -o $@ $<

solver: $(SOLVER_SRC)
	$(CC) $(CFLAGS) -o $@ $(SOLVER_SRC) $(LDLIBS)

preprocess-demo: $(PREPROCESS_SRC)
	$(CC) $(CFLAGS) -o $@ $(PREPROCESS_SRC) $(LDLIBS)

split-test: $(SPLIT_SRC)
	$(CC) $(CFLAGS) -o $@ $(SPLIT_SRC) $(LDLIBS)

detect-demo: src/detect_char.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

cut-demo: src/cut.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f ocr solver preprocess-demo split-test detect-demo cut-demo
	rm -f src/*.o

.PHONY: all clean

