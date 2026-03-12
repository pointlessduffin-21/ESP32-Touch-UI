/**
 * main.cpp — ESP32-P4 Automotive Dashboard (reference example)
 *
 * Boot sequence:
 *   1. display_init()  — MIPI-DSI + GT911 + LVGL via BSP
 *   2. ui_init()       — build LVGL widget tree
 *   3. sensors_init()  — seed mock data
 *   4. sensor_task     — calls sensors_update() every 50ms (core 1)
 *   5. ui_task         — pushes g_dash → LVGL widgets every 50ms (core 1)
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lvgl.h"
#include "display.h"
#include "sensors.h"
#include "../ui/ui.h"

static const char *TAG = "dash";

static const char* drive_mode_str(uint8_t mode)
{
    switch (mode) {
        case 0: return "Normal";
        case 1: return "Sport";
        case 2: return "Dynamic";
        case 3: return "Track";
        case 4: return "Eco";
        default: return "---";
    }
}

static void sensor_task(void *arg)
{
    (void)arg;
    while (true) {
        sensors_update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void ui_task(void *arg)
{
    (void)arg;
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(50));

        display_lock();

        lv_arc_set_value(ui_SpeedArc, g_dash.speed_mph);
        lv_label_set_text_fmt(ui_SpeedLabel,     "%d",    g_dash.speed_mph);
        lv_label_set_text_fmt(ui_FuelRangeLabel, "%d mi", g_dash.fuel_range_mi);

        lv_arc_set_value(ui_RpmArc, g_dash.rpm / 100);
        lv_label_set_text_fmt(ui_GearLabel,      "%d",    g_dash.gear);
        lv_label_set_text_fmt(ui_DriveModeLabel, "%s",    drive_mode_str(g_dash.drive_mode));
        lv_label_set_text_fmt(ui_CoolantLabel,   "%d\xc2\xb0""F", g_dash.coolant_temp_f);

        lv_label_set_text_fmt(ui_TireFrontLeft,  "%d\npsi", g_dash.tire_fl_psi);
        lv_label_set_text_fmt(ui_TireFrontRight, "%d\npsi", g_dash.tire_fr_psi);
        lv_label_set_text_fmt(ui_TireRearLeft,   "%d\npsi", g_dash.tire_rl_psi);
        lv_label_set_text_fmt(ui_TireRearRight,  "%d\npsi", g_dash.tire_rr_psi);

        lv_label_set_text_fmt(ui_AmbientTempLabel, "%d\xc2\xb0""F", g_dash.ambient_temp_f);
        lv_label_set_text_fmt(ui_OdometerLabel,    "%lu mi", (unsigned long)g_dash.odometer_mi);

        display_unlock();
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "ESP32-P4 Automotive Dashboard (reference) booting");

    for (int i = 0; i < 10; i++) {
        ESP_LOGI(TAG, "alive %d/10 — connect serial now", i + 1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    display_init();

    display_lock();
    ui_init();
    display_unlock();
    ESP_LOGI(TAG, "Automotive UI initialized");

    sensors_init();

    xTaskCreatePinnedToCore(sensor_task, "sensors", 4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(ui_task, "ui_upd",  8192, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "All tasks launched — automotive dashboard running");
}
