//cli et pour appeler le reste, formater ?
//
#include <stdio.h>
#include "../include/grid.h"
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <grid_file>\n", argv[0]);
        return 1;
    }
    Grid *g = grid_load(argv[1]);
    if (!g)
    {
        fprintf(stderr, "Error: load de la grille a %s\n", argv[1]);
        return 1;
    }
    printf("Grile loaded: %d colonnes x %d lignes\n", g->width, g->height);
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
            printf("%c", grid_at(g, x, y));
        printf("\n");
    }

    grid_free(g);
    return 0;
}
