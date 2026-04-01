#include <stdio.h>
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_log_level.h"
#include "esp_system.h"

#include "epd-spi.h"
#include "common-defs.h"
#include "graphics.h"
#include "watch-timer.h"

#include "graphics.h"


void app_main(void)
{
	esp_err_t ret;
	
	ESP_LOGI("misfits.c", "Arrived at app_main()\n");
	// initialize ePaper display
	ret = epd_init();
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: Couldn't initialize eDP display\n", ret);
		goto err;
	}

	ESP_LOGI("misfits.c", "ePaper display initialized\n");

	/*
	// Initialize timer for time-keeping
	ret = timer_init();
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: Couldn't initialize timer\n", ret);
		goto err;
	}
	*/

	ESP_LOGI("misfits.c", "setting local buffer to black for display\n");
	gra_clear((char) 0xFF);
	ESP_LOGI("misfits.c", "Updating display\n");
	ret = epd_update_display(gra_screen_buffer, 5000);
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: couldn't update display\n");
		goto err;
	}
	ESP_LOGI("misfits.c", "Display successfully updated\n");
	vTaskDelay(pdMS_TO_TICKS(1000));

	ESP_LOGI("misfits.c", "setting local buffer to white for display\n");
	gra_clear((char) 0x00);
	ret = epd_update_display(gra_screen_buffer, 5000);
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: couldn't update display\n");
		goto err;
	}
	ESP_LOGI("misfits.c", "display successfully updated\n");


err:
	ESP_LOGI("misfits.c", "performing restart now...\n");
	esp_restart();
}
