// cuts a word search image: the grid and the words of the list
// a letter = a bunch of black pixels stuck together
// in the list the letters are tight (they form words)
// in the grid they are spaced out (one per cell)
//
#include "../include/split.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x0, y0, x1, y1;
    int count;  // number of letters in the box
} Box;

//tools 

static int max_int (int a, int b) {
    if (a > b) return a;
    return b;
}

static int min_int (int a, int b) {
    if (a < b) return a;
    return b;
}

static int box_cx (const Box *b) {
    return (b->x0 + b->x1) / 2;
}

static int box_cy (const Box *b) {
    return (b->y0 + b->y1) / 2;
}

static int box_area (const Box *b) {
    return (b->x1 - b->x0) * (b->y1 - b->y0);
}

// grows a so that it also contains b
static void box_union (Box *a, const Box *b) {
    if (b->x0 < a->x0) a->x0 = b->x0;
    if (b->y0 < a->y0) a->y0 = b->y0;
    if (b->x1 > a->x1) a->x1 = b->x1;
    if (b->y1 > a->y1) a->y1 = b->y1;
}

// squared distance between the centers of two boxes
static int dist2 (const Box *a, const Box *b) {
    int dx = box_cx(a) - box_cx(b);
    int dy = box_cy(a) - box_cy(b);
    return dx * dx + dy * dy;
}

// 1 if the center of b is inside r (grown by margin on each side)
static int center_inside (const Box *b, const Box *r, int margin) {
    if (box_cx(b) < r->x0 - margin) return 0;
    if (box_cx(b) > r->x1 + margin) return 0;
    if (box_cy(b) < r->y0 - margin) return 0;
    if (box_cy(b) > r->y1 + margin) return 0;
    return 1;
}

static int cmp_int (const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int isqrt (int n) {
    int r = 0;
    while ((r + 1) * (r + 1) <= n) r++;
    return r;
}

static void free_all (void *a, void *b, void *c, void *d, void *e) {
    free(a);
    free(b);
    free(c);
    free(d);
    free(e);
}

//union find
static int *uf_new (int n) {
    int *p = malloc((size_t)n * sizeof(int));
    if (!p) return NULL;
    for (int i = 0; i < n; i++) p[i] = i;
    return p;
}

static int uf_find (int *p, int i) {
    while (p[i] != i) {
        p[i] = p[p[i]];
        i = p[i];
    }
    return i;
}

static void uf_union (int *p, int a, int b) {
    p[uf_find(p, a)] = uf_find(p, b);
}

//image
// loads the image and puts a white background in place of the transparency
static SDL_Surface *load_flat (const char *path) {
    SDL_Surface *raw = IMG_Load(path);
    if (!raw) return NULL;
    SDL_Surface *s = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!s) return NULL;

    for (int y = 0; y < s->h; y++) {
        unsigned char *row = (unsigned char *)s->pixels + y * s->pitch;
        for (int x = 0; x < s->w; x++) {
            unsigned char *p = row + 4 * x;
            int alpha = p[3];
            // we blend each color with white
            for (int c = 0; c < 3; c++) {
                int v = p[c] * alpha + 255 * (255 - alpha);
                p[c] = (unsigned char)(v / 255);
            }
            p[3] = 255;
        }
    }
    return s;
}

// finds the best black / white threshold from the histogram
static int otsu (const long *hist, long total) {
    long sum = 0;
    for (int i = 0; i < 256; i++) sum += i * hist[i];

    long w0 = 0;
    long sum0 = 0;
    double best = -1;
    int thr = 127;
    for (int t = 0; t < 256; t++) {
        w0 += hist[t];
        if (w0 == 0) continue;
        long w1 = total - w0;
        if (w1 == 0) break;
        sum0 += t * hist[t];
        double m0 = (double)sum0 / (double)w0;
        double m1 = (double)(sum - sum0) / (double)w1;
        double var = (double)w0 * (double)w1 * (m0 - m1) * (m0 - m1);
        if (var > best) {
            best = var;
            thr = t;
        }
    }
    return thr;
}

