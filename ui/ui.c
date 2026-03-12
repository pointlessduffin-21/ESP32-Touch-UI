/**
 * ui.c — CubeRTMS Manufacturing Dashboard LVGL widget tree
 *
 * Resolution: 1280 x 800
 * Theme:      dark (#0d0d0d bg), teal accents (#00e5cc), LVGL v9, ESP-IDF
 *
 * Section layout:
 *   y=0   h=44   Nav bar
 *   y=44  h=60   Line header
 *   y=104 h=44   Period bar
 *   y=148 h=100  KPI row (5 cards)
 *   y=248 h=552  Content area
 *     x=0   w=780  Left panel: chart (h=260) + table (h=292)
 *     x=780 w=500  Right panel: alerts
 */

#include "ui.h"
#include <stdio.h>
#include <string.h>

/* ── Color palette ─────────────────────────────────────────────────────────── */
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

/* ── Screen geometry ───────────────────────────────────────────────────────── */
#define SCREEN_W     1280
#define SCREEN_H     800

/* ── Widget handles (exported via ui.h) ────────────────────────────────────── */
lv_obj_t *ui_Screen1;
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

/* ── Internal state ────────────────────────────────────────────────────────── */

/* Period selector active button (0=Today, 1=Yesterday, 2=Last7) */
static int  s_period_active = 0;
/* OEE info label (shown/hidden on OEE button tap) */
static lv_obj_t *s_oee_info_label = NULL;
/* Alert row objects array (one per alert slot) */
static lv_obj_t *s_alert_rows[MAX_ALERTS];
static lv_obj_t *s_alert_ack_btns[MAX_ALERTS];

/* ═══════════════════════════════════════════════════════════════════════════
   HELPERS
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * make_label — create a styled label with absolute position.
 */
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

/**
 * make_container — create a styled container with absolute position.
 * Pass border_w=0 to suppress border.
 */
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

/**
 * make_button — create a styled button with a text label inside.
 */
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

/* ═══════════════════════════════════════════════════════════════════════════
   CALLBACKS
   ══════════════════════════════════════════════════════════════════════════ */

/* Period selector callback — highlight active button */
static void cb_period_btn(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);
    int which = (int)(intptr_t)lv_event_get_user_data(e);

    /* Deactivate all three */
    lv_obj_set_style_bg_color(ui_BtnToday,     lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_BtnYesterday, lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_color(ui_BtnLast7,     lv_color_hex(C_SURFACE2), LV_PART_MAIN);

    /* Activate selected */
    lv_obj_set_style_bg_color(btn, lv_color_hex(C_TEAL), LV_PART_MAIN);

    s_period_active = which;
    (void)which;
}

