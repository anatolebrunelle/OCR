# OCR
OCR pour mot mêlés en C

## Programme principal (ocr)

Dependances : SDL2, SDL2_image et pkg-config. Compilation : `make`.

```sh
./ocr img/level_1_image_1.png out
./ocr img/level_2_image_1.png out --rotate 25
./ocr solve tests/grid.txt horizontal
```

`./ocr IMAGE OUTDIR [--rotate DEGREES]` enchaine : rotation manuelle
(angle positif = antihoraire), suppression des couleurs, detection de la
grille et de la liste de mots, puis decoupage de chaque lettre en image.
Chaque etape est enregistree dans `OUTDIR` :

- `01_rotated.png` (seulement avec `--rotate`), `02_gray.png`, `03_bw.png`
- `split/` : `grid.png` et un `word<n>.png` par mot de la liste
- `grid/letter/` et `words/word<n>/letter/` : une image par lettre

`./ocr solve GRID_FILE WORD` resout un mot dans une grille texte, comme le
programme `solver` (`(x0,y0)(x1,y1)` ou `Not found`).

Chaque partie reste executable seule : `solver`, `preprocess-demo`,
`split-test`, `detect-demo` et `cut-demo`. `make clean` supprime tout.

## Pretraitement

Dependances : SDL2, SDL2_image et pkg-config.

```sh
make preprocess-demo
./preprocess-demo img/level_1_image_1.png gray.png --gray
./preprocess-demo img/level_2_image_1.png bw.png --bw
./preprocess-demo img/level_2_image_1.png rotated.png --rotate -8
./preprocess-demo img/level_2_image_1.png cleaned.png --rotate -8 --bw
```

Les options `--gray`, `--bw` et `--rotate DEGREES` peuvent etre combinees.
Un angle positif correspond a une rotation antihoraire.
Le resultat est enregistre au format PNG.

Les fonctions sont declarees dans `include/preprocessing.h`.
Elles retournent une nouvelle `SDL_Surface *` a liberer avec
`SDL_FreeSurface`. L'image d'entree n'est pas modifiee.

Compilation : `make solver`, `make preprocess-demo` ou `make split-test`.