// ink = 1 for dark pixels
// we take the smallest of the rgb channels so that colored text counts
static unsigned char *binarize (const SDL_Surface *s) {
    size_t n = (size_t)s->w * (size_t)s->h;
    unsigned char *ink = malloc(n);
    unsigned char *dark = malloc(n);
    if (!ink || !dark) {
        free(ink);
        free(dark);
        return NULL;
    }

    long hist[256] = { 0 };
    for (int y = 0; y < s->h; y++) {
        const unsigned char *row =
            (const unsigned char *)s->pixels + y * s->pitch;
        for (int x = 0; x < s->w; x++) {
            const unsigned char *p = row + 4 * x;
            unsigned char m = p[0];
            if (p[1] < m) m = p[1];
            if (p[2] < m) m = p[2];
            dark[(size_t)y * (size_t)s->w + (size_t)x] = m;
            hist[m]++;
        }
    }

    int thr = otsu(hist, (long)n);
    for (size_t i = 0; i < n; i++) {
        if (dark[i] <= thr) ink[i] = 1;
        else ink[i] = 0;
    }
    free(dark);
    return ink;
}

//letters

// finds all the bunches of black pixels stuck together
static Box *find_components (const unsigned char *ink, int w, int h, int *n) {
    size_t size = (size_t)w * (size_t)h;
    unsigned char *seen = calloc(size, 1);
    int *stack = malloc(size * sizeof(int));
    int cap = 256;
    Box *v = malloc((size_t)cap * sizeof(Box));
    *n = 0;
    if (!seen || !stack || !v) {
        free_all(seen, stack, v, NULL, NULL);
        return NULL;
    }
    for (int start = 0; start < (int)size; start++) {
        if (!ink[start] || seen[start]) continue;
        // we start from this pixel and visit all its black neighbours
        Box b = { start % w, start / w, start % w, start / w, 1 };
        int top = 0;
        stack[top++] = start;
        seen[start] = 1;
        while (top > 0) {
            int cur = stack[--top];
            int cx = cur % w;
            int cy = cur / w;
            if (cx < b.x0) b.x0 = cx;
            if (cx > b.x1) b.x1 = cx;
            if (cy < b.y0) b.y0 = cy;
            if (cy > b.y1) b.y1 = cy;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h) continue;
                    int ni = ny * w + nx;
                    if (ink[ni] && !seen[ni]) {
                        seen[ni] = 1;
                        stack[top++] = ni;
                    }
                }
            }
        }

        // if the array is full we double it
        if (*n == cap) {
            cap *= 2;
            Box *tmp = realloc(v, (size_t)cap * sizeof(Box));
            if (!tmp) {
                free_all(v, seen, stack, NULL, NULL);
                return NULL;
            }
            v = tmp;
        }
        v[*n] = b;
        (*n)++;
    }
    free(seen);
    free(stack);
    return v;
}

// height of a normal letter (the median of the heights)
static int letter_height (const Box *c, int n, int img_h) {
    int *hs = malloc((size_t)n * sizeof(int));
    int m = 0;
    if (!hs) return 0;
    for (int i = 0; i < n; i++) {
        int bw = c[i].x1 - c[i].x0 + 1;
        int bh = c[i].y1 - c[i].y0 + 1;
        if (bh >= 6 && bh <= img_h / 8 && bw >= 2 && bw <= 3 * bh) {
            hs[m] = bh;
            m++;
        }
    }
    int res = 0;
    if (m > 0) {
        qsort(hs, (size_t)m, sizeof(int), cmp_int);
        res = hs[m / 2];
    }
    free(hs);
    return res;
}

