/**
 * sensors.cpp — CubeRTMS Manufacturing Dashboard mock sensor simulation
 *
 * Simulates three production lines for Centralized Status, plus detailed
 * data for Harness Line 01: hourly plan/actual, OEE, station performance,
 * and alerts matching the CubeRTMS web dashboard screenshots.
 */

#include "sensors.h"
#include <string.h>
#include <math.h>

#ifdef SIMULATOR
#  include "../../sim/arduino_compat.h"
#else
#  include "esp_log.h"
#  include "esp_timer.h"
static inline uint32_t millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}
#endif

static const char *STAG = "sensors";

MfgData      g_mfg;
LineOverview g_lines[NUM_LINES];
StationPerf  g_stations[NUM_STATIONS];
OeeData      g_oee;

static uint32_t s_tick = 0;

static void recalc_totals(void)
{
    int total_plan   = 0;
    int total_actual = 0;
    for (int i = 0; i <= g_mfg.current_hour_idx; i++) {
        total_plan   += g_mfg.hourly_plan[i];
        total_actual += g_mfg.hourly_actual[i];
    }
    g_mfg.plan_today   = total_plan;
    g_mfg.actual_today = total_actual;
    if (total_plan > 0)
        g_mfg.efficiency = (float)total_actual / (float)total_plan * 100.0f;
    else
        g_mfg.efficiency = 0.0f;

    int total_defects = 0;
    for (int i = 0; i <= g_mfg.current_hour_idx; i++)
        total_defects += g_mfg.hourly_defects[i];
    if (total_actual > 0)
        g_mfg.defects_ippm = (int)((float)total_defects / (float)total_actual * 1000000.0f);
    else
        g_mfg.defects_ippm = 0;
}

static void init_line_overviews(void)
{
    /* Line 0: Harness Line 01 */
    memset(&g_lines[0], 0, sizeof(LineOverview));
    strncpy(g_lines[0].name, "Harness Line 01", sizeof(g_lines[0].name) - 1);
    strncpy(g_lines[0].part_number, "FG-HARN-002", sizeof(g_lines[0].part_number) - 1);
    g_lines[0].running = true;
    g_lines[0].oee = 26.4f;
    g_lines[0].availability = 100.0f;
    g_lines[0].performance  = 26.7f;
    g_lines[0].quality      = 98.8f;
    g_lines[0].plan_vs_actual_pct = 28.5f;
    g_lines[0].planned = 600;
    g_lines[0].actual  = 171;
    g_lines[0].defects = 2;

    /* Line 1: Main1 */
    memset(&g_lines[1], 0, sizeof(LineOverview));
    strncpy(g_lines[1].name, "Main1", sizeof(g_lines[1].name) - 1);
    strncpy(g_lines[1].part_number, "FG-HARN-001", sizeof(g_lines[1].part_number) - 1);
    g_lines[1].running = true;
    g_lines[1].oee = 0.0f;
    g_lines[1].availability = 100.0f;
    g_lines[1].performance  = 0.0f;
    g_lines[1].quality      = 100.0f;
    g_lines[1].plan_vs_actual_pct = 11.2f;
    g_lines[1].planned = 600;
    g_lines[1].actual  = 67;
    g_lines[1].defects = 2;

    /* Line 2: Engine */
    memset(&g_lines[2], 0, sizeof(LineOverview));
    strncpy(g_lines[2].name, "Engine", sizeof(g_lines[2].name) - 1);
    strncpy(g_lines[2].part_number, "SA-LOOM-E01", sizeof(g_lines[2].part_number) - 1);
    g_lines[2].running = true;
    g_lines[2].oee = 27.9f;
    g_lines[2].availability = 100.0f;
    g_lines[2].performance  = 28.3f;
    g_lines[2].quality      = 98.5f;
    g_lines[2].plan_vs_actual_pct = 22.7f;
    g_lines[2].planned = 600;
    g_lines[2].actual  = 136;
    g_lines[2].defects = 2;
}

