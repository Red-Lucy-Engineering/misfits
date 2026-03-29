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


static bool perminute_timer_on_alarm_cb(gptimer_handle_t timer,
					const gptimer_alarm_event_data_t *edata,
					void *user_ctx);

#endif //_WATCH_TIMER_H
