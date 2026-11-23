/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "config.h"
#include "pins-application.h"
#include "pins-window.h"
#include "pins-home-view.h"
#include "pins-app-grid.h"
#include "pins-app-tile.h"
#include "pins-desktop-file.h"
#include <glib/gstdio.h>
#include <locale.h>

static GtkWindow *window;
static GtkWidget *home;
static GtkDropDown *origins, *sort;
static GtkFlowBox *flow;
static GtkSearchEntry *search;
static GtkSearchBar *search_bar;
static gchar *fixture_root;
static guint stage;
static GApplication *application;
static gboolean real_preview;

static void snapshot (const gchar *name)
{
    g_autoptr (GdkPaintable) paintable = gtk_widget_paintable_new (GTK_WIDGET (window));
    GtkSnapshot *snap = gtk_snapshot_new ();
    gdk_paintable_snapshot (paintable, GDK_SNAPSHOT (snap),
        gtk_widget_get_width (GTK_WIDGET (window)), gtk_widget_get_height (GTK_WIDGET (window)));
    g_autoptr (GskRenderNode) node = gtk_snapshot_free_to_node (snap);
    g_assert_nonnull (node);
    GskRenderer *renderer = gtk_native_get_renderer (GTK_NATIVE (window));
    g_autoptr (GdkTexture) texture = gsk_renderer_render_texture (renderer, node, NULL);
    const gchar *directory = g_getenv ("PINS_TEST_SCREENSHOTS");
    g_assert_nonnull (directory);
    g_autofree gchar *path = g_build_filename (directory, name, NULL);
    g_assert_true (gdk_texture_save_to_png (texture, path));
}

static guint count_tiles (void)
{
    guint count = 0;
    while (gtk_flow_box_get_child_at_index (flow, count)) count++;
    return count;
}

