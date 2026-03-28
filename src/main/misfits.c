#include <stdio.h>
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"

#include "epd-spi.h"
#include "common-defs.h"


void app_main(void)
{
	esp_err_t ret;

	// initialize general purpose timer
	

	gptimer_handle_t gptimer = NULL;

	gptimer_config_t timer_config = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 1 * 1000 * 1000, // 1MHz (1 tick = 1 ms)
	};
	
	
	ret = gptimer_new_timer(&timer_config, &gptimer);
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't create timer instance\n", ret);
		goto err;
	}

	ret = gptimer_enable(gptimer);
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't enable timer\n", ret);
		goto err;
	}

	ret = gptimer_start(gptimer);
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't start timer\n", ret);
		goto err;
	}


	// initialize ePaper display
	ret = epd_init();
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't initialize eDP display\n", ret);
		goto err;
	}

	/* alarm function (basically a callback) to change the time displayed on
	*  epaper display
	*/


err:
	ESP_LOGI(TAG, "performing restart now...\n");
	esp_restart();
}
