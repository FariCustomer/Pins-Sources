/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pins-app-origin.h"
#include <glib/gi18n.h>

static gboolean
path_under (const gchar *path, const gchar *root)
{
    g_autofree gchar *canonical = g_canonicalize_filename (path, NULL);
    g_autofree gchar *prefix = g_strconcat (root, G_DIR_SEPARATOR_S, NULL);
    return g_str_has_prefix (canonical, prefix);
}

static gboolean
has_command (gchar **argv, const gchar *command)
{
    guint i = 0;
    if (!argv || !argv[0])
        return FALSE;
    g_autofree gchar *first = g_path_get_basename (argv[0]);
    if (g_str_equal (first, "env")) {
        i++;
        for (; argv[i]; i++) {
            if (g_str_equal (argv[i], "--")) { i++; break; }
            if (g_str_equal (argv[i], "-u") || g_str_equal (argv[i], "--unset")
                || g_str_equal (argv[i], "-C") || g_str_equal (argv[i], "--chdir")) {
                if (!argv[++i]) return FALSE;
                continue;
            }
            if (argv[i][0] == '-' || strchr (argv[i], '=')) continue;
            break;
        }
    }
    if (!argv[i]) return FALSE;
    g_autofree gchar *base = g_path_get_basename (argv[i]);
    return g_str_equal (base, command);
}

static gboolean
is_steam_game_id (const gchar *basename)
{
    if (!g_str_has_prefix (basename, "steam_app_")
        || !g_str_has_suffix (basename, ".desktop"))
        return FALSE;
    const gchar *number = basename + strlen ("steam_app_");
    const gchar *end = basename + strlen (basename) - strlen (".desktop");
    if (number == end)
        return FALSE;
    for (; number < end; number++)
        if (!g_ascii_isdigit (*number))
            return FALSE;
    return TRUE;
}

static gboolean
is_waydroid_app (gchar **argv)
{
    if (!has_command (argv, "waydroid")) return FALSE;
    for (guint i = 0; argv[i] && argv[i + 1]; i++)
        if (g_str_equal (argv[i], "app") && g_str_equal (argv[i + 1], "launch"))
            return TRUE;
    return FALSE;
}

PinsAppOrigin
pins_app_origin_detect (GKeyFile *key_file, const gchar *path)
{
    g_autofree gchar *base = g_path_get_basename (path);
    g_autofree gchar *exec = g_key_file_get_string (key_file, "Desktop Entry", "Exec", NULL);
    g_autofree gchar *categories = g_key_file_get_string (key_file, "Desktop Entry", "Categories", NULL);
    g_auto (GStrv) argv = NULL;
    g_auto (GStrv) category_list = g_strsplit (categories ? categories : "", ";", -1);
    if (exec)
        g_shell_parse_argv (exec, NULL, &argv, NULL);

    /* Waydroid's native host launchers also use the Waydroid category and
     * filename prefix. An actual Android launcher uses `app launch`.
     * Keep metadata-only local overrides recognizable when Exec is absent. */
    if (is_waydroid_app (argv)
        || ((!exec || !*exec)
            && (g_str_has_prefix (base, "waydroid.")
                || g_strv_contains ((const gchar * const *) category_list, "X-WayDroid-App"))))
        return PINS_APP_ORIGIN_WAYDROID;

    /* Steam games may be launched by the Flatpak Steam client. Games must
     * take precedence over the client's installation format. */
    if (is_steam_game_id (base))
        return PINS_APP_ORIGIN_STEAM;
    if (argv)
        for (guint i = 0; argv[i]; i++)
            if ((has_command (argv, "steam") || has_command (argv, "flatpak"))
                && (g_str_has_prefix (argv[i], "steam://rungameid/")
                    || g_str_has_prefix (argv[i], "steam://run/")))
                return PINS_APP_ORIGIN_STEAM;

    if (g_key_file_has_key (key_file, "Desktop Entry", "X-Flatpak", NULL)
        || has_command (argv, "flatpak")
        || strstr (path, "/flatpak/exports/share/applications/"))
        return PINS_APP_ORIGIN_FLATPAK;

    if (g_key_file_has_key (key_file, "Desktop Entry", "X-SnapInstanceName", NULL)
        || has_command (argv, "snap")
        || path_under (path, "/var/lib/snapd/desktop"))
        return PINS_APP_ORIGIN_SNAP;

    if (has_command (argv, "wine") || has_command (argv, "wine64")
        || has_command (argv, "wine-stable") || has_command (argv, "wine-development")
        || has_command (argv, "winebrowser") || strstr (path, "/applications/wine/"))
        return PINS_APP_ORIGIN_WINE;

    if (g_key_file_has_key (key_file, "Desktop Entry", "X-AppImage-Version", NULL)
        || g_key_file_has_key (key_file, "Desktop Entry", "X-AppImage-Name", NULL))
        return PINS_APP_ORIGIN_APPIMAGE;
    if (argv && argv[0] && g_str_has_suffix (argv[0], ".AppImage"))
        return PINS_APP_ORIGIN_APPIMAGE;

    if (path_under (path, "/usr/share/applications")
        || path_under (path, "/usr/local/share/applications")
        || path_under (path, "/run/host/usr/share/applications")
        || path_under (path, "/run/current-system/sw/share/applications"))
        return PINS_APP_ORIGIN_SYSTEM;

    g_autofree gchar *user_apps = g_build_filename (g_get_user_data_dir (), "applications", NULL);
    if (path_under (path, user_apps))
        return PINS_APP_ORIGIN_USER;
    return PINS_APP_ORIGIN_OTHER;
}

const gchar *
pins_app_origin_get_label (PinsAppOrigin origin)
{
    static const gchar *labels[PINS_APP_ORIGIN_COUNT] = {
        N_("All sources"), "Waydroid", "Steam", "Flatpak", "Snap", "Wine",
        "AppImage", N_("System packages"), N_("User shortcuts"), N_("Other sources")
    };
    if (origin < PINS_APP_ORIGIN_ALL || origin >= PINS_APP_ORIGIN_COUNT)
        return _("Other sources");
    return _(labels[origin]);
}
