//cli et pour appeler le reste, formater ?
//
#include <stdio.h>
#include "../include/grid.h"
#include "../include/solver.h"

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "Usage: %s <grid_file> <word>\n", argv[0]);
        return 1;
    }

    Grid *g = grid_load(argv[1]);
    if (!g)
    {
        fprintf(stderr, "Error: could not load grid from %s\n", argv[1]);
        return 1;
    }

    SearchResult r = solver_find(g, argv[2]);

    if (r.found)
        printf("(%d,%d)(%d,%d)\n", r.x0, r.y0, r.x1, r.y1);
    else
        printf("Not found\n");

    grid_free(g);
    return 0;
}
