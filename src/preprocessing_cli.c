#include "../include/preprocessing.h"

#include <SDL2/SDL_image.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *program)
{
    fprintf(stderr, "Usage: %s input.png output.png [operations...]\n",
            program);
    fprintf(stderr, "Operations: --gray --bw --rotate DEGREES\n");
    fprintf(stderr, "Example: %s input.png result.png --rotate 8 --bw\n",
            program);
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        usage(argv[0]);
        return 1;
    }
    SDL_Surface *image = IMG_Load(argv[1]);
    if (!image)
    {
        fprintf(stderr, "Cannot load image: %s\n", IMG_GetError());
        return 1;
    }

    for (int i = 3; i < argc; i++)
    {
        SDL_Surface *next = NULL;
        if (strcmp(argv[i], "--gray") == 0)
            next = preprocess_grayscale(image);
        else if (strcmp(argv[i], "--bw") == 0)
            next = preprocess_binarize(image);
        else if (strcmp(argv[i], "--rotate") == 0 && i + 1 < argc)
        {
            char *end;
            errno = 0;
            double angle = strtod(argv[++i], &end);
            if (errno == 0 && end != argv[i] && *end == '\0')
                next = preprocess_rotate(image, angle);
        }
        else
        {
            usage(argv[0]);
            SDL_FreeSurface(image);
            return 1;
        }
        if (!next)
        {
            fprintf(stderr, "Preprocessing failed at argument %d\n", i);
            SDL_FreeSurface(image);
            return 1;
        }
        SDL_FreeSurface(image);
        image = next;
    }

    int result = IMG_SavePNG(image, argv[2]);
    if (result != 0)
        fprintf(stderr, "Cannot save image: %s\n", IMG_GetError());
    else
        printf("Saved %s (%d x %d)\n", argv[2], image->w, image->h);
    SDL_FreeSurface(image);
    return result == 0 ? 0 : 1;
}
