#ifndef _BLE_H
#define _BLE_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_store.h"

#include "eink.h"

#define DEVICE_NAME "misfits"
#define WAKE_MARGIN_US 1000000ULL

extern volatile bool ble_time_written;
extern volatile bool ble_connected;
extern volatile bool ble_active;
extern volatile uint16_t ble_conn_handle;

void ble_store_config_init(void);
int ble_store_util_status_rr(struct ble_store_status_event *event, void *arg);

void ble_host_task(void *param);
void ble_on_reset(int reason);
void cts_pack(const struct tm *tm, uint8_t out[10]);
int current_time_access(uint16_t conn_handle, uint16_t attr_handle,
                        struct ble_gatt_access_ctxt *ctxt, void *arg);
int ble_gap_event(struct ble_gap_event *event, void *arg);
void ble_advertise(void);
void ble_on_sync(void);
void ble_init_persistent(void);
void sleep_until_next_minute(const gdey0154d67_disp_cfg_t * const disp_cfg);

#endif //_BLE_H