static gboolean step (gpointer unused)
{
    switch (stage++) {
    case 0: {
        window = GTK_WINDOW (gtk_application_get_windows (GTK_APPLICATION (application))->data);
        home = GTK_WIDGET (gtk_widget_get_template_child (GTK_WIDGET (window), PINS_TYPE_WINDOW, "home_view"));
        origins = GTK_DROP_DOWN (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "origin_dropdown"));
        sort = GTK_DROP_DOWN (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "sort_dropdown"));
        search = GTK_SEARCH_ENTRY (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "search_entry"));
        search_bar = GTK_SEARCH_BAR (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "search_bar"));
        GtkWidget *grid = GTK_WIDGET (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "app_grid"));
        flow = GTK_FLOW_BOX (gtk_widget_get_template_child (grid, PINS_TYPE_APP_GRID, "flow_box"));
        if (real_preview) {
            gtk_drop_down_set_selected (origins, PINS_APP_ORIGIN_WAYDROID);
            stage = 50;
            break;
        }
        g_assert_cmpuint (count_tiles (), ==, 5);
        snapshot ("all-sources.png");
        gtk_drop_down_set_selected (origins, PINS_APP_ORIGIN_WAYDROID);
        break;
    }
    case 1: {
        g_assert_cmpuint (count_tiles (), ==, 2);
        snapshot ("waydroid.png");
        gtk_search_bar_set_search_mode (search_bar, TRUE);
        gtk_editable_set_text (GTK_EDITABLE (search), "WeChat");
        break;
    }
    case 2: {
        g_assert_cmpuint (count_tiles (), ==, 1);
        gtk_editable_set_text (GTK_EDITABLE (search), "");
        GtkToggleButton *hidden = GTK_TOGGLE_BUTTON (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "hidden_search_chip"));
        gtk_toggle_button_set_active (hidden, TRUE);
        break;
    }
    case 3: {
        g_assert_cmpuint (count_tiles (), ==, 1);
        snapshot ("waydroid-hidden.png");
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "hidden_search_chip")), FALSE);
        gtk_drop_down_set_selected (origins, PINS_APP_ORIGIN_FLATPAK);
        break;
    }
    case 4: {
        g_assert_cmpuint (count_tiles (), ==, 1);
        gtk_drop_down_set_selected (origins, PINS_APP_ORIGIN_SNAP);
        break;
    }
    case 5: {
        g_assert_cmpuint (count_tiles (), ==, 0);
        snapshot ("empty-source.png");
        gtk_drop_down_set_selected (origins, PINS_APP_ORIGIN_ALL);
        gtk_drop_down_set_selected (sort, 1);
        break;
    }
    case 6: {
        g_assert_cmpuint (count_tiles (), ==, 5);
        GtkWidget *tile = gtk_flow_box_child_get_child (gtk_flow_box_get_child_at_index (flow, 0));
        GtkLabel *source_label = GTK_LABEL (gtk_widget_get_template_child (tile, PINS_TYPE_APP_TILE, "origin"));
        g_assert_cmpstr (gtk_label_get_text (source_label), ==, pins_app_origin_get_label (PINS_APP_ORIGIN_WAYDROID));
        snapshot ("grouped.png");
        /* Exercise the original editor as well as the new filter. */
        GtkWidget *grid = GTK_WIDGET (gtk_widget_get_template_child (home, PINS_TYPE_HOME_VIEW, "app_grid"));
        g_signal_emit_by_name (grid, "activate", 0);
        break;
    }
    case 7: {
        snapshot ("editor.png");
        g_autofree gchar *path = g_build_filename (fixture_root, "applications", "waydroid.org.example.calculator.desktop", NULL);
        g_autoptr (GFile) file = g_file_new_for_path (path);
        g_autoptr (GError) error = NULL;
        g_autoptr (PinsDesktopFile) desktop = pins_desktop_file_new (file, &error);
        g_assert_no_error (error);
        pins_desktop_file_set_boolean (desktop, "NoDisplay", FALSE);
        pins_desktop_file_save (desktop, &error, TRUE);
        g_assert_no_error (error);
        g_autoptr (GKeyFile) saved = g_key_file_new ();
        g_assert_true (g_key_file_load_from_file (saved, path, G_KEY_FILE_NONE, NULL));
        g_assert_false (g_key_file_get_boolean (saved, "Desktop Entry", "NoDisplay", NULL));
        g_assert_cmpint (pins_desktop_file_get_origin (desktop), ==, PINS_APP_ORIGIN_WAYDROID);
        AdwNavigationView *navigation = ADW_NAVIGATION_VIEW (gtk_widget_get_template_child (
            GTK_WIDGET (window), PINS_TYPE_WINDOW, "navigation_view"));
        adw_navigation_view_pop (navigation);
        gtk_search_bar_set_search_mode (search_bar, FALSE);
        gtk_window_set_default_size (window, 390, 600);
        break;
    }
    case 8: {
        g_assert_cmpint (gtk_widget_get_width (GTK_WIDGET (window)), <=, 430);
        snapshot ("narrow-window.png");
        g_print ("UI checks passed: source filters, search, hidden apps, empty source, sorting, editor, visibility persistence, narrow layout.\n");
        gtk_window_destroy (window);
        g_application_quit (application);
        return G_SOURCE_REMOVE;
    }
    case 50: {
        g_print ("Real Waydroid launchers recognized: %u\n", count_tiles ());
        if (g_getenv ("PINS_EXPECT_WAYDROID"))
            g_assert_cmpuint (count_tiles (), ==,
                g_ascii_strtoull (g_getenv ("PINS_EXPECT_WAYDROID"), NULL, 10));
        snapshot ("real-waydroid.png");
        gtk_window_destroy (window);
        g_application_quit (application);
        return G_SOURCE_REMOVE;
    }
    }
    return G_SOURCE_CONTINUE;
}

