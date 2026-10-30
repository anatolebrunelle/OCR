//just the word search logic in the grid
//
#include "../include/solver.h"
#include <string.h>
#include <ctype.h>

// the 8 possible directions: (dx, dy)
static const int DIRECTIONS[8][2] = {
    { 1,  0},   // right
    {-1,  0},   // left
    { 0,  1},   // down
    { 0, -1},   // up
    { 1,  1},   // diagonal down-right
    {-1, -1},   // diagonal up-left
    { 1, -1},   // diagonal up-right
    {-1,  1},   // diagonal down-left
};

// true if the neighbouring cell (x,y) + direction is still inside the grid
int check_voisins(int x, int y, int width, int height, const int *direction) {
    if (x + direction[0] < 0 || x + direction[0] >= width)   return 0;
    if (y + direction[1] < 0 || y + direction[1] >= height)  return 0;
    return 1;
}

// returns {x0, y0, x1, y1} if the word starts at (x,y), NULL otherwise
int *solver(const char *word, const Grid *g, int x, int y) {
    static int res[4];
    int width = g->width;
    int height = g->height;
    size_t len = strlen(word);

    for (int a = 0; a < 8; a++) {
        const int *dir = DIRECTIONS[a];
        int cx = x;
        int cy = y;
        int ok = 1;

        for (size_t b = 1; b < len; b++) {
            if (!check_voisins(cx, cy, width, height, dir)) {
                ok = 0;
                break;
            }
            cx += dir[0];
            cy += dir[1];
            if (toupper(grid_at(g, cx, cy)) != toupper(word[b])) {
                ok = 0;
                break;
            }
        }

        if (ok) {
            res[0] = x;  res[1] = y;
            res[2] = cx; res[3] = cy;
            return res;
        }
    }
    return NULL;
}

SearchResult solver_find(const Grid *g, const char *word) {
    SearchResult findings = {
        .x0 = 0, .y0 = 0,
        .x1 = 0, .y1 = 0,
        .found = 0
    };
    for (int y = 0; y < g->height; y++) {
        for (int x = 0; x < g->width; x++) {
            if (toupper(grid_at(g, x, y)) == toupper(word[0])) {
                if (strlen(word) == 1) {
                    findings.x0 = x;  findings.y0 = y;
                    findings.x1 = x;  findings.y1 = y;
                    findings.found = 1;
                    return findings;
                }
                int *res = solver(word, g, x, y);
                if (res) {
                    findings.x0 = res[0];  findings.y0 = res[1];
                    findings.x1 = res[2];  findings.y1 = res[3];
                    findings.found = 1;
                    return findings;
                }
            }
        }
    }
    return findings;
}
