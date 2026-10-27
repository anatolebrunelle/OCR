// decoupe une image de mots caches : la grille et les mots de la liste
// une lettre = un paquet de pixels noirs colles ensemble
// dans la liste les lettres sont serrees (elles forment des mots)
// dans la grille elles sont espacees (une par case)
//
#include "../include/split.h"
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int x0, y0, x1, y1;
    int count;  // nombre de lettres dans la boite
} Box;

static int box_cx(const Box *b)
{
    return (b->x0 + b->x1) / 2;
}

static int box_cy(const Box *b)
{
    return (b->y0 + b->y1) / 2;
}

static void box_union(Box *a, const Box *b)
{
    if (b->x0 < a->x0)
        a->x0 = b->x0;
    if (b->y0 < a->y0)
        a->y0 = b->y0;
    if (b->x1 > a->x1)
        a->x1 = b->x1;
    if (b->y1 > a->y1)
        a->y1 = b->y1;
}

static int cmp_int(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

static int isqrt(int n)
{
    int r = 0;
    while ((r + 1) * (r + 1) <= n)
        r++;
    return r;
}

static int *uf_new(int n)
{
    int *p = malloc((size_t)n * sizeof(int));
    for (int i = 0; p && i < n; i++)
        p[i] = i;
    return p;
}

static int uf_find(int *p, int i)
{
    while (p[i] != i)
    {
        p[i] = p[p[i]];
        i = p[i];
    }
    return i;
}

static void uf_union(int *p, int a, int b)
{
    p[uf_find(p, a)] = uf_find(p, b);
}

// charge l'image et met un fond blanc a la place de la transparence
static SDL_Surface *load_flat(const char *path)
{
    SDL_Surface *raw = IMG_Load(path);
    if (!raw)
        return NULL;
    SDL_Surface *s = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (!s)
        return NULL;

    for (int y = 0; y < s->h; y++)
    {
        unsigned char *row = (unsigned char *)s->pixels + y * s->pitch;
        for (int x = 0; x < s->w; x++)
        {
            unsigned char *p = row + 4 * x;
            for (int c = 0; c < 3; c++)
                p[c] = (unsigned char)((p[c] * p[3] + 255 * (255 - p[3]))
                                       / 255);
            p[3] = 255;
        }
    }
    return s;
}

static int otsu(const long *hist, long total)
{
    long sum = 0;
    for (int i = 0; i < 256; i++)
        sum += i * hist[i];

    long w0 = 0, sum0 = 0;
    double best = -1;
    int thr = 127;
    for (int t = 0; t < 256; t++)
    {
        w0 += hist[t];
        if (w0 == 0)
            continue;
        long w1 = total - w0;
        if (w1 == 0)
            break;
        sum0 += t * hist[t];
        double m0 = (double)sum0 / (double)w0;
        double m1 = (double)(sum - sum0) / (double)w1;
        double var = (double)w0 * (double)w1 * (m0 - m1) * (m0 - m1);
        if (var > best)
        {
            best = var;
            thr = t;
        }
    }
    return thr;
}

// ink = 1 pour les pixels sombres
// on prend le plus petit des canaux rgb pour que le texte colore compte
static unsigned char *binarize(const SDL_Surface *s)
{
    size_t n = (size_t)s->w * (size_t)s->h;
    unsigned char *ink = malloc(n);
    unsigned char *dark = malloc(n);
    if (!ink || !dark)
    {
        free(ink);
        free(dark);
        return NULL;
    }

    long hist[256] = { 0 };
    for (int y = 0; y < s->h; y++)
    {
        const unsigned char *row =
            (const unsigned char *)s->pixels + y * s->pitch;
        for (int x = 0; x < s->w; x++)
        {
            const unsigned char *p = row + 4 * x;
            unsigned char m = p[0] < p[1] ? p[0] : p[1];
            m = m < p[2] ? m : p[2];
            dark[(size_t)y * (size_t)s->w + (size_t)x] = m;
            hist[m]++;
        }
    }

    int thr = otsu(hist, (long)n);
    for (size_t i = 0; i < n; i++)
        ink[i] = dark[i] <= thr;
    free(dark);
    return ink;
}

static Box *find_components(const unsigned char *ink, int w, int h, int *n)
{
    size_t size = (size_t)w * (size_t)h;
    unsigned char *seen = calloc(size, 1);
    int *stack = malloc(size * sizeof(int));
    int cap = 256;
    Box *v = malloc((size_t)cap * sizeof(Box));
    *n = 0;
    if (!seen || !stack || !v)
    {
        free(seen);
        free(stack);
        free(v);
        return NULL;
    }

    for (int start = 0; start < (int)size; start++)
    {
        if (!ink[start] || seen[start])
            continue;

        Box b = { start % w, start / w, start % w, start / w, 1 };
        int top = 0;
        stack[top++] = start;
        seen[start] = 1;
        while (top > 0)
        {
            int cur = stack[--top];
            int cx = cur % w, cy = cur / w;
            if (cx < b.x0)
                b.x0 = cx;
            if (cx > b.x1)
                b.x1 = cx;
            if (cy < b.y0)
                b.y0 = cy;
            if (cy > b.y1)
                b.y1 = cy;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                {
                    int nx = cx + dx, ny = cy + dy;
                    if (nx < 0 || ny < 0 || nx >= w || ny >= h)
                        continue;
                    int ni = ny * w + nx;
                    if (ink[ni] && !seen[ni])
                    {
                        seen[ni] = 1;
                        stack[top++] = ni;
                    }
                }
        }

        if (*n == cap)
        {
            cap *= 2;
            Box *tmp = realloc(v, (size_t)cap * sizeof(Box));
            if (!tmp)
            {
                free(v);
                free(seen);
                free(stack);
                return NULL;
            }
            v = tmp;
        }
        v[(*n)++] = b;
    }
    free(seen);
    free(stack);
    return v;
}

static int letter_height(const Box *c, int n, int img_h)
{
    int *hs = malloc((size_t)n * sizeof(int));
    int m = 0;
    if (!hs)
        return 0;
    for (int i = 0; i < n; i++)
    {
        int bw = c[i].x1 - c[i].x0 + 1;
        int bh = c[i].y1 - c[i].y0 + 1;
        if (bh >= 6 && bh <= img_h / 8 && bw >= 2 && bw <= 3 * bh)
            hs[m++] = bh;
    }
    int res = 0;
    if (m > 0)
    {
        qsort(hs, (size_t)m, sizeof(int), cmp_int);
        res = hs[m / 2];
    }
    free(hs);
    return res;
}

// colle les lettres proches : un paquet = un mot, ou une lettre seule
// (les lettres seules sont celles de la grille)
static Box *merge_letters(const Box *comps, int nc, int hm, int *nb)
{
    Box *letters = malloc((size_t)nc * sizeof(Box));
    int nl = 0;
    *nb = 0;
    if (!letters)
        return NULL;
    for (int i = 0; i < nc; i++)
    {
        int bw = comps[i].x1 - comps[i].x0 + 1;
        int bh = comps[i].y1 - comps[i].y0 + 1;
        if (2 * bh >= hm && 10 * bh <= 16 * hm && bw >= 2 && bw <= 2 * hm)
            letters[nl++] = comps[i];
    }

    int *uf = uf_new(nl > 0 ? nl : 1);
    Box *blobs = calloc((size_t)(nl > 0 ? nl : 1), sizeof(Box));
    if (!uf || !blobs)
    {
        free(letters);
        free(uf);
        free(blobs);
        return NULL;
    }

    for (int i = 0; i < nl; i++)
        for (int j = i + 1; j < nl; j++)
        {
            int y0 = letters[i].y0 > letters[j].y0 ? letters[i].y0
                                                   : letters[j].y0;
            int y1 = letters[i].y1 < letters[j].y1 ? letters[i].y1
                                                   : letters[j].y1;
            int x0 = letters[i].x0 > letters[j].x0 ? letters[i].x0
                                                   : letters[j].x0;
            int x1 = letters[i].x1 < letters[j].x1 ? letters[i].x1
                                                   : letters[j].x1;
            int overlap = y1 - y0 + 1;
            int gap = x0 - x1 - 1;
            if (2 * overlap >= hm && 5 * gap <= 2 * hm)
                uf_union(uf, i, j);
        }

    int *slot = malloc((size_t)(nl > 0 ? nl : 1) * sizeof(int));
    if (!slot)
    {
        free(letters);
        free(uf);
        free(blobs);
        return NULL;
    }
    for (int i = 0; i < nl; i++)
        slot[i] = -1;
    for (int i = 0; i < nl; i++)
    {
        int r = uf_find(uf, i);
        if (slot[r] < 0)
        {
            slot[r] = (*nb)++;
            blobs[slot[r]] = letters[i];
            blobs[slot[r]].count = 0;
        }
        box_union(&blobs[slot[r]], &letters[i]);
        blobs[slot[r]].count++;
    }
    free(slot);
    free(uf);
    free(letters);
    return blobs;
}

// trouve le rectangle de la grille : le plus gros groupe de lettres seules
// in_grid[i] vaut 1 pour chaque paquet dans la grille
static int find_grid(const Box *bl, int nb, const Box *comps, int nc, int hm,
                     int img_w, int img_h, unsigned char *in_grid, Box *rect)
{
    int *single = malloc((size_t)(nb > 0 ? nb : 1) * sizeof(int));
    int ns = 0;
    if (!single)
        return 0;
    for (int i = 0; i < nb; i++)
        if (bl[i].count == 1)
            single[ns++] = i;
    if (ns < 4)
    {
        free(single);
        return 0;
    }

    // deux voisines de la grille sont a moins de 3 hauteurs de lettre
    int *uf = uf_new(ns);
    int *size = calloc((size_t)ns, sizeof(int));
    int lim = 32 * hm / 10;
    if (!uf || !size)
    {
        free(single);
        free(uf);
        free(size);
        return 0;
    }
    for (int i = 0; i < ns; i++)
        for (int j = i + 1; j < ns; j++)
        {
            int dx = box_cx(&bl[single[i]]) - box_cx(&bl[single[j]]);
            int dy = box_cy(&bl[single[i]]) - box_cy(&bl[single[j]]);
            if (dx * dx + dy * dy <= lim * lim)
                uf_union(uf, i, j);
        }
    int root = 0;
    for (int i = 0; i < ns; i++)
        size[uf_find(uf, i)]++;
    for (int i = 0; i < ns; i++)
        if (size[i] > size[root])
            root = i;
    if (size[root] < 4)
    {
        free(single);
        free(uf);
        free(size);
        return 0;
    }

    // pitch = distance entre deux cases
    int *mem = malloc((size_t)size[root] * sizeof(int));
    int *nn = malloc((size_t)size[root] * sizeof(int));
    if (!mem || !nn)
    {
        free(single);
        free(uf);
        free(size);
        free(mem);
        free(nn);
        return 0;
    }
    int m = 0;
    for (int i = 0; i < ns; i++)
        if (uf_find(uf, i) == root)
            mem[m++] = single[i];

    Box lb = bl[mem[0]];
    for (int i = 0; i < m; i++)
    {
        box_union(&lb, &bl[mem[i]]);
        in_grid[mem[i]] = 1;
        int best = -1;
        for (int j = 0; j < m; j++)
        {
            if (i == j)
                continue;
            int dx = box_cx(&bl[mem[i]]) - box_cx(&bl[mem[j]]);
            int dy = box_cy(&bl[mem[i]]) - box_cy(&bl[mem[j]]);
            int d = dx * dx + dy * dy;
            if (best < 0 || d < best)
                best = d;
        }
        nn[i] = best;
    }
    qsort(nn, (size_t)m, sizeof(int), cmp_int);
    int pitch = isqrt(nn[m / 2]);

    // deux lettres larges collees dans la grille ont pu etre ratees
    // on les reprend si elles sont juste a cote
    for (int i = 0; i < nb; i++)
        if (!in_grid[i] && bl[i].count <= 3
            && box_cx(&bl[i]) >= lb.x0 - 3 * pitch / 4
            && box_cx(&bl[i]) <= lb.x1 + 3 * pitch / 4
            && box_cy(&bl[i]) >= lb.y0 - 3 * pitch / 4
            && box_cy(&bl[i]) <= lb.y1 + 3 * pitch / 4)
            box_union(&lb, &bl[i]);

    // petite marge, pas plus pour ne garder que la grille
    *rect = lb;
    rect->x0 -= hm / 2;
    rect->y0 -= hm / 2;
    rect->x1 += hm / 2;
    rect->y1 += hm / 2;

    // si la grille a un cadre ou des bordures, on les garde
    const Box *frame = NULL;
    for (int i = 0; i < nc; i++)
    {
        const Box *c = &comps[i];
        if (c->x0 <= lb.x0 && c->y0 <= lb.y0 && c->x1 >= lb.x1
            && c->y1 >= lb.y1 && c->x0 >= lb.x0 - pitch
            && c->y0 >= lb.y0 - pitch && c->x1 <= lb.x1 + pitch
            && c->y1 <= lb.y1 + pitch)
            if (!frame || (c->x1 - c->x0) * (c->y1 - c->y0)
                              < (frame->x1 - frame->x0)
                                    * (frame->y1 - frame->y0))
                frame = c;
    }
    if (frame)
        box_union(rect, frame);

    if (rect->x0 < 0)
        rect->x0 = 0;
    if (rect->y0 < 0)
        rect->y0 = 0;
    if (rect->x1 >= img_w)
        rect->x1 = img_w - 1;
    if (rect->y1 >= img_h)
        rect->y1 = img_h - 1;

    for (int i = 0; i < nb; i++)
        if (box_cx(&bl[i]) >= rect->x0 && box_cx(&bl[i]) <= rect->x1
            && box_cy(&bl[i]) >= rect->y0 && box_cy(&bl[i]) <= rect->y1)
            in_grid[i] = 1;

    free(single);
    free(uf);
    free(size);
    free(mem);
    free(nn);
    return 1;
}

static int cmp_x0(const void *a, const void *b)
{
    return ((const Box *)a)->x0 - ((const Box *)b)->x0;
}

// tri par colonne puis de haut en bas
// la colonne est stockee dans count (inutile pour les mots)
static int cmp_col_y(const void *a, const void *b)
{
    const Box *p = a, *q = b;
    if (p->count != q->count)
        return p->count - q->count;
    return p->y0 - q->y0;
}

// garde les mots de la liste (hors grille) et renvoie leur nombre
// ordre : colonne par colonne, de haut en bas
static int pick_words(const Box *bl, int nb, const unsigned char *in_grid,
                      const Box *rect, int hm, Box *words)
{
    // la liste est du cote de la grille qui a le plus de paquets
    int side_count[4] = { 0, 0, 0, 0 };
    int side_of[nb > 0 ? nb : 1];
    for (int i = 0; i < nb; i++)
    {
        side_of[i] = -1;
        // les mots sont en majuscules, le texte plus petit est ignore
        if (in_grid[i] || bl[i].count < 2
            || 5 * (bl[i].y1 - bl[i].y0 + 1) < 4 * hm)
            continue;
        int cx = box_cx(&bl[i]), cy = box_cy(&bl[i]);
        int dx = cx < rect->x0 ? rect->x0 - cx : (cx > rect->x1 ? cx - rect->x1
                                                                : 0);
        int dy = cy < rect->y0 ? rect->y0 - cy : (cy > rect->y1 ? cy - rect->y1
                                                                : 0);
        if (dx > dy)
            side_of[i] = cx < rect->x0 ? 0 : 1;
        else
            side_of[i] = cy < rect->y0 ? 2 : 3;
        side_count[side_of[i]]++;
    }
    int side = 0;
    for (int s = 1; s < 4; s++)
        if (side_count[s] > side_count[side])
            side = s;

    int tol = hm / 2;  // tolerance pour l'alignement des colonnes
    int n = 0;
    for (int i = 0; i < nb; i++)
    {
        if (side_of[i] != side)
            continue;
        // les mots d'une liste sont alignes a gauche
        // on garde ceux qui ont un autre mot dans la meme colonne
        int aligned = 0;
        for (int j = 0; j < nb && !aligned; j++)
            if (j != i && side_of[j] == side
                && abs(bl[j].x0 - bl[i].x0) <= tol)
                aligned = 1;
        if (aligned)
            words[n++] = bl[i];
    }

    qsort(words, (size_t)n, sizeof(Box), cmp_x0);
    int col_x = 0;
    for (int i = 0; i < n; i++)
    {
        if (i == 0 || words[i].x0 - col_x > tol)
            col_x = words[i].x0;
        words[i].count = col_x;
    }
    qsort(words, (size_t)n, sizeof(Box), cmp_col_y);
    return n;
}

static SDL_Surface *crop(SDL_Surface *src, const Box *b, int pad)
{
    int x0 = b->x0 - pad < 0 ? 0 : b->x0 - pad;
    int y0 = b->y0 - pad < 0 ? 0 : b->y0 - pad;
    int x1 = b->x1 + pad >= src->w ? src->w - 1 : b->x1 + pad;
    int y1 = b->y1 + pad >= src->h ? src->h - 1 : b->y1 + pad;
    SDL_Rect r = { x0, y0, x1 - x0 + 1, y1 - y0 + 1 };

    SDL_Surface *dst = SDL_CreateRGBSurfaceWithFormat(0, r.w, r.h, 32,
                                                      SDL_PIXELFORMAT_RGBA32);
    if (!dst)
        return NULL;
    SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_NONE);
    SDL_BlitSurface(src, &r, dst, NULL);
    return dst;
}

