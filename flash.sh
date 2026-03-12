#!/usr/bin/env bash
# flash.sh — Build, flash, and monitor the dashboard in one command
#
# Usage:
#   ./flash.sh            # build + flash + monitor
#   ./flash.sh build      # build only
#   ./flash.sh flash      # flash only (must already be built)
#   ./flash.sh monitor    # serial monitor only
#
# Adjust PORT below if your device is on a different serial port.

set -e

PORT="/dev/cu.usbmodem1101"   # macOS default; Linux is usually /dev/ttyACM0

PIO="pio"
if [ -f "$HOME/.platformio/penv/bin/pio" ]; then
    PIO="$HOME/.platformio/penv/bin/pio"
fi

CMD="${1:-all}"

case "$CMD" in
    build)
        $PIO run
        ;;
    flash)
        $PIO run -t upload --upload-port "$PORT"
        ;;
    monitor)
        $PIO device monitor --port "$PORT" --baud 115200
        ;;
    all|*)
        $PIO run -t upload --upload-port "$PORT"
        $PIO device monitor --port "$PORT" --baud 115200
        ;;
esac
