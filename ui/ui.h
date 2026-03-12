/**
 * ui.h — CubeRTMS Manufacturing Dashboard (all screens)
 *
 * Screens (1280x800, swipe to navigate):
 *   Screen 0 — Centralized Line Status (all lines overview)
 *   Screen 1 — Line Dashboard (detail for selected line)
 *   Screen 2 — OEE Dashboard (OEE breakdown)
 *   Screen 3 — Station Performance + Quality
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "../src/sensors.h"

/* ── Screens ───────────────────────────────────────────────────────────────── */
extern lv_obj_t *ui_ScreenCentralized;   /* Screen 0: all lines overview */
extern lv_obj_t *ui_ScreenLineDash;      /* Screen 1: line detail */
extern lv_obj_t *ui_ScreenOEE;          /* Screen 2: OEE breakdown */
extern lv_obj_t *ui_ScreenStation;       /* Screen 3: station perf + quality */

/* ── Line Dashboard widgets ────────────────────────────────────────────────── */
extern lv_obj_t *ui_KpiActualVal;
extern lv_obj_t *ui_KpiEffVal;
extern lv_obj_t *ui_KpiDefVal;
extern lv_obj_t *ui_Chart;
extern lv_chart_series_t *ui_ChartSeriesPlan;
extern lv_chart_series_t *ui_ChartSeriesActual;
extern lv_obj_t *ui_Table;
extern lv_obj_t *ui_AlertList;
extern lv_obj_t *ui_BtnToday;
extern lv_obj_t *ui_BtnYesterday;
extern lv_obj_t *ui_BtnLast7;

/* ── Lifecycle ─────────────────────────────────────────────────────────────── */
void ui_init(void);

/* ── Data refresh (call under display_lock) ──────────────────────────────── */
void ui_update_kpi(void);
void ui_update_chart(void);
void ui_update_table(void);
void ui_update_alerts(void);
void ui_update_centralized(void);
void ui_update_oee(void);
void ui_update_stations(void);

#ifdef __cplusplus
}
#endif
