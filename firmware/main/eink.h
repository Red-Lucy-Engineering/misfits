#ifndef _EINK_H
#define _EINK_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "driver/gpio.h"
#include "esp_lcd_types.h"
#include "esp_lcd_panel_ops.h"

#define DISPLAY_W 200
#define DISPLAY_H 200

#define FULL_UPDATE_INTERVAL 5

extern uint8_t framebuffer[DISPLAY_W * DISPLAY_H / 8];
extern uint32_t partial_updates;

/**
 * @brief GDEY0154D67 display configuration structure.
 *   Used locally in get_eink() for easier readability.
 * 
 */
typedef struct {
    gpio_num_t sck;     // SPI Clock
    gpio_num_t sdi;     // SPI MOSI
    gpio_num_t dc;      // Data/Command
    gpio_num_t cs;      // SPI CS
    gpio_num_t busy;    // Busy
    gpio_num_t res;     // Reset
    bool retain_ram;    // Passed to esp_lcd_gdey0154d67_vendor_config_t, see gdey0154d67.h
} gdey0154d67_disp_cfg_t;


/**
 * @brief Configure and return the E-ink as esp_lcd_panel_handle_t
 * 
 * @param[in] disp_cfg Display configuration
 * @return esp_lcd_panel_handle_t E-ink handle
 */
esp_lcd_panel_handle_t get_eink(const gdey0154d67_disp_cfg_t * const disp_cfg);

void update_display(esp_lcd_panel_handle_t eink);

void render_screen(const struct tm *now);

void render_pairing_screen(void);

void framebuffer_clear(void);

#endif // _EINK_H