static void init_stations(void)
{
    memset(g_stations, 0, sizeof(g_stations));

    strncpy(g_stations[0].name, "SUBASSY", sizeof(g_stations[0].name) - 1);
    strncpy(g_stations[0].full_name, "Sub-Assembly Station", sizeof(g_stations[0].full_name) - 1);
    g_stations[0].yield_pct   = 94.4f;
    g_stations[0].pass_count  = 1627;
    g_stations[0].total_count = 1724;
    g_stations[0].alarm       = true;

    strncpy(g_stations[1].name, "OLTT", sizeof(g_stations[1].name) - 1);
    strncpy(g_stations[1].full_name, "Open/Long/Tight Test...", sizeof(g_stations[1].full_name) - 1);
    g_stations[1].yield_pct   = 91.5f;
    g_stations[1].pass_count  = 1512;
    g_stations[1].total_count = 1652;
    g_stations[1].alarm       = false;

    strncpy(g_stations[2].name, "INSPECTION", sizeof(g_stations[2].name) - 1);
    strncpy(g_stations[2].full_name, "Inspection Station", sizeof(g_stations[2].full_name) - 1);
    g_stations[2].yield_pct   = 96.4f;
    g_stations[2].pass_count  = 1400;
    g_stations[2].total_count = 1452;
    g_stations[2].alarm       = true;

    strncpy(g_stations[3].name, "PACKING", sizeof(g_stations[3].name) - 1);
    strncpy(g_stations[3].full_name, "Packing Station", sizeof(g_stations[3].full_name) - 1);
    g_stations[3].yield_pct   = 98.2f;
    g_stations[3].pass_count  = 1442;
    g_stations[3].total_count = 1468;
    g_stations[3].alarm       = false;
}

static void init_oee(void)
{
    memset(&g_oee, 0, sizeof(g_oee));
    g_oee.overall_oee   = 15.4f;
    g_oee.availability   = 15.7f;
    g_oee.performance    = 100.0f;
    g_oee.quality        = 97.8f;

    g_oee.shift_duration_min    = 480.0f;
    g_oee.planned_downtime_min  = 0.0f;
    g_oee.unplanned_downtime_min = 404.5f;
    g_oee.run_time_min          = 75.5f;

    g_oee.ideal_cycle_sec = 45.0f;
    g_oee.total_pieces    = 600;
    g_oee.speed_loss_min  = 0.0f;

    g_oee.good_pieces    = 587;
    g_oee.defect_count   = 13;

    g_oee.breakdown_min      = 0.0f;
    g_oee.changeover_min     = 0.0f;
    g_oee.minor_stop_min     = 404.5f;
    g_oee.speed_loss_perf_min = 0.0f;
    g_oee.process_defects    = 13;
    g_oee.startup_rejects    = 0;
}

