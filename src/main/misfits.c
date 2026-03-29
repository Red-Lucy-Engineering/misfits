#include <stdio.h>
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include "epd-spi.h"
#include "common-defs.h"
#include "watch-timer.h"


void app_main(void)
{
	esp_err_t ret;
	
	// initialize ePaper display
	ret = epd_init();
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't initialize eDP display\n", ret);
		goto err;
	}

	// Initialize timer for time-keeping
	ret = timer_init();
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't initialize timer\n", ret);
		goto err;
	}


err:
	ESP_LOGI(TAG, "performing restart now...\n");
	esp_restart();
}
