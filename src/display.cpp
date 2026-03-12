/**
 * display.cpp — JC8012P4A1C_I_W_Y board display init
 *
 * Hardware:
 *   Display  : 10.1" 800x1280 IPS, JD9365DA-H3 driver, MIPI-DSI 2-lane
 *   Touch    : GSL3680, I2C_NUM_1, SDA=GPIO7, SCL=GPIO8, RST=GPIO22, INT=GPIO21
 *   Backlight: GPIO23 (active high)
 *   LCD RST  : GPIO27
 *   LDO      : channel 3 @ 2500mV (must power up before DSI bus init)
 */

#include "display.h"

#include "driver/gpio.h"
#include "esp_ldo_regulator.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#include "lcd/esp_lcd_jd9365.h"
#include "lvgl_port_v9.h"

static const char *TAG = "display";

// ── Pin definitions ───────────────────────────────────────────────────────────
#define BSP_LCD_RST         GPIO_NUM_27
#define BSP_LCD_BACKLIGHT   GPIO_NUM_23

#define BSP_I2C_NUM         I2C_NUM_1
#define BSP_I2C_SDA         GPIO_NUM_7
#define BSP_I2C_SCL         GPIO_NUM_8
#define BSP_TP_RST          GPIO_NUM_22
#define BSP_TP_INT          GPIO_NUM_21

#define MIPI_PHY_LDO_CHAN   3
#define MIPI_PHY_LDO_MV     2500

#define LCD_H_RES   DISPLAY_WIDTH
#define LCD_V_RES   DISPLAY_HEIGHT

static esp_lcd_panel_handle_t    s_panel = NULL;
static esp_lcd_panel_io_handle_t s_io    = NULL;

IRAM_ATTR static bool display_on_vsync(esp_lcd_panel_handle_t panel,
                                        esp_lcd_dpi_panel_event_data_t *edata,
                                        void *user_ctx)
{
    return lvgl_port_notify_lcd_vsync();
}

