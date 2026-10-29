#ifndef CUT_H
#define CUT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

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

/* pix = pixel: read pixel (x, y) of an ARGB8888 surface */
Uint32 pix(const SDL_Surface *s, int x, int y);

/* fl = flood fill: bounding box of the same-colour area starting at (x, y) */
struct rect fl(const SDL_Surface *s, unsigned char *seen, int *stk, int x,
               int y);

/* mark = mark the whole rectangle as already visited */
void mark(const SDL_Surface *s, unsigned char *seen, struct rect r);

/* sv = save: write the rectangle r of the surface in DIR/word<n>.png */
int sv(SDL_Surface *s, struct rect r, int n);

/*
** isc = is colour: true if the pixel is opaque and not a shade of gray
** (white, gray, black and anti-aliased letter pixels are not colours)
*/
int isc(Uint32 c);

/* scan = walk through the image and cut every coloured rectangle */
int scan(SDL_Surface *s);

/*
** cut = cut "to_cut.png" into one image per coloured rectangle,
** stored in the "letter" directory (word1.png, word2.png, ...).
** Returns 0 on success, -1 on failure (and prints "to_cut.png failed").
*/
int cut(void);

#endif /* CUT_H */
