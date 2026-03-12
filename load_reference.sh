#!/usr/bin/env bash
# load_reference.sh — Flash the reference automotive dashboard
#
# Temporarily swaps in the reference_example source files (automotive
# speedometer/tachometer UI + matching sensors/main), builds and flashes,
# then restores the active manufacturing dashboard files.
#
# Usage:
#   ./load_reference.sh            # build + flash + monitor
#   ./load_reference.sh build      # build only
#   ./load_reference.sh flash      # flash only
#   ./load_reference.sh monitor    # serial monitor only
#
# The active dashboard files are never permanently overwritten — they are
# stashed as *.dashboard_bak and restored automatically on exit (even on
# error or Ctrl-C).

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REF="$SCRIPT_DIR/ui/reference_example"

UI_C="$SCRIPT_DIR/ui/ui.c"
UI_H="$SCRIPT_DIR/ui/ui.h"
SENSORS_H="$SCRIPT_DIR/src/sensors.h"
SENSORS_CPP="$SCRIPT_DIR/src/sensors.cpp"
MAIN_CPP="$SCRIPT_DIR/src/main.cpp"

# ── Restore function — called on EXIT (normal, error, or Ctrl-C) ──────────
restore_dashboard() {
    echo ""
    echo "Restoring manufacturing dashboard files..."
    [ -f "$UI_C.dashboard_bak"       ] && mv "$UI_C.dashboard_bak"       "$UI_C"
    [ -f "$UI_H.dashboard_bak"       ] && mv "$UI_H.dashboard_bak"       "$UI_H"
    [ -f "$SENSORS_H.dashboard_bak"  ] && mv "$SENSORS_H.dashboard_bak"  "$SENSORS_H"
    [ -f "$SENSORS_CPP.dashboard_bak"] && mv "$SENSORS_CPP.dashboard_bak" "$SENSORS_CPP"
    [ -f "$MAIN_CPP.dashboard_bak"   ] && mv "$MAIN_CPP.dashboard_bak"   "$MAIN_CPP"
    echo "Dashboard files restored."
}
trap restore_dashboard EXIT

# ── Validate reference files exist ────────────────────────────────────────
for f in ui.c ui.h sensors.h sensors.cpp main.cpp; do
    if [ ! -f "$REF/$f" ]; then
        echo "ERROR: Missing reference file: $REF/$f"
        exit 1
    fi
done

# ── Stash current dashboard files ─────────────────────────────────────────
echo "Stashing manufacturing dashboard files..."
cp "$UI_C"       "$UI_C.dashboard_bak"
cp "$UI_H"       "$UI_H.dashboard_bak"
cp "$SENSORS_H"  "$SENSORS_H.dashboard_bak"
cp "$SENSORS_CPP" "$SENSORS_CPP.dashboard_bak"
cp "$MAIN_CPP"   "$MAIN_CPP.dashboard_bak"

# ── Swap in reference files ────────────────────────────────────────────────
echo "Loading reference automotive UI..."
cp "$REF/ui.c"       "$UI_C"
cp "$REF/ui.h"       "$UI_H"
cp "$REF/sensors.h"  "$SENSORS_H"
cp "$REF/sensors.cpp" "$SENSORS_CPP"
cp "$REF/main.cpp"   "$MAIN_CPP"
echo "Reference files in place."

# ── Build / flash / monitor ────────────────────────────────────────────────
"$SCRIPT_DIR/flash.sh" "${1:-all}"

# trap will restore on EXIT
