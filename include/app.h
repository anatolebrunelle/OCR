#ifndef APP_H
#define APP_H

#include <SDL2/SDL.h>
#include <gtk/gtk.h>

/* A drawing area showing a pixbuf scaled to fit (or a placeholder text). */
typedef struct {
    GtkWidget *area;
    GdkPixbuf *pixbuf;
    const char *placeholder;
} ImagePane;

typedef struct {
    GtkWidget *window;
    ImagePane before;           /* left: image + live rotation */
    ImagePane after;            /* right: OCR result (not finished yet) */
    GtkWidget *rotation_slider;
    char *current_image_path;
    SDL_Surface *original;      /* full resolution, used by the OCR pipeline */
    SDL_Surface *preview;       /* downscaled copy, rotated live */
} AppState;

#endif