int main (int argc, char **argv)
{
    if (g_getenv ("PINS_PREVIEW_REAL")) {
        real_preview = TRUE;
        g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);
        g_autoptr (GSettings) settings = g_settings_new ("io.github.mechamorf.PinsSources");
        g_settings_set_boolean (settings, "show-all-apps", TRUE);
        application = G_APPLICATION (pins_application_new (
            "io.github.mechamorf.PinsSources.Preview", G_APPLICATION_NON_UNIQUE));
        g_timeout_add (600, step, NULL);
        return g_application_run (application, argc, argv);
    }
    fixture_root = g_dir_make_tmp ("pins-sources-ui-XXXXXX", NULL);
    g_assert_nonnull (fixture_root);
    g_setenv ("XDG_DATA_HOME", fixture_root, TRUE);
    g_setenv ("GSETTINGS_BACKEND", "memory", TRUE);
    setlocale (LC_ALL, "");
    bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
    g_autofree gchar *apps = g_build_filename (fixture_root, "applications", NULL);
    g_autofree gchar *autostart = g_build_filename (fixture_root, "autostart", NULL);
    g_mkdir_with_parents (apps, 0700);
    g_mkdir_with_parents (autostart, 0700);
    const gchar *files[][3] = {
        { "waydroid.org.example.calculator.desktop", "Calculator", "Exec=waydroid app launch org.example.calculator\nNoDisplay=true\nIcon=accessories-calculator" },
        { "waydroid.org.example.wechat.desktop", "WeChat", "Exec=waydroid app launch org.example.wechat\nIcon=org.gnome.Chats" },
        { "steam_app_123.desktop", "Portal", "Exec=flatpak run com.valvesoftware.Steam steam://rungameid/123\nX-Flatpak=com.valvesoftware.Steam\nIcon=applications-games" },
        { "org.videolan.VLC.desktop", "VLC", "Exec=flatpak run org.videolan.VLC\nX-Flatpak=org.videolan.VLC\nIcon=org.videolan.VLC" },
        { "notes.desktop", "Notes", "Exec=gedit\nIcon=accessories-text-editor" }
    };
    for (guint i = 0; i < G_N_ELEMENTS (files); i++) {
        g_autofree gchar *path = g_build_filename (apps, files[i][0], NULL);
        g_autofree gchar *text = g_strdup_printf ("[Desktop Entry]\nType=Application\nName=%s\n%s\n", files[i][1], files[i][2]);
        g_assert_true (g_file_set_contents (path, text, -1, NULL));
    }
    g_autoptr (GSettings) settings = g_settings_new ("io.github.mechamorf.PinsSources");
    g_settings_set_strv (settings, "search-paths", (const gchar * const []){ NULL });
    g_settings_set_string (settings, "user-path", apps);
    g_settings_set_string (settings, "autostart-path", autostart);
    g_settings_set_boolean (settings, "show-all-apps", TRUE);
    /* A minimal local override must retain the original application's source. */
    g_autofree gchar *original_dir = g_build_filename (fixture_root, "original", NULL);
    g_autofree gchar *override_dir = g_build_filename (fixture_root, "override", NULL);
    g_mkdir_with_parents (original_dir, 0700);
    g_mkdir_with_parents (override_dir, 0700);
    g_autofree gchar *original_path = g_build_filename (original_dir, "app.desktop", NULL);
    g_autofree gchar *override_path = g_build_filename (override_dir, "app.desktop", NULL);
    const gchar *original_data = "[Desktop Entry]\nName=Original\nExec=flatpak run org.example.App\nX-Flatpak=org.example.App\n";
    g_file_set_contents (original_path, original_data, -1, NULL);
    g_file_set_contents (override_path, "[Desktop Entry]\nNoDisplay=true\n", -1, NULL);
    {
        g_autoptr (GFile) original_file = g_file_new_for_path (original_path);
        g_autoptr (GFile) override_file = g_file_new_for_path (override_path);
        g_autoptr (PinsDesktopFile) overridden = pins_desktop_file_new_full (override_file, original_file, NULL);
        g_assert_cmpint (pins_desktop_file_get_origin (overridden), ==, PINS_APP_ORIGIN_FLATPAK);
        pins_desktop_file_set_boolean (overridden, "NoDisplay", FALSE);
        pins_desktop_file_save (overridden, NULL, FALSE);
        g_autofree gchar *after = NULL;
        g_file_get_contents (original_path, &after, NULL, NULL);
        g_assert_cmpstr (after, ==, original_data);
    }
    application = G_APPLICATION (pins_application_new ("io.github.mechamorf.PinsSources.Test", G_APPLICATION_NON_UNIQUE));
    g_timeout_add (600, step, NULL);
    return g_application_run (application, argc, argv);
}
