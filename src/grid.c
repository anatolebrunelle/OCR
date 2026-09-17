// pour charger une grille depuis un fichier et la stocker en mémoire.
//
// src/grid.c
#include "../include/grid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Grid *grid_load(const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
        return NULL;

    char lines[256][256];  //max 256 lignes de 256 colonnes
    int height = 0;
    int width = 0;

    while (fgets(lines[height], sizeof(lines[height]), f))
    {
        // enlever le '\n' final
        size_t len = strlen(lines[height]);
        if (len > 0 && lines[height][len - 1] == '\n')
            lines[height][len - 1] = '\0';

        int line_len = (int)strlen(lines[height]);
        if (line_len == 0)
            continue;  // ignore les lignes vides

        if (width == 0)
            width = line_len;
        else if (line_len != width)
        {
            fclose(f);
            return NULL;  // lignes de longueur incoherente
        }

        height++;
    }
    fclose(f);

    if (height == 0 || width == 0)
        return NULL;

    Grid *g = malloc(sizeof(Grid));
    if (!g)
        return NULL;

    g->width = width;
    g->height = height;
    g->cells = malloc((size_t)(width * height) * sizeof(char));
    if (!g->cells)
    {
        free(g);
        return NULL;
    }

    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            g->cells[y * width + x] = lines[y][x];

    return g;
}

char grid_at(const Grid *g, int x, int y)
{
    return g->cells[y * g->width + x];
}

void grid_free(Grid *g)
{
    if (!g)
        return;
    free(g->cells);
    free(g);
}
