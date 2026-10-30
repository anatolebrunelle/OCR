#ifndef APP_H
#define APP_H

#include <gtk/gtk.h>

typedef struct {
    GtkWidget *window;
    GtkWidget *image_view;
    GtkWidget *rotation_slider;
    char *current_image_path;
} AppState;

#endif