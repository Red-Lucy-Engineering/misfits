#include <stdio.h>
#include "driver/gptimer.h"
#include "esp-log.h"
#include "esp-system.h"

#include "epd-spi.h"
#include "common-defs.h"


void app_main(void)
{
	int ret;

	// initialize general purpose timer
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

	ret = gptimer_start();
	if (ret) {
		ESP_LOGE(TAG, "Error %d: Couldn't start timer\n", ret);
		goto err;
	}


	// initialize ePaper display
	ret = epd_gpio_init();
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
