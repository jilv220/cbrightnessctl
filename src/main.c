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

static void scroll_event_cb (AppIndicator * ci, gint delta, guint direction, gpointer data) {
    int new_brightness = curr_brightness;
    if (direction == GDK_SCROLL_UP) {
        new_brightness += BRIGHTNESS_STEP;
    } else if (direction == GDK_SCROLL_DOWN) {
        new_brightness -= BRIGHTNESS_STEP;
    }

    // Keep the brightness within the valid range
    if (new_brightness < BRIGHTNESS_MIN) {
        new_brightness = BRIGHTNESS_MIN;
    } else if (new_brightness > BRIGHTNESS_MAX) {
        new_brightness = BRIGHTNESS_MAX;
    }

    // Update the brightness value
    curr_brightness = new_brightness;
    char brightness_string[16];
    sprintf(brightness_string, "%d%%", curr_brightness);

    // Call the brightnessctl command to update the brightness
    char command[64];
    snprintf(command, 64, "brightnessctl -q set %d%%", curr_brightness);
    system(command);
}

int main(int argc, char *argv[]) {
    // Initialize the GTK library
    gtk_init(&argc, &argv);

    // Create a new AppIndicator
    AppIndicator *indicator 
        = app_indicator_new(APPINDICATOR_ID, "gpm-brightness-lcd", APP_INDICATOR_CATEGORY_SYSTEM_SERVICES);

    // Set the indicator status
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);

    curr_brightness = convert_to_percent(get_brightness());

    // Connect the 'scroll-event' signal of the GtkWidget to the callback function
    g_signal_connect (indicator, "scroll-event",
                      G_CALLBACK (scroll_event_cb), NULL);

    // Create the main loop
    GMainLoop *loop = g_main_loop_new(NULL, FALSE);

    // Run the main loop
    g_main_loop_run(loop);

    return 0;
}