void sensors_init(void)
{
    memset(&g_mfg, 0, sizeof(g_mfg));

    strncpy(g_mfg.part_number, "FG-HARN-002",    sizeof(g_mfg.part_number) - 1);
    strncpy(g_mfg.line_name,   "Harness Line 01", sizeof(g_mfg.line_name) - 1);
    strncpy(g_mfg.mode_label,  "Running",         sizeof(g_mfg.mode_label) - 1);
    g_mfg.running       = true;
    g_mfg.takt_sec      = 45;
    g_mfg.target_per_hr = 80;

    const char *labels[NUM_HOURS] = {
        "01:00","02:00","03:00","04:00","05:00",
        "06:00","07:00","08:00","09:00","10:00"
    };
    for (int i = 0; i < NUM_HOURS; i++)
        strncpy(g_mfg.hourly_label[i], labels[i], sizeof(g_mfg.hourly_label[i]) - 1);

    for (int i = 0; i < NUM_HOURS; i++)
        g_mfg.hourly_plan[i] = 75;

    /* Historical actual matching web screenshot patterns */
    static const int hist_actual[NUM_HOURS] = {
        0, 87, 103, 95, 72, 89, 92, 76, 45, 0
    };
    for (int i = 0; i < NUM_HOURS; i++)
        g_mfg.hourly_actual[i] = hist_actual[i];

    /* Station splits (SubAssy, OLTT, Inspection, Packing) */
    static const int h_s1[NUM_HOURS] = { 0, 103, 0, 0, 0, 89, 92, 76, 45, 0};
    static const int h_s2[NUM_HOURS] = { 0,  95, 0, 0, 0,  0,  0,  0, 43, 0};
    static const int h_s3[NUM_HOURS] = { 0,  73, 0, 0, 0,  0,  0,  0, 41, 0};
    static const int h_s4[NUM_HOURS] = { 0,  87, 0, 0, 0,  0,  0,  0, 30, 0};
    for (int i = 0; i < NUM_HOURS; i++) {
        g_mfg.hourly_s1[i] = h_s1[i];
        g_mfg.hourly_s2[i] = h_s2[i];
        g_mfg.hourly_s3[i] = h_s3[i];
        g_mfg.hourly_s4[i] = h_s4[i];
    }

    static const int hist_defects[NUM_HOURS] = {0, 1, 0, 0, 0, 0, 0, 0, 1, 0};
    for (int i = 0; i < NUM_HOURS; i++)
        g_mfg.hourly_defects[i] = hist_defects[i];

    g_mfg.current_hour_idx = 9;
    g_mfg.total_alert_count = 47;

    recalc_totals();

    /* Alerts matching CubeRTMS web screenshot */
    g_mfg.alert_count = 8;

    strncpy(g_mfg.alerts[0].message, "Packing material depleted", 47);
    strncpy(g_mfg.alerts[0].station, "Pack", 7);
    g_mfg.alerts[0].minutes_ago = 368; g_mfg.alerts[0].severity = 0;
    g_mfg.alerts[0].acknowledged = false;

    strncpy(g_mfg.alerts[1].message, "Missing clip C4", 47);
    strncpy(g_mfg.alerts[1].station, "Insp", 7);
    g_mfg.alerts[1].minutes_ago = 368; g_mfg.alerts[1].severity = 0;
    g_mfg.alerts[1].acknowledged = false;

    strncpy(g_mfg.alerts[2].message, "Tester calibration required", 47);
    strncpy(g_mfg.alerts[2].station, "OLTT", 7);
    g_mfg.alerts[2].minutes_ago = 368; g_mfg.alerts[2].severity = 0;
    g_mfg.alerts[2].acknowledged = false;

    strncpy(g_mfg.alerts[3].message, "Missing clip C4", 47);
    strncpy(g_mfg.alerts[3].station, "Insp", 7);
    g_mfg.alerts[3].minutes_ago = 15722; g_mfg.alerts[3].severity = 1;
    g_mfg.alerts[3].acknowledged = false;

    strncpy(g_mfg.alerts[4].message, "Label misalignment", 47);
    strncpy(g_mfg.alerts[4].station, "Insp", 7);
    g_mfg.alerts[4].minutes_ago = 15939; g_mfg.alerts[4].severity = 1;
    g_mfg.alerts[4].acknowledged = false;

    strncpy(g_mfg.alerts[5].message, "OLTT test failure", 47);
    strncpy(g_mfg.alerts[5].station, "OLTT", 7);
    g_mfg.alerts[5].minutes_ago = 15939; g_mfg.alerts[5].severity = 1;
    g_mfg.alerts[5].acknowledged = false;

    strncpy(g_mfg.alerts[6].message, "Open circuit detected", 47);
    strncpy(g_mfg.alerts[6].station, "OLTT", 7);
    g_mfg.alerts[6].minutes_ago = 16108; g_mfg.alerts[6].severity = 1;
    g_mfg.alerts[6].acknowledged = false;

    strncpy(g_mfg.alerts[7].message, "Material shortage sub-assy", 47);
    strncpy(g_mfg.alerts[7].station, "SubA", 7);
    g_mfg.alerts[7].minutes_ago = 16200; g_mfg.alerts[7].severity = 1;
    g_mfg.alerts[7].acknowledged = false;

    init_line_overviews();
    init_stations();
    init_oee();

    s_tick = 0;

    ESP_LOGI(STAG, "CubeRTMS init: %s | %s | plan=%d actual=%d eff=%.1f%%",
             g_mfg.line_name, g_mfg.part_number,
             g_mfg.plan_today, g_mfg.actual_today, g_mfg.efficiency);
}

void sensors_update(void)
{
    s_tick++;

    /* Produce one unit in the live hour every 1200 ticks (~1/min) */
    if ((s_tick % 1200) == 600) {
        int idx = g_mfg.current_hour_idx;
        g_mfg.hourly_actual[idx]++;
        int unit = g_mfg.hourly_actual[idx];
        g_mfg.hourly_s1[idx] = unit / 4 + (unit % 4 >= 1 ? 1 : 0);
        g_mfg.hourly_s2[idx] = unit / 4 + (unit % 4 >= 2 ? 1 : 0);
        g_mfg.hourly_s3[idx] = unit / 4 + (unit % 4 >= 3 ? 1 : 0);
        g_mfg.hourly_s4[idx] = unit / 4;

        if ((unit % 97) == 0)
            g_mfg.hourly_defects[idx]++;

        recalc_totals();

        /* Also update line overview for Harness Line 01 */
        g_lines[0].actual  = g_mfg.actual_today;
        g_lines[0].planned = g_mfg.plan_today;
        if (g_mfg.plan_today > 0)
            g_lines[0].plan_vs_actual_pct = (float)g_mfg.actual_today / (float)g_mfg.plan_today * 100.0f;
    }

    /* Age alert timestamps every 1200 ticks (~1 sim-minute) */
    if ((s_tick % 1200) == 0) {
        for (int i = 0; i < g_mfg.alert_count; i++)
            g_mfg.alerts[i].minutes_ago++;
    }
}
