#ifndef CUT_H
#define CUT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define SRC "to_cut.png" /* SRC = source image */
#define DIR "letter"     /* DIR = output directory */

/* rect = rectangle (top-left x1,y1 and bottom-right x2,y2, both inclusive) */
struct rect
{
    int x1;
    int y1;
    int x2;
    int y2;
};

Uint32 pix(const SDL_Surface *s, int x, int y);

struct rect fl(const SDL_Surface *s, unsigned char *seen, int *stk, int x,
               int y);

void mark(const SDL_Surface *s, unsigned char *seen, struct rect r);

struct rect inn(const SDL_Surface *s, struct rect r);

int sv(SDL_Surface *s, struct rect r, int n);

int isc(const SDL_Surface *s, int x, int y);

/* scan = walk through the image and cut every coloured rectangle */
int scan(SDL_Surface *s);

/*
cut = cut "to_cut.png" into one image per coloured rectangle
stored in the "letter" directory (word1.png, word2.png, ...)
Returns 0 on success, -1 on failure (and prints "to_cut.png failed")
*/
int cut(void);

#endif
