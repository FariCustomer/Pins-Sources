# Fork notes

Pins Sources is a personal fork of [Fabrizio Alberio's Pins](https://github.com/fabrialberio/Pins),
based on v2.4.8 (`4eedcdd4b8792f8e5ef3d6dd247e78b76d683ec6`). See the
[main README](README.md) for usage and installation of this fork, and
[the preserved upstream README](docs/UPSTREAM-README.md) for the original documentation.

## Scope of the changes

- Metadata-based application-source classification, source labels and a dropdown
  that combines with the existing search and visibility filters.
- Optional sorting by source, then localized application name.
- Saved source and sorting preferences under `io.github.mechamorf.PinsSources`.
- Retention of an original source when a launcher has a local override.
- Separate application identity, Russian strings, native build/launcher helpers
  and tests. The original Pins launcher editor and GPL-3.0-or-later license remain.
- Small object-lifetime and model-notification fixes needed when refreshing the
  filtered list and closing the editor; no replacement editor was introduced.

The classifier reads launcher metadata and parses `Exec` without executing it.
It does not query RPM, run Waydroid or verify installation provenance. Source
sorting is confined to the Pins Sources list, not the GNOME application grid.

## Development builds

The normal build and menu-launcher installation are documented in README.md.
`PINS_SDK_DIR` can alternatively point to an unpacked Fedora development SDK; it
supplies pkg-config files and headers while the resulting binary uses installed
runtime libraries. This is a development option, not a distributed SDK.

Meson is also supported for native build checks:

```sh
meson setup build-meson
meson compile -C build-meson
meson test -C build-meson --print-errorlogs
```

The UI integration test is built by `build-local.py`; run it as shown in README.md.
It uses temporary fixtures. Screenshots in `qa/` and compiled files are local
artifacts and are not part of the published source.

## Verification on 2026-11-23

- Native application and test executables rebuilt with `build-local.py`; all 29
  classification cases passed.
- Meson setup and compilation passed. Its four test groups passed: desktop-file
  validation, GSettings schema validation, source classification and AppStream
  validation.
- The existing GTK UI integration test passed in the running GNOME Wayland
  session using temporary launchers and in-memory settings. Its source-sorting
  screenshot is included in `docs/source-sorting.png` with synthetic launchers.
- The menu-launcher installer was exercised with a temporary `XDG_DATA_HOME`,
  including `desktop-file-validate`; Python and shell syntax checks passed.
- Compiler warnings remain, including warnings in upstream files. This is not a
  claim of a warning-free build or exhaustive testing.

## Validation limits

The native application and tests are checked on Fedora 45 / GNOME Wayland with
GTK 4.24.1 and libadwaita 1.10.0. The UI integration test exercises the application,
not GNOME Shell itself. No claim is made that it supports every launcher format,
all GNOME versions, or other desktops. Flatpak packaging is not validated.
