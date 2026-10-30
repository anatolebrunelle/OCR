#ifndef CUT_H
#define CUT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

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

/* ---------- pixel loading and pixel uses ---------- */
SDL_Surface *loadimg(const char *path);
Uint32 getpix(const SDL_Surface *img, int x, int y);
Uint8 *blackmap(const SDL_Surface *img, int thr);
 
/* ---------- lines and columns ---------- */
int runrow(const Uint8 *blk, int w, int y);
int runcol(const Uint8 *blk, int w, int h, int x);
void freelines(struct line *lns);
void freecols(struct col *cls);
struct line *findlines(const Uint8 *blk, int w, int h);
struct col *findcols(const Uint8 *blk, int w, int h);
void cleargrid(Uint8 *blk, const SDL_Surface *img);
 
/* ---------- Colors and drawing ---------- */
void nextcolor(struct rect *rc, int n);
void drawrect(SDL_Renderer *rdr, const struct rect *rc);
 
/* ---------- Blobs of pixels ---------- */
void flood(Uint8 *blk, int w, int h, int *stk, int sx, int sy, struct rect *rc);
int scan(SDL_Renderer *rdr, SDL_Surface *img, Uint8 *blk);
 
/* ---------- begin program ---------- */
int work(SDL_Surface *img, int grid);
int detect_char(const char *path);

#endif
