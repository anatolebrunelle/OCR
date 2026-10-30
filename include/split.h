#ifndef SPLIT_H
#define SPLIT_H

#include <SDL2/SDL.h>

typedef struct {
    SDL_Surface *grid;      // the grid alone
    SDL_Surface **words;    // one image per word of the list
    int word_count;
} Split;

// cuts the image into a grid and one image per word
// if out_dir is not null, also writes out_dir/grid.png,
// out_dir/word1.png, out_dir/word2.png, ... (the folder must exist)
// returns null on error, to be freed with split_free
Split *split_image(const char *path, const char *out_dir);
void split_free(Split *s);

#endif
