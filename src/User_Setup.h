/**
 * User_Setup.h — TFT_eSPI display driver configuration
 * Place in project root OR in the TFT_eSPI library folder.
 *
 * Configured for: ILI9488 3.5" 480×320 → adjust for your panel.
 * For 800×480 panels (ILI9488/ST7796 via SPI), update TFT_WIDTH/HEIGHT.
 *
 * !! EDIT PINS to match your physical wiring before building !!
 */

// ── Driver select ──────────────────────────────────────────────────────────
#define ILI9488_DRIVER     // 3.5" SPI — swap for ST7796_DRIVER if needed
// #define ST7796_DRIVER
// #define ILI9341_DRIVER

// ── Display dimensions ────────────────────────────────────────────────────
#define TFT_WIDTH  480     // Update to 800 for 800×480 panels
#define TFT_HEIGHT 320     // Update to 480 for 800×480 panels

// ── ESP32-S3 SPI pins — adjust to your PCB ────────────────────────────────
#define TFT_MISO  13
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_CS    10   // Chip select
#define TFT_DC     9   // Data/Command
#define TFT_RST    14  // Reset (or -1 to use EN/RST)
#define TFT_BL     -1  // Backlight (PWM via separate pin if needed)

// ── SPI frequency ─────────────────────────────────────────────────────────
#define SPI_FREQUENCY       40000000   // 40 MHz — reduce if unstable
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY  2500000

// ── Color depth ───────────────────────────────────────────────────────────
#define TFT_RGB_ORDER TFT_RGB   // or TFT_BGR if colors are swapped

// ── Fonts to load ─────────────────────────────────────────────────────────
#define LOAD_GLCD   // Font 1 (Adafruit GLCD, 5x7)
#define LOAD_FONT2  // Font 2
#define LOAD_FONT4  // Font 4
#define LOAD_FONT6  // Font 6 (48pt digits only)
#define LOAD_FONT7  // Font 7 (7-seg style)
#define LOAD_FONT8  // Font 8 (large 75pt digits only)
#define LOAD_GFXFF  // FreeFonts — enable if using GFX fonts
#define SMOOTH_FONT

// ── LVGL integration ──────────────────────────────────────────────────────
// TFT_eSPI works alongside LVGL — LVGL manages the framebuffer,
// TFT_eSPI provides the low-level flush via display_flush_cb() in display.cpp
