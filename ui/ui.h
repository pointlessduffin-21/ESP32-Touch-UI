/**
 * ui.h — CubeRTMS Manufacturing Dashboard widget declarations
 *
 * Layout (1280x800):
 *   Nav bar        (y=0,   h=44)
 *   Line header    (y=44,  h=60)
 *   Period bar     (y=104, h=44)
 *   KPI row        (y=148, h=100)
 *   Left panel     (x=0,   y=248, w=780, h=552)  chart + table
 *   Right panel    (x=780, y=248, w=500, h=552)  alerts
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "../src/sensors.h"

/* ── Screen ────────────────────────────────────────────────────────────────── */
extern lv_obj_t *ui_Screen1;

/* ── KPI value labels (updated by ui_update_kpi) ───────────────────────────── */
extern lv_obj_t *ui_KpiActualVal;     /* actual count, red when below plan */
extern lv_obj_t *ui_KpiEffVal;        /* efficiency %, red when < 100      */
extern lv_obj_t *ui_KpiDefVal;        /* defects IPPM                      */

/* ── Chart (left panel top) ─────────────────────────────────────────────────── */
extern lv_obj_t         *ui_Chart;
extern lv_chart_series_t *ui_ChartSeriesPlan;
extern lv_chart_series_t *ui_ChartSeriesActual;

/* ── Table (left panel bottom) ──────────────────────────────────────────────── */
extern lv_obj_t *ui_Table;

/* ── Alert list container (right panel) ─────────────────────────────────────── */
extern lv_obj_t *ui_AlertList;        /* scrollable container, parent of alert rows */

/* ── Period selector buttons ────────────────────────────────────────────────── */
extern lv_obj_t *ui_BtnToday;
extern lv_obj_t *ui_BtnYesterday;
extern lv_obj_t *ui_BtnLast7;

/* ── Lifecycle ───────────────────────────────────────────────────────────────── */
void ui_init(void);

/* ── Data refresh functions (call from ui_task under display_lock) ───────────── */
void ui_update_kpi(void);
void ui_update_chart(void);
void ui_update_table(void);
void ui_update_alerts(void);

#ifdef __cplusplus
}
#endif
