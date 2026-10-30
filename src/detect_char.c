#include "../include/detect_char.h"

/* ---------- Loading and pixel access ---------- */

// Loads any PNG and converts it to RGBA32 (NULL on error)
SDL_Surface *loadimg(const char *path)
{
    SDL_Surface *raw = IMG_Load(path); // image as stored in the file
    if (raw == NULL)
    {
        fprintf(stderr, "IMG_Load: %s\n", IMG_GetError());
        return NULL;
    }
    SDL_Surface *img = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    if (img == NULL)
        fprintf(stderr, "SDL_ConvertSurfaceFormat: %s\n", SDL_GetError());
    return img;
}

// Raw value of the pixel (x, y) of an RGBA32 surface
Uint32 getpix(const SDL_Surface *img, int x, int y)
{
    const Uint8 *p = (const Uint8 *)img->pixels + y * img->pitch + x * 4;
    Uint32 val = 0; // pixel value

    memcpy(&val, p, 4);
    return val;
}

// Map of the image: 1 where the gray level is under thr, 0 otherwise
Uint8 *blackmap(const SDL_Surface *img, int thr)
{
    int w = img->w; // width
    int h = img->h; // height
    Uint8 *blk = calloc((size_t)w * (size_t)h, 1); // black map

    if (blk == NULL)
        return NULL;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            Uint8 r; // red
            Uint8 g; // green
            Uint8 b; // blue
            SDL_GetRGB(getpix(img, x, y), img->format, &r, &g, &b);
            blk[y * w + x] = r < thr;
        }
    }
    return blk;
}

/* ---------- Grid lines and columns ---------- */

// Longest horizontal run of black pixels in row y
int runrow(const Uint8 *blk, int w, int y)
{
    int best = 0; // longest run so far
    int cur = 0;  // current run

    for (int x = 0; x < w; x++)
    {
        cur = blk[y * w + x] ? cur + 1 : 0;
        if (cur > best)
            best = cur;
    }
    return best;
}

// Longest vertical run of black pixels in column x
int runcol(const Uint8 *blk, int w, int h, int x)
{
    int best = 0; // longest run so far
    int cur = 0;  // current run

    for (int y = 0; y < h; y++)
    {
        cur = blk[y * w + x] ? cur + 1 : 0;
        if (cur > best)
            best = cur;
    }
    return best;
}

void freelines(struct line *lns)
{
    while (lns != NULL)
    {
        struct line *nx = lns->next; // next
        free(lns);
        lns = nx;
    }
}

void freecols(struct col *cls)
{
    while (cls != NULL)
    {
        struct col *nx = cls->next; // next
        free(cls);
        cls = nx;
    }
}

// Horizontal lines: rows whose black run covers at least LRATIO% of the width
struct line *findlines(const Uint8 *blk, int w, int h)
{
    struct line *head = NULL; // list head

    // Go backwards so that the list ends up sorted by ascending y
    for (int y = h - 1; y >= 0; y--)
    {
        if (runrow(blk, w, y) * 100 < w * LRATIO)
            continue;
        struct line *nw = malloc(sizeof(struct line)); // new node
        if (nw == NULL)
        {
            fprintf(stderr, "findlines: out of memory\n");
            freelines(head);
            return NULL;
        }
        nw->y = y;
        nw->next = head;
        head = nw;
    }
    return head;
}

// Vertical lines: columns whose black run covers at least LRATIO% of the height
struct col *findcols(const Uint8 *blk, int w, int h)
{
    struct col *head = NULL; // list head

    // Go backwards so that the list ends up sorted by ascending x
    for (int x = w - 1; x >= 0; x--)
    {
        if (runcol(blk, w, h, x) * 100 < h * LRATIO)
            continue;
        struct col *nw = malloc(sizeof(struct col)); // new node
        if (nw == NULL)
        {
            fprintf(stderr, "findcols: out of memory\n");
            freecols(head);
            return NULL;
        }
        nw->x = x;
        nw->next = head;
        head = nw;
    }
    return head;
}

