#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <gtk/gtk.h>
#include "/usr/include/libappindicator3-0.1/libappindicator/app-indicator.h"

#define APPINDICATOR_ID "cbrightnessctl"
#define BRIGHTNESS_MAX 100
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_STEP 5

int curr_brightness = 0;
GtkWidget *popover = NULL;

static int get_brightness() {
    FILE *pipe;
    char buffer[128];
    char *cmd = "brightnessctl get";

    // Open a pipe to run the command
    pipe = popen(cmd, "r");
    if (pipe == NULL) {
        fprintf(stderr, "Failed to open pipe for command\n");
        return -1;
    }

    // Read the output of the command
    if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        // Trim trailing newline from output
        char *pos = strchr(buffer, '\n');
        if (pos != NULL) {
            *pos = '\0';
        }

        // Convert output to integer
        int brightness = atoi(buffer);

        // Close the pipe and release resources
        pclose(pipe);

        return brightness;
    } else {
        fprintf(stderr, "Failed to read output of command\n");

        // Close the pipe and release resources
        pclose(pipe);

        return -1;
    }
}

double convert_to_percent(int value) {
    double percent = (double)value / 255.0 * 100.0;
    return percent;
}

void update_status_icon_tooltip(GtkStatusIcon *status_icon) {
    char tooltip_text[64];
    snprintf(tooltip_text, sizeof(tooltip_text), "Current brightness: %d%%", curr_brightness);
    _Pragma("GCC diagnostic push")
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
    gtk_status_icon_set_tooltip_text(status_icon, tooltip_text);
    _Pragma("GCC diagnostic pop")
}

static void on_slider_value_changed(GtkRange *range, gpointer user_data) {
    int new_brightness = (int)gtk_range_get_value(range);
    GtkStatusIcon *status_icon = (GtkStatusIcon *) user_data;

    // Update the brightness value
    curr_brightness = new_brightness;
    update_status_icon_tooltip(status_icon);

    // Call the brightnessctl command to update the brightness
    char command[64];
    snprintf(command, 64, "brightnessctl -q set %d%%", curr_brightness);
    system(command);
}

static GtkWidget *create_popover(GtkWidget *relative_to, GtkStatusIcon *status_icon) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
    gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(relative_to));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    uint margin_size = 5;
    gtk_widget_set_margin_start(box, margin_size);
    gtk_widget_set_margin_end(box, margin_size);
    gtk_widget_set_margin_top(box, margin_size);
    gtk_widget_set_margin_bottom(box, margin_size);

    GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, BRIGHTNESS_MIN, BRIGHTNESS_MAX, 1);
    gtk_range_set_value(GTK_RANGE(slider), curr_brightness);
    gtk_range_set_inverted(GTK_RANGE(slider), TRUE);
    gtk_widget_set_size_request(slider, -1, 200);
    gtk_box_pack_start(GTK_BOX(box), slider, FALSE, FALSE, 0);

    g_signal_connect(slider, "value-changed", G_CALLBACK(on_slider_value_changed), status_icon);

    gtk_widget_show_all(window);

    return window;
}

static void on_indicator_activated(GtkStatusIcon *status_icon, gpointer user_data) {
    GdkRectangle area;
    GdkScreen *screen;
    GtkOrientation orientation;

    if (popover == NULL) {
        if (gtk_status_icon_get_geometry(status_icon, &screen, &area, &orientation)) {
            popover = create_popover(NULL, status_icon);
            gtk_window_set_screen(GTK_WINDOW(popover), screen);
            gtk_window_move(GTK_WINDOW(popover), area.x, area.y + area.height);
        } else {
            g_warning("Failed to get status icon geometry.");
        }
    } else {
        gtk_widget_destroy(popover);
        popover = NULL;
    }
}

int main(int argc, char *argv[]) {
    // Initialize the GTK library
    gtk_init(&argc, &argv);

    // Create a new GtkStatusIcon
    GtkStatusIcon *status_icon = gtk_status_icon_new_from_icon_name("gpm-brightness-lcd");

    curr_brightness = convert_to_percent(get_brightness());
    update_status_icon_tooltip(status_icon);

    // Connect the 'activate' signal of the GtkWidget to the on_indicator_activated callback function
    g_signal_connect(G_OBJECT(status_icon), "activate", G_CALLBACK(on_indicator_activated), NULL);

    // Create the main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    // Run the main loop
    g_main_loop_run(loop);

    return 0;
}
