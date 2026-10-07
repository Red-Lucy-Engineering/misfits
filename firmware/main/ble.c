#include "ble.h"

#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"
#include "host/ble_store.h"
#include "host/ble_sm.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "os/os_mbuf.h"

static const char *TAG = "misfits_ble";

volatile bool ble_time_written;
volatile bool ble_connected;
volatile bool ble_active;
volatile uint16_t ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;

static const ble_uuid16_t cts_svc_uuid = BLE_UUID16_INIT(0x1805);
static const ble_uuid16_t cts_chr_uuid = BLE_UUID16_INIT(0x2A2B);
static const ble_uuid16_t adv_svc_uuid = BLE_UUID16_INIT(0x1805);

void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_on_reset(int reason) {
    ESP_LOGE(TAG, "nimble reset, reason %d", reason);
}

void cts_pack(const struct tm *tm, uint8_t out[10]) {
    uint16_t year = tm->tm_year + 1900;
    out[0] = year & 0xFF;
    out[1] = (year >> 8) & 0xFF;
    out[2] = tm->tm_mon + 1;
    out[3] = tm->tm_mday;
    out[4] = tm->tm_hour;
    out[5] = tm->tm_min;
    out[6] = tm->tm_sec;
    out[7] = (tm->tm_wday == 0) ? 7 : tm->tm_wday;
    out[8] = 0;
    out[9] = 0;
}

int current_time_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len < 7) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        uint8_t buf[10] = {0};
        uint16_t copied = 0;
        if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &copied) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        struct tm tm_set = {
            .tm_year = (buf[0] | (buf[1] << 8)) - 1900,
            .tm_mon = buf[2] - 1,
            .tm_mday = buf[3],
            .tm_hour = buf[4],
            .tm_min = buf[5],
            .tm_sec = buf[6],
            .tm_isdst = -1,
        };
        time_t t = mktime(&tm_set);
        if (t == (time_t)-1) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
        settimeofday(&tv, NULL);
        ble_time_written = true;
        ESP_LOGI(TAG, "time set via CTS");
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        time_t now = time(NULL);
        struct tm tm_now;
        localtime_r(&now, &tm_now);
        uint8_t buf[10];
        cts_pack(&tm_now, buf);
        if (os_mbuf_append(ctxt->om, buf, sizeof(buf)) != 0) {
            return BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &cts_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &cts_chr_uuid.u,
                .access_cb = current_time_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            { 0 },
        },
    },
    { 0 },
};

int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            ble_conn_handle = event->connect.conn_handle;
            ble_connected = true;
            ESP_LOGI(TAG, "connected");
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ble_connected = false;
        ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        ESP_LOGI(TAG, "disconnected, reason %d",
                 event->disconnect.reason);

        ble_advertise();
        break;

    default:
        break;
    }

    return 0;
}

void ble_advertise(void) {
    if (!ble_active) {
        return;
    }

    struct ble_hs_adv_fields fields = {0};

    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    fields.uuids16 = &adv_svc_uuid;
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;

    if (ble_gap_adv_set_fields(&fields) != 0) {
        return;
    }

    struct ble_gap_adv_params adv_params = {0};

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 160;
    adv_params.itvl_max = 240;

    ble_gap_adv_start(
        BLE_OWN_ADDR_PUBLIC,
        NULL,
        BLE_HS_FOREVER,
        &adv_params,
        ble_gap_event,
        NULL
    );
}

void ble_on_sync(void) {
    ble_hs_util_ensure_addr(0);
    ble_advertise();
    ESP_LOGI(TAG, "advertising");
}

void ble_init_persistent(void) {
    ESP_LOGI(TAG, "starting BLE persistent");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 0;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist =
        BLE_HS_KEY_DIST_ENC_KEY | BLE_HS_KEY_DIST_ID_KEY;
    ble_hs_cfg.sm_their_key_dist =
        BLE_HS_KEY_DIST_ENC_KEY | BLE_HS_KEY_DIST_ID_KEY;

    ble_svc_gap_init();
    ble_svc_gatt_init();
    ESP_ERROR_CHECK(ble_gatts_count_cfg(gatt_svcs));
    ESP_ERROR_CHECK(ble_gatts_add_svcs(gatt_svcs));
    ESP_ERROR_CHECK(ble_svc_gap_device_name_set(DEVICE_NAME));

    ble_store_config_init();

    ble_time_written = false;
    ble_connected = false;
    ble_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    ble_active = true;

    nimble_port_freertos_init(ble_host_task);
}

// Currently unused: deep sleep is disabled during development.
// Re-enable by calling this at the end of app_main() again.
__attribute__((unused))
void sleep_until_next_minute(const gdey0154d67_disp_cfg_t * const disp_cfg) {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    uint64_t sleep_us = (uint64_t)(60 - tv.tv_sec % 60) * 1000000ULL
                        - tv.tv_usec + WAKE_MARGIN_US;

    gpio_hold_en(disp_cfg->res);
    gpio_hold_en(disp_cfg->cs);
    gpio_deep_sleep_hold_en();

    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    rtc_gpio_pullup_en(GPIO_NUM_0);
    rtc_gpio_pulldown_dis(GPIO_NUM_0);

    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_us));
    esp_deep_sleep_start();
}
