#include <stdio.h>
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "esp_system.h"

#include "epd-spi.h"
#include "common-defs.h"
#include "freertos/projdefs.h"
#include "graphics.h"
#include "watch-timer.h"

#include "graphics.h"


void app_main(void)
{
	esp_err_t ret;

	ESP_LOGI("misfits.c", "Arrived at app_main()");

	// Wait 10ms after Power On
	vTaskDelay(pdMS_TO_TICKS(10));
	
	// initialize ePaper display
	ret = epd_init();
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: Couldn't initialize eDP display\n", ret);
		goto err;
	}

	ESP_LOGI("misfits.c", "ePaper display initialized");

	/*
	// Initialize timer for time-keeping
	ret = timer_init();
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: Couldn't initialize timer\n", ret);
		goto err;
	}
	*/

	ESP_LOGI("misfits.c", "setting local buffer to black for display");
	gra_clear((char) 0x00);
	ESP_LOGI("misfits.c", "Updating display");
	ret = epd_update_display(gra_screen_buffer, 5000);
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: couldn't update display");
		goto err;
	}
	ESP_LOGI("misfits.c", "Display successfully updated");
	vTaskDelay(pdMS_TO_TICKS(1000));

	ESP_LOGI("misfits.c", "setting local buffer to white for display");
	gra_clear((char) 0xFF);
	ret = epd_update_display(gra_screen_buffer, 5000);
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: couldn't update display");
		goto err;
	}
	ESP_LOGI("misfits.c", "display successfully updated");


err:
	ESP_LOGI("misfits.c", "performing restart now...");
	esp_restart();
}