// Removes the grid from the black map blk. Lines are searched in a second
// map made with the looser LINEMAX threshold, since they may be light gray.
void cleargrid(Uint8 *blk, const SDL_Surface *img)
{
    int w = img->w; // width
    int h = img->h; // height
    Uint8 *lmap = blackmap(img, LINEMAX); // everything that is not white
    if (lmap == NULL)
    {
        fprintf(stderr, "cleargrid: out of memory\n");
        return;
    }
    struct line *lns = findlines(lmap, w, h); // horizontal lines
    struct col *cls = findcols(lmap, w, h);   // vertical lines
    free(lmap);

    for (const struct line *l = lns; l != NULL; l = l->next)
        for (int x = 0; x < w; x++)
            blk[l->y * w + x] = 0;
    for (const struct col *c = cls; c != NULL; c = c->next)
        for (int y = 0; y < h; y++)
            blk[y * w + c->x] = 0;
    freelines(lns);
    freecols(cls);
}

/* ---------- Colors and drawing ---------- */

// Picks a palette color that no frame already drawn near rc uses.
// "Near" = within one character size around rc. The surface itself is read,
// so there is no need to remember the previous rects.
void pickcolor(const SDL_Surface *img, struct rect *rc, int n)
{
    const Uint8 pal[NPAL][3] = {
        { 230, 25, 75 },  { 60, 180, 75 },  { 0, 130, 200 }, { 245, 130, 48 },
        { 145, 30, 180 }, { 0, 128, 128 },  { 240, 50, 230 }, { 170, 110, 40 },
        { 128, 128, 0 },  { 255, 200, 0 },  { 0, 190, 255 },  { 128, 0, 0 }
    };
    int used[NPAL] = { 0 };  // used[i] = 1 if pal[i] is found nearby
    int sw = rc->x2 - rc->x1; // character width - 1
    int sh = rc->y2 - rc->y1; // character height - 1
    int mg = sw > sh ? sw : sh; // margin: the biggest side
    int xa = rc->x1 - mg;     // left of the zone
    int ya = rc->y1 - mg;     // top of the zone
    int xb = rc->x2 + mg;     // right of the zone
    int yb = rc->y2 + mg;     // bottom of the zone

    if (xa < 0)
        xa = 0;
    if (ya < 0)
        ya = 0;
    if (xb > img->w - 1)
        xb = img->w - 1;
    if (yb > img->h - 1)
        yb = img->h - 1;
    for (int y = ya; y <= yb; y++)
    {
        for (int x = xa; x <= xb; x++)
        {
            Uint8 r; // red
            Uint8 g; // green
            Uint8 b; // blue
            SDL_GetRGB(getpix(img, x, y), img->format, &r, &g, &b);
            if (r == g && g == b) // black, white or gray: not a frame
                continue;
            for (int i = 0; i < NPAL; i++)
                if (pal[i][0] == r && pal[i][1] == g && pal[i][2] == b)
                    used[i] = 1;
        }
    }
    // Start at n so colors vary; if every color is taken nearby, reuse one
    int k = n % NPAL; // chosen palette index
    for (int i = 0; i < NPAL; i++)
    {
        if (!used[(n + i) % NPAL])
        {
            k = (n + i) % NPAL;
            break;
        }
    }
    rc->r = pal[k][0];
    rc->g = pal[k][1];
    rc->b = pal[k][2];
}

// Draws the frame of rc with the renderer
void drawrect(SDL_Renderer *rdr, const struct rect *rc)
{
    SDL_Rect out = {                       // outer rectangle of the frame
        rc->x1 - PAD, rc->y1 - PAD,
        rc->x2 - rc->x1 + 1 + 2 * PAD, rc->y2 - rc->y1 + 1 + 2 * PAD
    };

    SDL_SetRenderDrawColor(rdr, rc->r, rc->g, rc->b, SDL_ALPHA_OPAQUE);
    for (int t = 0; t < THK; t++)
    {
        SDL_Rect in = { out.x + t, out.y + t, out.w - 2 * t, out.h - 2 * t };
        SDL_RenderDrawRect(rdr, &in);
    }
    // Draw commands are queued: flush so the pixels are really on the
    // surface before pickcolor reads them for the next character
    SDL_RenderFlush(rdr);
}

