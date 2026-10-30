/*
** ocr.c: entry point of the OCR Word Search Solver (first defense).
**
** It chains the parts written by each member of the group:
**   - load, colour removal, manual rotation   (preprocessing.c)
**   - grid and word list detection            (split.c)
**   - letter detection and cutting            (detect_char.c, cut.c)
**   - word search solving                     (grid.c, solver.c)
*/
#define _POSIX_C_SOURCE 200809L

#include "../include/grid.h"
#include "../include/preprocessing.h"
#include "../include/solver.h"
#include "../include/split.h"

#include <SDL2/SDL_image.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define PATH_BUF 512
#define LETTER_DIR "letter" /* directory written by cut(), see cut.c */

/*
** detect_char.h and cut.h cannot be included together (same include guard
** and two different struct rect): the two entry points are declared here.
*/
int detect_char(const char *path);
int cut(void);

static void usage(const char *prog)
{
    fprintf(stderr, "Usage: %s IMAGE OUTDIR [--rotate DEGREES]\n", prog);
    fprintf(stderr, "       %s solve GRID_FILE WORD\n", prog);
    fprintf(stderr, "\nIMAGE  word search picture to process\n");
    fprintf(stderr, "OUTDIR directory receiving every intermediate result\n");
    fprintf(stderr, "DEGREES rotation applied first (positive: ");
    fprintf(stderr, "counter-clockwise)\n");
}

