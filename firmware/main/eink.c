#include "eink.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "gdey0154d67.h"
#include "soc/soc_caps.h"

#include "freemono_fonts.h"
#include "glyph.h"
#include "localisation.h"

uint8_t framebuffer[DISPLAY_W * DISPLAY_H / 8];

void framebuffer_clear(void) {
    // Display image data: 1 = white, 0 = black.
    memset(framebuffer, 0xFF, sizeof(framebuffer));
}




/*static*/ esp_lcd_panel_handle_t get_eink(const gdey0154d67_disp_cfg_t * const disp_cfg) {
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

void update_display(esp_lcd_panel_handle_t eink) {
    if (partial_updates >= FULL_UPDATE_INTERVAL) {
        ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_full_update));
        partial_updates = 0;
    } else {
        ESP_ERROR_CHECK(esp_lcd_gdey0154d67_set_update_mode(eink, esp_lcd_gdey0154d67_partial_update));
        partial_updates++;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(eink, 0, 0, DISPLAY_W, DISPLAY_H, framebuffer));
}

void render_screen(const struct tm *now) {
    char time_text[8];
    char date_text[32];

    snprintf(time_text, sizeof(time_text), "%02d:%02d",
             now->tm_hour, now->tm_min);
    snprintf(date_text, sizeof(date_text), "%s, %d. %s",
             weekday_names[now->tm_wday], now->tm_mday,
             month_names[now->tm_mon]);

    framebuffer_clear();

    int time_x = (DISPLAY_W - text_width(&freemono_large, time_text)) / 2;
    int date_x = (DISPLAY_W - text_width(&freemono_small, date_text)) / 2;

    draw_text(&freemono_large, time_x, 88, time_text);
    draw_text(&freemono_small, date_x, 140, date_text);
}

void render_pairing_screen(void) {
    const char *line1 = "Willkommen bei";
    const char *line2 = "misfits";
    const char *line3 = "Bitte per Bluetooth";
    const char *line4 = "mit dem Smartphone";
    const char *line5 = "verbinden";

    framebuffer_clear();

    draw_text(&freemono_small,
              (DISPLAY_W - text_width(&freemono_small, line1)) / 2,
              52, line1);

    draw_text(&freemono_small,
              (DISPLAY_W - text_width(&freemono_small, line2)) / 2,
              76, line2);

    draw_text(&freemono_small,
              (DISPLAY_W - text_width(&freemono_small, line3)) / 2,
              120, line3);

    draw_text(&freemono_small,
              (DISPLAY_W - text_width(&freemono_small, line4)) / 2,
              144, line4);

    draw_text(&freemono_small,
              (DISPLAY_W - text_width(&freemono_small, line5)) / 2,
              168, line5);
}
