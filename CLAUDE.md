# ESP32 Touch UI — Automotive Dashboard

## Project Goal

Replicate a luxury automotive instrument cluster UI (Jaguar-style) on an ESP32-based touch display using SquareLine Studio and LVGL.

The target design features:
- **Left**: Speedometer gauge (0–220 mph), speed readout, fuel range
- **Center**: Tire pressure display (4 corners, PSI values), vehicle top-down graphic, odometer
- **Right**: Tachometer gauge (0–8 RPM ×1000), gear/mode indicator, coolant temp
- **Top center**: ADAS/driver assist icon, ambient temp, drive mode label

Design aesthetic: dark background (#0a0a0a), teal/cyan arc accents (`#00e5cc`), white text, red redline zone on tach.

---

## Hardware Target

| | |
|---|---|
| **Board** | ESP32-P4-Function-EV-Board |
| **MCU** | ESP32-P4 dual-core RISC-V 400MHz + 32MB PSRAM |
| **Display** | 7" 1024×600 MIPI-DSI, EK79007 driver IC (2-lane, 1 Gbps, 52 MHz DPI) |
| **Touch** | GT911 capacitive 10-point, I2C addr 0x5D, SDA=GPIO7, SCL=GPIO8 |
| **Backlight** | GPIO26 PWM |
| **USB** | Native ESP32-P4 USB JTAG/Serial (no external USB-to-serial chip) |
| **Flash** | 16MB QSPI |
| **macOS ports** | `/dev/cu.usbmodem1101` (CDC/upload), `/dev/cu.usbmodem0010000001` (JTAG) |

**Arduino framework is NOT supported on ESP32-P4. Must use ESP-IDF v5.3+.**

---

## Toolchain

| Tool | Purpose |
|------|---------|
| SquareLine Studio | UI design and LVGL C code export |
| ESP-IDF v5.3+ | Firmware framework (replaces Arduino) |
| LVGL v9.x | Graphics library (via idf_component.yml) |
| PlatformIO + pioarduino | Build system (official PlatformIO lacks P4 support) |
| Espressif BSP | `esp32_p4_function_ev_board` — handles MIPI-DSI + touch |
| esp_lvgl_port | LVGL ↔ esp_lcd bridge, mutex, vsync |

---

## SquareLine Studio Setup

### Canvas Settings
- Resolution: **1024 × 600** (ESP32-P4-Function-EV-Board 7" panel)
- Color depth: 16-bit (RGB565)
- LVGL version: **9.x** (must match idf_component.yml)

### Export
- Export path: `ui/` subfolder in this repo
- Export generates: `ui.c`, `ui.h`, and asset files
- Do not manually edit exported files — changes will be overwritten on re-export
- Custom logic goes in `main.c` / `app_main.cpp` which calls `ui_init()` and updates widget values

---

## UI Component Map

### Speedometer (left gauge)
- LVGL widget: `lv_arc` (background ring) + `lv_arc` (value indicator, teal)
- Center label: large white number (current speed)
- Sub-label: "mph"
- Bottom label: fuel icon + range miles
- Scale: 0–220, arc spans ~270°, starts bottom-left

### Tachometer (right gauge)
- LVGL widget: `lv_arc` (background) + `lv_arc` (value arc, teal → red at 7k+)
- Center label: gear number (large)
- Sub-label: drive mode ("Sport", "Dynamic", etc.)
- Bottom label: thermometer icon + coolant temp (°F)
- Red zone: 7000–8000 RPM, rendered as separate red arc segment
- Scale markings: 0–8 (×1000), tick marks

### Center Panel
- Tire pressure: 4 `lv_label` widgets positioned around vehicle image
  - Top-left, top-right: front tires
  - Bottom-left, bottom-right: rear tires
  - Each shows value + "psi"
- Vehicle graphic: `lv_img` — top-down car silhouette (silver/grey PNG, transparent bg)
- Odometer: `lv_label` centered below vehicle image
- ADAS icon: `lv_img` top-center
- Temp label: `lv_label` top-right

---

## Data Flow (Firmware Side)

UI values are updated by calling LVGL setter functions from the main application loop or FreeRTOS tasks:

```c
// Speed
lv_arc_set_value(ui_SpeedArc, speed_mph);
lv_label_set_text_fmt(ui_SpeedLabel, "%d", speed_mph);

// RPM
lv_arc_set_value(ui_RpmArc, rpm / 100);  // scale to arc range
lv_label_set_text_fmt(ui_GearLabel, "%d", current_gear);

// Tire pressure
lv_label_set_text_fmt(ui_TireFrontLeft,  "%d\npsi", psi_fl);
lv_label_set_text_fmt(ui_TireFrontRight, "%d\npsi", psi_fr);
lv_label_set_text_fmt(ui_TireRearLeft,   "%d\npsi", psi_rl);
lv_label_set_text_fmt(ui_TireRearRight,  "%d\npsi", psi_rr);

// Coolant temp
lv_label_set_text_fmt(ui_CoolantLabel, "%d°F", coolant_f);

// Fuel range
lv_label_set_text_fmt(ui_FuelRangeLabel, "%d mi", fuel_range_miles);
```

All UI updates must happen on the LVGL task/thread. Use `lv_timer_handler()` in the main loop or a dedicated LVGL FreeRTOS task.

---

## Naming Conventions

Prefix all SquareLine widget names to match firmware expectations:

| Widget | SLS Name |
|--------|----------|
| Speed arc | `SpeedArc` |
| Speed number label | `SpeedLabel` |
| RPM arc (main) | `RpmArc` |
| RPM red zone arc | `RpmRedArc` |
| Gear label | `GearLabel` |
| Drive mode label | `DriveModeLabel` |
| Coolant temp label | `CoolantLabel` |
| Fuel range label | `FuelRangeLabel` |
| Tire FL label | `TireFrontLeft` |
| Tire FR label | `TireFrontRight` |
| Tire RL label | `TireRearLeft` |
| Tire RR label | `TireRearRight` |
| Vehicle image | `VehicleImg` |
| Odometer label | `OdometerLabel` |
| Ambient temp label | `AmbientTempLabel` |

---

## Color Palette

| Role | Hex |
|------|-----|
| Background | `#0a0a0a` |
| Panel/gauge background | `#111111` – `#1a1a1a` |
| Accent / arc fill | `#00e5cc` (teal/cyan) |
| Primary text | `#ffffff` |
| Secondary text | `#aaaaaa` |
| Redline arc | `#ff2222` |
| Gauge tick marks | `#555555` |

---

## Fonts

- Speed/RPM value: large bold font (48–72pt equivalent), white
- Labels/units: small regular font (14–18pt), grey
- Use LVGL built-in fonts or import custom fonts via SquareLine Studio font tool
- Recommended: Montserrat or Roboto (bold variants for numbers)

---

## File Structure (planned)

```
esp32-touch-ui/
├── CLAUDE.md
├── ui/                  # SquareLine Studio export output (do not manually edit)
│   ├── ui.c
│   ├── ui.h
│   └── assets/
├── src/
│   ├── main.cpp         # app_main, LVGL init, data update loop
│   ├── display.cpp      # display driver init (SPI/RGB + touch)
│   └── sensors.cpp      # mock or real sensor data feeds
├── platformio.ini       # or CMakeLists.txt for ESP-IDF
└── README.md
```

---

## Key Constraints

- All UI edits happen in SquareLine Studio, not in exported `ui/` files
- LVGL version in firmware must match SquareLine Studio export version
- lv_arc value ranges set in SLS must align with firmware scaling logic
- Redline arc on tachometer: either a second overlapping arc (static, always visible from 7–8) or dynamically colored — static separate arc is simpler
- Touch input is optional for this read-only dashboard but should be wired up for future interaction (drive mode switching, screen brightness)
