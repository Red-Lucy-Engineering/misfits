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

	/*
	// Initialize timer for time-keeping
	ret = timer_init();
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: Couldn't initialize timer\n", ret);
		goto err;
	}
	*/

	gra_clear((char) 0xFF);
	ret = epd_update_display(gra_screen_buffer, 5000);
	if (ret) {
		ESP_LOGE("misfits.c", "Error %d: couldn't update display\n");
		goto err;
	}


err:
	ESP_LOGI("misfits.c", "performing restart now...\n");
	esp_restart();
}
