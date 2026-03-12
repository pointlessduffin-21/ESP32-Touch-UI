/**
 * sensors.cpp — CubeRTMS Manufacturing Dashboard mock sensor simulation
 *
 * Simulates a production line running SMT-Line-3, part XB-4402-R.
 * Historical data seeded for hours 05:00–13:00 (idx 0–8).
 * Current hour (idx 9, 14:xx) starts at 0 and increments live.
 *
 * Production rate: 1 unit per 1200 ticks  (~1 unit/minute at 50ms/tick)
 * New alert:       every 2400 ticks        (~2 min)
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

MfgData g_mfg;

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

    /* defects: count defects across all hours, convert to iPPM */
    int total_defects = 0;
    for (int i = 0; i <= g_mfg.current_hour_idx; i++)
        total_defects += g_mfg.hourly_defects[i];
    if (total_actual > 0)
        g_mfg.defects_ippm = (int)((float)total_defects / (float)total_actual * 1000000.0f);
    else
        g_mfg.defects_ippm = 0;
}

void sensors_init(void)
{
    memset(&g_mfg, 0, sizeof(g_mfg));

    strncpy(g_mfg.part_number, "XB-4402-R",  sizeof(g_mfg.part_number) - 1);
    strncpy(g_mfg.line_name,   "SMT-Line-3", sizeof(g_mfg.line_name)   - 1);
    strncpy(g_mfg.mode_label,  "RUNNING",    sizeof(g_mfg.mode_label)  - 1);
    g_mfg.running     = true;
    g_mfg.takt_sec    = 60;
    g_mfg.target_per_hr = 60;

    /* Hour labels 05:00–14:00 */
    const char *labels[NUM_HOURS] = {
        "05:00","06:00","07:00","08:00","09:00",
        "10:00","11:00","12:00","13:00","14:00"
    };
    for (int i = 0; i < NUM_HOURS; i++)
        strncpy(g_mfg.hourly_label[i], labels[i], sizeof(g_mfg.hourly_label[i]) - 1);

    /* Historical hourly plan (60 units/hr target each hour) */
    for (int i = 0; i < NUM_HOURS; i++)
        g_mfg.hourly_plan[i] = 60;

    /* Historical actual (idx 0–8 are past hours; idx 9 = live) */
    static const int historical_actual[NUM_HOURS] = {
        82, 100, 105, 30, 60, 0, 0, 60, 65, 0
    };
    for (int i = 0; i < NUM_HOURS; i++)
        g_mfg.hourly_actual[i] = historical_actual[i];

    /* Station splits for past hours (approximate breakdowns) */
    static const int hist_s1[NUM_HOURS] = {22,25,27, 8,16,0,0,16,17,0};
    static const int hist_s2[NUM_HOURS] = {20,25,26, 8,15,0,0,15,16,0};
    static const int hist_s3[NUM_HOURS] = {20,25,26, 7,15,0,0,15,16,0};
    static const int hist_s4[NUM_HOURS] = {20,25,26, 7,14,0,0,14,16,0};
    for (int i = 0; i < NUM_HOURS; i++) {
        g_mfg.hourly_s1[i] = hist_s1[i];
        g_mfg.hourly_s2[i] = hist_s2[i];
        g_mfg.hourly_s3[i] = hist_s3[i];
        g_mfg.hourly_s4[i] = hist_s4[i];
    }

    /* Defects per hour (historical) */
    static const int hist_defects[NUM_HOURS] = {1,0,1,2,0,0,0,0,1,0};
    for (int i = 0; i < NUM_HOURS; i++)
        g_mfg.hourly_defects[i] = hist_defects[i];

    g_mfg.current_hour_idx = 9;   /* currently in the 14:00 hour */

    recalc_totals();

    /* Pre-seed alerts matching CubeRTMS screenshot */
    g_mfg.alert_count = 6;

    strncpy(g_mfg.alerts[0].message,  "Feeder jam detected",       47);
    strncpy(g_mfg.alerts[0].station,  "ST-02",                      7);
    g_mfg.alerts[0].minutes_ago = 3;
    g_mfg.alerts[0].severity    = 1;   /* critical */
    g_mfg.alerts[0].acknowledged = false;

    strncpy(g_mfg.alerts[1].message,  "Vision check failed x3",    47);
    strncpy(g_mfg.alerts[1].station,  "ST-04",                      7);
    g_mfg.alerts[1].minutes_ago = 7;
    g_mfg.alerts[1].severity    = 1;
    g_mfg.alerts[1].acknowledged = false;

    strncpy(g_mfg.alerts[2].message,  "Solder paste low",          47);
    strncpy(g_mfg.alerts[2].station,  "ST-01",                      7);
    g_mfg.alerts[2].minutes_ago = 15;
    g_mfg.alerts[2].severity    = 0;   /* warning */
    g_mfg.alerts[2].acknowledged = false;

    strncpy(g_mfg.alerts[3].message,  "Conveyor speed drift",      47);
    strncpy(g_mfg.alerts[3].station,  "ST-03",                      7);
    g_mfg.alerts[3].minutes_ago = 22;
    g_mfg.alerts[3].severity    = 0;
    g_mfg.alerts[3].acknowledged = true;

    strncpy(g_mfg.alerts[4].message,  "Stencil misalign >0.1mm",   47);
    strncpy(g_mfg.alerts[4].station,  "ST-01",                      7);
    g_mfg.alerts[4].minutes_ago = 31;
    g_mfg.alerts[4].severity    = 0;
    g_mfg.alerts[4].acknowledged = true;

    strncpy(g_mfg.alerts[5].message,  "Reflow temp spike",         47);
    strncpy(g_mfg.alerts[5].station,  "ST-05",                      7);
    g_mfg.alerts[5].minutes_ago = 45;
    g_mfg.alerts[5].severity    = 1;
    g_mfg.alerts[5].acknowledged = true;

    s_tick = 0;

    ESP_LOGI(STAG, "Manufacturing sensor init: %s | %s | plan=%d actual=%d eff=%.1f%%",
             g_mfg.line_name, g_mfg.part_number,
             g_mfg.plan_today, g_mfg.actual_today, g_mfg.efficiency);
}

