# OCR
OCR pour mot mêlés en C

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
