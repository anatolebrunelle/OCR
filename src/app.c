#include "../include/app.h"

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

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        g_free(state->current_image_path);
        state->current_image_path = gtk_file_chooser_get_filename(
            GTK_FILE_CHOOSER(dialog));
        
        // Display image
        gtk_image_set_from_file(GTK_IMAGE(state->image_view), 
        state->current_image_path);
        g_print("Loaded image: %s\n", state->current_image_path);
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

    g_print("Starting complete OCR and solving pipeline...\n");
    
    //Call functions here:

    g_print("Grid solved successfully!\n");
}

// Main window initialization
static void activate(GtkApplication *app, 
    gpointer user_data __attribute__((unused)))
{
    AppState *state = g_malloc0(sizeof(AppState));

    state->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state->window), "OCR Word Search Solver");
    gtk_window_set_default_size(GTK_WINDOW(state->window), 900, 700);

    
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(state->window), main_box);

    // Toolbar
    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(main_box), toolbar, FALSE, FALSE, 5);

    GtkWidget *btn_open = gtk_button_new_with_label("Open Image");
    GtkWidget *btn_solve = gtk_button_new_with_label("Solve Grid");
    gtk_box_pack_start(GTK_BOX(toolbar), btn_open, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), btn_solve, FALSE, FALSE, 0);

    // Rotation
    GtkWidget *label_rot = gtk_label_new("Rotation Angle:");
    state->rotation_slider = gtk_scale_new_with_range(
        GTK_ORIENTATION_HORIZONTAL,
         -180, 180, 1);
    gtk_box_pack_start(GTK_BOX(toolbar), label_rot, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), 
        state->rotation_slider, 
        TRUE, 
        TRUE, 
        0);


    state->image_view = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(main_box), state->image_view, TRUE, TRUE, 0);

    g_signal_connect(btn_open, "clicked", G_CALLBACK(on_open_clicked), state);
    g_signal_connect(btn_solve, "clicked", 
        G_CALLBACK(on_solve_clicked), 
        state);

    gtk_widget_show_all(state->window);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("fr.epita.ocr.solver", 
        G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
