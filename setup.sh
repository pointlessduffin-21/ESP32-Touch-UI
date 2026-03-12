#!/usr/bin/env bash
# setup.sh — Bootstrap ESP32-P4 automotive dashboard on a fresh machine
#
# What this does:
#   1. Installs PlatformIO Core (CLI) via pip if not already present
#   2. Downloads managed components (LVGL, esp_lcd_touch, etc.) via idf_component.yml
#   3. Runs a full build to verify the toolchain works
#
# Requirements:
#   - macOS or Linux
#   - Python 3.8+ installed  (brew install python  OR  sudo apt install python3)
#   - USB cable connecting the ESP32-P4 board (for flashing, not required for build)
#
# Usage:
#   chmod +x setup.sh && ./setup.sh

set -e

echo "=== ESP32-P4 Automotive Dashboard — setup ==="

# ── 1. PlatformIO ──────────────────────────────────────────────────────────────
if ! command -v pio &>/dev/null && [ ! -f "$HOME/.platformio/penv/bin/pio" ]; then
    echo "[1/3] Installing PlatformIO Core..."
    pip3 install --user platformio
else
    echo "[1/3] PlatformIO already installed — skipping"
fi

# Prefer the virtualenv pio if available
PIO="pio"
if [ -f "$HOME/.platformio/penv/bin/pio" ]; then
    PIO="$HOME/.platformio/penv/bin/pio"
fi

# ── 2. Platform + toolchain ────────────────────────────────────────────────────
echo "[2/3] Installing ESP32-P4 platform and toolchain (first run downloads ~500MB)..."
$PIO platform install \
    "https://github.com/pioarduino/platform-espressif32/releases/download/53.03.13-1/platform-espressif32.zip" \
    || echo "  (already installed or will be fetched on first build)"

# ── 3. First build (downloads managed components via idf_component.yml) ────────
echo "[3/3] Running first build — this downloads LVGL and other components..."
$PIO run

echo ""
echo "=== Setup complete! ==="
echo ""
echo "Next steps:"
echo "  Build:       $PIO run"
echo "  Flash:       $PIO run -t upload"
echo "  Monitor:     $PIO device monitor"
echo "  Flash+watch: $PIO run -t upload && $PIO device monitor"
echo ""
echo "Board: ESP32-P4 + JC8012P4A1C_I_W_Y (10.1\" 800×1280 JD9365 MIPI-DSI)"
echo "Port:  /dev/cu.usbmodem1101  (macOS) — adjust in platformio.ini if different"
