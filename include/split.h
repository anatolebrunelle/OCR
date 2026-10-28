#ifndef SPLIT_H
#define SPLIT_H

#include <SDL2/SDL.h>

typedef struct {
    SDL_Surface *grid;      // la grille seule
    SDL_Surface **words;    // une image par mot de la liste
    int word_count;
} Split;

// decoupe l'image en une grille et une image par mot
// si out_dir n'est pas null, ecrit aussi out_dir/grid.png,
// out_dir/word1.png, out_dir/word2.png, ... (le dossier doit exister)
// renvoie null en cas d'erreur, a liberer avec split_free
Split *split_image(const char *path, const char *out_dir);
void split_free(Split *s);

#endif
