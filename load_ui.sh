#!/usr/bin/env bash
# load_ui.sh — Build and flash whatever is currently in ui/
#
# Builds and flashes the active dashboard (manufacturing dashboard by default).
# Does not swap any files — whatever ui/ui.c, src/sensors.*, src/main.cpp
# contain right now is what gets built.
#
# Usage:
#   ./load_ui.sh            # build + flash + monitor
#   ./load_ui.sh build      # build only
#   ./load_ui.sh flash      # flash only (must already be built)
#   ./load_ui.sh monitor    # serial monitor only

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Loading UI from ui/ ..."
echo "  ui/ui.c    : $(head -3 "$SCRIPT_DIR/ui/ui.c" | tail -1 | sed 's/^ *\* *//')"
echo "  src/main   : $(head -3 "$SCRIPT_DIR/src/main.cpp" | tail -1 | sed 's/^ *\* *//')"
echo ""

"$SCRIPT_DIR/flash.sh" "${1:-all}"