static void save_all(const Split *s, const char *dir)
{
    char path[512];
    snprintf(path, sizeof(path), "%s/grid.png", dir);
    if (IMG_SavePNG(s->grid, path) != 0)
        fprintf(stderr, "split: could not write %s\n", path);
    for (int i = 0; i < s->word_count; i++)
    {
        snprintf(path, sizeof(path), "%s/word%d.png", dir, i + 1);
        if (IMG_SavePNG(s->words[i], path) != 0)
            fprintf(stderr, "split: could not write %s\n", path);
    }
}

void split_free(Split *s)
{
    if (!s)
        return;
    SDL_FreeSurface(s->grid);
    for (int i = 0; i < s->word_count; i++)
        SDL_FreeSurface(s->words[i]);
    free(s->words);
    free(s);
}

Split *split_image(const char *path, const char *out_dir)
{
    SDL_Surface *img = load_flat(path);
    if (!img)
    {
        fprintf(stderr, "split: could not load %s\n", path);
        return NULL;
    }

    Split *res = NULL;
    unsigned char *ink = binarize(img);
    int nc = 0, nb = 0;
    Box *comps = ink ? find_components(ink, img->w, img->h, &nc) : NULL;
    free(ink);
    int hm = comps ? letter_height(comps, nc, img->h) : 0;
    Box *blobs = hm ? merge_letters(comps, nc, hm, &nb) : NULL;
    unsigned char *in_grid = blobs ? calloc((size_t)(nb + 1), 1) : NULL;
    Box *words = blobs ? malloc((size_t)(nb + 1) * sizeof(Box)) : NULL;
    Box rect;

    if (!in_grid || !words
        || !find_grid(blobs, nb, comps, nc, hm, img->w, img->h, in_grid, &rect))
    {
        fprintf(stderr, "split: no grid found in %s\n", path);
        goto end;
    }

    res = calloc(1, sizeof(Split));
    if (!res)
        goto end;
    res->grid = crop(img, &rect, 0);
    int nw = pick_words(blobs, nb, in_grid, &rect, hm, words);
    res->words = malloc((size_t)(nw + 1) * sizeof(SDL_Surface *));
    if (!res->grid || !res->words)
    {
        split_free(res);
        res = NULL;
        goto end;
    }
    for (int i = 0; i < nw; i++)
    {
        res->words[i] = crop(img, &words[i], 2);
        res->word_count++;
        if (!res->words[i])
        {
            split_free(res);
            res = NULL;
            goto end;
        }
    }
    if (out_dir)
        save_all(res, out_dir);

end:
    free(comps);
    free(blobs);
    free(in_grid);
    free(words);
    SDL_FreeSurface(img);
    return res;
}
