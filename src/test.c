//file to make the test mains
#include "../include/split.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: %s image dossier\n", argv[0]);
        return 1;
    }
    Split *s = split_image(argv[1], argv[2]);
    if (!s)
        return 1;
    printf("grille %dx%d, %d mots\n", s->grid->w, s->grid->h, s->word_count);
    split_free(s);
    return 0;
}
