#include "../include/app.h"
#include "../include/preprocessing.h"

#include <SDL2/SDL_image.h>

#define PREVIEW_MAX 800 /* longest side of the live-rotated preview, in px */

/* ---------- image pane (drawing area scaled to fit) ---------- */

static gboolean on_draw_pane(GtkWidget *widget, cairo_t *cr,
    gpointer user_data)
{
    ImagePane *pane = (ImagePane *)user_data;
    int width = gtk_widget_get_allocated_width(widget);
    int height = gtk_widget_get_allocated_height(widget);

    if (pane->pixbuf)
    {
        int iw = gdk_pixbuf_get_width(pane->pixbuf);
        int ih = gdk_pixbuf_get_height(pane->pixbuf);
        double scale = MIN((double)width / iw, (double)height / ih);

        cairo_translate(cr, (width - iw * scale) / 2,
            (height - ih * scale) / 2);
        cairo_scale(cr, scale, scale);
        gdk_cairo_set_source_pixbuf(cr, pane->pixbuf, 0, 0);
        cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_BILINEAR);
        cairo_paint(cr);
    }
    else if (pane->placeholder)
    {
        cairo_text_extents_t ext;

        cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);
        cairo_set_font_size(cr, 14);
        cairo_text_extents(cr, pane->placeholder, &ext);
        cairo_move_to(cr, (width - ext.width) / 2 - ext.x_bearing,
            (height - ext.height) / 2 - ext.y_bearing);
        cairo_show_text(cr, pane->placeholder);
    }
    return FALSE;
}

static void pane_init(ImagePane *pane, const char *placeholder)
{
    pane->pixbuf = NULL;
    pane->placeholder = placeholder;
    pane->area = gtk_drawing_area_new();
    gtk_widget_set_size_request(pane->area, 200, 200);
    g_signal_connect(pane->area, "draw", G_CALLBACK(on_draw_pane), pane);
}

/* Takes ownership of pixbuf (may be NULL to go back to the placeholder). */
static void pane_set_pixbuf(ImagePane *pane, GdkPixbuf *pixbuf)
{
    if (pane->pixbuf)
        g_object_unref(pane->pixbuf);
    pane->pixbuf = pixbuf;
    gtk_widget_queue_draw(pane->area);
}

/* ---------- SDL <-> GdkPixbuf ---------- */

static void free_surface(guchar *pixels __attribute__((unused)),
    gpointer surface)
{
    SDL_FreeSurface((SDL_Surface *)surface);
}

/* Takes ownership of an RGBA32 surface; it is freed with the pixbuf. */
static GdkPixbuf *surface_to_pixbuf(SDL_Surface *surface)
{
    return gdk_pixbuf_new_from_data((guchar *)surface->pixels,
        GDK_COLORSPACE_RGB, TRUE, 8, surface->w, surface->h, surface->pitch,
        free_surface, surface);
}

/* Converts to RGBA32 and shrinks so that the longest side is <= max_side. */
static SDL_Surface *make_preview(SDL_Surface *src, int max_side)
{
    SDL_Surface *rgba = SDL_ConvertSurfaceFormat(src,
        SDL_PIXELFORMAT_RGBA32, 0);
    if (!rgba)
        return NULL;

    int longest = rgba->w > rgba->h ? rgba->w : rgba->h;
    if (longest <= max_side)
        return rgba;

    double ratio = (double)max_side / longest;
    int nw = (int)(rgba->w * ratio);
    int nh = (int)(rgba->h * ratio);
    SDL_Surface *small = SDL_CreateRGBSurfaceWithFormat(0,
        nw > 0 ? nw : 1, nh > 0 ? nh : 1, 32, SDL_PIXELFORMAT_RGBA32);
    if (small)
    {
        SDL_SetSurfaceBlendMode(rgba, SDL_BLENDMODE_NONE);
        SDL_BlitScaled(rgba, NULL, small, NULL);
    }
    SDL_FreeSurface(rgba);
    return small;
}

/* ---------- live rotation ---------- */

/* Rotates the preview by the slider angle (positive = counterclockwise,
** same convention as preprocess_rotate) and shows it in the left pane. */
static void update_rotation(AppState *state)
{
    if (!state->preview)
        return;

    double angle = gtk_range_get_value(GTK_RANGE(state->rotation_slider));
    SDL_Surface *rotated = preprocess_rotate(state->preview, angle);

    if (rotated)
        pane_set_pixbuf(&state->before, surface_to_pixbuf(rotated));
}

static void on_rotation_changed(GtkRange *range __attribute__((unused)),
    gpointer user_data)
{
    update_rotation((AppState *)user_data);
}

static void on_reset_clicked(GtkWidget *button __attribute__((unused)),
    gpointer user_data)
{
    AppState *state = (AppState *)user_data;

    gtk_range_set_value(GTK_RANGE(state->rotation_slider), 0);
}

/* ---------- buttons ---------- */