/* OEE button callback — toggle info label */
static void cb_oee_btn(lv_event_t *e)
{
    (void)e;
    if (s_oee_info_label == NULL) return;
    bool hidden = lv_obj_has_flag(s_oee_info_label, LV_OBJ_FLAG_HIDDEN);
    if (hidden) {
        lv_obj_clear_flag(s_oee_info_label, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(s_oee_info_label, LV_OBJ_FLAG_HIDDEN);
    }
}

/* ACK button callback — mark alert acknowledged, update row appearance */
static void cb_ack_btn(lv_event_t *e)
{
    int idx = (int)(intptr_t)lv_event_get_user_data(e);
    if (idx < 0 || idx >= MAX_ALERTS) return;

    g_mfg.alerts[idx].acknowledged = true;

    /* Grey out the row background */
    if (s_alert_rows[idx] != NULL) {
        lv_obj_set_style_bg_color(s_alert_rows[idx],
                                  lv_color_hex(C_SURFACE2), LV_PART_MAIN);
        lv_obj_set_style_border_opa(s_alert_rows[idx], LV_OPA_TRANSP, LV_PART_MAIN);
    }
    /* Hide ACK button */
    if (s_alert_ack_btns[idx] != NULL) {
        lv_obj_add_flag(s_alert_ack_btns[idx], LV_OBJ_FLAG_HIDDEN);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   NAV BAR  (y=0, h=44)
   ══════════════════════════════════════════════════════════════════════════ */

static void build_nav_bar(lv_obj_t *screen)
{
    lv_obj_t *nav = make_container(screen, 0, 0, SCREEN_W, 44,
                                   C_NAV, C_BORDER, 1, 0);
    /* Bottom border only — achieved by making the container slightly taller
       and clipping, but simpler: use a 1px line object at y=43 */
    lv_obj_set_style_border_side(nav, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    /* Logo */
    make_label(nav, "CubeRTMS",
               &lv_font_montserrat_18, lv_color_hex(C_GREEN),
               16, 12, LV_TEXT_ALIGN_LEFT, 120);

    /* Nav links */
    const struct { const char *text; int x; } nav_links[] = {
        { "Dashboard", 180 },
        { "UNS Tree",  310 },
        { "Resources", 410 },
        { "Planner",   520 },
        { "Andon",     620 },
    };
    int num_links = (int)(sizeof(nav_links) / sizeof(nav_links[0]));
    for (int i = 0; i < num_links; i++) {
        lv_obj_t *lbl = make_label(nav, nav_links[i].text,
                                   &lv_font_montserrat_14,
                                   i == 0 ? lv_color_hex(C_TEAL)
                                          : lv_color_hex(C_GRAY),
                                   nav_links[i].x, 13,
                                   LV_TEXT_ALIGN_LEFT, 100);
        /* Underline style for active Dashboard link */
        if (i == 0) {
            lv_obj_set_style_text_decor(lbl, LV_TEXT_DECOR_UNDERLINE, LV_PART_MAIN);
        }
    }

    /* User info right-aligned */
    make_label(nav, "System Admin  Admin",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               1100, 15, LV_TEXT_ALIGN_LEFT, 160);
}

/* ═══════════════════════════════════════════════════════════════════════════
   LINE HEADER  (y=44, h=60)
   ══════════════════════════════════════════════════════════════════════════ */

static void build_line_header(lv_obj_t *screen)
{
    lv_obj_t *hdr = make_container(screen, 0, 44, SCREEN_W, 60,
                                   C_SURFACE, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(hdr, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    /* "Main1" large label */
    make_label(hdr, "Main1",
               &lv_font_montserrat_32, lv_color_hex(C_WHITE),
               16, 6, LV_TEXT_ALIGN_LEFT, 120);

    /* "Running" green badge */
    lv_obj_t *badge = make_container(hdr, 155, 16, 80, 26,
                                     C_GREEN, 0, 0, 4);
    make_label(badge, "Running",
               &lv_font_montserrat_12, lv_color_hex(C_WHITE),
               0, 5, LV_TEXT_ALIGN_CENTER, 80);

    /* Breadcrumb / takt info */
    make_label(hdr,
               "CubeRTMS > Gabriela > NissanB13B > L1  \xe2\x80\xa2  Takt: 60s  \xe2\x80\xa2  Target: 60/hr",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               16, 42, LV_TEXT_ALIGN_LEFT, 700);

    /* Right-side action buttons */
    lv_obj_t *oee_btn = make_button(hdr, "OEE",
                                    &lv_font_montserrat_14,
                                    1020, 16, 60, 28,
                                    C_SURFACE2, C_TEAL, 4);
    lv_obj_add_event_cb(oee_btn, cb_oee_btn, LV_EVENT_CLICKED, NULL);

    /* OEE info label — hidden by default */
    s_oee_info_label = make_label(screen, "OEE = Availability x Performance x Quality",
                                  &lv_font_montserrat_12, lv_color_hex(C_TEAL),
                                  900, 110, LV_TEXT_ALIGN_LEFT, 360);
    lv_obj_add_flag(s_oee_info_label, LV_OBJ_FLAG_HIDDEN);

    make_button(hdr, "Supervisor Control",
                &lv_font_montserrat_12,
                1088, 16, 130, 28,
                C_SURFACE2, C_GRAY, 4);

    make_button(hdr, "< All Lines",
                &lv_font_montserrat_12,
                1224, 16, 90, 28,
                C_SURFACE2, C_GRAY, 4);
}

/* ═══════════════════════════════════════════════════════════════════════════
   PERIOD BAR  (y=104, h=44)
   ══════════════════════════════════════════════════════════════════════════ */

static void build_period_bar(lv_obj_t *screen)
{
    lv_obj_t *bar = make_container(screen, 0, 104, SCREEN_W, 44,
                                   C_SURFACE2, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    /* "Period:" label */
    make_label(bar, "Period:",
               &lv_font_montserrat_14, lv_color_hex(C_GRAY),
               16, 12, LV_TEXT_ALIGN_LEFT, 70);

    /* Toggle buttons: Today (active=teal), Yesterday, Last 7 Days */
    ui_BtnToday = make_button(bar, "Today",
                              &lv_font_montserrat_14,
                              90, 7, 80, 30,
                              C_TEAL, C_WHITE, 4);
    lv_obj_add_event_cb(ui_BtnToday, cb_period_btn, LV_EVENT_CLICKED,
                        (void *)(intptr_t)0);

    ui_BtnYesterday = make_button(bar, "Yesterday",
                                  &lv_font_montserrat_14,
                                  178, 7, 96, 30,
                                  C_SURFACE2, C_GRAY, 4);
    lv_obj_add_event_cb(ui_BtnYesterday, cb_period_btn, LV_EVENT_CLICKED,
                        (void *)(intptr_t)1);

    ui_BtnLast7 = make_button(bar, "Last 7 Days",
                              &lv_font_montserrat_14,
                              282, 7, 110, 30,
                              C_SURFACE2, C_GRAY, 4);
    lv_obj_add_event_cb(ui_BtnLast7, cb_period_btn, LV_EVENT_CLICKED,
                        (void *)(intptr_t)2);

    /* Date range display */
    make_label(bar, "03/12/2026",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               420, 12, LV_TEXT_ALIGN_LEFT, 100);
    make_label(bar, "To",
               &lv_font_montserrat_14, lv_color_hex(C_GRAY),
               528, 12, LV_TEXT_ALIGN_LEFT, 30);
    make_label(bar, "03/12/2026",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               566, 12, LV_TEXT_ALIGN_LEFT, 100);

    /* Apply button */
    make_button(bar, "Apply",
                &lv_font_montserrat_14,
                680, 7, 70, 30,
                C_TEAL, C_WHITE, 4);
}

/* ═══════════════════════════════════════════════════════════════════════════
   KPI ROW  (y=148, h=100)
   ══════════════════════════════════════════════════════════════════════════ */

static void build_kpi_row(lv_obj_t *screen)
{
    lv_obj_t *row = make_container(screen, 0, 148, SCREEN_W, 100,
                                   C_BG, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    /* 5 cards, each 256px wide */
    const int card_w = 256;
    const struct { const char *title; const char *value; uint32_t val_color; int font_size; } cards[] = {
        { "PART NUMBER",    "FG-HARN-001", C_WHITE, 18 },
        { "PLAN TODAY",     "1800",        C_WHITE, 32 },
        { "ACTUAL",         "562",         C_RED,   32 },
        { "EFFICIENCY",     "31.2%",       C_RED,   32 },
        { "DEFECTS (IPPM)", "14",          C_WHITE, 32 },
    };
    int num_cards = (int)(sizeof(cards) / sizeof(cards[0]));

    for (int i = 0; i < num_cards; i++) {
        int cx = i * card_w;

        /* Vertical divider between cards (except before first) */
        if (i > 0) {
            lv_obj_t *div = lv_obj_create(row);
            lv_obj_set_pos(div, cx, 0);
            lv_obj_set_size(div, 1, 100);
            lv_obj_set_style_bg_color(div, lv_color_hex(C_BORDER), LV_PART_MAIN);
            lv_obj_set_style_bg_opa(div, LV_OPA_COVER, LV_PART_MAIN);
            lv_obj_set_style_border_opa(div, LV_OPA_TRANSP, LV_PART_MAIN);
        }

        /* Card container */
        lv_obj_t *card = make_container(row, cx + 1, 0, card_w - 1, 100,
                                        C_BG, 0, 0, 0);

        /* Title label (gray, top) */
        make_label(card, cards[i].title,
                   &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   12, 10, LV_TEXT_ALIGN_LEFT, card_w - 24);

        /* Value label (large, colored) */
        const lv_font_t *val_font = (cards[i].font_size == 32)
                                    ? &lv_font_montserrat_32
                                    : &lv_font_montserrat_18;

        lv_obj_t *val_lbl = make_label(card, cards[i].value,
                                       val_font,
                                       lv_color_hex(cards[i].val_color),
                                       12, 38, LV_TEXT_ALIGN_LEFT, card_w - 24);

        /* Save handles for updateable KPI values */
        if (i == 2) ui_KpiActualVal = val_lbl;
        if (i == 3) ui_KpiEffVal    = val_lbl;
        if (i == 4) ui_KpiDefVal    = val_lbl;
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   LEFT PANEL — CHART  (x=0, y=248, w=780, h=260)
   ══════════════════════════════════════════════════════════════════════════ */

static void build_chart_section(lv_obj_t *screen)
{
    lv_obj_t *section = make_container(screen, 0, 248, 780, 260,
                                       C_SURFACE2, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(section, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    /* Title bar */
    make_label(section, "Hourly Plan vs Actual",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               12, 8, LV_TEXT_ALIGN_LEFT, 300);

    /* Legend right side of title bar */
    lv_obj_t *leg_actual_dot = lv_obj_create(section);
    lv_obj_set_pos(leg_actual_dot, 540, 10);
    lv_obj_set_size(leg_actual_dot, 10, 10);
    lv_obj_set_style_radius(leg_actual_dot, 5, LV_PART_MAIN);
    lv_obj_set_style_bg_color(leg_actual_dot, lv_color_hex(C_TEAL), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(leg_actual_dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(leg_actual_dot, LV_OPA_TRANSP, LV_PART_MAIN);

    make_label(section, "Actual",
               &lv_font_montserrat_12, lv_color_hex(C_TEAL),
               556, 8, LV_TEXT_ALIGN_LEFT, 60);

    lv_obj_t *leg_plan_box = lv_obj_create(section);
    lv_obj_set_pos(leg_plan_box, 626, 10);
    lv_obj_set_size(leg_plan_box, 10, 10);
    lv_obj_set_style_radius(leg_plan_box, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(leg_plan_box, lv_color_hex(C_PLAN_BAR), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(leg_plan_box, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(leg_plan_box, LV_OPA_TRANSP, LV_PART_MAIN);

    make_label(section, "Planned",
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               642, 8, LV_TEXT_ALIGN_LEFT, 70);

    /* Chart widget */
    ui_Chart = lv_chart_create(section);
    lv_obj_set_pos(ui_Chart, 10, 32);
    lv_obj_set_size(ui_Chart, 750, 200);
    lv_chart_set_type(ui_Chart, LV_CHART_TYPE_BAR);
    lv_chart_set_range(ui_Chart, LV_CHART_AXIS_PRIMARY_Y, 0, 120);
    lv_chart_set_point_count(ui_Chart, NUM_HOURS);

    /* Chart appearance */
    lv_obj_set_style_bg_color(ui_Chart, lv_color_hex(C_SURFACE2), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_Chart, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(ui_Chart, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_left(ui_Chart, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_right(ui_Chart, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_top(ui_Chart, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(ui_Chart, 4, LV_PART_MAIN);


    /* Series */
    ui_ChartSeriesPlan   = lv_chart_add_series(ui_Chart,
                                               lv_color_hex(C_PLAN_BAR),
                                               LV_CHART_AXIS_PRIMARY_Y);
    ui_ChartSeriesActual = lv_chart_add_series(ui_Chart,
                                               lv_color_hex(C_TEAL),
                                               LV_CHART_AXIS_PRIMARY_Y);

    /* Seed initial data from g_mfg */
    for (int i = 0; i < NUM_HOURS; i++) {
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesPlan,   i, g_mfg.hourly_plan[i]);
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesActual, i, g_mfg.hourly_actual[i]);
    }
    lv_chart_refresh(ui_Chart);

    /* Hour labels below chart */
    int label_y = 238;  /* just below chart bottom in section */
    for (int i = 0; i < NUM_HOURS; i++) {
        int lx = 10 + i * (750 / NUM_HOURS) + (750 / NUM_HOURS) / 2 - 18;
        make_label(section, g_mfg.hourly_label[i],
                   &lv_font_montserrat_12, lv_color_hex(C_GRAY),
                   lx, label_y, LV_TEXT_ALIGN_CENTER, 36);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   LEFT PANEL — TABLE  (x=0, y=508, w=780, h=292)
   ══════════════════════════════════════════════════════════════════════════ */

static void build_table_section(lv_obj_t *screen)
{
    lv_obj_t *section = make_container(screen, 0, 508, 780, 292,
                                       C_SURFACE2, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(section, LV_BORDER_SIDE_RIGHT, LV_PART_MAIN);

    /* Title bar */
    make_label(section, "Hourly Detail",
               &lv_font_montserrat_14, lv_color_hex(C_WHITE),
               12, 8, LV_TEXT_ALIGN_LEFT, 200);

    /* Table */
    ui_Table = lv_table_create(section);
    lv_obj_set_pos(ui_Table, 0, 32);
    lv_obj_set_size(ui_Table, 780, 260);

    /* 10 columns: HOUR, PLANNED, S1, S2, S3, S4, TOTAL, VAR, DEF, STATUS */
    const struct { const char *name; int w; } cols[] = {
        { "HOUR",    75 },
        { "PLANNED", 70 },
        { "S1",      55 },
        { "S2",      55 },
        { "S3",      55 },
        { "S4",      55 },
        { "TOTAL",   70 },
        { "VAR",     65 },
        { "DEF",     55 },
        { "STATUS",  100 },
    };
    int num_cols = (int)(sizeof(cols) / sizeof(cols[0]));

    lv_table_set_column_count(ui_Table, num_cols);
    for (int c = 0; c < num_cols; c++) {
        lv_table_set_column_width(ui_Table, c, cols[c].w);
        lv_table_set_cell_value(ui_Table, 0, c, cols[c].name);
    }

    /* Data rows */
    lv_table_set_row_count(ui_Table, 1 + NUM_HOURS);
    for (int r = 0; r < NUM_HOURS; r++) {
        int row = r + 1;
        int total  = g_mfg.hourly_actual[r];
        int plan   = g_mfg.hourly_plan[r];
        int var    = total - plan;
        int def    = g_mfg.hourly_defects[r];
        char buf[32];

        lv_table_set_cell_value(ui_Table, row, 0, g_mfg.hourly_label[r]);

        snprintf(buf, sizeof(buf), "%d", plan);
        lv_table_set_cell_value(ui_Table, row, 1, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s1[r]);
        lv_table_set_cell_value(ui_Table, row, 2, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s2[r]);
        lv_table_set_cell_value(ui_Table, row, 3, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s3[r]);
        lv_table_set_cell_value(ui_Table, row, 4, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s4[r]);
        lv_table_set_cell_value(ui_Table, row, 5, buf);

        snprintf(buf, sizeof(buf), "%d", total);
        lv_table_set_cell_value(ui_Table, row, 6, buf);

        snprintf(buf, sizeof(buf), "%+d", var);
        lv_table_set_cell_value(ui_Table, row, 7, buf);

        snprintf(buf, sizeof(buf), "%d", def);
        lv_table_set_cell_value(ui_Table, row, 8, buf);

        if (var < -20) {
            lv_table_set_cell_value(ui_Table, row, 9, "Critical");
        } else if (var >= 0) {
            lv_table_set_cell_value(ui_Table, row, 9, "On Track");
        } else {
            lv_table_set_cell_value(ui_Table, row, 9, "Behind");
        }
    }

    /* Table style — header row */
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

/* ═══════════════════════════════════════════════════════════════════════════
   RIGHT PANEL — ALERTS  (x=780, y=248, w=500, h=552)
   ══════════════════════════════════════════════════════════════════════════ */

/* Build one alert row inside the scrollable alert list */
static void build_alert_row(lv_obj_t *parent, int idx)
{
    if (idx < 0 || idx >= g_mfg.alert_count) return;
    const alert_t *a = &g_mfg.alerts[idx];

    uint32_t border_col = (a->severity == 1) ? C_RED : C_ORANGE;
    uint32_t bg_col     = a->acknowledged ? C_SURFACE2 : C_SURFACE;

    /* Row container — full width of panel, h=76 */
    lv_obj_t *row = make_container(parent, 0, idx * 80, 498, 76,
                                   bg_col, border_col,
                                   a->acknowledged ? 0 : 3, 4);
    lv_obj_set_style_border_side(row, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    s_alert_rows[idx] = row;

    /* Station badge */
    lv_obj_t *badge = make_container(row, 8, 20, 36, 24,
                                     C_SURFACE2, C_TEAL, 1, 4);
    make_label(badge, a->station,
               &lv_font_montserrat_12, lv_color_hex(C_TEAL),
               0, 4, LV_TEXT_ALIGN_CENTER, 36);

    /* Message */
    make_label(row, a->message,
               &lv_font_montserrat_16, lv_color_hex(C_WHITE),
               52, 10, LV_TEXT_ALIGN_LEFT, 380);

    /* Time ago */
    char time_buf[24];
    snprintf(time_buf, sizeof(time_buf), "\xe2\x80\xa2 %dm ago", a->minutes_ago);
    make_label(row, time_buf,
               &lv_font_montserrat_12, lv_color_hex(C_GRAY),
               52, 46, LV_TEXT_ALIGN_LEFT, 120);

    /* ACK button (hidden if already acknowledged) */
    lv_obj_t *ack_btn = make_button(row, "ACK",
                                    &lv_font_montserrat_12,
                                    380, 24, 50, 26,
                                    C_BLUE_ACK, C_WHITE, 4);
    lv_obj_add_event_cb(ack_btn, cb_ack_btn, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);
    s_alert_ack_btns[idx] = ack_btn;

    if (a->acknowledged) {
        lv_obj_add_flag(ack_btn, LV_OBJ_FLAG_HIDDEN);
    }

    /* Checkmark button */
    lv_obj_t *chk_btn = make_button(row, "v",
                                    &lv_font_montserrat_12,
                                    436, 24, 30, 26,
                                    C_GREEN, C_WHITE, 4);
    lv_obj_add_event_cb(chk_btn, cb_ack_btn, LV_EVENT_CLICKED,
                        (void *)(intptr_t)idx);

    if (a->acknowledged) {
        lv_obj_add_flag(chk_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void build_right_panel(lv_obj_t *screen)
{
    lv_obj_t *panel = make_container(screen, 780, 248, 500, 552,
                                     C_BG, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(panel, LV_BORDER_SIDE_LEFT, LV_PART_MAIN);

    /* Title bar */
    lv_obj_t *title_bar = make_container(panel, 0, 0, 500, 44,
                                         C_SURFACE, C_BORDER, 1, 0);
    lv_obj_set_style_border_side(title_bar, LV_BORDER_SIDE_BOTTOM, LV_PART_MAIN);

    /* Red dot indicator */
    lv_obj_t *dot = lv_obj_create(title_bar);
    lv_obj_set_pos(dot, 12, 15);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_set_style_radius(dot, 6, LV_PART_MAIN);
    lv_obj_set_style_bg_color(dot, lv_color_hex(C_RED), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(dot, LV_OPA_TRANSP, LV_PART_MAIN);

    make_label(title_bar, "Active Alerts",
               &lv_font_montserrat_16, lv_color_hex(C_WHITE),
               32, 10, LV_TEXT_ALIGN_LEFT, 200);

    /* Scrollable alert list container */
    ui_AlertList = lv_obj_create(panel);
    lv_obj_set_pos(ui_AlertList, 0, 44);
    lv_obj_set_size(ui_AlertList, 500, 508);
    lv_obj_set_style_bg_color(ui_AlertList, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_AlertList, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_opa(ui_AlertList, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_all(ui_AlertList, 4, LV_PART_MAIN);
    lv_obj_add_flag(ui_AlertList, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(ui_AlertList, LV_DIR_VER);

    /* Initialize row pointer array */
    for (int i = 0; i < MAX_ALERTS; i++) {
        s_alert_rows[i]     = NULL;
        s_alert_ack_btns[i] = NULL;
    }

    /* Build initial alert rows */
    for (int i = 0; i < g_mfg.alert_count; i++) {
        build_alert_row(ui_AlertList, i);
    }
}

/* ═══════════════════════════════════════════════════════════════════════════
   UI INIT
   ══════════════════════════════════════════════════════════════════════════ */

void ui_init(void)
{
    ui_Screen1 = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_hex(C_BG), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui_Screen1, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_Screen1, SCREEN_W, SCREEN_H);

    build_nav_bar(ui_Screen1);
    build_line_header(ui_Screen1);
    build_period_bar(ui_Screen1);
    build_kpi_row(ui_Screen1);
    build_chart_section(ui_Screen1);
    build_table_section(ui_Screen1);
    build_right_panel(ui_Screen1);

    lv_scr_load(ui_Screen1);
}

/* ═══════════════════════════════════════════════════════════════════════════
   DATA REFRESH FUNCTIONS
   ══════════════════════════════════════════════════════════════════════════ */

/**
 * ui_update_kpi — refresh KPI card values from g_mfg.
 * Call every 50ms from ui_task.
 */
void ui_update_kpi(void)
{
    if (ui_KpiActualVal == NULL || ui_KpiEffVal == NULL || ui_KpiDefVal == NULL)
        return;

    char buf[24];

    /* Actual count */
    snprintf(buf, sizeof(buf), "%d", g_mfg.actual_today);
    lv_label_set_text(ui_KpiActualVal, buf);
    lv_color_t actual_col = (g_mfg.actual_today < g_mfg.plan_today)
                            ? lv_color_hex(C_RED) : lv_color_hex(C_GREEN);
    lv_obj_set_style_text_color(ui_KpiActualVal, actual_col, LV_PART_MAIN);

    /* Efficiency */
    snprintf(buf, sizeof(buf), "%.1f%%", g_mfg.efficiency);
    lv_label_set_text(ui_KpiEffVal, buf);
    lv_color_t eff_col = (g_mfg.efficiency < 100.0f)
                         ? lv_color_hex(C_RED) : lv_color_hex(C_GREEN);
    lv_obj_set_style_text_color(ui_KpiEffVal, eff_col, LV_PART_MAIN);

    /* Defects IPPM */
    snprintf(buf, sizeof(buf), "%d", g_mfg.defects_ippm);
    lv_label_set_text(ui_KpiDefVal, buf);
}

/**
 * ui_update_chart — refresh chart series from g_mfg hourly arrays.
 * Call every ~2 seconds from ui_task.
 */
void ui_update_chart(void)
{
    if (ui_Chart == NULL) return;

    for (int i = 0; i < NUM_HOURS; i++) {
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesPlan,   i, g_mfg.hourly_plan[i]);
        lv_chart_set_value_by_id(ui_Chart, ui_ChartSeriesActual, i, g_mfg.hourly_actual[i]);
    }
    lv_chart_refresh(ui_Chart);
}

/**
 * ui_update_table — refresh table cells from g_mfg hourly arrays.
 * Call every ~2 seconds from ui_task.
 */
void ui_update_table(void)
{
    if (ui_Table == NULL) return;

    char buf[32];
    for (int r = 0; r < NUM_HOURS; r++) {
        int row   = r + 1;
        int total = g_mfg.hourly_actual[r];
        int plan  = g_mfg.hourly_plan[r];
        int var   = total - plan;
        int def   = g_mfg.hourly_defects[r];

        lv_table_set_cell_value(ui_Table, row, 0, g_mfg.hourly_label[r]);

        snprintf(buf, sizeof(buf), "%d", plan);
        lv_table_set_cell_value(ui_Table, row, 1, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s1[r]);
        lv_table_set_cell_value(ui_Table, row, 2, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s2[r]);
        lv_table_set_cell_value(ui_Table, row, 3, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s3[r]);
        lv_table_set_cell_value(ui_Table, row, 4, buf);

        snprintf(buf, sizeof(buf), "%d", g_mfg.hourly_s4[r]);
        lv_table_set_cell_value(ui_Table, row, 5, buf);

        snprintf(buf, sizeof(buf), "%d", total);
        lv_table_set_cell_value(ui_Table, row, 6, buf);

        snprintf(buf, sizeof(buf), "%+d", var);
        lv_table_set_cell_value(ui_Table, row, 7, buf);

        snprintf(buf, sizeof(buf), "%d", def);
        lv_table_set_cell_value(ui_Table, row, 8, buf);

        if (var < -20) {
            lv_table_set_cell_value(ui_Table, row, 9, "Critical");
        } else if (var >= 0) {
            lv_table_set_cell_value(ui_Table, row, 9, "On Track");
        } else {
            lv_table_set_cell_value(ui_Table, row, 9, "Behind");
        }
    }
}

/**
 * ui_update_alerts — rebuild new alert rows for any newly added alerts.
 * Existing rows are updated in-place (acknowledged state already handled
 * by the ACK button callback). Only adds rows for alerts that do not yet
 * have an allocated row object.
 * Call every ~5 seconds from ui_task.
 */
void ui_update_alerts(void)
{
    if (ui_AlertList == NULL) return;

    for (int i = 0; i < g_mfg.alert_count && i < MAX_ALERTS; i++) {
        if (s_alert_rows[i] == NULL) {
            /* New alert — create the row */
            build_alert_row(ui_AlertList, i);
        }
        /* Update acknowledged appearance for existing rows */
        else if (g_mfg.alerts[i].acknowledged) {
            lv_obj_set_style_bg_color(s_alert_rows[i],
                                      lv_color_hex(C_SURFACE2), LV_PART_MAIN);
            lv_obj_set_style_border_opa(s_alert_rows[i], LV_OPA_TRANSP, LV_PART_MAIN);
            if (s_alert_ack_btns[i] != NULL) {
                lv_obj_add_flag(s_alert_ack_btns[i], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}