/* ---------- Blobs of black pixels ---------- */

// Flood fill (8 neighbours) from (sx, sy). Clears the visited pixels and
// stores the bounding box of the blob in rc. stk is a scratch stack of w*h ints.
void flood(Uint8 *blk, int w, int h, int *stk, int sx, int sy, struct rect *rc)
{
    int top = 0; // stack size

    rc->x1 = sx;
    rc->y1 = sy;
    rc->x2 = sx;
    rc->y2 = sy;
    blk[sy * w + sx] = 0;
    stk[top++] = sy * w + sx;
    while (top > 0)
    {
        int p = stk[--top]; // current pixel index
        int px = p % w;     // current x
        int py = p / w;     // current y

        if (px < rc->x1)
            rc->x1 = px;
        if (px > rc->x2)
            rc->x2 = px;
        if (py < rc->y1)
            rc->y1 = py;
        if (py > rc->y2)
            rc->y2 = py;
        for (int dy = -1; dy <= 1; dy++)
        {
            for (int dx = -1; dx <= 1; dx++)
            {
                int nx = px + dx; // neighbour x
                int ny = py + dy; // neighbour y
                if (nx < 0 || ny < 0 || nx >= w || ny >= h)
                    continue;
                if (blk[ny * w + nx])
                {
                    // Cleared when pushed: each pixel is pushed at most once
                    blk[ny * w + nx] = 0;
                    stk[top++] = ny * w + nx;
                }
            }
        }
    }
}

// Frames every blob of black pixels. Returns their number, -1 on error.
int scan(SDL_Renderer *rdr, SDL_Surface *img, Uint8 *blk)
{
    int w = img->w; // width
    int h = img->h; // height
    int n = 0;      // number of characters found
    int *stk = malloc((size_t)w * (size_t)h * sizeof(int)); // scratch stack
    struct rect rc = { 0, 0, 0, 0, 0, 0, 0 }; // the one rect, reused

    if (stk == NULL)
        return -1;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            if (!blk[y * w + x])
                continue;
            flood(blk, w, h, stk, x, y, &rc);
            pickcolor(img, &rc, n);
            drawrect(rdr, &rc);
            n++;
        }
    }
    free(stk);
    return n;
}

/* ---------- Entry point ---------- */

// Does the work once the image is loaded. Returns 0 on success, 1 on error.
int work(SDL_Surface *img, int grid)
{
    int ret = 1; // exit code
    Uint8 *blk = blackmap(img, BLKMAX);        // black map
    SDL_Renderer *rdr = SDL_CreateSoftwareRenderer(img); // draws on img

    if (blk == NULL)
        fprintf(stderr, "blackmap: out of memory\n");
    else if (rdr == NULL)
        fprintf(stderr, "SDL_CreateSoftwareRenderer: %s\n", SDL_GetError());
    else
    {
        if (grid)
            cleargrid(blk, img);
        if (scan(rdr, img, blk) < 0)
            fprintf(stderr, "scan: out of memory\n");
        else if (IMG_SavePNG(img, "to_cut.png") != 0)
            fprintf(stderr, "IMG_SavePNG: %s\n", IMG_GetError());
        else
            ret = 0;
    }
    if (rdr != NULL)
        SDL_DestroyRenderer(rdr);
    free(blk);
    return ret;
}

int detect_char(const char *path)
{
    const char *base = strrchr(path, '/'); // file name without directories
    base = base != NULL ? base + 1 : path;
    int grid = strncmp(base, "grille", 6) == 0; // 1: grid, 0: word
    if (!grid && strncmp(base, "word", 4) != 0)
    {
        fprintf(stderr, "expected grille.png or word<n>.png, got %s\n", base);
        return 1;
    }
    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0)
    {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        return 1;
    }

    int ret = 1;                      // exit code
    SDL_Surface *img = loadimg(path); // image, converted to RGBA32
    if (img != NULL)
    {
        ret = work(img, grid);
        SDL_FreeSurface(img);
    }
    IMG_Quit();
    return ret;
}

int main(int argc, char **argv){
    detect_char(argv[1]);
    return argc;

}
