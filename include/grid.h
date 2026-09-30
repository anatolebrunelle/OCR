#ifndef GRID_H
#define GRID_H

typedef struct {
    char *cells;
    int width;
    int height;
} Grid;

Grid *grid_load(const char *filename);
void grid_free(Grid *g);
char grid_at(const Grid *g, int x, int y);

#endif
