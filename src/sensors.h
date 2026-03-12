/**
 * sensors.h — CubeRTMS Manufacturing Dashboard data model
 *
 * Replaces the automotive DashData struct with manufacturing MES data
 * suitable for a line-level production dashboard.
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ALERTS  8
#define NUM_HOURS   10

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
 * MfgData — complete manufacturing line state.
 * All fields are updated by sensors_update() and read by the UI task.
 */
typedef struct {
    /* Part / production identity */
    char     part_number[16];       // e.g. "FG-HARN-001"
    char     line_name[16];         // e.g. "Main1"
    char     mode_label[16];        // e.g. "Running"
    bool     running;

    /* Daily totals */
    int      plan_today;            // e.g. 1800
    int      actual_today;          // increments dynamically
    float    efficiency;            // actual / plan * 100.0
    int      defects_ippm;          // defects per million parts

    /* Takt / target */
    int      takt_sec;              // cycle time in seconds (60)
    int      target_per_hr;         // units per hour target (60)

    /* Hourly arrays — indexed 0..NUM_HOURS-1 */
    int      hourly_plan[NUM_HOURS];
    int      hourly_actual[NUM_HOURS];
    int      hourly_s1[NUM_HOURS];
    int      hourly_s2[NUM_HOURS];
    int      hourly_s3[NUM_HOURS];
    int      hourly_s4[NUM_HOURS];
    int      hourly_defects[NUM_HOURS];
    char     hourly_label[NUM_HOURS][8]; // "05:00" … "14:00"
    int      current_hour_idx;           // index of the live (current) hour

    /* Alerts */
    alert_t  alerts[MAX_ALERTS];
    int      alert_count;
} MfgData;

/* Global manufacturing state — written by sensor task, read by UI task */
extern MfgData g_mfg;

/**
 * sensors_init() — seed g_mfg with historical data and pre-seeded alerts.
 * Call once before starting sensor_task.
 */
void sensors_init(void);

/**
 * sensors_update() — advance simulation by one tick (~50ms).
 * Call from a FreeRTOS task at ~50ms intervals.
 */
void sensors_update(void);

#ifdef __cplusplus
}
#endif