// sticks close letters together: one bunch = a word, or a single letter
// (the single letters are the ones in the grid)
static Box *merge_letters (const Box *comps, int nc, int hm, int *nb) {
    Box *letters = malloc((size_t)nc * sizeof(Box));
    int nl = 0;
    *nb = 0;
    if (!letters) return NULL;

    // we only keep the bunches that look like a letter
    for (int i = 0; i < nc; i++) {
        int bw = comps[i].x1 - comps[i].x0 + 1;
        int bh = comps[i].y1 - comps[i].y0 + 1;
        if (2 * bh >= hm && 10 * bh <= 16 * hm && bw >= 2 && bw <= 2 * hm) {
            letters[nl] = comps[i];
            nl++;
        }
    }
    // at least one slot so that malloc works even with no letter
    int size = nl;
    if (size == 0) size = 1;
    int *uf = uf_new(size);
    Box *blobs = calloc((size_t)size, sizeof(Box));
    if (!uf || !blobs) {
        free_all(letters, uf, blobs, NULL, NULL);
        return NULL;
    }
    // two close letters on the same line are in the same word
    for (int i = 0; i < nl; i++) {
        for (int j = i + 1; j < nl; j++) {
            int y0 = max_int(letters[i].y0, letters[j].y0);
            int y1 = min_int(letters[i].y1, letters[j].y1);
            int x0 = max_int(letters[i].x0, letters[j].x0);
            int x1 = min_int(letters[i].x1, letters[j].x1);
            int overlap = y1 - y0 + 1;
            int gap = x0 - x1 - 1;
            if (2 * overlap >= hm && 5 * gap <= 2 * hm) uf_union(uf, i, j);
        }
    }
    int *slot = malloc((size_t)size * sizeof(int));
    if (!slot) {
        free_all(letters, uf, blobs, NULL, NULL);
        return NULL;
    }
    for (int i = 0; i < nl; i++) slot[i] = -1;
    for (int i = 0; i < nl; i++) {
        int r = uf_find(uf, i);
        if (slot[r] < 0) {
            slot[r] = *nb;
            (*nb)++;
            blobs[slot[r]] = letters[i];
            blobs[slot[r]].count = 0;
        }
        box_union(&blobs[slot[r]], &letters[i]);
        blobs[slot[r]].count++;
    }
    free_all(slot, uf, letters, NULL, NULL);
    return blobs;
}

//grid 

// 1 if the box c surrounds the box lb (within pitch): it is a frame
static int is_frame (const Box *c, const Box *lb, int pitch) {
    if (c->x0 > lb->x0 || c->y0 > lb->y0) return 0;
    if (c->x1 < lb->x1 || c->y1 < lb->y1) return 0;
    if (c->x0 < lb->x0 - pitch || c->y0 < lb->y0 - pitch) return 0;
    if (c->x1 > lb->x1 + pitch || c->y1 > lb->y1 + pitch) return 0;
    return 1;
}

// finds the rectangle of the grid: the biggest group of single letters
// in_grid[i] is 1 for each bunch inside the grid
static int find_grid (const Box *bl, int nb, const Box *comps, int nc, int hm,
                      int img_w, int img_h, unsigned char *in_grid, Box *rect) {
    int *single = NULL;
    int *uf = NULL;
    int *size = NULL;
    int *mem = NULL;
    int *nn = NULL;
    int ns = 0;
    int m = 0;
    int root = 0;

    // we keep the bunches that have only one letter
    single = malloc((size_t)(nb + 1) * sizeof(int));
    if (!single) return 0;
    for (int i = 0; i < nb; i++) {
        if (bl[i].count == 1) single[ns++] = i;
    }
    if (ns < 4) {
        free(single);
        return 0;
    }

    // two neighbours of the grid are less than 3 letter heights apart
    int lim = 32 * hm / 10;
    uf = uf_new(ns);
    size = calloc((size_t)ns, sizeof(int));
    if (!uf || !size) {
        free_all(single, uf, size, mem, nn);
        return 0;
    }
    for (int i = 0; i < ns; i++) {
        for (int j = i + 1; j < ns; j++) {
            if (dist2(&bl[single[i]], &bl[single[j]]) <= lim * lim)
                uf_union(uf, i, j);
        }
    }

    // the biggest group is the grid
    for (int i = 0; i < ns; i++) size[uf_find(uf, i)]++;
    for (int i = 0; i < ns; i++) {
        if (size[i] > size[root]) root = i;
    }
    if (size[root] < 4) {
        free_all(single, uf, size, mem, nn);
        return 0;
    }

    mem = malloc((size_t)size[root] * sizeof(int));
    nn = malloc((size_t)size[root] * sizeof(int));
    if (!mem || !nn) {
        free_all(single, uf, size, mem, nn);
        return 0;
    }
    for (int i = 0; i < ns; i++) {
        if (uf_find(uf, i) == root) mem[m++] = single[i];
    }

    // for each letter of the grid we look for the distance to its neighbour
    Box lb = bl[mem[0]];
    for (int i = 0; i < m; i++) {
        box_union(&lb, &bl[mem[i]]);
        in_grid[mem[i]] = 1;
        int best = -1;
        for (int j = 0; j < m; j++) {
            if (i == j) continue;
            int d = dist2(&bl[mem[i]], &bl[mem[j]]);
            if (best < 0 || d < best) best = d;
        }
        nn[i] = best;
    }

    // pitch = distance between two cells (the median)
    qsort(nn, (size_t)m, sizeof(int), cmp_int);
    int pitch = isqrt(nn[m / 2]);

    // two wide letters stuck together in the grid may have been missed
    // we take them back if they are right next to it
    for (int i = 0; i < nb; i++) {
        if (in_grid[i] || bl[i].count > 3) continue;
        if (center_inside(&bl[i], &lb, 3 * pitch / 4)) box_union(&lb, &bl[i]);
    }

    // small margin, no more so that we only keep the grid
    *rect = lb;
    rect->x0 -= hm / 2;
    rect->y0 -= hm / 2;
    rect->x1 += hm / 2;
    rect->y1 += hm / 2;

    // if the grid has a frame or borders, we keep them
    // (we take the smallest possible frame)
    const Box *frame = NULL;
    for (int i = 0; i < nc; i++) {
        if (!is_frame(&comps[i], &lb, pitch)) continue;
        if (!frame || box_area(&comps[i]) < box_area(frame)) frame = &comps[i];
    }
    if (frame) box_union(rect, frame);

    // we stay inside the image
    if (rect->x0 < 0) rect->x0 = 0;
    if (rect->y0 < 0) rect->y0 = 0;
    if (rect->x1 >= img_w) rect->x1 = img_w - 1;
    if (rect->y1 >= img_h) rect->y1 = img_h - 1;

    // everything inside the rectangle is part of the grid
    for (int i = 0; i < nb; i++) {
        if (center_inside(&bl[i], rect, 0)) in_grid[i] = 1;
    }

    free_all(single, uf, size, mem, nn);
    return 1;
}

