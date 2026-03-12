/**
 * ui.c — Dashboard LVGL widget tree
 *
 * Layout (1280×800 landscape via PPA 270° rotation):
 *   [  Speedometer  |  Center panel  |  Tachometer  ]
 *   cx=260           480–800          cx=1020
 */

#include "ui.h"
#include <stdio.h>
#include <math.h>

// ── Widget handles ────────────────────────────────────────────────────────────
lv_obj_t *ui_Screen1;
lv_obj_t *ui_SpeedPanel;
lv_obj_t *ui_SpeedBgArc;
lv_obj_t *ui_SpeedArc;
lv_obj_t *ui_SpeedLabel;
lv_obj_t *ui_SpeedUnitLabel;
lv_obj_t *ui_FuelRangeLabel;

lv_obj_t *ui_RpmPanel;
lv_obj_t *ui_RpmBgArc;
lv_obj_t *ui_RpmArc;
lv_obj_t *ui_RpmRedArc;
lv_obj_t *ui_GearLabel;
lv_obj_t *ui_DriveModeLabel;
lv_obj_t *ui_CoolantLabel;

lv_obj_t *ui_CenterPanel;
lv_obj_t *ui_VehicleImg;
lv_obj_t *ui_TireFrontLeft;
lv_obj_t *ui_TireFrontRight;
lv_obj_t *ui_TireRearLeft;
lv_obj_t *ui_TireRearRight;
lv_obj_t *ui_OdometerLabel;

lv_obj_t *ui_AmbientTempLabel;
lv_obj_t *ui_DriveModeHeader;
lv_obj_t *ui_AdasIcon;

// ── Layout constants ──────────────────────────────────────────────────────────
// Screen: 1280×800 (LVGL canvas after PPA 270° rotation)
#define SCREEN_W       1280
#define SCREEN_H       800

// Gauge geometry — fits left/right thirds of 1280px screen
#define GAUGE_DIAM     460    // px (radius 230)
#define GAUGE_R        230
#define ARC_BG_W       24     // grey background ring width
#define ARC_VAL_W      22     // teal value arc width
#define TICK_LABEL_R   265    // radius for scale numbers
#define TICK_LW        36     // label bounding box width
#define TICK_LH        24     // label bounding box height

// Speedometer center (absolute screen coords)
#define SPD_CX         260
#define SPD_CY         420

// Tachometer center (absolute screen coords)
#define TAC_CX         1020
#define TAC_CY         420

// Arc angles: start=135°, end=45° → 270° clockwise sweep, gap at bottom
#define ARC_DEG_START  135
#define ARC_DEG_END    45

// Center panel extents
#define CTR_X          480
#define CTR_W          320
#define CTR_CX         640   // 480 + 160
#define CTR_VEH_CY     400   // vehicle image vertical center

static const float DEG2RAD = 3.14159265f / 180.0f;

// ── Helpers ───────────────────────────────────────────────────────────────────

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                              const lv_font_t *font, lv_color_t color,
                              int x, int y, lv_text_align_t align, int w)
{
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, align, LV_PART_MAIN);
    lv_obj_set_pos(lbl, x, y);
    if (w > 0) lv_obj_set_width(lbl, w);
    return lbl;
}

// Place a scale tick label centered at (cx + r*cos(deg), cy + r*sin(deg))
static void tick_label(lv_obj_t *parent, int cx, int cy, int r,
                        float deg, const char *text,
                        const lv_font_t *font, lv_color_t color)
{
    float rad = deg * DEG2RAD;
    int x = (int)(cx + r * cosf(rad)) - TICK_LW / 2;
    int y = (int)(cy + r * sinf(rad)) - TICK_LH / 2;
    lv_obj_t *lbl = lv_label_create(parent);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(lbl, TICK_LW, TICK_LH);
    lv_obj_set_pos(lbl, x, y);
}

static lv_obj_t *make_gauge_bg(lv_obj_t *parent, int cx, int cy)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, GAUGE_DIAM, GAUGE_DIAM);
    lv_obj_set_pos(arc, cx - GAUGE_R, cy - GAUGE_R);
    lv_arc_set_bg_angles(arc, ARC_DEG_START, ARC_DEG_END);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x2a2a2a), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, ARC_BG_W, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