// JD9365 init command table (from board demo lvgl_sw_rotation.c)
static const jd9365_lcd_init_cmd_t s_lcd_init_cmds[] = {
    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0xE1, (uint8_t[]){0x93}, 1, 0},
    {0xE2, (uint8_t[]){0x65}, 1, 0},
    {0xE3, (uint8_t[]){0xF8}, 1, 0},
    {0x80, (uint8_t[]){0x01}, 1, 0},

    {0xE0, (uint8_t[]){0x01}, 1, 0},
    {0x00, (uint8_t[]){0x00}, 1, 0},
    {0x01, (uint8_t[]){0x39}, 1, 0},
    {0x03, (uint8_t[]){0x10}, 1, 0},
    {0x04, (uint8_t[]){0x41}, 1, 0},

    {0x0C, (uint8_t[]){0x74}, 1, 0},
    {0x17, (uint8_t[]){0x00}, 1, 0},
    {0x18, (uint8_t[]){0xD7}, 1, 0},
    {0x19, (uint8_t[]){0x00}, 1, 0},
    {0x1A, (uint8_t[]){0x00}, 1, 0},

    {0x1B, (uint8_t[]){0xD7}, 1, 0},
    {0x1C, (uint8_t[]){0x00}, 1, 0},
    {0x24, (uint8_t[]){0xFE}, 1, 0},
    {0x35, (uint8_t[]){0x26}, 1, 0},
    {0x37, (uint8_t[]){0x69}, 1, 0},

    {0x38, (uint8_t[]){0x05}, 1, 0},
    {0x39, (uint8_t[]){0x06}, 1, 0},
    {0x3A, (uint8_t[]){0x08}, 1, 0},
    {0x3C, (uint8_t[]){0x78}, 1, 0},
    {0x3D, (uint8_t[]){0xFF}, 1, 0},

    {0x3E, (uint8_t[]){0xFF}, 1, 0},
    {0x3F, (uint8_t[]){0xFF}, 1, 0},
    {0x40, (uint8_t[]){0x06}, 1, 0},
    {0x41, (uint8_t[]){0xA0}, 1, 0},
    {0x43, (uint8_t[]){0x14}, 1, 0},

    {0x44, (uint8_t[]){0x0B}, 1, 0},
    {0x45, (uint8_t[]){0x30}, 1, 0},
    {0x4B, (uint8_t[]){0x04}, 1, 0},
    {0x55, (uint8_t[]){0x02}, 1, 0},
    {0x57, (uint8_t[]){0x89}, 1, 0},

    {0x59, (uint8_t[]){0x0A}, 1, 0},
    {0x5A, (uint8_t[]){0x28}, 1, 0},
    {0x5B, (uint8_t[]){0x15}, 1, 0},
    {0x5D, (uint8_t[]){0x50}, 1, 0},
    {0x5E, (uint8_t[]){0x37}, 1, 0},

    {0x5F, (uint8_t[]){0x29}, 1, 0},
    {0x60, (uint8_t[]){0x1E}, 1, 0},
    {0x61, (uint8_t[]){0x1D}, 1, 0},
    {0x62, (uint8_t[]){0x12}, 1, 0},
    {0x63, (uint8_t[]){0x1A}, 1, 0},

    {0x64, (uint8_t[]){0x08}, 1, 0},
    {0x65, (uint8_t[]){0x25}, 1, 0},
    {0x66, (uint8_t[]){0x26}, 1, 0},
    {0x67, (uint8_t[]){0x28}, 1, 0},
    {0x68, (uint8_t[]){0x49}, 1, 0},

    {0x69, (uint8_t[]){0x3A}, 1, 0},
    {0x6A, (uint8_t[]){0x43}, 1, 0},
    {0x6B, (uint8_t[]){0x3A}, 1, 0},
    {0x6C, (uint8_t[]){0x3B}, 1, 0},
    {0x6D, (uint8_t[]){0x32}, 1, 0},

    {0x6E, (uint8_t[]){0x1F}, 1, 0},
    {0x6F, (uint8_t[]){0x0E}, 1, 0},
    {0x70, (uint8_t[]){0x50}, 1, 0},
    {0x71, (uint8_t[]){0x37}, 1, 0},
    {0x72, (uint8_t[]){0x29}, 1, 0},

    {0x73, (uint8_t[]){0x1E}, 1, 0},
    {0x74, (uint8_t[]){0x1D}, 1, 0},
    {0x75, (uint8_t[]){0x12}, 1, 0},
    {0x76, (uint8_t[]){0x1A}, 1, 0},
    {0x77, (uint8_t[]){0x08}, 1, 0},

    {0x78, (uint8_t[]){0x25}, 1, 0},
    {0x79, (uint8_t[]){0x26}, 1, 0},
    {0x7A, (uint8_t[]){0x28}, 1, 0},
    {0x7B, (uint8_t[]){0x49}, 1, 0},
    {0x7C, (uint8_t[]){0x3A}, 1, 0},

    {0x7D, (uint8_t[]){0x43}, 1, 0},
    {0x7E, (uint8_t[]){0x3A}, 1, 0},
    {0x7F, (uint8_t[]){0x3B}, 1, 0},
    {0x80, (uint8_t[]){0x32}, 1, 0},
    {0x81, (uint8_t[]){0x1F}, 1, 0},

    {0x82, (uint8_t[]){0x0E}, 1, 0},
    {0xE0, (uint8_t[]){0x02}, 1, 0},
    {0x00, (uint8_t[]){0x1F}, 1, 0},
    {0x01, (uint8_t[]){0x1F}, 1, 0},
    {0x02, (uint8_t[]){0x52}, 1, 0},

    {0x03, (uint8_t[]){0x51}, 1, 0},
    {0x04, (uint8_t[]){0x50}, 1, 0},
    {0x05, (uint8_t[]){0x4B}, 1, 0},
    {0x06, (uint8_t[]){0x4A}, 1, 0},
    {0x07, (uint8_t[]){0x49}, 1, 0},

    {0x08, (uint8_t[]){0x48}, 1, 0},
    {0x09, (uint8_t[]){0x47}, 1, 0},
    {0x0A, (uint8_t[]){0x46}, 1, 0},
    {0x0B, (uint8_t[]){0x45}, 1, 0},
    {0x0C, (uint8_t[]){0x44}, 1, 0},

    {0x0D, (uint8_t[]){0x40}, 1, 0},
    {0x0E, (uint8_t[]){0x41}, 1, 0},
    {0x0F, (uint8_t[]){0x1F}, 1, 0},
    {0x10, (uint8_t[]){0x1F}, 1, 0},
    {0x11, (uint8_t[]){0x1F}, 1, 0},

    {0x12, (uint8_t[]){0x1F}, 1, 0},
    {0x13, (uint8_t[]){0x1F}, 1, 0},
    {0x14, (uint8_t[]){0x1F}, 1, 0},
    {0x15, (uint8_t[]){0x1F}, 1, 0},
    {0x16, (uint8_t[]){0x1F}, 1, 0},

    {0x17, (uint8_t[]){0x1F}, 1, 0},
    {0x18, (uint8_t[]){0x52}, 1, 0},
    {0x19, (uint8_t[]){0x51}, 1, 0},
    {0x1A, (uint8_t[]){0x50}, 1, 0},
    {0x1B, (uint8_t[]){0x4B}, 1, 0},

    {0x1C, (uint8_t[]){0x4A}, 1, 0},
    {0x1D, (uint8_t[]){0x49}, 1, 0},
    {0x1E, (uint8_t[]){0x48}, 1, 0},
    {0x1F, (uint8_t[]){0x47}, 1, 0},
    {0x20, (uint8_t[]){0x46}, 1, 0},

    {0x21, (uint8_t[]){0x45}, 1, 0},
    {0x22, (uint8_t[]){0x44}, 1, 0},
    {0x23, (uint8_t[]){0x40}, 1, 0},
    {0x24, (uint8_t[]){0x41}, 1, 0},
    {0x25, (uint8_t[]){0x1F}, 1, 0},

    {0x26, (uint8_t[]){0x1F}, 1, 0},
    {0x27, (uint8_t[]){0x1F}, 1, 0},
    {0x28, (uint8_t[]){0x1F}, 1, 0},
    {0x29, (uint8_t[]){0x1F}, 1, 0},
    {0x2A, (uint8_t[]){0x1F}, 1, 0},

    {0x2B, (uint8_t[]){0x1F}, 1, 0},
    {0x2C, (uint8_t[]){0x1F}, 1, 0},
    {0x2D, (uint8_t[]){0x1F}, 1, 0},
    {0x2E, (uint8_t[]){0x52}, 1, 0},
    {0x2F, (uint8_t[]){0x40}, 1, 0},

    {0x30, (uint8_t[]){0x41}, 1, 0},
    {0x31, (uint8_t[]){0x48}, 1, 0},
    {0x32, (uint8_t[]){0x49}, 1, 0},
    {0x33, (uint8_t[]){0x4A}, 1, 0},
    {0x34, (uint8_t[]){0x4B}, 1, 0},

    {0x35, (uint8_t[]){0x44}, 1, 0},
    {0x36, (uint8_t[]){0x45}, 1, 0},
    {0x37, (uint8_t[]){0x46}, 1, 0},
    {0x38, (uint8_t[]){0x47}, 1, 0},
    {0x39, (uint8_t[]){0x51}, 1, 0},

    {0x3A, (uint8_t[]){0x50}, 1, 0},
    {0x3B, (uint8_t[]){0x1F}, 1, 0},
    {0x3C, (uint8_t[]){0x1F}, 1, 0},
    {0x3D, (uint8_t[]){0x1F}, 1, 0},
    {0x3E, (uint8_t[]){0x1F}, 1, 0},

    {0x3F, (uint8_t[]){0x1F}, 1, 0},
    {0x40, (uint8_t[]){0x1F}, 1, 0},
    {0x41, (uint8_t[]){0x1F}, 1, 0},
    {0x42, (uint8_t[]){0x1F}, 1, 0},
    {0x43, (uint8_t[]){0x1F}, 1, 0},

    {0x44, (uint8_t[]){0x52}, 1, 0},
    {0x45, (uint8_t[]){0x40}, 1, 0},
    {0x46, (uint8_t[]){0x41}, 1, 0},
    {0x47, (uint8_t[]){0x48}, 1, 0},
    {0x48, (uint8_t[]){0x49}, 1, 0},

    {0x49, (uint8_t[]){0x4A}, 1, 0},
    {0x4A, (uint8_t[]){0x4B}, 1, 0},
    {0x4B, (uint8_t[]){0x44}, 1, 0},
    {0x4C, (uint8_t[]){0x45}, 1, 0},
    {0x4D, (uint8_t[]){0x46}, 1, 0},

    {0x4E, (uint8_t[]){0x47}, 1, 0},
    {0x4F, (uint8_t[]){0x51}, 1, 0},
    {0x50, (uint8_t[]){0x50}, 1, 0},
    {0x51, (uint8_t[]){0x1F}, 1, 0},
    {0x52, (uint8_t[]){0x1F}, 1, 0},

    {0x53, (uint8_t[]){0x1F}, 1, 0},
    {0x54, (uint8_t[]){0x1F}, 1, 0},
    {0x55, (uint8_t[]){0x1F}, 1, 0},
    {0x56, (uint8_t[]){0x1F}, 1, 0},
    {0x57, (uint8_t[]){0x1F}, 1, 0},

    {0x58, (uint8_t[]){0x40}, 1, 0},
    {0x59, (uint8_t[]){0x00}, 1, 0},
    {0x5A, (uint8_t[]){0x00}, 1, 0},
    {0x5B, (uint8_t[]){0x10}, 1, 0},
    {0x5C, (uint8_t[]){0x05}, 1, 0},

    {0x5D, (uint8_t[]){0x50}, 1, 0},
    {0x5E, (uint8_t[]){0x01}, 1, 0},
    {0x5F, (uint8_t[]){0x02}, 1, 0},
    {0x60, (uint8_t[]){0x50}, 1, 0},
    {0x61, (uint8_t[]){0x06}, 1, 0},

    {0x62, (uint8_t[]){0x04}, 1, 0},
    {0x63, (uint8_t[]){0x03}, 1, 0},
    {0x64, (uint8_t[]){0x64}, 1, 0},
    {0x65, (uint8_t[]){0x65}, 1, 0},
    {0x66, (uint8_t[]){0x0B}, 1, 0},

    {0x67, (uint8_t[]){0x73}, 1, 0},
    {0x68, (uint8_t[]){0x07}, 1, 0},
    {0x69, (uint8_t[]){0x06}, 1, 0},
    {0x6A, (uint8_t[]){0x64}, 1, 0},
    {0x6B, (uint8_t[]){0x08}, 1, 0},

    {0x6C, (uint8_t[]){0x00}, 1, 0},
    {0x6D, (uint8_t[]){0x32}, 1, 0},
    {0x6E, (uint8_t[]){0x08}, 1, 0},
    {0xE0, (uint8_t[]){0x04}, 1, 0},
    {0x2C, (uint8_t[]){0x6B}, 1, 0},

    {0x35, (uint8_t[]){0x08}, 1, 0},
    {0x37, (uint8_t[]){0x00}, 1, 0},
    {0xE0, (uint8_t[]){0x00}, 1, 0},
    {0x11, (uint8_t[]){0x00}, 1, 0},
    {0x29, (uint8_t[]){0x00}, 1, 5},
    {0x11, (uint8_t[]){0x00}, 1, 120},
    {0x35, (uint8_t[]){0x00}, 1, 0},
};

