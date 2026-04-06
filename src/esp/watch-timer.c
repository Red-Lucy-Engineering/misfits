#include "watch-timer.h"
#include "esp_log.h"
#include "driver/gptimer.h"

#include "common-defs.h"

gptimer_handle_t gptimer = NULL;

gptimer_config_t timer_config = {
	.clk_src = GPTIMER_CLK_SRC_DEFAULT,
	.direction = GPTIMER_COUNT_UP,
	.resolution_hz = 1 * 1000 * 1000, // 1MHz (1 tick = 1 ms)
};

gptimer_alarm_config_t alarm_config = {
	.reload_count = 0,
	// resolution = 1us, so this is one minute
	.alarm_count = 60000000,
	.flags.auto_reload_on_alarm = true,
};

gptimer_event_callbacks_t cbs = {
	.on_alarm = perminute_timer_on_alarm_cb,
};

esp_err_t timer_init() {
	esp_err_t ret = 0;
	ret = gptimer_new_timer(&timer_config, &gptimer);
	if (ret) {
		ESP_LOGE("timer_init", "Error %d: Couldn't create timer instance", ret);
		return ret;
	}

	ret = gptimer_set_alarm_action(gptimer, &alarm_config);
	if (ret) {
		ESP_LOGE("timer_init", "Error %d: Couldn't set timer alarm action", ret);
		return ret;
	}

	ret = gptimer_register_event_callbacks(gptimer, &cbs, NULL);
	if (ret) {
		ESP_LOGE("timer_init", "Error %d: Couldn't register timer callback", ret);
		return ret;
	}


	ret = gptimer_enable(gptimer);
	if (ret) {
		ESP_LOGE("timer_init", "Error %d: Couldn't enable timer", ret);
		return ret;
	}

	ret = gptimer_start(gptimer);
	if (ret) {
		ESP_LOGE("timer_init", "Error %d: Couldn't start timer", ret);
		return ret;
	}

	return ret;
}

