#include "../include/cut.h"

/* ---------- pixel part ---------- */
//basicaly  get_pixel
Uint32 pix(const SDL_Surface *s, int x, int y)
{
    const Uint32 *p = s->pixels;

    return p[y * (s->pitch / 4) + x];
}

/* seen = already visited, stk = stack, col = colour */
// fl -> flood fill -> research with neighbours
struct rect fl(const SDL_Surface *s, unsigned char *seen, int *stk, int x,
               int y)
{
    const int dx[4] = { 1, -1, 0, 0 };
    const int dy[4] = { 0, 0, 1, -1 };
    struct rect r = { x, y, x, y };
    Uint32 col = pix(s, x, y);
    int top = 0;
    stk[top++] = y * s->w + x;
    seen[y * s->w + x] = 1;
    while (top > 0)
    {
        int id = stk[--top];
        int cx = id % s->w; /* cx = current x */
        int cy = id / s->w; /* cy = current y */
        int i;

        if (cx < r.x1)
            r.x1 = cx;
        if (cx > r.x2)
            r.x2 = cx;
        if (cy < r.y1)
            r.y1 = cy;
        if (cy > r.y2)
            r.y2 = cy;
        for (i = 0; i < 4; i++)
        {
            int nx = cx + dx[i]; /* nx = next x */
            int ny = cy + dy[i]; /* ny = next y */

            if (nx < 0 || ny < 0 || nx >= s->w || ny >= s->h)
                continue;
            if (seen[ny * s->w + nx] || pix(s, nx, ny) != col)
                continue;
            seen[ny * s->w + nx] = 1;
            stk[top++] = ny * s->w + nx;
        }
    }
    return r;
}
// mark pixels visited
void mark(const SDL_Surface *s, unsigned char *seen, struct rect r)
{
    int x;
    int y;

    for (y = r.y1; y <= r.y2; y++)
        for (x = r.x1; x <= r.x2; x++)
            seen[y * s->w + x] = 1;
}

//save result

int sv(SDL_Surface *s, struct rect r, int n)
{
    char path[64];
    SDL_Rect area; /* area = zone to copy */
    SDL_Surface *out;
    int ret;

    area.x = r.x1;
    area.y = r.y1;
    area.w = r.x2 - r.x1 + 1;
    area.h = r.y2 - r.y1 + 1;
    out = SDL_CreateRGBSurfaceWithFormat(0, area.w, area.h, 32,
                                         SDL_PIXELFORMAT_ARGB8888);
    if (out == NULL)
        return -1;
    SDL_SetSurfaceBlendMode(s, SDL_BLENDMODE_NONE);
    snprintf(path, sizeof(path), DIR "/word%d.png", n);
    ret = SDL_BlitSurface(s, &area, out, NULL);
    if (ret == 0)
        ret = IMG_SavePNG(out, path);
    SDL_FreeSurface(out);
    return ret == 0 ? 0 : -1;
}
// is colour : return 0 if is opaque
int isc(const SDL_Surface *s, int x, int y)
{
    Uint8 r; /* r = red */
    Uint8 g; /* g = green */
    Uint8 b; /* b = blue */
    Uint8 a; /* a = alpha */

    SDL_GetRGBA(pix(s, x, y), s->format, &r, &g, &b, &a);
    return a != 0 && !(r == g && g == b);
}

// remove the coloured rectangle after detection
struct rect inn(const SDL_Surface *s, struct rect r)
{
    Uint32 col = pix(s, r.x1, r.y1); /* col = colour of the frame */
    struct rect in = r;              /* in = inner rectangle */
    int t = 0;                       /* t = thickness of the frame */

    while (r.x1 + t <= r.x2 && r.y1 + t <= r.y2
           && pix(s, r.x1 + t, r.y1 + t) == col)
        t++;
    in.x1 += t;
    in.y1 += t;
    in.x2 -= t;
    in.y2 -= t;
    if (in.x1 > in.x2 || in.y1 > in.y2)
        return r;
    return in;
}

int scan(SDL_Surface *s)
{
    unsigned char *seen;
    int *stk;
    int n = 0;
    int ret = 0;
    int x;
    int y;

    seen = calloc((size_t)s->w * (size_t)s->h, 1);
    stk = malloc((size_t)s->w * (size_t)s->h * sizeof(int));
    if (seen == NULL || stk == NULL)
        ret = -1;
    for (y = 0; ret == 0 && y < s->h; y++)
    {
        for (x = 0; ret == 0 && x < s->w; x++)
        {
            struct rect r; /* r = rectangle found */

            if (seen[y * s->w + x] || !isc(s, x, y))
                continue;
            r = fl(s, seen, stk, x, y);
            mark(s, seen, r);
            n++;
            ret = sv(s, inn(s, r), n);
        }
    }
    free(seen);
    free(stk);
    return ret;
}

// main function called in main
int cut(void)
{
    SDL_Surface *raw; /* raw = image as loaded */
    SDL_Surface *img; /* img = image converted */
    int ret = -1;

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        fprintf(stderr, SRC " failed\n");
        return -1;
    }
    raw = IMG_Load(SRC);
    if (raw != NULL)
    {
        img = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(raw);
        if (img != NULL)
        {
            if (system("mkdir -p " DIR) == 0)
                ret = scan(img);
            SDL_FreeSurface(img);
        }
    }
    IMG_Quit();
    if (ret != 0)
        fprintf(stderr, SRC " failed\n");
    return ret;
}

int main(void){
    cut();
    return 0;
    
}
