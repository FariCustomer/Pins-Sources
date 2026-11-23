#!/usr/bin/env bash
set -euo pipefail
pins_source_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
export GSETTINGS_SCHEMA_DIR="$pins_source_root/build-local/schemas"
exec "$pins_source_root/build-local/pins-sources" "$@"
