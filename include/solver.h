#ifndef SOLVER_H
#define SOLVER_H

#include "grid.h"

typedef struct {
    int x0, y0;
    int x1, y1;
    int found;
} SearchResult;

SearchResult solver_find(const Grid *g, const char *word);

#endif
