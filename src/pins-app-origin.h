/* SPDX-License-Identifier: GPL-3.0-or-later */
#pragma once

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
    PINS_APP_ORIGIN_ALL,
    PINS_APP_ORIGIN_WAYDROID,
    PINS_APP_ORIGIN_STEAM,
    PINS_APP_ORIGIN_FLATPAK,
    PINS_APP_ORIGIN_SNAP,
    PINS_APP_ORIGIN_WINE,
    PINS_APP_ORIGIN_APPIMAGE,
    PINS_APP_ORIGIN_SYSTEM,
    PINS_APP_ORIGIN_USER,
    PINS_APP_ORIGIN_OTHER,
    PINS_APP_ORIGIN_COUNT
} PinsAppOrigin;

/* Inspect launcher metadata only. Never run the launcher's command. */
PinsAppOrigin pins_app_origin_detect (GKeyFile *key_file, const gchar *path);
const gchar *pins_app_origin_get_label (PinsAppOrigin origin);

G_END_DECLS
