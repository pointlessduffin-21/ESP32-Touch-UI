#!/usr/bin/env bash
# load_touch_test.sh — build and flash the touch verification app
#
# Usage:
#   ./load_touch_test.sh           # build + flash
#   ./load_touch_test.sh build     # build only
#   ./load_touch_test.sh flash     # flash only (already built)

set -e

PORT="/dev/cu.usbmodem1101"

PIO="pio"
if [ -f "$HOME/.platformio/penv/bin/pio" ]; then
    PIO="$HOME/.platformio/penv/bin/pio"
fi

CMD="${1:-all}"
PROJECT_DIR="$(dirname "$0")/touch_test"

case "$CMD" in
    build)
        $PIO run --project-dir "$PROJECT_DIR"
        ;;
    flash)
        $PIO run --project-dir "$PROJECT_DIR" -t upload --upload-port "$PORT"
        ;;
    all|*)
        $PIO run --project-dir "$PROJECT_DIR" -t upload --upload-port "$PORT"
        ;;
esac

echo ""
echo "Touch test flashed. Watch serial output with:"
echo "  $PIO device monitor --port $PORT --baud 115200"
