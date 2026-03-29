#ifndef _WATCH_TIMER_H
#define _WATCH_TIMER_H


#include "esp_log.h"
#include "common-defs.h"
#include "driver/gptimer.h"


extern gptimer_handle_t gptimer;
extern gptimer_config_t timer_config;
extern gptimer_alarm_config_t alarm_config;
extern gptimer_event_callbacks_t cbs;

esp_err_t timer_init();



// TODO: Implement proper way of drawing graphics to the screen first
static inline bool perminute_timer_on_alarm_cb(gptimer_handle_t timer,
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

	return false;
}

#endif //_WATCH_TIMER_H
