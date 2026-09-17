#include "grid.h"

typedef struct {
    int x0, y0, x1, y1;
    int found;
} SearchResult;

SearchResult solver_find(const Grid *g, const char *word);
