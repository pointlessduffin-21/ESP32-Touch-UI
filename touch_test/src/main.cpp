/**
 * touch_test/main.cpp — GSL3680 touch verification for JC8012P4A1C_I_W_Y
 *
 * Display: 1280×800 (LVGL after 90° PPA rotation)
 *
 * Shows:
 *  - Live X/Y coordinates and touch state (PRESSED / RELEASED)
 *  - Large touch canvas — a crosshair follows your finger
 *  - 3 tap-test buttons (highlight on press)
 *  - Slider (drag to test swipe/drag input)
 *  - "CLEAR" button to wipe the canvas dot trail
 *
 * If touch is broken the status will stay "RELEASED" and coords won't change.
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "display.h"

static const char *TAG = "touch-test";

// ── Widget handles ────────────────────────────────────────────────────────────
static lv_obj_t *lbl_status;
static lv_obj_t *lbl_coords;
static lv_obj_t *lbl_event;
static lv_obj_t *canvas;
static lv_obj_t *crosshair_h;   // horizontal line of crosshair
static lv_obj_t *crosshair_v;   // vertical line of crosshair
static lv_obj_t *dot;           // touch dot
static lv_obj_t *lbl_slider_val;

static bool s_touching = false;

// ── Touch canvas: move crosshair + dot to touch position ─────────────────────
static void canvas_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSING || code == LV_EVENT_PRESSED) {
        lv_indev_t *indev = lv_indev_get_act();
        if (!indev) return;

        lv_point_t p;
        lv_indev_get_point(indev, &p);

        // Show absolute LVGL screen coordinates
        lv_label_set_text_fmt(lbl_coords, "X: %4d   Y: %4d", (int)p.x, (int)p.y);

        if (!s_touching) {
            s_touching = true;
            lv_label_set_text(lbl_status, "TOUCH: PRESSED");
            lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x00e5cc), 0);
            lv_obj_remove_flag(crosshair_h, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(crosshair_v, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(dot, LV_OBJ_FLAG_HIDDEN);
        }

        // Crosshair + dot positions are relative to the canvas
        lv_coord_t cx = lv_obj_get_x(canvas);
        lv_coord_t cy = lv_obj_get_y(canvas);
        lv_coord_t cw = lv_obj_get_width(canvas);
        lv_coord_t ch = lv_obj_get_height(canvas);

        lv_coord_t rx = p.x - cx;   // position within canvas
        lv_coord_t ry = p.y - cy;

        // Clamp to canvas bounds
        if (rx < 0) rx = 0;
        if (ry < 0) ry = 0;
        if (rx > cw) rx = cw;
        if (ry > ch) ry = ch;

        // Move crosshair lines
        lv_obj_set_pos(crosshair_h, 0, ry - 1);
        lv_obj_set_pos(crosshair_v, rx - 1, 0);

        // Move dot (50×50, centered on touch point)
        lv_obj_set_pos(dot, rx - 25, ry - 25);

        ESP_LOGD(TAG, "Touch LVGL x=%d y=%d", (int)p.x, (int)p.y);
    }
    else if (code == LV_EVENT_RELEASED) {
        s_touching = false;
        lv_label_set_text(lbl_status, "TOUCH: RELEASED");
        lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x666666), 0);
        lv_obj_add_flag(crosshair_h, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(crosshair_v, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    }
}

// ── Button callbacks ──────────────────────────────────────────────────────────
static void btn_event_cb(lv_event_t *e)
{
    const char *name = (const char *)lv_event_get_user_data(e);
    lv_label_set_text_fmt(lbl_event, "TAPPED: %s", name);
    ESP_LOGI(TAG, "Button tapped: %s", name);
}

static void btn_clear_cb(lv_event_t *e)
{
    lv_label_set_text(lbl_coords, "X:    0   Y:    0");
    lv_label_set_text(lbl_event, "Canvas cleared");
    ESP_LOGI(TAG, "Clear tapped");
}

// ── Slider callback ───────────────────────────────────────────────────────────
static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target_obj(e);
    int val = lv_slider_get_value(slider);
    lv_label_set_text_fmt(lbl_slider_val, "%d%%", val);
    lv_label_set_text_fmt(lbl_event, "Slider: %d%%", val);
}

// ── Build the UI ──────────────────────────────────────────────────────────────
static void ui_init(void)
{
    // LVGL display is 1280 wide × 800 tall after 90° PPA rotation
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0a0a0a), 0);

    // ── Header bar ───────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(scr);
    lv_obj_set_size(hdr, 1280, 80);
    lv_obj_set_pos(hdr, 0, 0);
    lv_obj_set_style_bg_color(hdr, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(hdr, 0, 0);
    lv_obj_set_style_radius(hdr, 0, 0);
    lv_obj_clear_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(hdr);
    lv_label_set_text(title, "TOUCH TEST");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00e5cc), 0);
    lv_obj_align(title, LV_ALIGN_LEFT_MID, 20, 0);

    lbl_status = lv_label_create(hdr);
    lv_label_set_text(lbl_status, "TOUCH: RELEASED");
    lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_status, lv_color_hex(0x666666), 0);
    lv_obj_align(lbl_status, LV_ALIGN_CENTER, 0, 0);

    lbl_coords = lv_label_create(hdr);
    lv_label_set_text(lbl_coords, "X:    0   Y:    0");
    lv_obj_set_style_text_font(lbl_coords, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_coords, lv_color_hex(0xffffff), 0);
    lv_obj_align(lbl_coords, LV_ALIGN_RIGHT_MID, -20, 0);

    // ── Event label (below header) ────────────────────────────────────────────
    lbl_event = lv_label_create(scr);
    lv_label_set_text(lbl_event, "Touch the canvas, tap a button, or drag the slider");
    lv_obj_set_style_text_font(lbl_event, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_event, lv_color_hex(0x888888), 0);
    lv_obj_set_pos(lbl_event, 20, 86);

    // ── Touch canvas ─────────────────────────────────────────────────────────
    // Sits between the header (y=80) and the bottom bar (y=700)
    canvas = lv_obj_create(scr);
    lv_obj_set_pos(canvas, 0, 110);
    lv_obj_set_size(canvas, 1280, 580);
    lv_obj_set_style_bg_color(canvas, lv_color_hex(0x0d0d0d), 0);
    lv_obj_set_style_border_color(canvas, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_border_width(canvas, 1, 0);
    lv_obj_set_style_radius(canvas, 0, 0);
    lv_obj_set_style_pad_all(canvas, 0, 0);
    lv_obj_clear_flag(canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(canvas, LV_OBJ_FLAG_CLICKABLE);

    // Center hint
    lv_obj_t *hint = lv_label_create(canvas);
    lv_label_set_text(hint, "Touch anywhere");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0x222222), 0);
    lv_obj_center(hint);
    lv_obj_clear_flag(hint, LV_OBJ_FLAG_CLICKABLE);

    // Crosshair — horizontal line (full canvas width, 2px tall)
    crosshair_h = lv_obj_create(canvas);
    lv_obj_set_size(crosshair_h, 1280, 2);
    lv_obj_set_style_bg_color(crosshair_h, lv_color_hex(0x00e5cc), 0);
    lv_obj_set_style_bg_opa(crosshair_h, LV_OPA_50, 0);
    lv_obj_set_style_border_width(crosshair_h, 0, 0);
    lv_obj_set_style_radius(crosshair_h, 0, 0);
    lv_obj_clear_flag(crosshair_h, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(crosshair_h, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(crosshair_h, LV_OBJ_FLAG_HIDDEN);

    // Crosshair — vertical line (2px wide, full canvas height)
    crosshair_v = lv_obj_create(canvas);
    lv_obj_set_size(crosshair_v, 2, 580);
    lv_obj_set_style_bg_color(crosshair_v, lv_color_hex(0x00e5cc), 0);
    lv_obj_set_style_bg_opa(crosshair_v, LV_OPA_50, 0);
    lv_obj_set_style_border_width(crosshair_v, 0, 0);
    lv_obj_set_style_radius(crosshair_v, 0, 0);
    lv_obj_clear_flag(crosshair_v, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(crosshair_v, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(crosshair_v, LV_OBJ_FLAG_HIDDEN);

    // Touch dot (50×50 circle)
    dot = lv_obj_create(canvas);
    lv_obj_set_size(dot, 50, 50);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0x00e5cc), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_90, 0);
    lv_obj_set_style_border_color(dot, lv_color_hex(0xffffff), 0);
    lv_obj_set_style_border_width(dot, 2, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);

    // Canvas events
    lv_obj_add_event_cb(canvas, canvas_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(canvas, canvas_event_cb, LV_EVENT_PRESSED,  NULL);
    lv_obj_add_event_cb(canvas, canvas_event_cb, LV_EVENT_RELEASED, NULL);

    // ── Bottom bar ────────────────────────────────────────────────────────────
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_pos(bar, 0, 700);
    lv_obj_set_size(bar, 1280, 100);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x111111), 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);

    // 3 test buttons
    static const char *btn_names[] = {"BTN A", "BTN B", "BTN C"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_btn_create(bar);
        lv_obj_set_size(btn, 160, 60);
        lv_obj_set_pos(btn, 16 + i * 180, 18);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1e1e1e), 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x00e5cc), LV_STATE_PRESSED);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x333333), 0);
        lv_obj_set_style_border_width(btn, 1, 0);
        lv_obj_set_style_radius(btn, 8, 0);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_names[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xffffff), 0);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0x000000), LV_STATE_PRESSED);
        lv_obj_center(lbl);

        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)btn_names[i]);
    }

    // CLEAR button
    lv_obj_t *btn_clr = lv_btn_create(bar);
    lv_obj_set_size(btn_clr, 160, 60);
    lv_obj_set_pos(btn_clr, 556, 18);
    lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_bg_color(btn_clr, lv_color_hex(0xff4444), LV_STATE_PRESSED);
    lv_obj_set_style_border_color(btn_clr, lv_color_hex(0x555555), 0);
    lv_obj_set_style_border_width(btn_clr, 1, 0);
    lv_obj_set_style_radius(btn_clr, 8, 0);
    lv_obj_t *lbl_clr = lv_label_create(btn_clr);
    lv_label_set_text(lbl_clr, "CLEAR");
    lv_obj_set_style_text_font(lbl_clr, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_clr, lv_color_hex(0xffffff), 0);
    lv_obj_center(lbl_clr);
    lv_obj_add_event_cb(btn_clr, btn_clear_cb, LV_EVENT_CLICKED, NULL);

    // Slider
    lv_obj_t *slider = lv_slider_create(bar);
    lv_obj_set_size(slider, 400, 16);
    lv_obj_set_pos(slider, 740, 42);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x333333), 0);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x00e5cc), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0xffffff), LV_PART_KNOB);
    lv_obj_set_style_radius(slider, 8, LV_PART_KNOB);
    lv_slider_set_range(slider, 0, 100);
    lv_slider_set_value(slider, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    lbl_slider_val = lv_label_create(bar);
    lv_label_set_text(lbl_slider_val, "0%");
    lv_obj_set_style_text_font(lbl_slider_val, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_slider_val, lv_color_hex(0xaaaaaa), 0);
    lv_obj_set_pos(lbl_slider_val, 1158, 42);

    ESP_LOGI(TAG, "Touch test UI ready — 1280x800");
}

extern "C" void app_main(void)
{
    display_init();

    display_lock();
    ui_init();
    display_unlock();

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
