/**
 * lv_conf.h — LVGL configuration for ESP32-P4 automotive dashboard
 * Target: ESP32-P4-Function-EV-Board, 1024x600 MIPI-DSI, LVGL v9
 *
 * NOTE: When using the Espressif BSP + esp_lvgl_port, many of these
 * settings are overridden by Kconfig (sdkconfig). This file serves
 * as the fallback and simulator reference.
 */

#if 1

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR
 *====================*/
#define LV_COLOR_DEPTH 16        /* RGB565 */
#define LV_COLOR_16_SWAP 0

/*====================
   MEMORY
 *====================*/
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (512 * 1024U)   /* 512KB; actual allocation from PSRAM via BSP */

/*====================
   HAL
 *====================*/
#define LV_DISP_DEF_REFR_PERIOD  16   /* ~60fps */
#define LV_INDEV_DEF_READ_PERIOD 16

/*====================
   FEATURES
 *====================*/
#define LV_USE_ARC       1
#define LV_USE_LABEL     1
#define LV_USE_IMG       1
#define LV_USE_IMAGE     1   /* LVGL v9 alias */
#define LV_USE_LINE      1
#define LV_USE_METER     1
#define LV_USE_ANIM      1
#define LV_USE_ANIMATION 1

/*====================
   FONTS
 *====================*/
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  1
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT &lv_font_montserrat_16

/*====================
   LOG
 *====================*/
#define LV_USE_LOG      1
#define LV_LOG_LEVEL    LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF   1

/*====================
   ASSERT
 *====================*/
#define LV_USE_ASSERT_NULL   1
#define LV_USE_ASSERT_MALLOC 1

#endif /* LV_CONF_H */
#endif /* Enable */
