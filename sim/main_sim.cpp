/**
 * sim/main_sim.cpp — Desktop simulator for ESP32 automotive dashboard
 *
 * Self-contained SDL2 driver for LVGL 8.3 — no lv_drivers dependency.
 * Opens an 800×480 window and runs the exact same ui.c + sensors.cpp
 * as the ESP32 firmware target.
 *
 * Build: cd esp32-touch-ui && make -f sim/Makefile run
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#include <SDL.h>

extern "C" {
#include "lvgl/lvgl.h"
#include "../ui/ui.h"
}

#include "../src/sensors.h"

#define SIM_W 1024
#define SIM_H 600

// ── SDL2 state ────────────────────────────────────────────────────────────────
static SDL_Window   *sdl_window   = NULL;
static SDL_Renderer *sdl_renderer = NULL;
static SDL_Texture  *sdl_texture  = NULL;

// ── LVGL flush callback ───────────────────────────────────────────────────────
static void sdl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int w = area->x2 - area->x1 + 1;
    int h = area->y2 - area->y1 + 1;

    SDL_Rect rect = { area->x1, area->y1, w, h };
    SDL_UpdateTexture(sdl_texture, &rect, color_p, w * (int)sizeof(lv_color_t));
    SDL_RenderCopy(sdl_renderer, sdl_texture, NULL, NULL);
    SDL_RenderPresent(sdl_renderer);

    lv_disp_flush_ready(drv);
}

// ── LVGL mouse input callback ─────────────────────────────────────────────────
static void sdl_mouse_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    int mx, my;
    uint32_t buttons = SDL_GetMouseState(&mx, &my);

    data->point.x = (lv_coord_t)mx;
    data->point.y = (lv_coord_t)my;
    data->state   = (buttons & SDL_BUTTON_LMASK) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
}

// ── Tick thread ───────────────────────────────────────────────────────────────
static void * tick_thread(void *arg)
{
    (void)arg;
    struct timespec ts = { 0, 5 * 1000000L };
    while (true) {
        lv_tick_inc(5);
        nanosleep(&ts, NULL);
    }
    return NULL;
}

// ── Drive mode string ─────────────────────────────────────────────────────────
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

// ── Push g_dash values to LVGL widgets ───────────────────────────────────────
static void update_ui(void)
{
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
}

// ── main ──────────────────────────────────────────────────────────────────────
int main(void)
{
    printf("[sim] ESP32 Dashboard Simulator\n");
    printf("[sim] %dx%d — close window or Esc to quit\n\n", SIM_W, SIM_H);

    // SDL2 init
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "[sim] SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    sdl_window = SDL_CreateWindow(
        "ESP32 Automotive Dashboard",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SIM_W, SIM_H,
        SDL_WINDOW_SHOWN
    );
    if (!sdl_window) {
        fprintf(stderr, "[sim] SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }

    sdl_renderer = SDL_CreateRenderer(sdl_window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!sdl_renderer)
        sdl_renderer = SDL_CreateRenderer(sdl_window, -1, 0);

    // Pixel format: 32-bit ARGB — matches lv_color_t at LV_COLOR_DEPTH=32
    sdl_texture = SDL_CreateTexture(sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SIM_W, SIM_H);

    // LVGL init
    lv_init();

    // Draw buffer
    static lv_color_t buf1[SIM_W * (SIM_H / 10)];
    static lv_color_t buf2[SIM_W * (SIM_H / 10)];
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, buf2, SIM_W * (SIM_H / 10));

    // Display driver
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SIM_W;
    disp_drv.ver_res  = SIM_H;
    disp_drv.flush_cb = sdl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    // Mouse input driver
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = sdl_mouse_cb;
    lv_indev_drv_register(&indev_drv);

    // Tick thread
    pthread_t tid;
    pthread_create(&tid, NULL, tick_thread, NULL);
    pthread_detach(tid);

    // Dashboard UI + sensors
    ui_init();
    sensors_init();
    printf("[sim] UI + sensors ready — running mock drive cycle...\n");

    // Main loop
    uint32_t last_sensor = 0, last_ui = 0, last_log = 0;
    bool running = true;

    while (running) {
        // SDL event pump — must be on main thread
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT ||
               (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)) {
                running = false;
            }
        }

        uint32_t now = lv_tick_get();

        if ((now - last_sensor) >= 50) {
            sensors_update();
            last_sensor = now;
        }

        if ((now - last_ui) >= 50) {
            update_ui();
            last_ui = now;
        }

        if ((now - last_log) >= 1000) {
            printf("[dash] %3d mph | %4d RPM | gear %d | %-7s | %d\xc2\xb0""F | fuel:%d%%\n",
                g_dash.speed_mph, g_dash.rpm, g_dash.gear,
                drive_mode_str(g_dash.drive_mode),
                g_dash.coolant_temp_f, g_dash.fuel_pct);
            last_log = now;
        }

        lv_timer_handler();
        SDL_Delay(2);
    }

    SDL_DestroyTexture(sdl_texture);
    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();
    return 0;
}
