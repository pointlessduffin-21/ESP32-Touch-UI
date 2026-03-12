/**
 * ui.c -- CubeRTMS Manufacturing Dashboard, LVGL v9, ESP32-P4
 *
 * Resolution: 1280 x 800 (landscape)
 * Theme:      dark (#0d0d0d), teal accents (#00e5cc), red alerts
 *
 * Screens:
 *   Screen 0 -- Centralized Line Status (overview of all lines)
 *   Screen 1 -- Line Dashboard (Harness Line 01 detail: KPI, chart, table, alerts)
 *   Screen 2 -- OEE Dashboard (A x P x Q breakdown, losses, trend)
 *   Screen 3 -- Station Performance (yield cards, daily progress)
 *
 * Navigation: swipe left/right, or tap nav bar to jump between screens.
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>

/* -- Color palette --------------------------------------------------------- */
#define C_BG         0x0d0d0d
#define C_NAV        0x0d1117
#define C_SURFACE    0x1a1a1a
#define C_SURFACE2   0x111111
#define C_BORDER     0x2a2a2a
#define C_GREEN      0x00b894
#define C_TEAL       0x00e5cc
#define C_RED        0xe74c3c
#define C_ORANGE     0xf39c12
#define C_WHITE      0xffffff
#define C_GRAY       0xaaaaaa
#define C_DARK_GRAY  0x555555
#define C_PLAN_BAR   0x444444
#define C_BLUE_ACK   0x2563eb

/* Screen geometry */
#define SCREEN_W     1280
#define SCREEN_H     800

/* -- Widget handles -------------------------------------------------------- */
lv_obj_t *ui_ScreenCentralized;
lv_obj_t *ui_ScreenLineDash;
lv_obj_t *ui_ScreenOEE;
lv_obj_t *ui_ScreenStation;

lv_obj_t *ui_KpiActualVal;
lv_obj_t *ui_KpiEffVal;
lv_obj_t *ui_KpiDefVal;
lv_obj_t *ui_Chart;
lv_chart_series_t *ui_ChartSeriesPlan;
lv_chart_series_t *ui_ChartSeriesActual;
lv_obj_t *ui_Table;
lv_obj_t *ui_AlertList;
lv_obj_t *ui_BtnToday;
lv_obj_t *ui_BtnYesterday;
lv_obj_t *ui_BtnLast7;

/* -- Internal state -------------------------------------------------------- */
static int s_period_active = 0;
static lv_obj_t *s_alert_rows[MAX_ALERTS];
static lv_obj_t *s_alert_ack_btns[MAX_ALERTS];

/* Centralized screen line card labels */
static lv_obj_t *s_line_actual_lbl[NUM_LINES];
static lv_obj_t *s_line_pva_lbl[NUM_LINES];
static lv_obj_t *s_line_plan_lbl[NUM_LINES];

/* OEE screen labels */
static lv_obj_t *s_oee_overall_lbl;
static lv_obj_t *s_oee_avail_lbl;
static lv_obj_t *s_oee_perf_lbl;
static lv_obj_t *s_oee_qual_lbl;

/* Station screen labels */
static lv_obj_t *s_stn_yield_lbl[NUM_STATIONS];
static lv_obj_t *s_stn_pass_lbl[NUM_STATIONS];
static lv_obj_t *s_daily_progress_bar;
static lv_obj_t *s_daily_progress_lbl;

/* Current screen index */
static int s_cur_screen = 0;

/* Forward decls */
static void nav_go_screen(int idx);
static void build_alert_row(lv_obj_t *parent, int idx);

/* =========================================================================
   HELPERS
   ========================================================================= */

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