// ── Public API ────────────────────────────────────────────────────────────────
void display_init(void)
{
    ESP_LOGI(TAG, "display_init %dx%d JD9365 MIPI-DSI", LCD_H_RES, LCD_V_RES);

    // 0. Power the MIPI-DSI PHY via LDO channel 3 @ 2500mV
    esp_ldo_channel_handle_t ldo_mipi_phy = NULL;
    esp_ldo_channel_config_t ldo_cfg = {
        .chan_id    = MIPI_PHY_LDO_CHAN,
        .voltage_mv = MIPI_PHY_LDO_MV,
    };
    ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_cfg, &ldo_mipi_phy));
    ESP_LOGI(TAG, "MIPI DSI PHY LDO ON");

    // 1. MIPI-DSI bus — 2 lanes, 1500 Mbps
    esp_lcd_dsi_bus_handle_t dsi_bus;
    esp_lcd_dsi_bus_config_t dsi_bus_cfg = JD9365_PANEL_BUS_DSI_2CH_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_dsi_bus(&dsi_bus_cfg, &dsi_bus));
    ESP_LOGI(TAG, "DSI bus OK");

    // 3. DBI (command) IO
    esp_lcd_dbi_io_config_t dbi_cfg = JD9365_PANEL_IO_DBI_CONFIG();
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_dbi(dsi_bus, &dbi_cfg, &s_io));
    ESP_LOGI(TAG, "DBI IO OK");

    // 4. DPI timing — 800x1280 @ 63 MHz
    static const esp_lcd_dpi_panel_config_t dpi_cfg = {
        .virtual_channel    = 0,
        .dpi_clk_src        = MIPI_DSI_DPI_CLK_SRC_DEFAULT,
        .dpi_clock_freq_mhz = 63,
        .pixel_format       = LCD_COLOR_PIXEL_FORMAT_RGB565,
        .num_fbs            = LVGL_PORT_LCD_BUFFER_NUMS,
        .video_timing = {
            .h_size            = 800,
            .v_size            = 1280,
            .hsync_pulse_width = 20,
            .hsync_back_porch  = 20,
            .hsync_front_porch = 40,
            .vsync_pulse_width = 4,
            .vsync_back_porch  = 8,
            .vsync_front_porch = 20,
        },
        .flags = { .use_dma2d = true },
    };

    // 5. JD9365 vendor config + panel
    jd9365_vendor_config_t vendor_cfg = {
        .init_cmds      = s_lcd_init_cmds,
        .init_cmds_size = sizeof(s_lcd_init_cmds) / sizeof(jd9365_lcd_init_cmd_t),
        .mipi_config = {
            .dsi_bus    = dsi_bus,
            .dpi_config = &dpi_cfg,
            .lane_num   = 2,
        },
    };

    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = BSP_LCD_RST,
        .rgb_ele_order  = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
        .vendor_config  = &vendor_cfg,
    };

    ESP_LOGI(TAG, "Creating JD9365 panel...");
    ESP_ERROR_CHECK(esp_lcd_new_panel_jd9365(s_io, &panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));
    ESP_LOGI(TAG, "JD9365 panel ready");

    // 6. Register vsync callback + init vendor LVGL port (PPA 270° rotation)
    esp_lcd_dpi_panel_event_callbacks_t cbs = {
        .on_refresh_done = display_on_vsync,
    };
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_register_event_callbacks(s_panel, &cbs, NULL));
    ESP_ERROR_CHECK(lvgl_port_init(s_panel, NULL, LVGL_PORT_INTERFACE_MIPI_DSI_DMA));
    ESP_LOGI(TAG, "LVGL port + PPA rotation init OK");

    // 7. Backlight on
    gpio_config_t bk_cfg = {
        .pin_bit_mask = 1ULL << BSP_LCD_BACKLIGHT,
        .mode         = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bk_cfg);
    gpio_set_level(BSP_LCD_BACKLIGHT, 1);
    ESP_LOGI(TAG, "Backlight ON — Display ready (270° PPA, LVGL 1280x800)");
}

void display_lock(void)   { lvgl_port_lock(-1); }
void display_unlock(void) { lvgl_port_unlock(); }