static int make_dir(const char *path)
{
    if (mkdir(path, 0755) != 0 && errno != EEXIST)
    {
        fprintf(stderr, "Cannot create %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

static int has_png_extension(const char *name)
{
    size_t len = strlen(name);

    return len > 4 && strcmp(name + len - 4, ".png") == 0;
}

/* Counts the .png files of a directory (0 if it does not exist). */
static int count_pngs(const char *dir)
{
    int count = 0;
    DIR *d = opendir(dir);

    if (!d)
        return 0;
    for (struct dirent *e = readdir(d); e; e = readdir(d))
        if (has_png_extension(e->d_name))
            count++;
    closedir(d);
    return count;
}

/* Removes the .png files left in a directory by a previous run. */
static void clear_pngs(const char *dir)
{
    char path[PATH_BUF];
    DIR *d = opendir(dir);

    if (!d)
        return;
    for (struct dirent *e = readdir(d); e; e = readdir(d))
    {
        if (!has_png_extension(e->d_name))
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, e->d_name);
        remove(path);
    }
    closedir(d);
}

static int save_png(SDL_Surface *img, const char *dir, const char *name)
{
    char path[PATH_BUF];

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    if (IMG_SavePNG(img, path) != 0)
    {
        fprintf(stderr, "Cannot save %s: %s\n", path, IMG_GetError());
        return -1;
    }
    return 0;
}

/*
** Saves the black and white piece (the grid or one word) as dir/name, then
** frames and cuts every character of it into dir/letter/.
** detect_char() and cut() work in the current directory with fixed file
** names, so each piece is processed from inside its own directory.
** Returns the number of letters saved, or -1 on error.
*/
static int cut_letters(const char *dir, const char *name, SDL_Surface *bw)
{
    char letters[PATH_BUF];
    int count = -1;
    int origin = open(".", O_RDONLY);

    if (origin < 0)
        return -1;
    if (make_dir(dir) == 0 && save_png(bw, dir, name) == 0 && chdir(dir) == 0)
    {
        clear_pngs(LETTER_DIR);
        if (detect_char(name) == 0 && cut() == 0)
            count = count_pngs(LETTER_DIR);
        if (fchdir(origin) != 0)
        {
            fprintf(stderr, "Cannot go back to the working directory\n");
            count = -1;
        }
    }
    close(origin);
    if (count < 0)
    {
        snprintf(letters, sizeof(letters), "%s/%s", dir, name);
        fprintf(stderr, "Letter detection failed on %s\n", letters);
    }
    return count;
}

/* Black and white version of a piece, then its letters. */
static int process_piece(const char *dir, const char *name,
                         const SDL_Surface *piece)
{
    SDL_Surface *bw = preprocess_binarize(piece);
    int count;

    if (!bw)
    {
        fprintf(stderr, "Cannot binarize %s/%s\n", dir, name);
        return -1;
    }
    count = cut_letters(dir, name, bw);
    SDL_FreeSurface(bw);
    return count;
}

/* Saves the colour removal steps of the whole image (demo purpose). */
static int save_steps(const SDL_Surface *image, const char *outdir)
{
    SDL_Surface *gray = preprocess_grayscale(image);
    SDL_Surface *bw = preprocess_binarize(image);
    int ret = -1;

    if (gray && bw && save_png(gray, outdir, "02_gray.png") == 0
        && save_png(bw, outdir, "03_bw.png") == 0)
        ret = 0;
    else if (!gray || !bw)
        fprintf(stderr, "Colour removal failed\n");
    if (gray)
        SDL_FreeSurface(gray);
    if (bw)
        SDL_FreeSurface(bw);
    return ret;
}

/* Cuts the grid, then every word of the list, into letters. */
static int process_split(const Split *sp, const char *outdir)
{
    char dir[PATH_BUF];
    char name[PATH_BUF];
    int count;

    snprintf(dir, sizeof(dir), "%s/grid", outdir);
    count = process_piece(dir, "grid.png", sp->grid);
    if (count < 0)
        return -1;
    printf("grid : %d x %d px, %d letters\n", sp->grid->w, sp->grid->h, count);

    snprintf(dir, sizeof(dir), "%s/words", outdir);
    if (make_dir(dir) != 0)
        return -1;
    for (int i = 0; i < sp->word_count; i++)
    {
        snprintf(dir, sizeof(dir), "%s/words/word%d", outdir, i + 1);
        snprintf(name, sizeof(name), "word%d.png", i + 1);
        count = process_piece(dir, name, sp->words[i]);
        if (count < 0)
            return -1;
        printf("word %-2d: %d x %d px, %d letters\n", i + 1,
               sp->words[i]->w, sp->words[i]->h, count);
    }
    return 0;
}

/*
** Whole first-defense chain on one picture. Every step is written in outdir
** so that it can be shown during the defense:
**   01_rotated.png   manual rotation (only with --rotate)
**   02_gray.png      grayscale
**   03_bw.png        black and white
**   split/           grid.png and word<n>.png
**   grid/, words/    one directory per piece, with its letter/ directory
*/
static int run_pipeline(const char *input, const char *outdir, int rotate,
                        double angle)
{
    char source[PATH_BUF];
    char split_dir[PATH_BUF];
    SDL_Surface *image = IMG_Load(input);
    Split *sp = NULL;
    int ret = 1;

    if (!image)
    {
        fprintf(stderr, "Cannot load %s: %s\n", input, IMG_GetError());
        return 1;
    }
    snprintf(source, sizeof(source), "%s", input);
    if (make_dir(outdir) != 0)
        goto end;
    if (rotate)
    {
        SDL_Surface *rotated = preprocess_rotate(image, angle);

        SDL_FreeSurface(image);
        image = rotated;
        if (!image || save_png(image, outdir, "01_rotated.png") != 0)
        {
            fprintf(stderr, "Rotation failed\n");
            goto end;
        }
        snprintf(source, sizeof(source), "%s/01_rotated.png", outdir);
    }
    if (save_steps(image, outdir) != 0)
        goto end;

    /* The split works on the colour image: it keeps pale words visible. */
    snprintf(split_dir, sizeof(split_dir), "%s/split", outdir);
    if (make_dir(split_dir) != 0)
        goto end;
    sp = split_image(source, split_dir);
    if (!sp)
        goto end;
    printf("%d word(s) found in the list\n", sp->word_count);
    if (process_split(sp, outdir) == 0)
        ret = 0;
end:
    if (sp)
        split_free(sp);
    if (image)
        SDL_FreeSurface(image);
    return ret;
}

/* Same output format as the solver program. */
static int run_solve(const char *file, const char *word)
{
    Grid *g = grid_load(file);
    SearchResult r;

    if (!g)
    {
        fprintf(stderr, "Error: could not load grid from %s\n", file);
        return 1;
    }
    r = solver_find(g, word);
    if (r.found)
        printf("(%d,%d)(%d,%d)\n", r.x0, r.y0, r.x1, r.y1);
    else
        printf("Not found\n");
    grid_free(g);
    return 0;
}

int main(int argc, char **argv)
{
    double angle = 0.0;
    int rotate = 0;

    if (argc == 4 && strcmp(argv[1], "solve") == 0)
        return run_solve(argv[2], argv[3]);
    if (argc != 3 && argc != 5)
    {
        usage(argv[0]);
        return 1;
    }
    if (argc == 5)
    {
        char *end;

        errno = 0;
        angle = strtod(argv[4], &end);
        if (strcmp(argv[3], "--rotate") != 0 || errno != 0
            || end == argv[4] || *end != '\0')
        {
            usage(argv[0]);
            return 1;
        }
        rotate = 1;
    }
    return run_pipeline(argv[1], argv[2], rotate, angle);
}
