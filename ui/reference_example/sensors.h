#pragma once

#include <stdint.h>

/**
 * sensors.h — Automotive dashboard data model (reference example).
 *
 * All values stored in display units; updated by sensors_update()
 * which animates values to simulate driving without real hardware.
 */

struct DashData {
    // Drivetrain
    uint16_t speed_mph;       // 0–220
    uint16_t rpm;             // 0–8000
    uint8_t  gear;            // 0=N, 1–8=gear, 9=P, 10=R
    bool     reverse;

    // Drive mode: 0=Normal 1=Sport 2=Dynamic 3=Track 4=Eco
    uint8_t  drive_mode;

    // Temperatures (°F)
    uint16_t coolant_temp_f;  // 100–250°F (warn >230)
    int8_t   ambient_temp_f;  // -40–120°F

    // Fuel
    uint8_t  fuel_pct;        // 0–100%
    uint16_t fuel_range_mi;   // estimated range in miles

    // Tire pressure (PSI)
    uint8_t  tire_fl_psi;     // front-left
    uint8_t  tire_fr_psi;     // front-right
    uint8_t  tire_rl_psi;     // rear-left
    uint8_t  tire_rr_psi;     // rear-right

    // Odometer
    uint32_t odometer_mi;

    // ADAS
    bool     lane_keep_active;
    bool     adaptive_cruise_active;
};

// Global dashboard state — read from UI update task
extern DashData g_dash;

// Drive mode name strings
static const char* DRIVE_MODE_NAMES[] = {
    "Normal", "Sport", "Dynamic", "Track", "Eco"
};

void sensors_init(void);
void sensors_update(void);
