/**
 * sensors.h — CubeRTMS Manufacturing Dashboard data model
 *
 * Data structures for all dashboard screens:
 *   - MfgData: single-line detailed production data
 *   - LineOverview: summary per line (centralized status)
 *   - OeeData: OEE breakdown for detail screen
 *   - StationPerf: yield-by-station cards
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALERTS   8
#define NUM_HOURS    10
#define NUM_LINES    3
#define NUM_STATIONS 4

/**
 * alert_t — a single active alert from the production line.
 * severity: 0 = warning (orange), 1 = critical (red)
 */
typedef struct {
    char  message[48];
    char  station[8];
    int   minutes_ago;
    int   severity;
    bool  acknowledged;
} alert_t;

/**
 * MfgData — complete manufacturing line state (single line detail).
 */
typedef struct {
    char     part_number[16];
    char     line_name[24];
    char     mode_label[16];
    bool     running;

    int      plan_today;
    int      actual_today;
    float    efficiency;
    int      defects_ippm;

    int      takt_sec;
    int      target_per_hr;

    int      hourly_plan[NUM_HOURS];
    int      hourly_actual[NUM_HOURS];
    int      hourly_s1[NUM_HOURS];
    int      hourly_s2[NUM_HOURS];
    int      hourly_s3[NUM_HOURS];
    int      hourly_s4[NUM_HOURS];
    int      hourly_defects[NUM_HOURS];
    char     hourly_label[NUM_HOURS][8];
    int      current_hour_idx;

    alert_t  alerts[MAX_ALERTS];
    int      alert_count;
    int      total_alert_count;       /* all alerts across all lines */
} MfgData;

/**
 * LineOverview — summary card for centralized line status screen.
 */
typedef struct {
    char     name[24];
    char     part_number[16];
    bool     running;
    float    oee;
    float    availability;
    float    performance;
    float    quality;
    float    plan_vs_actual_pct;
    int      planned;
    int      actual;
    int      defects;
} LineOverview;

/**
 * StationPerf — station yield card data.
 */
typedef struct {
    char     name[16];
    char     full_name[32];
    float    yield_pct;
    int      pass_count;
    int      total_count;
    bool     alarm;
} StationPerf;

/**
 * OeeData — detailed OEE breakdown.
 */
typedef struct {
    float    overall_oee;
    float    availability;
    float    performance;
    float    quality;
    /* Availability breakdown */
    float    shift_duration_min;
    float    planned_downtime_min;
    float    unplanned_downtime_min;
    float    run_time_min;
    /* Performance breakdown */
    float    ideal_cycle_sec;
    int      total_pieces;
    float    speed_loss_min;
    /* Quality breakdown */
    int      good_pieces;
    int      defect_count;
    /* Six big losses */
    float    breakdown_min;
    float    changeover_min;
    float    minor_stop_min;
    float    speed_loss_perf_min;
    int      process_defects;
    int      startup_rejects;
} OeeData;

/* ── Globals ─────────────────────────────────────────────────────────────── */
extern MfgData      g_mfg;
extern LineOverview g_lines[NUM_LINES];
extern StationPerf  g_stations[NUM_STATIONS];
extern OeeData      g_oee;

void sensors_init(void);
void sensors_update(void);

#ifdef __cplusplus
}
#endif