void sensors_update(void)
{
    s_tick++;

    /* Age all alert minute counters roughly every 1200 ticks (1 sim-minute) */
    if ((s_tick % 1200) == 0) {
        for (int i = 0; i < g_mfg.alert_count; i++)
            g_mfg.alerts[i].minutes_ago++;
    }

    /* Produce one unit in the live hour every 1200 ticks */
    if ((s_tick % 1200) == 600) {   /* offset from alert aging */
        int idx = g_mfg.current_hour_idx;
        g_mfg.hourly_actual[idx]++;
        /* Distribute roughly across 4 stations */
        int unit = g_mfg.hourly_actual[idx];
        g_mfg.hourly_s1[idx] = unit / 4 + (unit % 4 >= 1 ? 1 : 0);
        g_mfg.hourly_s2[idx] = unit / 4 + (unit % 4 >= 2 ? 1 : 0);
        g_mfg.hourly_s3[idx] = unit / 4 + (unit % 4 >= 3 ? 1 : 0);
        g_mfg.hourly_s4[idx] = unit / 4;

        /* Occasional defect (~1%) */
        if ((unit % 97) == 0) {
            g_mfg.hourly_defects[idx]++;
        }

        recalc_totals();
    }

    /* Add a new alert every 2400 ticks (~2 min) if space available */
    if ((s_tick % 2400) == 1200 && g_mfg.alert_count < MAX_ALERTS) {
        /* Shift existing alerts down (newest at top) */
        int n = g_mfg.alert_count;
        if (n >= MAX_ALERTS) n = MAX_ALERTS - 1;
        for (int i = n; i > 0; i--)
            g_mfg.alerts[i] = g_mfg.alerts[i - 1];

        /* Insert a new alert at index 0 */
        static const char *new_msgs[] = {
            "Pick-and-place nozzle clog",
            "AOI reject rate high",
            "Board warp detected",
            "Paste volume OOC",
            "Pin-in-paste misplace",
        };
        static const char *new_stations[] = {"ST-02","ST-04","ST-03","ST-01","ST-02"};
        static int msg_idx = 0;
        int m = msg_idx % 5;

        strncpy(g_mfg.alerts[0].message,  new_msgs[m],      47);
        strncpy(g_mfg.alerts[0].station,  new_stations[m],   7);
        g_mfg.alerts[0].minutes_ago  = 0;
        g_mfg.alerts[0].severity     = (m % 2 == 0) ? 1 : 0;
        g_mfg.alerts[0].acknowledged = false;

        if (g_mfg.alert_count < MAX_ALERTS)
            g_mfg.alert_count++;

        msg_idx++;
    }
}