static lv_obj_t *make_container(lv_obj_t *parent,
                                 int x, int y, int w, int h,
                                 uint32_t bg_color, uint32_t border_color,
                                 int border_w, int radius)
{
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_style_bg_color(obj, lv_color_hex(bg_color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    if (border_w > 0) {
        lv_obj_set_style_border_color(obj, lv_color_hex(border_color), LV_PART_MAIN);
        lv_obj_set_style_border_width(obj, border_w, LV_PART_MAIN);
        lv_obj_set_style_border_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    } else {
        lv_obj_set_style_border_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    }
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static lv_obj_t *make_button(lv_obj_t *parent, const char *text,
                              const lv_font_t *font,
                              int x, int y, int w, int h,
                              uint32_t bg_color, uint32_t text_color, int radius)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, w, h);
    lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_font(lbl, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl, lv_color_hex(text_color), LV_PART_MAIN);
    lv_obj_center(lbl);
    return btn;
}

static lv_obj_t *make_dot(lv_obj_t *parent, int x, int y, int r, uint32_t color)
{
    lv_obj_t *d = lv_obj_create(parent);
    lv_obj_set_pos(d, x, y);
    lv_obj_set_size(d, r * 2, r * 2);
    lv_obj_set_style_radius(d, r, LV_PART_MAIN);
    lv_obj_set_style_bg_color(d, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(d, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);
    return d;
}

/* =========================================================================
   CALLBACKS
   ========================================================================= */

static lv_obj_t *screen_for_idx(int idx)
{
    switch (idx) {
        case 0: return ui_ScreenCentralized;
        case 1: return ui_ScreenLineDash;
        case 2: return ui_ScreenOEE;
        case 3: return ui_ScreenStation;
        default: return ui_ScreenCentralized;
    }
}

static void nav_go_screen(int idx)
{
    if (idx < 0) idx = 0;
    if (idx > 3) idx = 3;
    if (idx == s_cur_screen) return;

    lv_scr_load_anim_t anim = (idx > s_cur_screen)
        ? LV_SCR_LOAD_ANIM_MOVE_LEFT : LV_SCR_LOAD_ANIM_MOVE_RIGHT;
    s_cur_screen = idx;
    lv_scr_load_anim(screen_for_idx(idx), anim, 300, 0, false);
}

static void cb_nav_btn(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    nav_go_screen(idx);
}

static void cb_gesture(lv_event_t *e)
{
    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev) return;
    lv_dir_t dir = lv_indev_get_gesture_dir(indev);
    if (dir == LV_DIR_LEFT)  nav_go_screen(s_cur_screen + 1);
    if (dir == LV_DIR_RIGHT) nav_go_screen(s_cur_screen - 1);
}

static void cb_period_btn(lv_event_t *e)
{
    int which = (int)(intptr_t)lv_event_get_user_data(e);
    lv_obj_set_style_bg_color(ui_BtnToday,     lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_BtnYesterday, lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_BtnLast7,     lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_t *active = (which == 0) ? ui_BtnToday
                     : (which == 1) ? ui_BtnYesterday
                     : ui_BtnLast7;
    lv_obj_set_style_bg_color(active, lv_color_hex(C_TEAL), LV_PART_MAIN);
    s_period_active = which;
}

static void cb_oee_btn(lv_event_t *e)
{
    (void)e;
    nav_go_screen(2);
}

static void cb_ack_btn(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= MAX_ALERTS) return;
    g_mfg.alerts[idx].acknowledged = true;
    if (s_alert_rows[idx])
        lv_obj_set_style_border_opa(s_alert_rows[idx], LV_OPA_TRANSP, LV_PART_MAIN);
    if (s_alert_ack_btns[idx])
        lv_obj_add_flag(s_alert_ack_btns[idx], LV_OBJ_FLAG_HIDDEN);
}

static void cb_line_card_tap(lv_event_t *e)
{
    (void)e;
    nav_go_screen(1);
}

static void cb_all_lines_btn(lv_event_t *e)
{
    (void)e;
    nav_go_screen(0);
}

/* =========================================================================
   NAV BAR  (y=0, h=48)
   ========================================================================= */

static void build_nav_bar(lv_obj_t *screen, int active_tab)
{
    lv_obj_t *nav = make_container(screen, 0, 0, SCREEN_W, 48,
                                   C_NAV, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(nav, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    make_dot(nav, 16, 14, 10, C_GREEN);
    make_label(nav, "CubeRTMS",
               &lv_font_montserrat_18, lv_color_hex(C_WHITE),
               40, 12, LV_TEXT_ALIGN_LEFT, 130);

    const char *nav_labels[] = {
        "Dashboard", "UNS Tree", "Resources", "Planner", "Andon",
        "Traceability", "Materials"
    };
    const int nav_x[] = { 200, 320, 430, 540, 640, 730, 860 };

    for (int i = 0; i < 7; i++) {
        bool is_active = (i == active_tab);
        lv_color_t col = is_active ? lv_color_hex(C_WHITE) : lv_color_hex(C_GRAY);
        lv_obj_t *lbl = make_label(nav, nav_labels[i],
                                   &lv_font_montserrat_14, col,
                                   nav_x[i], 15, LV_TEXT_ALIGN_LEFT, 110);
        lv_obj_add_flag(lbl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_pad_all(lbl, 4, LV_PART_MAIN);
        if (i == 0)
            lv_obj_add_event_cb(lbl, cb_nav_btn, LV_EVENT_CLICKED, (void *)(intptr_t)0);
    }

    make_label(nav, "System Admin",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               1080, 16, LV_TEXT_ALIGN_LEFT, 110);
    lv_obj_t *admin_badge = make_container(nav, 1190, 12, 50, 24, C_GREEN, 0, 0, 4);
    make_label(admin_badge, "Admin",
               &lv_font_montserrat_12, lv_color_hex(C_WHITE),
               0, 4, LV_TEXT_ALIGN_CENTER, 50);
}

/* =========================================================================
   SCREEN 0: CENTRALIZED LINE STATUS
   ========================================================================= */

static void build_line_card(lv_obj_t *parent, int idx, int x, int y)
{
    const LineOverview *ln = &g_lines[idx];
    int cw = 370, ch = 280;

    lv_obj_t *card = make_container(parent, x, y, cw, ch, C_SURFACE, C_BORDER, 1, 8);
    lv_obj_add_flag(card, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(card, cb_line_card_tap, LV_EVENT_CLICKED, NULL);

    make_label(card, ln->name,
               &lv_font_montserrat_18, lv_color_hex(C_WHITE),
               16, 12, LV_TEXT_ALIGN_LEFT, 220);

    uint32_t badge_col = ln->running ? C_GREEN : C_RED;
    lv_obj_t *badge = make_container(card, cw - 100, 12, 80, 26, badge_col, 0, 0, 4);
    make_label(badge, ln->running ? "Running" : "Stopped",
               &lv_font_montserrat_12, lv_color_hex(C_WHITE),
               0, 5, LV_TEXT_ALIGN_CENTER, 80);

    make_label(card, ln->part_number,
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               16, 40, LV_TEXT_ALIGN_LEFT, 200);

    /* OEE circle */
    uint32_t oee_col = (ln->oee > 50) ? C_GREEN : (ln->oee > 0) ? C_RED : C_DARK_GRAY;
    lv_obj_t *oee_c = make_container(card, 16, 68, 70, 70, oee_col, 0, 0, 35);
    char buf[32];
    make_label(oee_c, "OEE",
               &lv_font_montserrat_12, lv_color_hex(C_WHITE),
               0, 16, LV_TEXT_ALIGN_CENTER, 70);
    snprintf(buf, sizeof(buf), "%.1f%%", ln->oee);
    make_label(oee_c, buf,
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               0, 32, LV_TEXT_ALIGN_CENTER, 70);

    /* A P Q */
    snprintf(buf, sizeof(buf), "A:%.1f%%", ln->availability);
    make_label(card, buf, &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               95, 78, LV_TEXT_ALIGN_LEFT, 80);
    snprintf(buf, sizeof(buf), "P:%.1f%%", ln->performance);
    make_label(card, buf, &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               180, 78, LV_TEXT_ALIGN_LEFT, 80);
    snprintf(buf, sizeof(buf), "Q:%.1f%%", ln->quality);
    make_label(card, buf, &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               265, 78, LV_TEXT_ALIGN_LEFT, 80);

    /* Plan vs Actual */
    make_label(card, "Plan vs Actual",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               16, 150, LV_TEXT_ALIGN_LEFT, 150);
    snprintf(buf, sizeof(buf), "%.1f%%", ln->plan_vs_actual_pct);
    s_line_pva_lbl[idx] = make_label(card, buf,
               &lv_font_montserrat_18, lv_color_hex(C_RED),
               cw - 100, 148, LV_TEXT_ALIGN_RIGHT, 80);

    /* Progress bar */
    make_container(card, 16, 180, cw - 32, 8, C_DARK_GRAY, 0, 0, 4);
    float pct = ln->plan_vs_actual_pct;
    if (pct > 100.0f) pct = 100.0f;
    int bar_w = (int)((cw - 32) * pct / 100.0f);
    if (bar_w < 4) bar_w = 4;
    make_container(card, 16, 180, bar_w, 8, C_RED, 0, 0, 4);

    /* Bottom: Planned / Actual / Defects */
    int col_w = (cw - 32) / 3;
    const char *titles[] = {"Planned", "Actual", "Defects"};
    int vals[] = {ln->planned, ln->actual, ln->defects};
    uint32_t cols[] = {C_WHITE, C_GREEN, C_RED};
    for (int c = 0; c < 3; c++) {
        int cx = 16 + c * col_w;
        make_label(card, titles[c],
                   &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   cx, 200, LV_TEXT_ALIGN_CENTER, col_w);
        snprintf(buf, sizeof(buf), "%d", vals[c]);
        lv_obj_t *vl = make_label(card, buf,
                   &lv_font_montserrat_18, lv_color_hex(cols[c]),
                   cx, 218, LV_TEXT_ALIGN_CENTER, col_w);
        if (c == 1) s_line_actual_lbl[idx] = vl;
        if (c == 0) s_line_plan_lbl[idx] = vl;
    }
}

static void build_centralized_screen(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, SCREEN_W, SCREEN_H);

    build_nav_bar(screen, 0);

    make_label(screen, "Centralized Line Status",
               &lv_font_montserrat_24, lv_color_hex(C_WHITE),
               30, 64, LV_TEXT_ALIGN_LEFT, 400);
    make_label(screen, "Thursday, March 12, 2026 - All production lines",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               30, 96, LV_TEXT_ALIGN_LEFT, 500);

    char abuf[32];
    snprintf(abuf, sizeof(abuf), "  %d Active Alerts", g_mfg.total_alert_count);
    make_button(screen, abuf, &lv_font_montserrat_14,
                830, 72, 190, 36, C_RED, C_WHITE, 6);

    lv_obj_t *oee_rpt = make_button(screen, "OEE Report", &lv_font_montserrat_14,
                1040, 72, 110, 36, C_SURFACE2, C_TEAL, 6);
    lv_obj_add_event_cb(oee_rpt, cb_oee_btn, LV_EVENT_CLICKED, NULL);
    make_button(screen, "Efficiency", &lv_font_montserrat_14,
                1160, 72, 100, 36, C_SURFACE2, C_TEAL, 6);

    for (int i = 0; i < NUM_LINES; i++) {
        int x = 30 + i * 390;
        build_line_card(screen, i, x, 140);
    }

    make_label(screen, "CubeRTMS (c) 2026 - Real-Time Monitoring System",
               &lv_font_montserrat_12, lv_color_hex(C_DARK_GRAY),
               0, 750, LV_TEXT_ALIGN_CENTER, SCREEN_W);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, cb_gesture, LV_EVENT_GESTURE, NULL);
}

/* =========================================================================
   SCREEN 1: LINE DASHBOARD (detail)
   ========================================================================= */

static void build_line_header(lv_obj_t *screen)
{
    lv_obj_t *hdr = make_container(screen, 0, 48, SCREEN_W, 56,
                                   C_SURFACE, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    make_label(hdr, g_mfg.line_name,
               &lv_font_montserrat_24, lv_color_hex(C_WHITE),
               16, 8, LV_TEXT_ALIGN_LEFT, 250);

    lv_obj_t *badge = make_container(hdr, 270, 14, 80, 26, C_GREEN, 0, 0, 4);
    make_label(badge, g_mfg.mode_label,
               &lv_font_montserrat_12, lv_color_hex(C_WHITE),
               0, 5, LV_TEXT_ALIGN_CENTER, 80);

    char info[128];
    snprintf(info, sizeof(info),
             "CubeRTMS > Cebu > Harness > Line_01  |  Takt: %ds  |  Target: %d/hr",
             g_mfg.takt_sec, g_mfg.target_per_hr);
    make_label(hdr, info,
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               16, 38, LV_TEXT_ALIGN_LEFT, 600);

    lv_obj_t *oee_btn = make_button(hdr, "OEE", &lv_font_montserrat_14,
                                    950, 14, 70, 28, C_GREEN, C_WHITE, 4);
    lv_obj_add_event_cb(oee_btn, cb_oee_btn, LV_EVENT_CLICKED, NULL);

    make_button(hdr, "Supervisor Control", &lv_font_montserrat_12,
                1030, 14, 140, 28, C_SURFACE2, C_GRAY, 4);

    lv_obj_t *all_btn = make_button(hdr, "< All Lines", &lv_font_montserrat_12,
                1180, 14, 90, 28, C_SURFACE2, C_GRAY, 4);
    lv_obj_add_event_cb(all_btn, cb_all_lines_btn, LV_EVENT_CLICKED, NULL);
}

static void build_period_bar(lv_obj_t *screen)
{
    lv_obj_t *bar = make_container(screen, 0, 104, SCREEN_W, 40,
                                   C_SURFACE2, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    make_label(bar, "Period:", &lv_font_montserrat_14, lv_color_hex(C_GRAY),
               16, 10, LV_TEXT_ALIGN_LEFT, 60);

    ui_BtnToday = make_button(bar, "Today", &lv_font_montserrat_14,
                              85, 5, 76, 30, C_TEAL, C_WHITE, 4);
    lv_obj_add_event_cb(ui_BtnToday, cb_period_btn, LV_EVENT_CLICKED, (void *)(intptr_t)0);

    ui_BtnYesterday = make_button(bar, "Yesterday", &lv_font_montserrat_14,
                                  170, 5, 90, 30, C_SURFACE2, C_GRAY, 4);
    lv_obj_add_event_cb(ui_BtnYesterday, cb_period_btn, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    ui_BtnLast7 = make_button(bar, "Last 7 Days", &lv_font_montserrat_14,
                              268, 5, 108, 30, C_SURFACE2, C_GRAY, 4);
    lv_obj_add_event_cb(ui_BtnLast7, cb_period_btn, LV_EVENT_CLICKED, (void *)(intptr_t)2);

    make_label(bar, "From  03/12/2026  To  03/12/2026",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               400, 12, LV_TEXT_ALIGN_LEFT, 280);
    make_button(bar, "Apply", &lv_font_montserrat_14, 700, 5, 70, 30, C_TEAL, C_WHITE, 4);
}

static void build_kpi_row(lv_obj_t *screen)
{
    lv_obj_t *row = make_container(screen, 0, 144, SCREEN_W, 90,
                                   C_BG, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    const int card_w = 256;
    const char *titles[] = { "PART NUMBER", "PLAN TODAY", "ACTUAL", "EFFICIENCY", "DEFECTS (IPPM)" };
    char buf[32];

    for (int i = 0; i < 5; i++) {
        int cx = i * card_w;
        if (i > 0) {
            lv_obj_t *div = lv_obj_create(row);
            lv_obj_set_pos(div, cx, 0); lv_obj_set_size(div, 1, 90);
            lv_obj_set_style_bg_color(div, lv_color_hex(C_BORDER), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(div, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_opa(div, LV_OPA_TRANSP, LV_PART_MAIN);
        }
        lv_obj_t *card = make_container(row, cx + 1, 0, card_w - 1, 90, C_BG, 0, 0, 0);
        make_label(card, titles[i],
                   &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   12, 8, LV_TEXT_ALIGN_LEFT, card_w - 24);
        switch (i) {
            case 0:
                make_label(card, g_mfg.part_number,
                           &lv_font_montserrat_18, lv_color_hex(C_WHITE),
                           12, 36, LV_TEXT_ALIGN_LEFT, card_w - 24);
                break;
            case 1:
                snprintf(buf, sizeof(buf), "%d", g_mfg.plan_today);
                make_label(card, buf, &lv_font_montserrat_32, lv_color_hex(C_WHITE),
                           12, 30, LV_TEXT_ALIGN_LEFT, card_w - 24);
                break;
            case 2:
                snprintf(buf, sizeof(buf), "%d", g_mfg.actual_today);
                ui_KpiActualVal = make_label(card, buf,
                           &lv_font_montserrat_32, lv_color_hex(C_RED),
                           12, 30, LV_TEXT_ALIGN_LEFT, card_w - 24);
                break;
            case 3:
                snprintf(buf, sizeof(buf), "%.1f%%", g_mfg.efficiency);
                ui_KpiEffVal = make_label(card, buf,
                           &lv_font_montserrat_32, lv_color_hex(C_RED),
                           12, 30, LV_TEXT_ALIGN_LEFT, card_w - 24);
                break;
            case 4:
                snprintf(buf, sizeof(buf), "%d", g_mfg.defects_ippm);
                ui_KpiDefVal = make_label(card, buf,
                           &lv_font_montserrat_32, lv_color_hex(C_WHITE),
                           12, 30, LV_TEXT_ALIGN_LEFT, card_w - 24);
                break;
        }
    }
}

static void build_chart_section(lv_obj_t *screen)
{
    lv_obj_t *section = make_container(screen, 0, 234, 780, 260,
                                       C_SURFACE2, C_BORDER, 1, 0);
    make_label(section, "Hourly Plan vs Actual",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               12, 8, LV_TEXT_ALIGN_LEFT, 300);
    make_label(section, "Auto-refreshes via WebSocket",
               &lv_font_montserrat_12, lv_color_hex(C_DARK_GRAY),
               400, 10, LV_TEXT_ALIGN_LEFT, 250);

    make_dot(section, 660, 10, 5, C_TEAL);
    make_label(section, "Actual", &lv_font_montserrat_12, lv_color_hex(C_TEAL),
               676, 8, LV_TEXT_ALIGN_LEFT, 50);
    make_dot(section, 730, 10, 5, C_PLAN_BAR);
    make_label(section, "Planned", &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               746, 8, LV_TEXT_ALIGN_LEFT, 50);

    ui_Chart = lv_chart_create(section);
    lv_obj_set_pos(ui_Chart, 10, 30);
    lv_obj_set_size(ui_Chart, 750, 200);
    lv_chart_set_type(ui_Chart, LV_CHART_TYPE_BAR);
    lv_chart_set_range(ui_Chart, LV_CHART_AXIS_PRIMARY_Y, 0, 120);
    lv_chart_set_point_count(ui_Chart, NUM_HOURS);
    lv_obj_set_style_bg_color(ui_Chart, lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_Chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(ui_Chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_Chart, 4, LV_PART_MAIN);

    ui_ChartSeriesPlan = lv_chart_add_series(ui_Chart, lv_color_hex(C_PLAN_BAR),
                                              LV_CHART_AXIS_PRIMARY_Y);
    ui_ChartSeriesActual = lv_chart_add_series(ui_Chart, lv_color_hex(C_TEAL),
                                                LV_CHART_AXIS_PRIMARY_Y);
    for (int i = 0; i < NUM_HOURS; i++) {
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesPlan, i, g_mfg.hourly_plan[i]);
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesActual, i, g_mfg.hourly_actual[i]);
    }
    lv_chart_refresh(ui_Chart);

    for (int i = 0; i < NUM_HOURS; i++) {
        int lx = 10 + i * (750 / NUM_HOURS) + (750 / NUM_HOURS) / 2 - 18;
        make_label(section, g_mfg.hourly_label[i],
                   &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   lx, 236, LV_TEXT_ALIGN_CENTER, 36);
    }
}

static void build_table_section(lv_obj_t *screen)
{
    lv_obj_t *section = make_container(screen, 0, 494, 780, 306,
                                       C_SURFACE2, C_BORDER, 1, 0);
    make_label(section, "Hourly Detail",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               12, 8, LV_TEXT_ALIGN_LEFT, 200);

    ui_Table = lv_table_create(section);
    lv_obj_set_pos(ui_Table, 0, 32);
    lv_obj_set_size(ui_Table, 780, 274);

    const char *col_names[] = {"HOUR","PLANNED","SUBASSY","OLTT","INSP","PACK","TOTAL","VAR","DEF","STATUS"};
    const int col_widths[] = {80, 70, 65, 60, 60, 60, 70, 60, 50, 90};
    int nc = 10;
    lv_table_set_column_count(ui_Table, nc);
    for (int c = 0; c < nc; c++) {
        lv_table_set_column_width(ui_Table, c, col_widths[c]);
        lv_table_set_cell_value(ui_Table, 0, c, col_names[c]);
    }
    lv_table_set_row_count(ui_Table, 1 + NUM_HOURS);

    char buf[32];
    for (int r = 0; r < NUM_HOURS; r++) {
        int row = r + 1;
        int total = g_mfg.hourly_actual[r];
        int plan  = g_mfg.hourly_plan[r];
        int var   = total - plan;
        char hr[16];
        snprintf(hr, sizeof(hr), "%02d:00-%02d:00", r, r + 1);
        lv_table_set_cell_value(ui_Table, row, 0, hr);
        snprintf(buf, sizeof(buf), "%d", plan); lv_table_set_cell_value(ui_Table, row, 1, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s1[r]); lv_table_set_cell_value(ui_Table, row, 2, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s2[r]); lv_table_set_cell_value(ui_Table, row, 3, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s3[r]); lv_table_set_cell_value(ui_Table, row, 4, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s4[r]); lv_table_set_cell_value(ui_Table, row, 5, buf);
        snprintf(buf, sizeof(buf), "%d", total); lv_table_set_cell_value(ui_Table, row, 6, buf);
        snprintf(buf, sizeof(buf), "%+d", var); lv_table_set_cell_value(ui_Table, row, 7, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_defects[r]); lv_table_set_cell_value(ui_Table, row, 8, buf);
        lv_table_set_cell_value(ui_Table, row, 9,
            (var < -20) ? "Critical" : (var >= 0) ? "On Track" : "Behind");
    }

    lv_obj_set_style_bg_color(ui_Table, lv_color_hex(C_SURFACE), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(ui_Table, lv_color_hex(C_GRAY), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Table, &lv_font_montserrat_12, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(ui_Table, lv_color_hex(C_BORDER), LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(ui_Table, 1, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(ui_Table, 4, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(ui_Table, 4, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_left(ui_Table, 4, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(ui_Table, 4, LV_PART_ITEMS | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_Table, lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_Table, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(ui_Table, lv_color_hex(C_BORDER), LV_PART_MAIN);
    lv_obj_set_style_border_width(ui_Table, 1, LV_PART_MAIN);
}

static void build_alert_row(lv_obj_t *parent, int idx)
{
    if (idx < 0 || idx >= g_mfg.alert_count) return;
    const alert_t *a = &g_mfg.alerts[idx];
    uint32_t border_col = (a->severity == 1) ? C_RED : C_ORANGE;

    lv_obj_t *row = make_container(parent, 0, idx * 76, 498, 72,
                                   C_SURFACE, border_col, 3, 6);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    s_alert_rows[idx] = row;

    make_label(row, a->message,
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               12, 8, LV_TEXT_ALIGN_LEFT, 340);

    char detail[48];
    snprintf(detail, sizeof(detail), "%s  |  %dm ago", a->station, a->minutes_ago);
    make_label(row, detail, &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               12, 38, LV_TEXT_ALIGN_LEFT, 250);

    lv_obj_t *ack_btn = make_button(row, "ACK", &lv_font_montserrat_12,
                                    380, 10, 52, 28, C_BLUE_ACK, C_WHITE, 4);
    lv_obj_add_event_cb(ack_btn, cb_ack_btn, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
    s_alert_ack_btns[idx] = ack_btn;
    if (a->acknowledged) lv_obj_add_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t *chk = make_button(row, "OK", &lv_font_montserrat_14,
                                440, 10, 36, 28, C_GREEN, C_WHITE, 4);
    lv_obj_add_event_cb(chk, cb_ack_btn, LV_EVENT_CLICKED, (void *)(intptr_t)idx);
}

static void build_right_panel(lv_obj_t *screen)
{
    lv_obj_t *panel = make_container(screen, 780, 234, 500, 566,
                                     C_BG, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(panel, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);

    lv_obj_t *title_bar = make_container(panel, 0, 0, 500, 40,
                                         C_SURFACE, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(title_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    make_dot(title_bar, 12, 14, 6, C_RED);
    make_label(title_bar, "Active Alerts",
               &lv_font_montserrat_16, lv_color_hex(C_WHITE),
               32, 9, LV_TEXT_ALIGN_LEFT, 200);

    ui_AlertList = lv_obj_create(panel);
    lv_obj_set_pos(ui_AlertList, 0, 40);
    lv_obj_set_size(ui_AlertList, 500, 526);
    lv_obj_set_style_bg_color(ui_AlertList, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_AlertList, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(ui_AlertList, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_AlertList, 4, LV_PART_MAIN);
    lv_obj_add_flag(ui_AlertList, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui_AlertList, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui_AlertList, LV_SCROLLBAR_MODE_ACTIVE);

    for (int i = 0; i < MAX_ALERTS; i++) {
        s_alert_rows[i] = NULL;
        s_alert_ack_btns[i] = NULL;
    }
    for (int i = 0; i < g_mfg.alert_count; i++)
        build_alert_row(ui_AlertList, i);
}

static void build_line_dash_screen(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, SCREEN_W, SCREEN_H);

    build_nav_bar(screen, 0);
    build_line_header(screen);
    build_period_bar(screen);
    build_kpi_row(screen);
    build_chart_section(screen);
    build_table_section(screen);
    build_right_panel(screen);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, cb_gesture, LV_EVENT_GESTURE, NULL);
}

/* =========================================================================
   SCREEN 2: OEE DASHBOARD
   ========================================================================= */

static lv_obj_t *make_oee_gauge(lv_obj_t *parent, int x, int y,
                                 const char *title, float value,
                                 uint32_t color, float target)
{
    int gw = 200, gh = 160;
    lv_obj_t *card = make_container(parent, x, y, gw, gh, C_SURFACE, C_BORDER, 1, 8);

    make_label(card, title, &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               0, 8, LV_TEXT_ALIGN_CENTER, gw);

    lv_obj_t *arc = lv_arc_create(card);
    lv_obj_set_pos(arc, gw / 2 - 45, 28);
    lv_obj_set_size(arc, 90, 90);
    lv_arc_set_range(arc, 0, 1000);
    lv_arc_set_value(arc, (int)(value * 10));
    lv_arc_set_bg_angles(arc, 135, 405);
    lv_obj_set_style_arc_color(arc, lv_color_hex(C_DARK_GRAY), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, lv_color_hex(color), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 8, LV_PART_INDICATOR);
    lv_obj_remove_style(arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f%%", value);
    lv_obj_t *vlbl = make_label(card, buf,
               &lv_font_montserrat_24, lv_color_hex(color),
               0, 55, LV_TEXT_ALIGN_CENTER, gw);

    if (value < 50.0f)
        make_label(card, "Critical", &lv_font_montserrat_12, lv_color_hex(C_RED),
                   0, 85, LV_TEXT_ALIGN_CENTER, gw);

    snprintf(buf, sizeof(buf), "Target: %.0f%%", target);
    make_label(card, buf, &lv_font_montserrat_12, lv_color_hex(C_DARK_GRAY),
               0, 130, LV_TEXT_ALIGN_CENTER, gw);
    return vlbl;
}

static void build_oee_breakdown_card(lv_obj_t *parent, int x, int y,
                                      const char *title, uint32_t title_col,
                                      const char *lines[], int nlines)
{
    int cw = 380, ch = 180;
    lv_obj_t *card = make_container(parent, x, y, cw, ch, C_SURFACE, C_BORDER, 1, 8);
    make_label(card, title, &lv_font_montserrat_14, lv_color_hex(title_col),
               16, 10, LV_TEXT_ALIGN_LEFT, cw - 32);
    for (int i = 0; i < nlines && i < 6; i++)
        make_label(card, lines[i], &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   16, 36 + i * 22, LV_TEXT_ALIGN_LEFT, cw - 32);
}

static void build_oee_screen(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, SCREEN_W, SCREEN_H);

    build_nav_bar(screen, 0);

    lv_obj_t *hdr = make_container(screen, 0, 48, SCREEN_W, 56,
                                   C_SURFACE, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    make_label(hdr, "OEE Dashboard",
               &lv_font_montserrat_24, lv_color_hex(C_WHITE),
               16, 8, LV_TEXT_ALIGN_LEFT, 300);
    make_label(hdr, "Harness Line 01 - Thursday, March 12, 2026",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               16, 38, LV_TEXT_ALIGN_LEFT, 500);
    lv_obj_t *back_btn = make_button(hdr, "< Line Dashboard", &lv_font_montserrat_12,
                1050, 14, 130, 28, C_SURFACE2, C_TEAL, 4);
    lv_obj_add_event_cb(back_btn, cb_nav_btn, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    /* Period bar */
    lv_obj_t *pbar = make_container(screen, 0, 104, SCREEN_W, 40,
                                    C_SURFACE2, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(pbar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    make_label(pbar, "Period:", &lv_font_montserrat_14, lv_color_hex(C_GRAY),
               16, 10, LV_TEXT_ALIGN_LEFT, 60);
    make_button(pbar, "Today", &lv_font_montserrat_14, 85, 5, 76, 30, C_TEAL, C_WHITE, 4);
    make_button(pbar, "Last 7 Days", &lv_font_montserrat_14, 170, 5, 108, 30, C_SURFACE2, C_GRAY, 4);

    /* OEE Gauges */
    int gy = 154;
    s_oee_overall_lbl = make_oee_gauge(screen, 30, gy, "OVERALL OEE", g_oee.overall_oee, C_RED, 85.0f);

    /* Availability card */
    {
        int cx = 250, cw = 240, ch = 180;
        lv_obj_t *card = make_container(screen, cx, gy, cw, ch, C_SURFACE, C_BORDER, 1, 8);
        make_label(card, "AVAILABILITY", &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   0, 8, LV_TEXT_ALIGN_CENTER, cw);
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f%%", g_oee.availability);
        s_oee_avail_lbl = make_label(card, buf, &lv_font_montserrat_32, lv_color_hex(C_GREEN),
                   0, 30, LV_TEXT_ALIGN_CENTER, cw);
        char d1[48], d2[48], d3[48], d4[48];
        snprintf(d1, sizeof(d1), "Shift Duration     %.0f min", g_oee.shift_duration_min);
        snprintf(d2, sizeof(d2), "Planned DT    %.1f min", g_oee.planned_downtime_min);
        snprintf(d3, sizeof(d3), "Unplanned DT  %.1f min", g_oee.unplanned_downtime_min);
        snprintf(d4, sizeof(d4), "Run Time      %.1f min", g_oee.run_time_min);
        make_label(card, d1, &lv_font_montserrat_12, lv_color_hex(C_GRAY), 12, 75, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d2, &lv_font_montserrat_12, lv_color_hex(C_GRAY), 12, 95, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d3, &lv_font_montserrat_12, lv_color_hex(C_RED),  12, 115, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d4, &lv_font_montserrat_12, lv_color_hex(C_GRAY), 12, 135, LV_TEXT_ALIGN_LEFT, cw-24);
    }

    /* Performance card */
    {
        int cx = 510, cw = 240, ch = 180;
        lv_obj_t *card = make_container(screen, cx, gy, cw, ch, C_SURFACE, C_BORDER, 1, 8);
        make_label(card, "PERFORMANCE", &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   0, 8, LV_TEXT_ALIGN_CENTER, cw);
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f%%", g_oee.performance);
        s_oee_perf_lbl = make_label(card, buf, &lv_font_montserrat_32, lv_color_hex(C_GREEN),
                   0, 30, LV_TEXT_ALIGN_CENTER, cw);
        char d1[48], d2[48], d3[48];
        snprintf(d1, sizeof(d1), "Ideal Cycle   %.1fs", g_oee.ideal_cycle_sec);
        snprintf(d2, sizeof(d2), "Total Pieces  %d", g_oee.total_pieces);
        snprintf(d3, sizeof(d3), "Speed Loss    %.1f min", g_oee.speed_loss_min);
        make_label(card, d1, &lv_font_montserrat_12, lv_color_hex(C_GRAY), 12, 75, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d2, &lv_font_montserrat_12, lv_color_hex(C_GRAY), 12, 95, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d3, &lv_font_montserrat_12, lv_color_hex(C_RED),  12, 115, LV_TEXT_ALIGN_LEFT, cw-24);
    }

    /* Quality card */
    {
        int cx = 770, cw = 240, ch = 180;
        lv_obj_t *card = make_container(screen, cx, gy, cw, ch, C_SURFACE, C_BORDER, 1, 8);
        make_label(card, "QUALITY", &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   0, 8, LV_TEXT_ALIGN_CENTER, cw);
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f%%", g_oee.quality);
        s_oee_qual_lbl = make_label(card, buf, &lv_font_montserrat_32, lv_color_hex(C_GREEN),
                   0, 30, LV_TEXT_ALIGN_CENTER, cw);
        char d1[48], d2[48], d3[48];
        snprintf(d1, sizeof(d1), "Total Pieces  %d", g_oee.total_pieces);
        snprintf(d2, sizeof(d2), "Good Pieces   %d", g_oee.good_pieces);
        snprintf(d3, sizeof(d3), "Defects       %d", g_oee.defect_count);
        make_label(card, d1, &lv_font_montserrat_12, lv_color_hex(C_GRAY),  12, 75, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d2, &lv_font_montserrat_12, lv_color_hex(C_GREEN), 12, 95, LV_TEXT_ALIGN_LEFT, cw-24);
        make_label(card, d3, &lv_font_montserrat_12, lv_color_hex(C_RED),   12, 115, LV_TEXT_ALIGN_LEFT, cw-24);
    }

    /* Six Big Losses */
    int ly = 350;
    {
        const char *lines[] = {"1. Breakdowns       0.0 min", "2. Changeovers      0.0 min",
                                "   Minor Stops    404.5 min", "", "Total DT  404.5 min"};
        build_oee_breakdown_card(screen, 30, ly, "Availability Losses", C_TEAL, lines, 5);
    }
    {
        const char *lines[] = {"3. Minor Stoppages  Included", "4. Speed Loss       0.0 min",
                                "", "Ideal vs Actual  45.0s/pc"};
        build_oee_breakdown_card(screen, 430, ly, "Performance Losses", C_ORANGE, lines, 4);
    }
    {
        const char *lines[] = {"5. Process Defects  13 pcs", "6. Startup Rejects  N/A",
                                "", "Good Output  587 / 600"};
        build_oee_breakdown_card(screen, 830, ly, "Quality Losses", C_GREEN, lines, 4);
    }

    /* OEE Trend */
    {
        lv_obj_t *tc = make_container(screen, 30, 545, 590, 240, C_SURFACE, C_BORDER, 1, 8);
        make_label(tc, "OEE Trend (Mar 12)", &lv_font_montserrat_14, lv_color_hex(C_WHITE),
                   16, 10, LV_TEXT_ALIGN_LEFT, 300);
        lv_obj_t *chart = lv_chart_create(tc);
        lv_obj_set_pos(chart, 10, 35);
        lv_obj_set_size(chart, 560, 190);
        lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
        lv_chart_set_point_count(chart, 20);
        lv_obj_set_style_bg_color(chart, lv_color_hex(C_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_line_width(chart, 2, LV_PART_ITEMS);
        lv_chart_series_t *trend = lv_chart_add_series(chart, lv_color_hex(C_TEAL), LV_CHART_AXIS_PRIMARY_Y);
        static const int pts[] = {76,78,75,80,82,60,55,65,70,85,88,82,84,86,80,78,82,85,15,15};
        for (int i = 0; i < 20; i++)
            lv_chart_set_value_by_id(chart, trend, i, pts[i]);
        lv_chart_series_t *tgt = lv_chart_add_series(chart, lv_color_hex(C_ORANGE), LV_CHART_AXIS_PRIMARY_Y);
        for (int i = 0; i < 20; i++)
            lv_chart_set_value_by_id(chart, tgt, i, 85);
        lv_chart_refresh(chart);
    }

    /* Time Loss Waterfall */
    {
        lv_obj_t *wc = make_container(screen, 640, 545, 620, 240, C_SURFACE, C_BORDER, 1, 8);
        make_label(wc, "Time Loss Waterfall", &lv_font_montserrat_14, lv_color_hex(C_WHITE),
                   16, 10, LV_TEXT_ALIGN_LEFT, 300);
        lv_obj_t *chart = lv_chart_create(wc);
        lv_obj_set_pos(chart, 10, 35);
        lv_obj_set_size(chart, 590, 190);
        lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
        lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 500);
        lv_chart_set_point_count(chart, 4);
        lv_obj_set_style_bg_color(chart, lv_color_hex(C_SURFACE), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_chart_series_t *bars = lv_chart_add_series(chart, lv_color_hex(C_TEAL), LV_CHART_AXIS_PRIMARY_Y);
        lv_chart_set_value_by_id(chart, bars, 0, 480);
        lv_chart_set_value_by_id(chart, bars, 1, 75);
        lv_chart_set_value_by_id(chart, bars, 2, 405);
        lv_chart_set_value_by_id(chart, bars, 3, 0);
        lv_chart_refresh(chart);
    }

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, cb_gesture, LV_EVENT_GESTURE, NULL);
}

/* =========================================================================
   SCREEN 3: STATION PERFORMANCE + QUALITY
   ========================================================================= */

static void build_station_card(lv_obj_t *parent, int idx, int x, int y)
{
    const StationPerf *st = &g_stations[idx];
    int cw = 280, ch = 180;
    uint32_t border_col = st->alarm ? C_RED : C_GREEN;
    lv_obj_t *card = make_container(parent, x, y, cw, ch, C_SURFACE, border_col, 2, 8);

    make_label(card, st->name,
               &lv_font_montserrat_18, lv_color_hex(st->alarm ? C_RED : C_GREEN),
               0, 12, LV_TEXT_ALIGN_CENTER, cw);
    make_label(card, st->full_name,
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               0, 36, LV_TEXT_ALIGN_CENTER, cw);
    char buf[32];
    snprintf(buf, sizeof(buf), "%.1f%%", st->yield_pct);
    s_stn_yield_lbl[idx] = make_label(card, buf,
               &lv_font_montserrat_48, lv_color_hex(C_WHITE),
               0, 58, LV_TEXT_ALIGN_CENTER, cw);
    snprintf(buf, sizeof(buf), "%d/%d pass", st->pass_count, st->total_count);
    s_stn_pass_lbl[idx] = make_label(card, buf,
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               0, 116, LV_TEXT_ALIGN_CENTER, cw);
    make_label(card, st->alarm ? "Alarm" : "Running",
               &lv_font_montserrat_14, lv_color_hex(st->alarm ? C_RED : C_GREEN),
               0, 140, LV_TEXT_ALIGN_CENTER, cw);
}

static void build_station_screen(lv_obj_t *screen)
{
    lv_obj_set_style_bg_color(screen, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(screen, SCREEN_W, SCREEN_H);

    build_nav_bar(screen, 0);

    lv_obj_t *hdr = make_container(screen, 0, 48, SCREEN_W, 56,
                                   C_SURFACE, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);
    make_label(hdr, "Station Performance (Yield by Station)",
               &lv_font_montserrat_18, lv_color_hex(C_WHITE),
               16, 16, LV_TEXT_ALIGN_LEFT, 500);
    lv_obj_t *back_btn = make_button(hdr, "< Line Dashboard", &lv_font_montserrat_12,
                1050, 14, 130, 28, C_SURFACE2, C_TEAL, 4);
    lv_obj_add_event_cb(back_btn, cb_nav_btn, LV_EVENT_CLICKED, (void *)(intptr_t)1);

    for (int i = 0; i < NUM_STATIONS; i++)
        build_station_card(screen, i, 30 + i * 290, 124);

    /* Daily Progress */
    lv_obj_t *pc = make_container(screen, 30, 330, SCREEN_W - 60, 80, C_SURFACE, C_BORDER, 1, 8);
    make_label(pc, "Daily Progress", &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               16, 8, LV_TEXT_ALIGN_LEFT, 200);
    char pbuf[32];
    snprintf(pbuf, sizeof(pbuf), "%d / %d", g_mfg.actual_today, g_mfg.plan_today);
    s_daily_progress_lbl = make_label(pc, pbuf, &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               SCREEN_W - 200, 8, LV_TEXT_ALIGN_RIGHT, 120);
    int bw = SCREEN_W - 92;
    make_container(pc, 16, 40, bw, 20, C_DARK_GRAY, 0, 0, 10);
    float prog_pct = 0;
    if (g_mfg.plan_today > 0) prog_pct = (float)g_mfg.actual_today / (float)g_mfg.plan_today * 100.0f;
    if (prog_pct > 100.0f) prog_pct = 100.0f;
    int fw = (int)(bw * prog_pct / 100.0f);
    if (fw < 8) fw = 8;
    s_daily_progress_bar = make_container(pc, 16, 40, fw, 20, C_RED, 0, 0, 10);

    /* Quality Analysis */
    lv_obj_t *qc = make_container(screen, 30, 430, 600, 350, C_SURFACE, C_BORDER, 1, 8);
    make_label(qc, "Quality Analysis", &lv_font_montserrat_18, lv_color_hex(C_WHITE),
               16, 12, LV_TEXT_ALIGN_LEFT, 300);
    make_label(qc, "Harness Line 01 - Mar 12, 2026", &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               16, 36, LV_TEXT_ALIGN_LEFT, 300);

    const char *qt[] = {"QUALITY","GOOD PIECES","DEFECTS","DPPM"};
    const char *qv[] = {"97.8%","587","13","21667"};
    uint32_t qclrs[] = {C_RED, C_GREEN, C_ORANGE, C_RED};
    for (int i = 0; i < 4; i++) {
        int kx = 16 + i * 142;
        lv_obj_t *kk = make_container(qc, kx, 64, 130, 60, C_SURFACE2, 0, 0, 6);
        make_label(kk, qt[i], &lv_font_montserrat_12, lv_color_hex(qclrs[i]),
                   0, 6, LV_TEXT_ALIGN_CENTER, 130);
        make_label(kk, qv[i], &lv_font_montserrat_18, lv_color_hex(qclrs[i]),
                   0, 28, LV_TEXT_ALIGN_CENTER, 130);
    }

    make_label(qc, "Quality Calculation", &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               16, 140, LV_TEXT_ALIGN_LEFT, 250);
    const char *cl[] = {"Total Pieces Produced       600","- Defective Pieces           13",
                        "= Good Pieces              587","Quality %                 97.8%","DPPM                     21667"};
    uint32_t cc[] = {C_GRAY, C_GRAY, C_GRAY, C_GREEN, C_RED};
    for (int i = 0; i < 5; i++)
        make_label(qc, cl[i], &lv_font_montserrat_12, lv_color_hex(cc[i]),
                   16, 164 + i * 20, LV_TEXT_ALIGN_LEFT, 560);

    /* Defect Pareto */
    lv_obj_t *dp = make_container(screen, 650, 430, 600, 350, C_SURFACE, C_BORDER, 1, 8);
    make_label(dp, "Defect Pareto (by Reason)", &lv_font_montserrat_18, lv_color_hex(C_WHITE),
               16, 12, LV_TEXT_ALIGN_LEFT, 400);

    const char *pr[] = {"No reason code","[DF-021] Open Load Tight Fail","[DF-007] Missing Clip",
                        "[DF-005] Exposed Wire","[DF-006] Wire Crossover","[DF-008] Tape Misalignment",
                        "[DF-014] Wrong Connector","[DF-009] Bent Terminal"};
    const char *pc2[] = {"77 pcs","47 pcs","20 pcs","18 pcs","18 pcs","15 pcs","13 pcs","11 pcs"};
    for (int i = 0; i < 8; i++) {
        make_label(dp, pr[i], &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   16, 44 + i * 28, LV_TEXT_ALIGN_LEFT, 380);
        make_label(dp, pc2[i], &lv_font_montserrat_12, lv_color_hex(C_RED),
                   430, 44 + i * 28, LV_TEXT_ALIGN_RIGHT, 140);
    }

    make_label(dp, "Defects by Station", &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               16, 280, LV_TEXT_ALIGN_LEFT, 300);
    make_label(dp, "Inspection    116", &lv_font_montserrat_12, lv_color_hex(C_RED),
               16, 304, LV_TEXT_ALIGN_LEFT, 300);
    make_label(dp, "OLTT           47", &lv_font_montserrat_12, lv_color_hex(C_RED),
               16, 326, LV_TEXT_ALIGN_LEFT, 300);

    lv_obj_add_flag(screen, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(screen, cb_gesture, LV_EVENT_GESTURE, NULL);
}

/* =========================================================================
   UI INIT
   ========================================================================= */

void ui_init(void)
{
    ui_ScreenCentralized = lv_obj_create(NULL);
    ui_ScreenLineDash    = lv_obj_create(NULL);
    ui_ScreenOEE         = lv_obj_create(NULL);
    ui_ScreenStation     = lv_obj_create(NULL);

    build_centralized_screen(ui_ScreenCentralized);
    build_line_dash_screen(ui_ScreenLineDash);
    build_oee_screen(ui_ScreenOEE);
    build_station_screen(ui_ScreenStation);

    s_cur_screen = 0;
    lv_scr_load(ui_ScreenCentralized);
}

/* =========================================================================
   DATA REFRESH
   ========================================================================= */

void ui_update_kpi(void)
{
    if (!ui_KpiActualVal || !ui_KpiEffVal || !ui_KpiDefVal) return;
    char buf[24];
    snprintf(buf, sizeof(buf), "%d", g_mfg.actual_today);
    lv_label_set_text(ui_KpiActualVal, buf);
    lv_obj_set_style_text_color(ui_KpiActualVal,
        (g_mfg.actual_today < g_mfg.plan_today) ? lv_color_hex(C_RED) : lv_color_hex(C_GREEN),
        LV_PART_MAIN);
    snprintf(buf, sizeof(buf), "%.1f%%", g_mfg.efficiency);
    lv_label_set_text(ui_KpiEffVal, buf);
    lv_obj_set_style_text_color(ui_KpiEffVal,
        (g_mfg.efficiency < 100.0f) ? lv_color_hex(C_RED) : lv_color_hex(C_GREEN),
        LV_PART_MAIN);
    snprintf(buf, sizeof(buf), "%d", g_mfg.defects_ippm);
    lv_label_set_text(ui_KpiDefVal, buf);
}

void ui_update_chart(void)
{
    if (!ui_Chart) return;
    for (int i = 0; i < NUM_HOURS; i++) {
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesPlan, i, g_mfg.hourly_plan[i]);
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesActual, i, g_mfg.hourly_actual[i]);
    }
    lv_chart_refresh(ui_Chart);
}

void ui_update_table(void)
{
    if (!ui_Table) return;
    char buf[32];
    for (int r = 0; r < NUM_HOURS; r++) {
        int row = r + 1;
        int total = g_mfg.hourly_actual[r];
        int plan  = g_mfg.hourly_plan[r];
        int var   = total - plan;
        char hr[16];
        snprintf(hr, sizeof(hr), "%02d:00-%02d:00", r, r + 1);
        lv_table_set_cell_value(ui_Table, row, 0, hr);
        snprintf(buf, sizeof(buf), "%d", plan); lv_table_set_cell_value(ui_Table, row, 1, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s1[r]); lv_table_set_cell_value(ui_Table, row, 2, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s2[r]); lv_table_set_cell_value(ui_Table, row, 3, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s3[r]); lv_table_set_cell_value(ui_Table, row, 4, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s4[r]); lv_table_set_cell_value(ui_Table, row, 5, buf);
        snprintf(buf, sizeof(buf), "%d", total); lv_table_set_cell_value(ui_Table, row, 6, buf);
        snprintf(buf, sizeof(buf), "%+d", var); lv_table_set_cell_value(ui_Table, row, 7, buf);
        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_defects[r]); lv_table_set_cell_value(ui_Table, row, 8, buf);
        lv_table_set_cell_value(ui_Table, row, 9,
            (var < -20) ? "Critical" : (var >= 0) ? "On Track" : "Behind");
    }
}

void ui_update_alerts(void)
{
    if (!ui_AlertList) return;
    for (int i = 0; i < g_mfg.alert_count && i < MAX_ALERTS; i++) {
        if (s_alert_rows[i] == NULL) {
            build_alert_row(ui_AlertList, i);
        } else if (g_mfg.alerts[i].acknowledged) {
            lv_obj_set_style_border_opa(s_alert_rows[i], LV_OPA_TRANSP, LV_PART_MAIN);
            if (s_alert_ack_btns[i])
                lv_obj_add_flag(s_alert_ack_btns[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void ui_update_centralized(void)
{
    char buf[32];
    for (int i = 0; i < NUM_LINES; i++) {
        if (s_line_actual_lbl[i]) {
            snprintf(buf, sizeof(buf), "%d", g_lines[i].actual);
            lv_label_set_text(s_line_actual_lbl[i], buf);
        }
        if (s_line_pva_lbl[i]) {
            snprintf(buf, sizeof(buf), "%.1f%%", g_lines[i].plan_vs_actual_pct);
            lv_label_set_text(s_line_pva_lbl[i], buf);
        }
    }
}

void ui_update_oee(void)
{
    if (!s_oee_avail_lbl) return;
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f%%", g_oee.availability);
    lv_label_set_text(s_oee_avail_lbl, buf);
    snprintf(buf, sizeof(buf), "%.1f%%", g_oee.performance);
    lv_label_set_text(s_oee_perf_lbl, buf);
    snprintf(buf, sizeof(buf), "%.1f%%", g_oee.quality);
    lv_label_set_text(s_oee_qual_lbl, buf);
}

void ui_update_stations(void)
{
    char buf[32];
    for (int i = 0; i < NUM_STATIONS; i++) {
        if (s_stn_yield_lbl[i]) {
            snprintf(buf, sizeof(buf), "%.1f%%", g_stations[i].yield_pct);
            lv_label_set_text(s_stn_yield_lbl[i], buf);
        }
        if (s_stn_pass_lbl[i]) {
            snprintf(buf, sizeof(buf), "%d/%d pass", g_stations[i].pass_count, g_stations[i].total_count);
            lv_label_set_text(s_stn_pass_lbl[i], buf);
        }
    }
    if (s_daily_progress_lbl) {
        snprintf(buf, sizeof(buf), "%d / %d", g_mfg.actual_today, g_mfg.plan_today);
        lv_label_set_text(s_daily_progress_lbl, buf);
    }
}
