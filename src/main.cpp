/**
 * main.cpp — CubeRTMS Manufacturing Dashboard, ESP32-P4
 *
 * Framework: ESP-IDF (Arduino not supported on ESP32-P4)
 * Entry:     app_main() — replaces Arduino setup()/loop()
 *
 * Boot sequence:
 *   1. display_init()  — MIPI-DSI + GT911 + LVGL via BSP
 *   2. ui_init()       — build LVGL widget tree (manufacturing dashboard)
 *   3. sensors_init()  — seed mock manufacturing data
 *   4. sensor_task     — calls sensors_update() every 50ms (core 1)
 *   5. ui_task         — pushes g_mfg → LVGL widgets on a schedule (core 1)
 *
 * Update schedule (all under display_lock):
 *   Every 50ms  (every tick)  : ui_update_kpi()
 *   Every 2s    (tick % 40)   : ui_update_chart(), ui_update_table()
 *   Every 5s    (tick % 100)  : ui_update_alerts()
 *
 * LVGL is NOT thread-safe. All lv_* calls are wrapped in
 * display_lock() / display_unlock() which delegate to esp_lvgl_port_lock().
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "lvgl.h"
#include "display.h"
#include "sensors.h"
#include "../ui/ui.h"

static const char *TAG = "cubertms";

/* ── Sensor polling task ─────────────────────────────────────────────────── */

static void sensor_task(void *arg)
{
    (void)arg;
    while (true) {
        sensors_update();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

/* ── UI update task ──────────────────────────────────────────────────────── */

static void ui_task(void *arg)
{
    (void)arg;
    uint32_t tick = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(50));
        tick++;

        display_lock();

        /* KPI cards — every 50ms (every tick) */
        ui_update_kpi();

        /* Chart + table — every 2 seconds (40 ticks × 50ms) */
        if ((tick % 40) == 0) {
            ui_update_chart();
            ui_update_table();
        }

        /* Alerts — every 5 seconds (100 ticks × 50ms) */
        if ((tick % 100) == 0) {
            ui_update_alerts();
        }

        display_unlock();

        /* Periodic log at 10-second intervals */
        if ((tick % 200) == 0) {
            ESP_LOGI(TAG, "Line: %s | Part: %s | Actual: %d / Plan: %d | Eff: %.1f%% | Alerts: %d",
                     g_mfg.line_name,
                     g_mfg.part_number,
                     g_mfg.actual_today,
                     g_mfg.plan_today,
                     g_mfg.efficiency,
                     g_mfg.alert_count);
        }
    }
}

/* ── app_main — ESP-IDF entry point ─────────────────────────────────────── */

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "CubeRTMS Manufacturing Dashboard booting on ESP32-P4");

    /* Brief startup delay — allows serial monitor to connect */
    for (int i = 0; i < 10; i++) {
        ESP_LOGI(TAG, "alive %d/10 — connect serial now", i + 1);
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    /* 1. Display: MIPI-DSI + GT911 + LVGL init via BSP */
    display_init();
    ESP_LOGI(TAG, "Display initialized");

    /* 2. Build LVGL widget tree — manufacturing dashboard */
    display_lock();
    sensors_init();   /* init g_mfg before ui_init so chart seeds correctly */
    ui_init();
    display_unlock();
    ESP_LOGI(TAG, "UI initialized — %s line dashboard active", g_mfg.line_name);

    /* 3. Sensor polling — core 1, 4KB stack */
    xTaskCreatePinnedToCore(sensor_task, "mfg_sensors", 4096, NULL, 5, NULL, 1);

    /* 4. UI update — core 1, 8KB stack */
    xTaskCreatePinnedToCore(ui_task, "ui_upd", 8192, NULL, 5, NULL, 1);

    ESP_LOGI(TAG, "All tasks launched — CubeRTMS dashboard running");

    /* app_main may return; idle task keeps the scheduler alive */
}