//words

static int cmp_x0 (const void *a, const void *b) {
    return ((const Box *)a)->x0 - ((const Box *)b)->x0;
}

// sort by column then from top to bottom
// the column is stored in count (useless for words)
static int cmp_col_y (const void *a, const void *b) {
    const Box *p = a;
    const Box *q = b;
    if (p->count != q->count) return p->count - q->count;
    return p->y0 - q->y0;
}

// keeps the words of the list (outside the grid) and returns their number
// order: column by column, from top to bottom
static int pick_words (const Box *bl, int nb, const unsigned char *in_grid,
                       const Box *rect, int hm, Box *words) {
    // side_of[i]: 0 left, 1 right, 2 top, 3 bottom (-1 if not a word)
    int *side_of = malloc((size_t)(nb + 1) * sizeof(int));
    if (!side_of) return 0;

    // the list is on the side of the grid that has the most bunches
    int side_count[4] = { 0, 0, 0, 0 };
    for (int i = 0; i < nb; i++) {
        side_of[i] = -1;
        // the words are in capitals, smaller text is ignored
        if (in_grid[i] || bl[i].count < 2) continue;
        if (5 * (bl[i].y1 - bl[i].y0 + 1) < 4 * hm) continue;

        // distance between the box and the grid, in x and in y
        int cx = box_cx(&bl[i]);
        int cy = box_cy(&bl[i]);
        int dx = 0;
        int dy = 0;
        if (cx < rect->x0) dx = rect->x0 - cx;
        else if (cx > rect->x1) dx = cx - rect->x1;
        if (cy < rect->y0) dy = rect->y0 - cy;
        else if (cy > rect->y1) dy = cy - rect->y1;

        if (dx > dy) {
            if (cx < rect->x0) side_of[i] = 0;
            else side_of[i] = 1;
        } else {
            if (cy < rect->y0) side_of[i] = 2;
            else side_of[i] = 3;
        }
        side_count[side_of[i]]++;
    }
    int side = 0;
    for (int s = 1; s < 4; s++) {
        if (side_count[s] > side_count[side]) side = s;
    }

    int tol = hm / 2;  // tolerance for the column alignment
    int n = 0;
    for (int i = 0; i < nb; i++) {
        if (side_of[i] != side) continue;
        // the words of a list are left aligned
        // we keep those that have another word in the same column
        int aligned = 0;
        for (int j = 0; j < nb; j++) {
            if (j == i || side_of[j] != side) continue;
            if (abs(bl[j].x0 - bl[i].x0) <= tol) {
                aligned = 1;
                break;
            }
        }
        if (aligned) {
            words[n] = bl[i];
            n++;
        }
    }
    free(side_of);

    // we give a column number to each word
    qsort(words, (size_t)n, sizeof(Box), cmp_x0);
    int col_x = 0;
    for (int i = 0; i < n; i++) {
        if (i == 0 || words[i].x0 - col_x > tol) col_x = words[i].x0;
        words[i].count = col_x;
    }
    qsort(words, (size_t)n, sizeof(Box), cmp_col_y);
    return n;
}

