#pragma once

/**
 * display.h — ESP32-P4-Function-EV-Board display abstraction
 *
 * The P4 EV board uses a 7" 1024×600 MIPI-DSI panel (EK79007 driver)
 * with GT911 capacitive touch. All low-level init is handled by the
 * Espressif BSP component (espressif/esp32_p4_function_ev_board).
 *
 * This header exposes one function: display_init().
 * After it returns, LVGL is ready and lv_* calls are safe from any task
 * protected by display_lock() / display_unlock().
 */

#include "lvgl.h"

#ifndef DISPLAY_WIDTH
#define DISPLAY_WIDTH  800
#endif
#ifndef DISPLAY_HEIGHT
#define DISPLAY_HEIGHT  1280
#endif

/**
 * Initialize MIPI-DSI display, GT911 touch, LVGL, and esp_lvgl_port.
 * Turns on the backlight. Call once at startup before ui_init().
 */
void display_init(void);

/**
 * Acquire the LVGL mutex. All lv_* calls must be inside lock/unlock.
 * Wraps esp_lvgl_port_lock() with portMAX_DELAY.
 */
void display_lock(void);

/**
 * Release the LVGL mutex.
 */
void display_unlock(void);
