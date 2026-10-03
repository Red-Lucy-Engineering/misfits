/**
 * @file demo.c
 * @author Jakub Kral (jakub6kral@centrum.cz), Lucy Mielke (engineering@redlucy404.de)
 * @brief misfits firmware based on gdey0154d67 democode
 * @note Full/fast display update is recommended every 5 screen partial updates.
 *   Otherwise, previous screen images may be partially visible (Usually called ghosting).
 *   Whole screen update is intended to fix the ghosting.
 * @version 0.1
 * @copyright 2026 Jakub Kral (jakub6kral@centrum.cz), Lucy Mielke (engineering@redlucy404.de)
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

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "gdey0154d67.h"
#include "driver/gpio.h"

#include "freemono_fonts.h"
#include <assert.h>
#include <string.h>


#define DISPLAY_W 200
#define DISPLAY_H 200


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

static uint8_t framebuffer[DISPLAY_W * DISPLAY_H / 8];

static void framebuffer_clear(void)
{
    // Display image data: 1 = white, 0 = black.
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}

static const bitmap_glyph_t *find_glyph(
    const bitmap_font_t *font, uint32_t codepoint)
{
    // table was sorted by converter by unicode-codepoint
    size_t lo = 0;
    size_t hi = font->count;

    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        uint32_t current = font->glyphs[mid].codepoint;

        if (current < codepoint) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo < font->count && font->glyphs[lo].codepoint == codepoint) {
        return &font->glyphs[lo];
    }
    return NULL;
}

// read in utf8, so umlauts and similar can be displayed.
// Invalid sequences will be skipped
static uint32_t next_utf8(const char **text)
{
    const unsigned char *p = (const unsigned char *)*text;
    uint32_t cp;

    if (p[0] < 0x80) {
        cp = p[0];
        *text += 1;
    } else if ((p[0] & 0xE0) == 0xC0 && p[1] &&
               (p[1] & 0xC0) == 0x80) {
        cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *text += 2;
    } else if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2] &&
               (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
        cp = ((uint32_t)(p[0] & 0x0F) << 12) |
             ((uint32_t)(p[1] & 0x3F) << 6) |
             (p[2] & 0x3F);
        *text += 3;
    } else if ((p[0] & 0xF8) == 0xF0 && p[1] && p[2] && p[3] &&
               (p[1] & 0xC0) == 0x80 &&
               (p[2] & 0xC0) == 0x80 &&
               (p[3] & 0xC0) == 0x80) {
        cp = ((uint32_t)(p[0] & 0x07) << 18) |
             ((uint32_t)(p[1] & 0x3F) << 12) |
             ((uint32_t)(p[2] & 0x3F) << 6) |
             (p[3] & 0x3F);
        *text += 4;
    } else {
        cp = '?';
        *text += 1;
    }

    return cp;
}

static int text_width(const bitmap_font_t *font, const char *text)
{
    int width = 0;

    while (*text) {
        const bitmap_glyph_t *g = find_glyph(font, next_utf8(&text));
        if (g) {
            width += g->advance;
        }
    }
    return width;
}

// x: Text begin. baseline_y: Font baseline.
static void draw_text(const bitmap_font_t *font,
                      int x, int baseline_y, const char *text)
{
    while (*text) {
        const bitmap_glyph_t *g = find_glyph(font, next_utf8(&text));
        if (!g) {
            continue; // Symbol not found in this size
        }

        int row_bytes = (g->width + 7) / 8;

        for (int gy = 0; gy < g->height; gy++) {
            for (int gx = 0; gx < g->width; gx++) {
                uint8_t mask = 0x80 >> (gx % 8);

                // in glyph array: set bit = black Pixel.
                if (!(g->bitmap[gy * row_bytes + gx / 8] & mask)) {
                    continue;
                }

                int px = x + g->x_offset + gx;
                int py = baseline_y + g->y_offset + gy;

                if (px >= 0 && px < DISPLAY_W &&
                    py >= 0 && py < DISPLAY_H) {
                    // in framebuffer: deleted Bit = black.
                    framebuffer[py * (DISPLAY_W / 8) + px / 8] &=
                        (uint8_t)~(0x80 >> (px % 8));
                }
            }
        }

        x += g->advance;
    }
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

    framebuffer_clear();

	const char *time_text = "12:34";
	int time_x = (DISPLAY_W - text_width(&freemono_large, time_text)) / 2;

	draw_text(&freemono_large, time_x, 88, time_text);
	draw_text(&freemono_small, 12, 140, "Di, 29. September");
	draw_text(&freemono_small, 12, 170, "Nachricht: Grüße!");

	ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_full_update));

	ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, DISPLAY_W, DISPLAY_H, framebuffer));

    // Release the E-ink handle
    ESP_ERROR_CHECK(esp_lcd_panel_del(eink));
}
