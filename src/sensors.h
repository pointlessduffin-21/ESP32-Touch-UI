#pragma once

#include <stdint.h>

/**
 * sensors.h
 *
 * Dashboard data model. In production this would read from CAN bus, OBD-II,
 * or individual sensors over I2C/SPI/ADC. For development, sensors_update()
 * animates all values so the UI can be tested without real hardware.
 *
 * All values stored in SI-adjacent units; conversion to display units
 * (mph, °F, psi) happens here before the UI reads them.
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

/**
 * Initialize sensor subsystem.
 * In mock mode: seeds demo values.
 * In production: initializes CAN/OBD peripherals.
 */
void sensors_init(void);

/**
 * Update g_dash with latest sensor readings.
 * Call periodically (every 50–100ms) from a FreeRTOS task or timer.
 * In mock mode: smoothly animates values to simulate driving.
 */
void sensors_update(void);
