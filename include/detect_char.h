#ifndef CUT_H
#define CUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define BLKMAX 128 // gray level under which a pixel belongs to a character
#define LINEMAX 240 // gray level under which a pixel may belong to a grid line
                    // (lines can be light gray, so this is almost white)
#define LRATIO 80  // min % of an axis a black run must span to be a grid line
#define PAD 3      // gap between a character and its frame
#define THK 2      // thickness of a frame
#define NPAL 12    // number of colors in the palette

// Bounding box of a character, plus the color of its frame
struct rect
{
    int x1;  // x of the top left corner
    int y1;  // y of the top left corner
    int x2;  // x of the bottom right corner
    int y2;  // y of the bottom right corner
    Uint8 r; // red
    Uint8 g; // green
    Uint8 b; // blue
};

// Horizontal grid line: y is its ordinate
struct line
{
    int y;
    struct line *next;
};

// Vertical grid line: x is its abscissa
struct col
{
    int x;
    struct col *next;
};

// Reads grille.png or word<n>.png, frames every character with a colored
// rect and saves the result as to_cut.png. Returns 0 on success, 1 on error.
int detect_char(const char *path);

#endif
