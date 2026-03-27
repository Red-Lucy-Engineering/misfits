#include "esp-log.h"
#include "common-defs.h"
#include "driver/gptimer.h"

gptimer_handle_t gptimer = NULL;

gptimer_config_t timer_config = {
	.clk_src = GPTIMER_CLK_SRC_DEFAULT,
	.direction = GPTIMER_COUNT_UP,
	.resolution = 1 * 1000 * 1000, // 1MHz (1 tick = 1 ms)
};

static bool perminute_timer_on_alarm_cb(gptimer_handle_t timer,
					const gptimer_alarm_event_data_t *edata,
					void *user_ctx) {
	// ...
	/* General process for handling event callbacks:
	 * 1. Retrieve user context data from user_ctx (passed in from
	 *    gptimer_register_event_callbacks)
	 * 2. Get alarm event data from edata, such as edata->count_value
	 * 3. Perform user-defined operations
	 * 4. Return whether a high-priority task was awakened during the
	 *    above operations to notify the scheduler to switch tasks

	 *    Reference: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/peripherals/gptimer.html#triggering-periodic-alarm-events
	 */
}