static void show_error(AppState *state, const char *message)
{
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s",
        message);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static void load_image(AppState *state, const char *path)
{
    SDL_Surface *original = IMG_Load(path);

    if (!original)
    {
        show_error(state, IMG_GetError());
        return;
    }
    SDL_Surface *preview = make_preview(original, PREVIEW_MAX);
    if (!preview)
    {
        SDL_FreeSurface(original);
        show_error(state, "Cannot prepare the image preview.");
        return;
    }

    /* Drop the old image first so that resetting the slider does not
    ** trigger a useless rotation of it. */
    SDL_FreeSurface(state->original);
    SDL_FreeSurface(state->preview);
    state->original = NULL;
    state->preview = NULL;
    gtk_range_set_value(GTK_RANGE(state->rotation_slider), 0);

    state->original = original;
    state->preview = preview;
    g_free(state->current_image_path);
    state->current_image_path = g_strdup(path);

    pane_set_pixbuf(&state->after, NULL);
    update_rotation(state);
    g_print("Loaded image: %s\n", path);
}

// click open button
static void on_open_clicked(GtkWidget *button __attribute__((unused)),
gpointer user_data)
{
    AppState *state = (AppState *)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new("Open Word Search Image",
    GTK_WINDOW(state->window),
    GTK_FILE_CHOOSER_ACTION_OPEN,
    "_Cancel",
    GTK_RESPONSE_CANCEL,
    "_Open",
    GTK_RESPONSE_ACCEPT,
    NULL);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        load_image(state, path);
        g_free(path);
    }

    gtk_widget_destroy(dialog);
}

// click Solve button
static void on_solve_clicked(GtkWidget *button __attribute__((unused)),
gpointer user_data)
{
    AppState *state = (AppState *)user_data;

    if (state->current_image_path == NULL) {
        g_print("Error: No image loaded yet!\n");
        return;
    }

    double angle = gtk_range_get_value(GTK_RANGE(state->rotation_slider));
    g_print("Starting complete OCR and solving pipeline (rotation %.0f)...\n",
        angle);

    // TODO: rotate state->original by `angle` (preprocess_rotate), then
    // call the rest of the pipeline and show the result with
    // pane_set_pixbuf(&state->after, ...).

    g_print("Grid solved successfully!\n");
}

/* ---------- window ---------- */

static GtkWidget *make_title(const char *text)
{
    GtkWidget *label = gtk_label_new(NULL);
    char *markup = g_markup_printf_escaped("<b>%s</b>", text);

    gtk_label_set_markup(GTK_LABEL(label), markup);
    g_free(markup);
    return label;
}

// Main window initialization
static void activate(GtkApplication *app,
    gpointer user_data __attribute__((unused)))
{
    AppState *state = g_malloc0(sizeof(AppState));

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "OCR Word Search Solver");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 1100, 700);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(state->window), main_box);

    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), toolbar, FALSE, FALSE, 5);

    GtkWidget *btn_open = gtk_button_new_with_label("Open Image");
    GtkWidget *btn_solve = gtk_button_new_with_label("Solve Grid");
    gtk_box_pack_start(GTK_BOX(toolbar), btn_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_solve, FALSE, FALSE, 0);

    // Split view: before (left) | after OCR (right)
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_pack_start(GTK_BOX(main_box), paned, TRUE, TRUE, 0);

    // Left: image + rotation slider
    GtkWidget *left = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(left), make_title("Before"), FALSE, FALSE, 0);

    pane_init(&state->before, "Open an image to start");
    gtk_box_pack_start(GTK_BOX(left), state->before.area, TRUE, TRUE, 0);

    GtkWidget *rot_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *label_rot = gtk_label_new("Rotation Angle:");
    state->rotation_slider = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL,
        -180, 180, 1);
    gtk_range_set_value(GTK_RANGE(state->rotation_slider), 0);
    gtk_scale_add_mark(GTK_SCALE(state->rotation_slider), 0,
        GTK_POS_BOTTOM, NULL);
    GtkWidget *btn_reset = gtk_button_new_with_label("Reset");
    gtk_box_pack_start(GTK_BOX(rot_box), label_rot, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(rot_box), state->rotation_slider,
        TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(rot_box), btn_reset, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(left), rot_box, FALSE, FALSE, 5);

    // Right: OCR result
    GtkWidget *right = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_pack_start(GTK_BOX(right), make_title("After OCR"),
        FALSE, FALSE, 0);

    pane_init(&state->after, "OCR result will appear here");
    gtk_box_pack_start(GTK_BOX(right), state->after.area, TRUE, TRUE, 0);

    gtk_paned_pack1(GTK_PANED(paned), left, TRUE, FALSE);
    gtk_paned_pack2(GTK_PANED(paned), right, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(paned), 550);

    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_clicked), state);
    g_signal_connect(btn_solve, "clicked",
        G_CALLBACK(on_solve_clicked),
        state);
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(on_reset_clicked),
        state);
    g_signal_connect(state->rotation_slider, "value-changed",
        G_CALLBACK(on_rotation_changed), state);

    gtk_widget_show_all(state->window);
}

int main(int argc, char **argv)
{
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    GtkApplication *app = gtk_application_new("fr.epita.ocr.solver",
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    IMG_Quit();
    return status;
}
