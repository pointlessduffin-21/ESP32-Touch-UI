# ui/ — SquareLine Studio Export Directory

This folder is the **export target** for SquareLine Studio.

## DO NOT manually edit files in this directory.
They are regenerated every time you export from SquareLine Studio.

## Contents after export:
- `ui.c` / `ui.h` — generated widget tree and init function
- `ui_events.c` — event handler stubs (safe to extend in a copy outside ui/)
- `assets/` — imported images converted to C arrays by SLS

## Assets (source files, NOT generated):
- `assets/vehicle_top.svg` — top-down car silhouette source
  - Convert to PNG (80×160px or 160×320 for 2x) before importing to SLS
  - Import via SLS Assets panel → Image → add file

## SquareLine Studio project settings:
- Canvas: 800 × 480
- Color depth: 16-bit (RGB565)
- LVGL version: 8.3.x (must match platformio.ini)
- Export path: this directory (`ui/`)
