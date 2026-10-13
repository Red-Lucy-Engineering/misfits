#include "ans.h"

#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "os/os_mbuf.h"

#include "ble_sec.h"

static const char *TAG = "misfits_ans";

static const ble_uuid16_t ans_svc_uuid        = BLE_UUID16_INIT(0x1811);
static const ble_uuid16_t chr_sup_new_uuid    = BLE_UUID16_INIT(0x2A47);
static const ble_uuid16_t chr_sup_unread_uuid = BLE_UUID16_INIT(0x2A48);
static const ble_uuid16_t chr_unread_uuid     = BLE_UUID16_INIT(0x2A45);
static const ble_uuid16_t chr_new_uuid        = BLE_UUID16_INIT(0x2A46);

/* all 10 categories from assigned numbers (bits 0..9) */
#define SUPPORTED_CATEGORIES 0x03FF

/* Access comes from NimBLE-Host-Task,gets read from app_main -> Spinlock. */
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;

static uint8_t s_unread[ANS_NUM_CATEGORIES];
static char    s_text[ANS_TEXT_MAX + 1];
static uint8_t s_text_cat;
static bool    s_has_text;
static bool    s_changed;

/* cuts off at max bytes, without destroying utf-8 characters. */
static size_t utf8_clip(const uint8_t *s, size_t len, size_t max)
{
    if (len <= max) {
        return len;
    }
    size_t n = max;
    while (n > 0 && (s[n] & 0xC0) == 0x80) {
        n--;
    }
    return n;
}

static int append(struct ble_gatt_access_ctxt *ctxt, const void *data, size_t len)
{
    return os_mbuf_append(ctxt->om, data, len) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int supported_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }
    const uint8_t v[2] = { SUPPORTED_CATEGORIES & 0xFF, SUPPORTED_CATEGORIES >> 8 };
    return append(ctxt, v, sizeof(v));
}

static int unread_status_access(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        if (OS_MBUF_PKTLEN(ctxt->om) != 2) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        uint8_t buf[2];
        uint16_t copied = 0;
        if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &copied) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        uint8_t cat = buf[0];
        uint8_t cnt = buf[1];

        if (cat == ANS_CATEGORY_ALL) {
            if (cnt != 0) {
                return BLE_ATT_ERR_UNLIKELY; /* "alle" only makes sense as clear */
            }
        } else if (cat >= ANS_NUM_CATEGORIES) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        portENTER_CRITICAL(&s_lock);
        if (cat == ANS_CATEGORY_ALL) {
            for (int i = 0; i < ANS_NUM_CATEGORIES; i++) {
                if (s_unread[i] != 0) {
                    s_unread[i] = 0;
                    s_changed = true;
                }
            }
        } else if (s_unread[cat] != cnt) {
            s_unread[cat] = cnt;
            s_changed = true;
        }
        portEXIT_CRITICAL(&s_lock);

        ESP_LOGI(TAG, "unread status: cat %u count %u", cat, cnt);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        /* only for debugging: total sum (max 255) over all categories */
        uint16_t total = ans_get_unread_total();
        const uint8_t v[2] = { ANS_CATEGORY_ALL, total > 255 ? 255 : (uint8_t)total };
        return append(ctxt, v, sizeof(v));
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static int new_alert_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        uint8_t buf[2 + 64];
        uint16_t copied = 0;

        if (len < 2 || len > sizeof(buf)) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }
        if (ble_hs_mbuf_to_flat(ctxt->om, buf, sizeof(buf), &copied) != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        uint8_t cat = buf[0];
        if (cat >= ANS_NUM_CATEGORIES) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        /* buf[1] = no. of new alerts: ignored, count exclusively comes via
         * "Unread Alert Status". */
        size_t tlen = utf8_clip(&buf[2], len - 2, ANS_TEXT_MAX);

        portENTER_CRITICAL(&s_lock);
        if (!s_has_text || s_text_cat != cat ||
            strlen(s_text) != tlen || memcmp(s_text, &buf[2], tlen) != 0) {
            memcpy(s_text, &buf[2], tlen);
            s_text[tlen] = '\0';
            s_text_cat = cat;
            s_has_text = true;
            s_changed = true;
        }
        portEXIT_CRITICAL(&s_lock);

        ESP_LOGI(TAG, "new alert: cat %u, %u byte text", cat, (unsigned)tlen);
        return 0;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t buf[2 + ANS_TEXT_MAX];
        size_t tlen = 0;

        portENTER_CRITICAL(&s_lock);
        buf[0] = s_has_text ? s_text_cat : 0;
        buf[1] = s_has_text ? 1 : 0;
        if (s_has_text) {
            tlen = strlen(s_text);
            memcpy(&buf[2], s_text, tlen);
        }
        portEXIT_CRITICAL(&s_lock);

        return append(ctxt, buf, 2 + tlen);
    }

    return BLE_ATT_ERR_UNLIKELY;
}

#define ANS_RW_FLAGS (BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_DATA_SEC_FLAGS)

static const struct ble_gatt_svc_def ans_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &ans_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &chr_sup_new_uuid.u,
                .access_cb = supported_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_DATA_SEC_FLAGS,
            },
            {
                .uuid = &chr_new_uuid.u,
                .access_cb = new_alert_access,
                .flags = ANS_RW_FLAGS,
            },
            {
                .uuid = &chr_sup_unread_uuid.u,
                .access_cb = supported_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_DATA_SEC_FLAGS,
            },
            {
                .uuid = &chr_unread_uuid.u,
                .access_cb = unread_status_access,
                .flags = ANS_RW_FLAGS,
            },
            { 0 },
        },
    },
    { 0 },
};

int ans_init(void)
{
    int rc = ble_gatts_count_cfg(ans_svcs);
    if (rc != 0) {
        return rc;
    }
    return ble_gatts_add_svcs(ans_svcs);
}

uint16_t ans_get_unread_total(void)
{
    uint16_t total = 0;
    portENTER_CRITICAL(&s_lock);
    for (int i = 0; i < ANS_NUM_CATEGORIES; i++) {
        total += s_unread[i];
    }
    portEXIT_CRITICAL(&s_lock);
    return total;
}

uint8_t ans_get_unread(uint8_t category)
{
    if (category >= ANS_NUM_CATEGORIES) {
        return 0;
    }
    portENTER_CRITICAL(&s_lock);
    uint8_t v = s_unread[category];
    portEXIT_CRITICAL(&s_lock);
    return v;
}

bool ans_get_last_alert(char *out, size_t out_size, uint8_t *category)
{
    if (out == NULL || out_size == 0) {
        return false;
    }
    bool has;
    portENTER_CRITICAL(&s_lock);
    has = s_has_text;
    if (has) {
        strlcpy(out, s_text, out_size);
        if (category != NULL) {
            *category = s_text_cat;
        }
    }
    portEXIT_CRITICAL(&s_lock);
    if (!has) {
        out[0] = '\0';
    }
    return has;
}

bool ans_take_changed(void)
{
    portENTER_CRITICAL(&s_lock);
    bool c = s_changed;
    s_changed = false;
    portEXIT_CRITICAL(&s_lock);
    return c;
}
