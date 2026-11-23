/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pins-app-origin.h"

typedef struct { const gchar *path; const gchar *data; PinsAppOrigin expected; } Case;

static void classify (gconstpointer data)
{
    const Case *test = data;
    g_autoptr (GKeyFile) key = g_key_file_new ();
    g_autofree gchar *contents = g_strconcat ("[Desktop Entry]\nType=Application\nName=Test\n", test->data, NULL);
    g_assert_true (g_key_file_load_from_data (key, contents, -1, G_KEY_FILE_NONE, NULL));
    g_assert_cmpint (pins_app_origin_detect (key, test->path), ==, test->expected);
}

int main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);
    static const Case cases[] = {
        { "/tmp/waydroid.org.example.desktop", "Exec=other\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Categories=Utility;X-WayDroid-App;\n", PINS_APP_ORIGIN_WAYDROID },
        { "/tmp/a.desktop", "Exec=env FOO=bar /usr/bin/waydroid app launch test\n", PINS_APP_ORIGIN_WAYDROID },
        { "/tmp/steam_app_123.desktop", "Exec=flatpak run com.valvesoftware.Steam steam://rungameid/123\nX-Flatpak=com.valvesoftware.Steam\n", PINS_APP_ORIGIN_STEAM },
        { "/tmp/game.desktop", "Exec=steam steam://run/123\n", PINS_APP_ORIGIN_STEAM },
        { "/tmp/game.desktop", "Exec=flatpak run com.valvesoftware.Steam steam://rungameid/123\n", PINS_APP_ORIGIN_STEAM },
        { "/tmp/steam.desktop", "Exec=flatpak run com.valvesoftware.Steam\nX-Flatpak=com.valvesoftware.Steam\n", PINS_APP_ORIGIN_FLATPAK },
        { "/usr/share/applications/a.desktop", "Exec=foo\nX-Flatpak=org.foo.App\n", PINS_APP_ORIGIN_FLATPAK },
        { "/tmp/a.desktop", "Exec=/usr/bin/flatpak run org.foo.App\n", PINS_APP_ORIGIN_FLATPAK },
        { "/var/lib/flatpak/exports/share/applications/a.desktop", "Hidden=true\n", PINS_APP_ORIGIN_FLATPAK },
        { "/tmp/a.desktop", "X-SnapInstanceName=test\n", PINS_APP_ORIGIN_SNAP },
        { "/var/lib/snapd/desktop/applications/a.desktop", "Exec=foo\n", PINS_APP_ORIGIN_SNAP },
        { "/tmp/a.desktop", "Exec=env WINEPREFIX=/tmp/prefix wine64 app.exe\n", PINS_APP_ORIGIN_WINE },
        { "/tmp/a.desktop", "Exec=\"/opt/My App.AppImage\" %U\n", PINS_APP_ORIGIN_APPIMAGE },
        { "/tmp/a.desktop", "X-AppImage-Version=1.0\n", PINS_APP_ORIGIN_APPIMAGE },
        { "/usr/share/applications/a.desktop", "Exec=/usr/bin/test\n", PINS_APP_ORIGIN_SYSTEM },
        { "/usr/share/applications/a.desktop", "Hidden=true\nNoDisplay=true\n", PINS_APP_ORIGIN_SYSTEM },
        { "/usr/share/applications-other/a.desktop", "Exec=test\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/steam_app_not-a-game.desktop", "Exec=test\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Exec=editor /tmp/waydroid\nComment=flatpak steam wine\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Exec=editor file.AppImage steam://run/123\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Exec=\"malformed\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Exec=env\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Exec=env -u\n", PINS_APP_ORIGIN_OTHER },
        { "/tmp/a.desktop", "Exec=env -u OLD FOO=x -- waydroid app launch pkg\n", PINS_APP_ORIGIN_WAYDROID },
        { "/usr/share/applications/Waydroid.desktop", "Exec=waydroid\nCategories=X-WayDroid-App;Utility;\n", PINS_APP_ORIGIN_SYSTEM },
        { "/usr/share/applications/waydroid.app.install.desktop", "Exec=waydroid app install %f\n", PINS_APP_ORIGIN_SYSTEM },
        { "/usr/share/applications/waydroid.market.desktop", "Exec=waydroid app intent android.intent.action.VIEW %u\n", PINS_APP_ORIGIN_SYSTEM },
        { "/tmp/waydroid.org.example.desktop", "NoDisplay=true\n", PINS_APP_ORIGIN_WAYDROID }
    };
    for (guint i = 0; i < G_N_ELEMENTS (cases); i++) {
        g_autofree gchar *name = g_strdup_printf ("/origin/case-%02u", i);
        g_test_add_data_func (name, &cases[i], classify);
    }
    return g_test_run ();
}