//result 
// copies the area b of src into a new image (with pad pixels around)
static SDL_Surface *crop (SDL_Surface *src, const Box *b, int pad) {
    int x0 = b->x0 - pad;
    int y0 = b->y0 - pad;
    int x1 = b->x1 + pad;
    int y1 = b->y1 + pad;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= src->w) x1 = src->w - 1;
    if (y1 >= src->h) y1 = src->h - 1;
    SDL_Rect r = { x0, y0, x1 - x0 + 1, y1 - y0 + 1 };

    SDL_Surface *dst = SDL_CreateRGBSurfaceWithFormat(0, r.w, r.h, 32,
                                                      SDL_PIXELFORMAT_RGBA32);
    if (!dst) return NULL;
    SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(src, &r, dst, NULL);
    return dst;
}

static void save_all (const Split *s, const char *dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/grid.png", dir);
    if (IMG_SavePNG(s->grid, path) != 0)
        fprintf(stderr, "split: could not write %s\n", path);
    for (int i = 0; i < s->word_count; i++) {
        snprintf(path, sizeof(path), "%s/word%d.png", dir, i + 1);
        if (IMG_SavePNG(s->words[i], path) != 0)
            fprintf(stderr, "split: could not write %s\n", path);
    }
}

void split_free (Split *s) {
    if (!s) return;
    SDL_FreeSurface(s->grid);
    for (int i = 0; i < s->word_count; i++) SDL_FreeSurface(s->words[i]);
    free(s->words);
    free(s);
}

// builds the result: the grid and one image per word
static Split *build_split (SDL_Surface *img, const Box *rect, const Box *words,
                           int nw, const char *out_dir) {
    Split *res = calloc(1, sizeof(Split));
    if (!res) return NULL;

    res->grid = crop(img, rect, 0);
    res->words = malloc((size_t)(nw + 1) * sizeof(SDL_Surface *));
    if (!res->grid || !res->words) {
        split_free(res);
        return NULL;
    }
    for (int i = 0; i < nw; i++) {
        res->words[i] = crop(img, &words[i], 2);
        res->word_count++;
        if (!res->words[i]) {
            split_free(res);
            return NULL;
        }
    }
    if (out_dir) save_all(res, out_dir);
    return res;
}

Split *split_image (const char *path, const char *out_dir) {
    SDL_Surface *img = load_flat(path);
    if (!img) {
        fprintf(stderr, "split: could not load %s\n", path);
        return NULL;
    }

    Split *res = NULL;
    Box *comps = NULL;
    Box *blobs = NULL;
    unsigned char *in_grid = NULL;
    Box *words = NULL;
    Box rect = { 0, 0, 0, 0, 0 };
    int nc = 0;
    int nb = 0;
    int hm = 0;  // height of a letter

    // we go step by step, if a step fails we stop there
    unsigned char *ink = binarize(img);
    if (ink) comps = find_components(ink, img->w, img->h, &nc);
    free(ink);
    if (comps) hm = letter_height(comps, nc, img->h);
    if (hm) blobs = merge_letters(comps, nc, hm, &nb);
    if (blobs) {
        in_grid = calloc((size_t)(nb + 1), 1);
        words = malloc((size_t)(nb + 1) * sizeof(Box));
    }

    int found = 0;
    if (in_grid && words) {
        found = find_grid(blobs, nb, comps, nc, hm, img->w, img->h, in_grid,
                          &rect);
    }

    if (!found) {
        fprintf(stderr, "split: no grid found in %s\n", path);
    } else {
        int nw = pick_words(blobs, nb, in_grid, &rect, hm, words);
        res = build_split(img, &rect, words, nw, out_dir);
    }

    free_all(comps, blobs, in_grid, words, NULL);
    SDL_FreeSurface(img);
    return res;
}
