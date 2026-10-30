# OCR
OCR for word searches in C

## Main program (ocr)

Dependencies: SDL2, SDL2_image and pkg-config. Build: `make`.

```sh
./ocr img/level_1_image_1.png out
./ocr img/level_2_image_1.png out --rotate 25
./ocr solve tests/grid.txt horizontal
```

`./ocr IMAGE OUTDIR [--rotate DEGREES]` chains: manual rotation
(positive angle = counterclockwise), colour removal, detection of the
grid and of the word list, then cutting of each letter into an image.
Each step is saved in `OUTDIR`:

- `01_rotated.png` (only with `--rotate`), `02_gray.png`, `03_bw.png`
- `split/` : `grid.png` and one `word<n>.png` per word of the list
- `grid/letter/` and `words/word<n>/letter/`: one image per letter

`./ocr solve GRID_FILE WORD` solves a word in a text grid, like the
`solver` program (`(x0,y0)(x1,y1)` or `Not found`).

Each part can still be run on its own: `solver`, `preprocess-demo`,
`split-test`, `detect-demo` and `cut-demo`. `make clean` removes everything.

## Preprocessing

Dependencies: SDL2, SDL2_image and pkg-config.

```sh
make preprocess-demo
./preprocess-demo img/level_1_image_1.png gray.png --gray
./preprocess-demo img/level_2_image_1.png bw.png --bw
./preprocess-demo img/level_2_image_1.png rotated.png --rotate -8
./preprocess-demo img/level_2_image_1.png cleaned.png --rotate -8 --bw
```

The options `--gray`, `--bw` and `--rotate DEGREES` can be combined.
A positive angle corresponds to a counterclockwise rotation.
The result is saved in PNG format.

The functions are declared in `include/preprocessing.h`.
They return a new `SDL_Surface *` to be freed with
`SDL_FreeSurface`. The input image is not modified.

Build: `make solver`, `make preprocess-demo` or `make split-test`.
