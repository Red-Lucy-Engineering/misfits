/**
 * @file demo.c
 * @author Jakub Kral (jakub6kral@centrum.cz)
 * @brief Simple demonstration project for GDEY0154D67 1.54' E-Ink display
 * @note Screen flickering during updates is normal.
 *   The flickering is caused by the SSD1681 driver, modifying the pixels of the display.
 *   The flickering is dependent on the temperature.
 * @note Full/fast display update is recommended every 5 screen partial updates.
 *   Otherwise, previous screen images may be partially visible (Usually called ghosting).
 *   Whole screen update is intended to fix the ghosting.
 * @version 0.1
 * @date 2026-02-07
 * @copyright 2026 Jakub Kral (jakub6kral@centrum.cz)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#include "esp_err.h"            // ESP_ERROR_CHECK()
#include "freertos/FreeRTOS.h"  // vTaskDelay(), pdMS_TO_TICKS()
#include "esp_lcd_panel_io.h"   // spi_bus_config_t, spi_bus_initialize(), esp_lcd_panel_io_spi_config_t, esp_lcd_panel_io_handle_t, esp_lcd_new_panel_io_spi()...
#include "esp_lcd_panel_ops.h"  //  esp_lcd_panel_del(), esp_lcd_panel_disp_sleep(), esp_lcd_panel_draw_bitmap(), esp_lcd_panel_init(), esp_lcd_panel_reset()
#include "gdey0154d67.h"        // esp_lcd_gdey0154d67_set_update_mode(), esp_lcd_gdey0154d67_whitescreen()
#include "driver/gpio.h"        // gpio_num_t
//#include "sdkconfig.h"          // CONFIG_EINK_BUSY, CONFIG_EINK_CS, CONFIG_EINK_DC, CONFIG_EINK_RES, CONFIG_EINK_SPI_MOSI...
#include "example_images.h"     // demo_image_...
#include <assert.h>             // assert()

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
static esp_lcd_panel_handle_t get_eink(const gdey0154d67_disp_cfg_t * const disp_cfg) {
    assert(disp_cfg != NULL);

	// initialize SPI interface
	const spi_bus_config_t buscfg = {
		.sclk_io_num = disp_cfg->sck,
		.mosi_io_num = disp_cfg->sdi,
		.miso_io_num = -1,
		.quadwp_io_num = -1,
		.quadhd_io_num = -1,
		.max_transfer_sz = SOC_SPI_MAXIMUM_BUFFER_SIZE
	};
	ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO));

	// create new LCD panel IO handle derived from SPI
	const esp_lcd_panel_io_spi_config_t io_config = {
		.dc_gpio_num = disp_cfg->dc,
		.cs_gpio_num = disp_cfg->cs,
		.pclk_hz = 1000000,
		.lcd_cmd_bits = 8,
		.lcd_param_bits = 8,
		.spi_mode = 0,
		.trans_queue_depth = 10,
		.on_color_trans_done = NULL
	};
	esp_lcd_panel_io_handle_t io_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI2_HOST, &io_config, &io_handle));

	// create LCD panel handle
	esp_lcd_gdey0154d67_vendor_config_t esp_lcd_gdey0154d67_cfg = {
		.busy_pin = disp_cfg->busy,
        .retain_ram = disp_cfg->retain_ram,
	};
	const esp_lcd_panel_dev_config_t panel_config = {
		.reset_gpio_num = disp_cfg->res,
		.flags.reset_active_high = false,
		.vendor_config = &esp_lcd_gdey0154d67_cfg,
	};
	esp_lcd_panel_handle_t panel_handle = NULL;
	ESP_ERROR_CHECK(esp_lcd_new_panel_gdey0154d67(io_handle, &panel_config, &panel_handle));

	return panel_handle;
}

/**
 * @brief User code entry point
 * 
 */
void app_main(void) {
    // Initialize the E-ink interface
    gdey0154d67_disp_cfg_t disp_cfg = {
        .sck   = GPIO_NUM_12,   // SCLK
        .sdi   = GPIO_NUM_11,   // MOSI / SDI
        .dc    = GPIO_NUM_9,    // DC
        .cs    = GPIO_NUM_10,   // CS
        .busy  = GPIO_NUM_18,   // BUSY
        .res   = GPIO_NUM_8,    // RES
        .retain_ram = true,
    };
    esp_lcd_panel_handle_t eink = get_eink(&disp_cfg);

    // Initialize the E-ink
    ESP_ERROR_CHECK(esp_lcd_panel_reset(eink));
    ESP_ERROR_CHECK(esp_lcd_panel_init(eink));
    // Note: ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, false)); would to the same

    // Draw basic info using fast update mode
    ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_fast_update));
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, 200, 200, demo_image_basic));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, true));
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Draw technical details using full update mode
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, false));
    ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_full_update));
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, 200, 200, demo_image_specs));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, true));
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Draw some digits using fast update mode
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, false));
    ESP_ERROR_CHECK(esp_lcd_gdey0154d67_whitescreen(eink));
    ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_partial_update));
    for (int i = 0; i < sizeof(demo_images_numbers) / sizeof(*demo_images_numbers); i++) {
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, 200, 200, demo_images_numbers[i]));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, true));
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Draw qr code containing link to project GitHub page (https://github.com/kk-kk-k/gdey0154D67)
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, false));
    ESP_ERROR_CHECK(esp_lcd_gdey0154d67_whitescreen(eink));
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, 200, 200, demo_image_qr_code_link_to_gh));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_sleep(eink, true));
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Release the E-ink handle
    ESP_ERROR_CHECK(esp_lcd_panel_del(eink));
}
