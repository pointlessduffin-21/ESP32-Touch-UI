/**
 * sensors.cpp — Automotive dashboard mock sensor simulation (reference example).
 *
 * Animation sequence:
 *   0–5s   : startup, values at idle
 *   5–15s  : accelerate to 60 mph / 3000 RPM (Sport mode)
 *   15–25s : cruise at 60 mph
 *   25–35s : accelerate to 120 mph / 5500 RPM
 *   35–50s : cruise at 120 mph
 *   50–65s : decelerate back to idle
 *   (loop)
 */

#include "sensors.h"
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

DashData g_dash;

static uint32_t anim_start_ms = 0;

static float lerpf(float a, float b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return a + (b - a) * t;
}

static float smoothstep(float t) {
    return t * t * (3.0f - 2.0f * t);
}

void sensors_init(void)
{
    g_dash.speed_mph      = 0;
    g_dash.rpm            = 800;
    g_dash.gear           = 0;
    g_dash.reverse        = false;
    g_dash.drive_mode     = 1;   // Sport

    g_dash.coolant_temp_f = 195;
    g_dash.ambient_temp_f = 72;

    g_dash.fuel_pct       = 75;
    g_dash.fuel_range_mi  = 200;

    g_dash.tire_fl_psi    = 41;
    g_dash.tire_fr_psi    = 41;
    g_dash.tire_rl_psi    = 45;
    g_dash.tire_rr_psi    = 45;

    g_dash.odometer_mi    = 1250;

    g_dash.lane_keep_active       = true;
    g_dash.adaptive_cruise_active = false;

    anim_start_ms = millis();

    ESP_LOGI(STAG, "Mock automotive sensor init complete");
}

void sensors_update(void)
{
    uint32_t now_ms  = millis();
    uint32_t elapsed = now_ms - anim_start_ms;
    float    t_sec   = elapsed / 1000.0f;

    float cycle = fmod(t_sec, 65.0f);

    float   target_speed = 0;
    float   target_rpm   = 800;
    uint8_t target_gear  = 0;

    if (cycle < 5.0f) {
        target_speed = 0;
        target_rpm   = 800;
        target_gear  = 0;
    } else if (cycle < 15.0f) {
        float t = smoothstep((cycle - 5.0f) / 10.0f);
        target_speed = lerpf(0, 60, t);
        target_rpm   = lerpf(1000, 3000, t);
        target_gear  = (target_speed < 20) ? 1 : (target_speed < 40) ? 2 : 3;
    } else if (cycle < 25.0f) {
        target_speed = 60;
        target_rpm   = 2800;
        target_gear  = 3;
    } else if (cycle < 35.0f) {
        float t = smoothstep((cycle - 25.0f) / 10.0f);
        target_speed = lerpf(60, 120, t);
        target_rpm   = lerpf(2800, 5500, t);
        target_gear  = (target_speed < 80) ? 4 : 5;
    } else if (cycle < 50.0f) {
        target_speed = 120;
        target_rpm   = 5200;
        target_gear  = 5;
    } else {
        float t = smoothstep((cycle - 50.0f) / 15.0f);
        target_speed = lerpf(120, 0, t);
        target_rpm   = lerpf(5200, 800, t);
        target_gear  = (target_speed > 80) ? 4 :
                       (target_speed > 40) ? 3 :
                       (target_speed > 10) ? 2 :
                       (target_speed > 0)  ? 1 : 0;
    }

    const float alpha = 0.15f;
    g_dash.speed_mph = (uint16_t)lerpf((float)g_dash.speed_mph, target_speed, alpha);
    g_dash.rpm       = (uint16_t)lerpf((float)g_dash.rpm,       target_rpm,   alpha);
    g_dash.gear      = target_gear;

    if (g_dash.speed_mph > 0 && (elapsed % 5000 < 100)) {
        if (g_dash.fuel_pct > 0) g_dash.fuel_pct--;
        g_dash.fuel_range_mi = (uint16_t)(g_dash.fuel_pct * 2.8f);
    }

    if (g_dash.coolant_temp_f < 195) g_dash.coolant_temp_f += 1;

    float pressure_delta = (g_dash.speed_mph > 60) ? 1.0f : 0.0f;
    g_dash.tire_fl_psi = (uint8_t)(41 + pressure_delta);
    g_dash.tire_fr_psi = (uint8_t)(41 + pressure_delta);
    g_dash.tire_rl_psi = (uint8_t)(45 + pressure_delta);
    g_dash.tire_rr_psi = (uint8_t)(45 + pressure_delta);

    static uint32_t last_odo_ms = 0;
    if (g_dash.speed_mph > 0 && (now_ms - last_odo_ms) > (3600000UL / g_dash.speed_mph)) {
        g_dash.odometer_mi++;
        last_odo_ms = now_ms;
    }
}
