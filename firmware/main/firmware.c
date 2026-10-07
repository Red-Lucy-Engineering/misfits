/**
 * @file main.c
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_ops.h"

#include "eink.h"
#include "ble.h"

#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

#define SYNC_INTERVAL_SEC 3600
#define SYNC_RETRY_SEC 300
#define BLE_SYNC_WINDOW_MS 300000
#define TIME_VALID_THRESHOLD 1700000000

static const char *TAG = "misfits";

RTC_DATA_ATTR uint32_t partial_updates = FULL_UPDATE_INTERVAL;
static RTC_DATA_ATTR time_t last_sync_time;
static RTC_DATA_ATTR time_t last_sync_attempt __attribute__((unused));

static void time_init(void)
{
    setenv("TZ", TIMEZONE, 1);
    tzset();

    if (time(NULL) > 1700000000) {
        return;
    }

    struct tm tm_default = {
        .tm_year = 126,
        .tm_mon = 8,
        .tm_mday = 29,
        .tm_hour = 12,
        .tm_min = 34,
        .tm_isdst = -1,
    };
    struct timeval tv = {
        .tv_sec = mktime(&tm_default),
    };
    settimeofday(&tv, NULL);
}

void app_main(void) {
    gdey0154d67_disp_cfg_t disp_cfg = {
        .sck   = GPIO_NUM_12,
        .sdi   = GPIO_NUM_11,
        .dc    = GPIO_NUM_9,
        .cs    = GPIO_NUM_10,
        .busy  = GPIO_NUM_18,
        .res   = GPIO_NUM_8,
        .retain_ram = true,
    };

    gpio_deep_sleep_hold_dis();
    gpio_hold_dis(disp_cfg.res);
    gpio_hold_dis(disp_cfg.cs);

    time_init();

    esp_lcd_panel_handle_t eink = get_eink(&disp_cfg);
    ESP_ERROR_CHECK(esp_lcd_panel_reset(eink));
    ESP_ERROR_CHECK(esp_lcd_panel_init(eink));

    bool has_time_sync = last_sync_time >= TIME_VALID_THRESHOLD;
    time_t now = time(NULL);

    if (!has_time_sync) {
        ESP_LOGI(TAG, "no valid time, showing pairing screen");
        partial_updates = FULL_UPDATE_INTERVAL;
        render_pairing_screen();
        update_display(eink);
    } else {
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        render_screen(&tm_now);
        update_display(eink);
    }

    struct tm tm_init;
    localtime_r(&now, &tm_init);
    int last_min = tm_init.tm_min;

    ble_init_persistent();

    ESP_LOGI(TAG, "test mode: staying awake, BLE persistent, display on");
    for (;;) {
        if (ble_time_written) {
            ble_time_written = false;
            last_sync_time = time(NULL);
            time_t cur = last_sync_time;
            struct tm tm_now;
            localtime_r(&cur, &tm_now);
            last_min = tm_now.tm_min;
            partial_updates = FULL_UPDATE_INTERVAL;
            render_screen(&tm_now);
            update_display(eink);
        } else {
            time_t cur = time(NULL);
            struct tm tm_cur;
            localtime_r(&cur, &tm_cur);
            bool synced = last_sync_time >= TIME_VALID_THRESHOLD;
            if (synced && tm_cur.tm_min != last_min) {
                last_min = tm_cur.tm_min;
                render_screen(&tm_cur);
                update_display(eink);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