static lv_obj_t *make_value_arc(lv_obj_t *parent, int cx, int cy, int range_max)
{
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_set_size(arc, GAUGE_DIAM, GAUGE_DIAM);
    lv_obj_set_pos(arc, cx - GAUGE_R, cy - GAUGE_R);
    lv_arc_set_bg_angles(arc, ARC_DEG_START, ARC_DEG_END);
    lv_arc_set_range(arc, 0, range_max);
    lv_arc_set_value(arc, 0);
    lv_obj_set_style_arc_opa(arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x00e5cc), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, ARC_VAL_W, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

// ── Speedometer ───────────────────────────────────────────────────────────────

static void build_speedometer(lv_obj_t *screen)
{
    // Thin outer decorative rim
    lv_obj_t *rim = lv_arc_create(screen);
    lv_obj_set_size(rim, GAUGE_DIAM + 16, GAUGE_DIAM + 16);
    lv_obj_set_pos(rim, SPD_CX - GAUGE_R - 8, SPD_CY - GAUGE_R - 8);
    lv_arc_set_bg_angles(rim, ARC_DEG_START, ARC_DEG_END);
    lv_arc_set_range(rim, 0, 100);
    lv_arc_set_value(rim, 0);
    lv_obj_set_style_arc_color(rim, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(rim, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(rim, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(rim, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(rim, LV_OBJ_FLAG_CLICKABLE);

    // Dummy panel handle (unused but kept for compat)
    ui_SpeedPanel = screen;

    // Background arc
    ui_SpeedBgArc = make_gauge_bg(screen, SPD_CX, SPD_CY);

    // Value arc (teal, range 0–220 mph)
    ui_SpeedArc = make_value_arc(screen, SPD_CX, SPD_CY, 220);

    // Scale numbers around the arc (0, 20, 40, … 220)
    const lv_color_t tick_col = lv_color_hex(0xaaaaaa);
    const lv_font_t *tick_font = &lv_font_montserrat_14;
    const int speed_ticks[] = {0, 20, 40, 60, 80, 100, 120, 140, 160, 180, 200, 220};
    char buf[8];
    for (int i = 0; i < 12; i++) {
        int v = speed_ticks[i];
        float angle = ARC_DEG_START + (v / 220.0f) * 270.0f;
        snprintf(buf, sizeof(buf), "%d", v);
        tick_label(screen, SPD_CX, SPD_CY, TICK_LABEL_R, angle, buf, tick_font, tick_col);
    }

    // Speed number — large, centered on gauge
    ui_SpeedLabel = lv_label_create(screen);
    lv_label_set_text(ui_SpeedLabel, "0");
    lv_obj_set_style_text_font(ui_SpeedLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_SpeedLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_SpeedLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_SpeedLabel, 120, 64);
    lv_obj_set_pos(ui_SpeedLabel, SPD_CX - 60, SPD_CY - 48);

    // "mph"
    ui_SpeedUnitLabel = make_label(screen, "mph",
        &lv_font_montserrat_16, lv_color_hex(0x888888),
        SPD_CX - 30, SPD_CY + 22, LV_TEXT_ALIGN_CENTER, 60);

    // Fuel range — below gauge, centered
    ui_FuelRangeLabel = make_label(screen, "200 mi",
        &lv_font_montserrat_14, lv_color_hex(0xcccccc),
        SPD_CX - 60, SPD_CY + GAUGE_R + 26, LV_TEXT_ALIGN_CENTER, 120);
}

// ── Tachometer ────────────────────────────────────────────────────────────────

static void build_tachometer(lv_obj_t *screen)
{
    // Thin outer decorative rim
    lv_obj_t *rim = lv_arc_create(screen);
    lv_obj_set_size(rim, GAUGE_DIAM + 16, GAUGE_DIAM + 16);
    lv_obj_set_pos(rim, TAC_CX - GAUGE_R - 8, TAC_CY - GAUGE_R - 8);
    lv_arc_set_bg_angles(rim, ARC_DEG_START, ARC_DEG_END);
    lv_arc_set_range(rim, 0, 100);
    lv_arc_set_value(rim, 0);
    lv_obj_set_style_arc_color(rim, lv_color_hex(0x444444), LV_PART_MAIN);
    lv_obj_set_style_arc_width(rim, 3, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(rim, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(rim, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(rim, LV_OBJ_FLAG_CLICKABLE);

    ui_RpmPanel = screen;

    // Background arc (grey)
    ui_RpmBgArc = make_gauge_bg(screen, TAC_CX, TAC_CY);

    // Main value arc (teal, 0–80 internal = 0–8000 RPM)
    ui_RpmArc = make_value_arc(screen, TAC_CX, TAC_CY, 80);

    // Static redline arc — always visible from 7000–8000 rpm (70–80 internal)
    // angle for 70/80 at 270° sweep: 135 + (70/80)*270 = 371.25° → 11°
    ui_RpmRedArc = lv_arc_create(screen);
    lv_obj_set_size(ui_RpmRedArc, GAUGE_DIAM, GAUGE_DIAM);
    lv_obj_set_pos(ui_RpmRedArc, TAC_CX - GAUGE_R, TAC_CY - GAUGE_R);
    lv_arc_set_bg_angles(ui_RpmRedArc, 11, ARC_DEG_END);
    lv_arc_set_range(ui_RpmRedArc, 0, 100);
    lv_arc_set_value(ui_RpmRedArc, 100);
    lv_obj_set_style_arc_opa(ui_RpmRedArc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_arc_color(ui_RpmRedArc, lv_color_hex(0xFF2222), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ui_RpmRedArc, ARC_VAL_W, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(ui_RpmRedArc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_clear_flag(ui_RpmRedArc, LV_OBJ_FLAG_CLICKABLE);

    // Scale numbers 0–8 (RPM ×1000)
    const lv_color_t tick_col = lv_color_hex(0xaaaaaa);
    const lv_font_t *tick_font = &lv_font_montserrat_14;
    char buf[4];
    for (int v = 0; v <= 8; v++) {
        float angle = ARC_DEG_START + (v / 8.0f) * 270.0f;
        snprintf(buf, sizeof(buf), "%d", v);
        tick_label(screen, TAC_CX, TAC_CY, TICK_LABEL_R, angle, buf, tick_font, tick_col);
    }

    // "×1000" small label near 8 position (lower-right)
    make_label(screen, "x1000",
        &lv_font_montserrat_12, lv_color_hex(0x666666),
        TAC_CX + GAUGE_R - 40, TAC_CY + GAUGE_R - 20,
        LV_TEXT_ALIGN_RIGHT, 80);

    // Gear number — centered
    ui_GearLabel = lv_label_create(screen);
    lv_label_set_text(ui_GearLabel, "1");
    lv_obj_set_style_text_font(ui_GearLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_GearLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_GearLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_GearLabel, 80, 64);
    lv_obj_set_pos(ui_GearLabel, TAC_CX - 40, TAC_CY - 48);

    // Drive mode
    ui_DriveModeLabel = make_label(screen, "Sport",
        &lv_font_montserrat_16, lv_color_hex(0x888888),
        TAC_CX - 36, TAC_CY + 22, LV_TEXT_ALIGN_CENTER, 72);

    // Coolant temp
    ui_CoolantLabel = make_label(screen, "195\xc2\xb0""F",
        &lv_font_montserrat_14, lv_color_hex(0xcccccc),
        TAC_CX - 60, TAC_CY + GAUGE_R + 26, LV_TEXT_ALIGN_CENTER, 120);
}

// ── Center panel ──────────────────────────────────────────────────────────────

static void build_center_panel(lv_obj_t *screen)
{
    // Transparent container for center region
    ui_CenterPanel = lv_obj_create(screen);
    lv_obj_set_size(ui_CenterPanel, CTR_W, SCREEN_H);
    lv_obj_set_pos(ui_CenterPanel, CTR_X, 0);
    lv_obj_set_style_bg_opa(ui_CenterPanel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_opa(ui_CenterPanel, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(ui_CenterPanel, LV_OBJ_FLAG_SCROLLABLE);

    // Vehicle silhouette — 80×160, centered at (160, 390) within panel
    ui_VehicleImg = lv_img_create(ui_CenterPanel);
    lv_img_set_src(ui_VehicleImg, &vehicle_top);
    lv_obj_set_pos(ui_VehicleImg, 80, 310);   // 160-40=120 for x center, 390-80=310

    // Tire pressure labels — flanking the vehicle image
    // Vehicle occupies roughly x:80..160 (w=80), y:310..470 (h=160) in panel
    // Front tires at ~y=330, rear at ~y=435

    ui_TireFrontLeft = lv_label_create(ui_CenterPanel);
    lv_label_set_text(ui_TireFrontLeft, "41\npsi");
    lv_obj_set_style_text_font(ui_TireFrontLeft, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_TireFrontLeft, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_TireFrontLeft, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_TireFrontLeft, 60, 50);
    lv_obj_set_pos(ui_TireFrontLeft, 6, 318);

    ui_TireFrontRight = lv_label_create(ui_CenterPanel);
    lv_label_set_text(ui_TireFrontRight, "41\npsi");
    lv_obj_set_style_text_font(ui_TireFrontRight, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_TireFrontRight, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_TireFrontRight, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_TireFrontRight, 60, 50);
    lv_obj_set_pos(ui_TireFrontRight, 254, 318);

    ui_TireRearLeft = lv_label_create(ui_CenterPanel);
    lv_label_set_text(ui_TireRearLeft, "45\npsi");
    lv_obj_set_style_text_font(ui_TireRearLeft, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_TireRearLeft, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_TireRearLeft, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_TireRearLeft, 60, 50);
    lv_obj_set_pos(ui_TireRearLeft, 6, 420);

    ui_TireRearRight = lv_label_create(ui_CenterPanel);
    lv_label_set_text(ui_TireRearRight, "45\npsi");
    lv_obj_set_style_text_font(ui_TireRearRight, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_TireRearRight, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_TireRearRight, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_TireRearRight, 60, 50);
    lv_obj_set_pos(ui_TireRearRight, 254, 420);

    // Odometer — below vehicle
    ui_OdometerLabel = lv_label_create(ui_CenterPanel);
    lv_label_set_text(ui_OdometerLabel, "1250 mi");
    lv_obj_set_style_text_font(ui_OdometerLabel, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ui_OdometerLabel, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui_OdometerLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_size(ui_OdometerLabel, 160, 24);
    lv_obj_set_pos(ui_OdometerLabel, 80, 490);
}

// ── Header bar ────────────────────────────────────────────────────────────────

static void build_header(lv_obj_t *screen)
{
    // ADAS icon — top center (above vehicle)
    ui_AdasIcon = make_label(screen, "[ADAS]",
        &lv_font_montserrat_12, lv_color_hex(0x00e5cc),
        CTR_CX - 40, 28, LV_TEXT_ALIGN_CENTER, 80);

    // Ambient temp — top-right area of center
    ui_AmbientTempLabel = make_label(screen, "72\xc2\xb0""F",
        &lv_font_montserrat_18, lv_color_hex(0xFFFFFF),
        CTR_CX + 10, 22, LV_TEXT_ALIGN_LEFT, 80);

    // Drive mode header — next to temp
    ui_DriveModeHeader = make_label(screen, "M",
        &lv_font_montserrat_18, lv_color_hex(0xFFFFFF),
        CTR_CX + 72, 22, LV_TEXT_ALIGN_LEFT, 24);

    // Subtle vertical divider lines between panels (cosmetic)
    // Left divider at x=480
    lv_obj_t *div_l = lv_obj_create(screen);
    lv_obj_set_size(div_l, 1, SCREEN_H);
    lv_obj_set_pos(div_l, CTR_X, 0);
    lv_obj_set_style_bg_color(div_l, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div_l, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(div_l, LV_OPA_TRANSP, LV_PART_MAIN);

    // Right divider at x=800
    lv_obj_t *div_r = lv_obj_create(screen);
    lv_obj_set_size(div_r, 1, SCREEN_H);
    lv_obj_set_pos(div_r, CTR_X + CTR_W, 0);
    lv_obj_set_style_bg_color(div_r, lv_color_hex(0x222222), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(div_r, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(div_r, LV_OPA_TRANSP, LV_PART_MAIN);
}

// ── Startup sweep animation ───────────────────────────────────────────────────

static void startup_anim(void)
{
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, ui_SpeedArc);
    lv_anim_set_values(&a, 0, 220);
    lv_anim_set_time(&a, 1200);
    lv_anim_set_playback_time(&a, 800);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_arc_set_value);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
    lv_anim_start(&a);

    lv_anim_t b;
    lv_anim_init(&b);
    lv_anim_set_var(&b, ui_RpmArc);
    lv_anim_set_values(&b, 0, 80);
    lv_anim_set_time(&b, 1200);
    lv_anim_set_playback_time(&b, 800);
    lv_anim_set_exec_cb(&b, (lv_anim_exec_xcb_t)lv_arc_set_value);
    lv_anim_set_path_cb(&b, lv_anim_path_ease_in_out);
    lv_anim_start(&b);
}

// ── ui_init ───────────────────────────────────────────────────────────────────

void ui_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(0x0a0a0a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);

    build_speedometer(ui_Screen1);
    build_tachometer(ui_Screen1);
    build_center_panel(ui_Screen1);
    build_header(ui_Screen1);

    lv_scr_load(ui_Screen1);
    startup_anim();
}
